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
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <stdio.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

// Helper function to get pixel format string
const char* GetPixelFormatString(GUID format)
{
    if (format == MFVideoFormat_RGB32) return "RGB32";
    if (format == MFVideoFormat_RGB24) return "RGB24";
    if (format == MFVideoFormat_RGB555) return "RGB555";
    if (format == MFVideoFormat_RGB565) return "RGB565";
    if (format == MFVideoFormat_YUY2) return "YUY2";
    if (format == MFVideoFormat_YVYU) return "YVYU";
    if (format == MFVideoFormat_NV12) return "NV12";
    if (format == MFVideoFormat_NV11) return "NV11";
    if (format == MFVideoFormat_I420) return "I420";
    if (format == MFVideoFormat_IYUV) return "IYUV";
    if (format == MFVideoFormat_MJPG) return "MJPG";
    if (format == MFVideoFormat_H264) return "H264";

    return "UNKNOWN";
}

// Helper function to capture frames from a camera
void CaptureFramesFromCamera(IMFActivate *pDevice,
                             int deviceIndex,
                             const WCHAR *deviceName)
{
    IMFMediaSource *pSource = nullptr;
    IMFSourceReader *pReader = nullptr;

    // Activate the device
    HRESULT hr = pDevice->ActivateObject(IID_PPV_ARGS(&pSource));

    if (FAILED(hr)) {
        printf("  Error activating device: 0x%08lX\n", static_cast<unsigned long>(hr));

        return;
    }

    // Create source reader from media source
    hr = MFCreateSourceReaderFromMediaSource(pSource, nullptr, &pReader);

    if (FAILED(hr)) {
        printf("  Error creating source reader: 0x%08lX\n", static_cast<unsigned long>(hr));
        pSource->Release();

        return;
    }

    // Select the first video stream
    hr = pReader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);
    hr = pReader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);

    // Get the native media type
    IMFMediaType* pNativeType = nullptr;
    hr = pReader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &pNativeType);

    if (FAILED(hr)) {
        printf("  Error getting native media type: 0x%08lX\n", static_cast<unsigned long>(hr));
        pReader->Release();
        pSource->Release();

        return;
    }

    // Get format, width, height
    GUID format = MFVideoFormat_RGB32;
    UINT32 width = 0, height = 0;

    MFGetAttributeSize(pNativeType, MF_MT_FRAME_SIZE, &width, &height);

    // Get the media subtype (format)
    hr = pNativeType->GetGUID(MF_MT_SUBTYPE, &format);

    if (FAILED(hr))
        printf("  Error getting media subtype: 0x%08lX\n", static_cast<unsigned long>(hr));

    const char* formatStr = GetPixelFormatString(format);

    printf("  Media type: %s %ux%u\n", formatStr, width, height);
    printf("  Captured frames:\n");

    // Set the media type on the source reader
    hr = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                      nullptr,
                                      pNativeType);

    if (FAILED(hr)) {
        printf("  Error setting media type: 0x%08lX\n", static_cast<unsigned long>(hr));
        pNativeType->Release();
        pReader->Release();
        pSource->Release();

        return;
    }

    pNativeType->Release();

    // Capture up to 10 frames
    int frameCount = 0;
    const int MAX_FRAMES = 10;

    while (frameCount < MAX_FRAMES) {
        IMFSample* pSample = nullptr;
        DWORD streamIndex, flags;
        LONGLONG llTimeStamp;

        hr = pReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                 0,
                                 &streamIndex,
                                 &flags,
                                 &llTimeStamp,
                                 &pSample);

        if (FAILED(hr)) {
            printf("  Error reading sample %d: 0x%08lX\n", frameCount, static_cast<unsigned long>(hr));

            break;
        }

        if (flags & MF_SOURCE_READERF_STREAMTICK) {
            // Stream tick, continue
            if (pSample)
                pSample->Release();

            continue;
        }

        if (pSample) {
            // Get the total bytes in the frame
            IMFMediaBuffer* pBuffer = nullptr;
            DWORD bufferCount = 0;
            DWORD totalBytes = 0;

            hr = pSample->GetBufferCount(&bufferCount);

            if (SUCCEEDED(hr)) {
                for (DWORD i = 0; i < bufferCount; i++) {
                    hr = pSample->GetBufferByIndex(i, &pBuffer);

                    if (SUCCEEDED(hr)) {
                        BYTE* pData = nullptr;
                        DWORD maxLen = 0, currentLen = 0;
                        hr = pBuffer->Lock(&pData, &maxLen, &currentLen);

                        if (SUCCEEDED(hr)) {
                            totalBytes += currentLen;
                            pBuffer->Unlock();
                        }

                        pBuffer->Release();
                    }
                }
            }

            printf("    %d: %s %ux%u %u bytes\n", frameCount, formatStr, width, height, totalBytes);
            pSample->Release();
            frameCount++;
        }

        // Small delay to allow camera to provide next frame
        Sleep(100);
    }

    if (frameCount == 0)
        printf("  No frames captured\n");

    pReader->Release();
    pSource->Release();
}

int main()
{
    auto hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    if (FAILED(hr)) {
        printf("Error initializing COM: 0x%08lX\n", static_cast<unsigned long>(hr));

        return -1;
    }

    hr = MFStartup(MF_VERSION, 0);

    if (FAILED(hr)) {
        printf("Error initializing Media Foundation: 0x%08lX\n", static_cast<unsigned long>(hr));
        CoUninitialize();

        return -1;
    }

    IMFAttributes *pAttributes = nullptr;
    hr = MFCreateAttributes(&pAttributes, 1);

    if (FAILED(hr)) {
        printf("Error creating attributtes: 0x%08lX\n", static_cast<unsigned long>(hr));
        MFShutdown();
        CoUninitialize();

        return -1;
    }

    hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                              MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

    if (FAILED(hr)) {
        printf("Error configuring attributtes: 0x%08lX\n", static_cast<unsigned long>(hr));
        pAttributes->Release();
        MFShutdown();
        CoUninitialize();

        return -1;
    }

    IMFActivate **ppDevices = nullptr;
    UINT32 count = 0;
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);

    if (FAILED(hr)) {
        printf("Error enumerating devices: 0x%08lX\n", static_cast<unsigned long>(hr));
        pAttributes->Release();
        MFShutdown();
        CoUninitialize();

        return -1;
    }

    printf("Found cameras: %u\n\n", count);

    for (UINT32 i = 0; i < count; i++) {
        WCHAR *pName = nullptr;
        hr = ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
                                              &pName,
                                              nullptr);

        if (SUCCEEDED(hr)) {
            printf("%u: %S\n\n", i, pName);

            // Capture frames from this camera
            CaptureFramesFromCamera(ppDevices[i], i, pName);

            printf("\n");
            CoTaskMemFree(pName);
        } else {
            printf("Error reading the device name %u: 0x%08lX\n", i, hr);
        }

        ppDevices[i]->Release();
    }

    CoTaskMemFree(ppDevices);
    pAttributes->Release();
    MFShutdown();
    CoUninitialize();

    return 0;
}
