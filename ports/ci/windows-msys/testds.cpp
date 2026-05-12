/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
 *
 * akvirtualcamera is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * akvirtualcamera is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with akvirtualcamera. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#include <windows.h>
#include <dshow.h>
#include <stdio.h>
#include <qedit.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32")
#pragma comment(lib, "strmiids.lib")
#pragma comment(lib, "quartz.lib")

// Helper function to get pixel format from media type
const char* GetPixelFormatFromMediaType(AM_MEDIA_TYPE *pmt)
{
    if (pmt->formattype == FORMAT_VideoInfo) {
        auto pVih = reinterpret_cast<VIDEOINFOHEADER *>(pmt->pbFormat);
        BITMAPINFOHEADER *pBmi = &pVih->bmiHeader;

        if (pBmi->biCompression == BI_RGB) {
            if (pBmi->biBitCount == 32) return "RGB32";
            if (pBmi->biBitCount == 24) return "RGB24";
            if (pBmi->biBitCount == 16) return "RGB16";
            if (pBmi->biBitCount == 8) return "RGB8";

            return "RGB";
        } else {
            // FourCC code
            DWORD fourcc = pBmi->biCompression;
            char fourccStr[5] = {0};
            memcpy(fourccStr, &fourcc, 4);

            return _strdup(fourccStr);
        }
    }

    return "UNKNOWN";
}

// Helper function to capture frames from a camera
void CaptureFramesFromCamera(IMoniker* pMoniker,
                             int deviceIndex,
                             const wchar_t* deviceName)
{
    IBaseFilter* pCaptureFilter = nullptr;
    IGraphBuilder* pGraph = nullptr;
    IMediaControl* pMediaControl = nullptr;
    ISampleGrabber* pSampleGrabber = nullptr;
    IBaseFilter* pGrabberFilter = nullptr;
    IMediaEvent* pMediaEvent = nullptr;
    int frameCount = 0;

    // Bind moniker to filter
    HRESULT hr = pMoniker->BindToObject(0,
                                        0,
                                        IID_IBaseFilter, reinterpret_cast<void**>(&pCaptureFilter));

    if (FAILED(hr)) {
        printf("  Error binding camera filter: 0x%08lX\n", static_cast<unsigned long>(hr));

        return;
    }

    // Create filter graph
    hr = CoCreateInstance(CLSID_FilterGraph,
                          nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_IGraphBuilder,
                          reinterpret_cast<void**>(&pGraph));

    if (FAILED(hr)) {
        printf("  Error creating filter graph: 0x%08lX\n", static_cast<unsigned long>(hr));
        pCaptureFilter->Release();

        return;
    }

    // Get media control interface
    hr = pGraph->QueryInterface(IID_IMediaControl,
                                reinterpret_cast<void**>(&pMediaControl));

    if (FAILED(hr)) {
        printf("  Error getting media control: 0x%08lX\n", static_cast<unsigned long>(hr));
        pGraph->Release();
        pCaptureFilter->Release();

        return;
    }

    // Get media event interface
    hr = pGraph->QueryInterface(IID_IMediaEvent,
                                reinterpret_cast<void**>(&pMediaEvent));

    if (FAILED(hr)) {
        printf("  Error getting media event: 0x%08lX\n", static_cast<unsigned long>(hr));
        pMediaControl->Release();
        pGraph->Release();
        pCaptureFilter->Release();

        return;
    }

    // Create sample grabber
    hr = CoCreateInstance(CLSID_SampleGrabber,
                          nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_IBaseFilter,
                          reinterpret_cast<void**>(&pGrabberFilter));

    if (FAILED(hr)) {
        printf("  Error creating sample grabber: 0x%08lX\n", static_cast<unsigned long>(hr));
        pMediaEvent->Release();
        pMediaControl->Release();
        pGraph->Release();
        pCaptureFilter->Release();

        return;
    }

    hr = pGrabberFilter->QueryInterface(IID_ISampleGrabber,
                                        reinterpret_cast<void**>(&pSampleGrabber));

    if (FAILED(hr)) {
        printf("  Error getting sample grabber interface: 0x%08lX\n", static_cast<unsigned long>(hr));
        pGrabberFilter->Release();
        pMediaEvent->Release();
        pMediaControl->Release();
        pGraph->Release();
        pCaptureFilter->Release();

        return;
    }

    // Add filters to graph
    hr = pGraph->AddFilter(pCaptureFilter, L"Capture Filter");

    if (FAILED(hr)) {
        printf("  Error adding capture filter: 0x%08lX\n", static_cast<unsigned long>(hr));
        pSampleGrabber->Release();
        pGrabberFilter->Release();
        pMediaEvent->Release();
        pMediaControl->Release();
        pGraph->Release();
        pCaptureFilter->Release();

        return;
    }

    hr = pGraph->AddFilter(pGrabberFilter, L"Sample Grabber");

    if (FAILED(hr)) {
        printf("  Error adding sample grabber filter: 0x%08lX\n", static_cast<unsigned long>(hr));
        pSampleGrabber->Release();
        pGrabberFilter->Release();
        pMediaEvent->Release();
        pMediaControl->Release();
        pGraph->Release();
        pCaptureFilter->Release();

        return;
    }

    // Set media type for sample grabber
    AM_MEDIA_TYPE mt;
    ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
    mt.majortype = MEDIATYPE_Video;
    mt.subtype = MEDIASUBTYPE_RGB24; // Request RGB24

    hr = pSampleGrabber->SetMediaType(&mt);

    if (FAILED(hr)) {
        printf("  Error setting media type: 0x%08lX\n", static_cast<unsigned long>(hr));
        pSampleGrabber->Release();
        pGrabberFilter->Release();
        pMediaEvent->Release();
        pMediaControl->Release();
        pGraph->Release();
        pCaptureFilter->Release();

        return;
    }

    // Set grabber to buffer samples
    hr = pSampleGrabber->SetOneShot(FALSE);
    hr = pSampleGrabber->SetBufferSamples(TRUE);

    // Find capture pin and connect to sample grabber
    IEnumPins *pEnumPins = nullptr;
    IPin *pCapturePin = nullptr;

    hr = pCaptureFilter->EnumPins(&pEnumPins);

    if (SUCCEEDED(hr)) {
        // Find the first output pin
        while (pEnumPins->Next(1, &pCapturePin, nullptr) == S_OK) {
            PIN_DIRECTION pinDir;
            pCapturePin->QueryDirection(&pinDir);

            if (pinDir == PINDIR_OUTPUT)
                break;

            pCapturePin->Release();
            pCapturePin = nullptr;
        }

        pEnumPins->Release();
    }

    if (pCapturePin) {
        // Get the sample grabber's input pin
        IEnumPins *pGrabberEnum = nullptr;
        IPin *pGrabberPin = nullptr;

        hr = pGrabberFilter->EnumPins(&pGrabberEnum);

        if (SUCCEEDED(hr)) {
            if (pGrabberEnum->Next(1, &pGrabberPin, nullptr) == S_OK) {
                // Connect the pins
                hr = pGraph->Connect(pCapturePin, pGrabberPin);

                if (FAILED(hr))
                    printf("  Error connecting pins: 0x%08lX\n", static_cast<unsigned long>(hr));

                pGrabberPin->Release();
            }

            pGrabberEnum->Release();
        }

        pCapturePin->Release();
    } else {
        printf("  Error: Could not find capture pin\n");
        pSampleGrabber->Release();
        pGrabberFilter->Release();
        pMediaEvent->Release();
        pMediaControl->Release();
        pGraph->Release();
        pCaptureFilter->Release();

        return;
    }

    // Get the actual media type
    hr = pSampleGrabber->GetConnectedMediaType(&mt);

    if (SUCCEEDED(hr)) {
        if (mt.formattype == FORMAT_VideoInfo) {
            auto pVih = reinterpret_cast<VIDEOINFOHEADER*>(mt.pbFormat);
            auto pBmi = &pVih->bmiHeader;
            auto formatStr = GetPixelFormatFromMediaType(&mt);

            printf("  Media type: %s %dx%d\n", formatStr, pBmi->biWidth, abs(pBmi->biHeight));
            printf("  Captured frames:\n");

            // Capture up to 10 frames
            const int MAX_FRAMES = 10;

            // Run the graph
            hr = pMediaControl->Run();

            if (SUCCEEDED(hr)) {
                while (frameCount < MAX_FRAMES) {
                    long evCode = 0;
                    hr = pMediaEvent->WaitForCompletion(100, &evCode);

                    // Get sample
                    long cbBuffer = 0;
                    BYTE *pBuffer = nullptr;
                    hr = pSampleGrabber->GetCurrentBuffer(&cbBuffer, nullptr);

                    if (hr == S_OK && cbBuffer > 0) {
                        pBuffer = new BYTE[cbBuffer];
                        hr = pSampleGrabber->GetCurrentBuffer(&cbBuffer,
                                                              reinterpret_cast<LONG *>(pBuffer));

                        if (SUCCEEDED(hr)) {
                            printf("    %d: %s %dx%d %ld bytes\n", frameCount, formatStr,
                                   pBmi->biWidth, abs(pBmi->biHeight), cbBuffer);
                            frameCount++;
                        }

                        delete[] pBuffer;
                    }

                    Sleep(100);
                }

                // Stop the graph
                pMediaControl->Stop();
            } else {
                printf("  Error running graph: 0x%08lX\n", static_cast<unsigned long>(hr));
            }
        }

        // Free media type
        if (mt.cbFormat)
            CoTaskMemFree(mt.pbFormat);
    } else {
        printf("  Error getting media type\n");
    }

    if (frameCount == 0)
        printf("  No frames captured\n");

    // Cleanup
    pSampleGrabber->Release();
    pGrabberFilter->Release();
    pMediaEvent->Release();
    pMediaControl->Release();
    pGraph->Release();
    pCaptureFilter->Release();
}

int main()
{
    auto hr = CoInitialize(nullptr);

    if (FAILED(hr)) {
        printf("Error initializing COM: 0x%08lX\n", static_cast<unsigned long>(hr));

        return -11;
    }

    ICreateDevEnum *pDevEnum = nullptr;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                          nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_ICreateDevEnum,
                          reinterpret_cast<void **>(&pDevEnum));

    if (FAILED(hr)) {
        printf("Error creating the device enumerator: 0x%08lX\n", static_cast<unsigned long>(hr));
        CoUninitialize();

        return -1;
    }

    IEnumMoniker *pEnum = nullptr;
    hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
                                         &pEnum,
                                         0);

    if (hr == S_FALSE) {
        printf("Camaras not found.\n");
        pDevEnum->Release();
        CoUninitialize();

        return 0;
    } else if (FAILED(hr)) {
        printf("Error enumerating devices: 0x%08lX\n", static_cast<unsigned long>(hr));
        pDevEnum->Release();
        CoUninitialize();

        return -1;
    }

    IMoniker *pMoniker = nullptr;
    ULONG count = 0;
    printf("Found cameras:\n\n");

    while (pEnum->Next(1, &pMoniker, nullptr) == S_OK) {
        IPropertyBag *pPropBag = nullptr;
        hr = pMoniker->BindToStorage(0,
                                     0,
                                     IID_IPropertyBag,
                                     reinterpret_cast<void **>(&pPropBag));

        if (SUCCEEDED(hr)) {
            VARIANT var;
            VariantInit(&var);
            hr = pPropBag->Read(L"FriendlyName", &var, 0);

            if (SUCCEEDED(hr)) {
                printf("%lu: %S\n\n", count, var.bstrVal);

                // Capture frames from this camera
                CaptureFramesFromCamera(pMoniker, count, var.bstrVal);

                printf("\n");
                VariantClear(&var);
            } else {
                printf("Error reading the device name %lu: 0x%08lX\n", count, hr);
            }

            pPropBag->Release();
        } else {
            printf("Error reading the device properties %lu: 0x%08lX\n", count, hr);
        }

        pMoniker->Release();
        count++;
    }

    pEnum->Release();
    pDevEnum->Release();
    CoUninitialize();

    if (count == 0)
        printf("No camaras found.\n");

    return 0;
}
