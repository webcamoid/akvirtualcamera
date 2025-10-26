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
#include <shlwapi.h>
#include <cstdint>
#include <iostream>
#include <dshow.h>
#include <amvideo.h>
#include <dvdmedia.h>

#include "basefilter.h"
#include "PlatformUtils/src/utils.h"
#include "PlatformUtils/src/preferences.h"
#include "VCamUtils/src/logger.h"

#define VIDEO_WIDTH  800
#define VIDEO_HEIGHT 600

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
IPin *getPin(IBaseFilter *pFilter, PIN_DIRECTION dir);
HRESULT setFormat(IPin *pPin, int width, int height);

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    UNUSED(hPrevInstance);
    UNUSED(lpCmdLine);
    AkVCam::logSetup();

    // Initialize COM for DirectShow
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    if (FAILED(hr)) {
        MessageBox(nullptr,
                   TEXT("Failed to initialize COM."),
                   TEXT("Error"),
                   MB_OK | MB_ICONERROR);
        return -1;
    }

    auto cameras = AkVCam::listRegisteredCameras();

    if (cameras.empty()) {
        MessageBox(nullptr,
                   TEXT("No cameras defined. Please, create at least one camera using the manager."),
                   TEXT("Error"),
                   MB_OK | MB_ICONERROR);
        CoUninitialize();

        return -1;
    }

    // Register the window class
    WNDCLASS wc;
    memset(&wc, 0, sizeof(WNDCLASS));
    wc.lpfnWndProc = wndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("VideoWindowClass");
    RegisterClass(&wc);

    // Create the window
    HWND hwnd = CreateWindow(wc.lpszClassName,
                             TEXT("DirectShow virtual camera test"),
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             VIDEO_WIDTH,
                             VIDEO_HEIGHT,
                             nullptr,
                             nullptr,
                             hInstance,
                             nullptr);

    if (!hwnd) {
        MessageBox(nullptr,
                   TEXT("Failed to create the window."),
                   TEXT("Error"),
                   MB_OK | MB_ICONERROR);
        CoUninitialize();

        return -1;
    }

    // Initialize DirectShow filter graph
    IGraphBuilder *pGraph = nullptr;
    hr = CoCreateInstance(CLSID_FilterGraph,
                          nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_IGraphBuilder,
                          reinterpret_cast<void **>(&pGraph));
    if (FAILED(hr)) {
        MessageBox(nullptr,
                   TEXT("Failed to create filter graph."),
                   TEXT("Error"),
                   MB_OK | MB_ICONERROR);
        CoUninitialize();

        return -1;
    }

    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pGraph));
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Create the source filter (assuming AkVCam::BaseFilter provides IBaseFilter)

    auto pSourceFilter = new AkVCam::BaseFilter(cameras.front());
    hr = pGraph->AddFilter(pSourceFilter, L"Source Filter");

    if (FAILED(hr)) {
        MessageBox(nullptr,
                   TEXT("Failed to add source filter."),
                   TEXT("Error"),
                   MB_OK | MB_ICONERROR);
        pSourceFilter->Release();
        pGraph->Release();
        CoUninitialize();

        return -1;
    }

    // Try VMR9 first, fall back to VideoRenderer
    IBaseFilter *pRenderer = nullptr;
    hr = CoCreateInstance(CLSID_VideoMixingRenderer9,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IBaseFilter,
                          reinterpret_cast<void **>(&pRenderer));

    if (FAILED(hr)) {
        hr = CoCreateInstance(CLSID_VideoRenderer,
                              nullptr,
                              CLSCTX_INPROC_SERVER,
                              IID_IBaseFilter,
                              reinterpret_cast<void **>(&pRenderer));

        if (FAILED(hr)) {
            MessageBox(nullptr,
                       TEXT("Failed to create video renderer."),
                       TEXT("Error"),
                       MB_OK | MB_ICONERROR);
            pSourceFilter->Release();
            pGraph->Release();
            CoUninitialize();

            return -1;
        }
    }

    hr = pGraph->AddFilter(pRenderer, L"Video Renderer");

    if (FAILED(hr)) {
        MessageBox(nullptr,
                   TEXT("Failed to add video renderer."),
                   TEXT("Error"),
                   MB_OK | MB_ICONERROR);
        pRenderer->Release();
        pSourceFilter->Release();
        pGraph->Release();
        CoUninitialize();

        return -1;
    }

    // Connect the source to the renderer
    auto sourcePin = getPin(pSourceFilter, PINDIR_OUTPUT);

    if (!sourcePin) {
        MessageBox(nullptr,
                   TEXT("Failed to get the source pin."),
                   TEXT("Error"),
                   MB_OK | MB_ICONERROR);
        pRenderer->Release();
        pSourceFilter->Release();
        pGraph->Release();
        CoUninitialize();

        return -1;
    }

    if (FAILED(setFormat(sourcePin, VIDEO_WIDTH, VIDEO_HEIGHT))) {
        MessageBox(nullptr,
                   TEXT("Only RGB32 and RGB24 are supported."),
                   TEXT("Error"),
                   MB_OK | MB_ICONERROR);
        pRenderer->Release();
        pSourceFilter->Release();
        pGraph->Release();
        CoUninitialize();

        return -1;
    }

    hr = pGraph->Render(sourcePin);
    sourcePin->Release();

    if (FAILED(hr)) {
        MessageBox(nullptr,
                   TEXT("Failed to connect filters."),
                   TEXT("Error"),
                   MB_OK | MB_ICONERROR);
        pRenderer->Release();
        pSourceFilter->Release();
        pGraph->Release();
        CoUninitialize();

        return -1;
    }

    // Set the video window
    IVideoWindow *pVideoWindow = nullptr;
    hr = pGraph->QueryInterface(IID_IVideoWindow,
                                reinterpret_cast<void **>(&pVideoWindow));

    if (SUCCEEDED(hr)) {
        pVideoWindow->put_Owner((OAHWND)hwnd);
        pVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
        RECT rc;
        GetClientRect(hwnd, &rc);
        pVideoWindow->SetWindowPosition(0, 0, rc.right, rc.bottom);
        pVideoWindow->Release();
    }

    // Run the graph
    IMediaControl *pControl = nullptr;
    hr = pGraph->QueryInterface(IID_IMediaControl,
                                reinterpret_cast<void **>(&pControl));

    if (SUCCEEDED(hr)) {
        hr = pControl->Run();

        if (FAILED(hr)) {
            MessageBox(nullptr,
                       TEXT("Failed to run the graph."),
                       TEXT("Error"),
                       MB_OK | MB_ICONERROR);
            pRenderer->Release();
            pSourceFilter->Release();
            pGraph->Release();
            CoUninitialize();

            return -1;
        }
    }

    // Message loop
    MSG msg;
    memset(&msg, 0, sizeof(MSG));
    bool running = true;

    while (running)
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;

                break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

    // Cleanup
    pControl->Stop();
    pControl->Release();
    pRenderer->Release();
    pSourceFilter->Release();
    pGraph->Release();
    CoUninitialize();

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);

            return 0;

        case WM_PAINT:
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);

            return 0;

        case WM_SIZE:
            // Update video window size using the existing graph
            auto pGraph =
                    reinterpret_cast<IGraphBuilder *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

            if (pGraph) {
                IVideoWindow *pVideoWindow = nullptr;
                auto hr = pGraph->QueryInterface(IID_IVideoWindow,
                                                 reinterpret_cast<void **>(&pVideoWindow));

                if (SUCCEEDED(hr)) {
                    RECT rc;
                    GetClientRect(hwnd, &rc);
                    pVideoWindow->SetWindowPosition(0, 0, rc.right, rc.bottom);
                    pVideoWindow->Release();
                }
            }

            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

IPin *getPin(IBaseFilter *pFilter, PIN_DIRECTION dir)
{
    IEnumPins *pEnum = nullptr;
    IPin *pPin = nullptr;
    auto hr = pFilter->EnumPins(&pEnum);

    if (FAILED(hr))
        return nullptr;


    while (pEnum->Next(1, &pPin, nullptr) == S_OK && pPin) {
        PIN_DIRECTION pinDir;
        pPin->QueryDirection(&pinDir);

        if (pinDir == dir) {
            pEnum->Release();

            return pPin;
        }

        pPin->Release();
    }

    pEnum->Release();


    return nullptr;
}

HRESULT setFormat(IPin *pPin, int width, int height)
{
    if (!pPin)
        return E_POINTER;

    // Get IAMStreamConfig from the output pin
    IAMStreamConfig *pConfig = nullptr;
    auto hr = pPin->QueryInterface(IID_IAMStreamConfig,
                                   reinterpret_cast<void **>(&pConfig));

    if (FAILED(hr) || !pConfig)
        return hr;

    IPin *connectedPin = nullptr;

    if (FAILED(pPin->ConnectedTo(&connectedPin)))
        connectedPin = nullptr;

    // Select the best media type nearest to width x height
    int iCount = 0;
    int iSize = 0;
    hr = pConfig->GetNumberOfCapabilities(&iCount, &iSize);

    if (FAILED(hr)) {
        pConfig->Release();

        if (connectedPin)
            connectedPin->Release();

        return hr;
    }

    std::vector<BYTE> pSCC(iSize);
    long targetPixels = (long)width * height;
    long minDiff = LONG_MAX;
    ULONG bestIndex = ULONG_MAX;

    for (ULONG i = 0; i < iCount; ++i) {
        AM_MEDIA_TYPE *pmt = nullptr;
        hr = pConfig->GetStreamCaps(i, &pmt, pSCC.data());

        if (FAILED(hr) || !pmt)
            continue;

        if (pmt->majortype != MEDIATYPE_Video) {
            AkVCam::deleteMediaType(&pmt);

            continue;
        }

        if (connectedPin && connectedPin->QueryAccept(pmt) != S_OK) {
            AkVCam::deleteMediaType(&pmt);

            continue;
        }

        bool isRGB = false;
        int thisWidth = 0;
        int thisHeight = 0;

        if (pmt->formattype == FORMAT_VideoInfo) {
            auto pvi = reinterpret_cast<VIDEOINFOHEADER *>(pmt->pbFormat);

            if (pvi->bmiHeader.biCompression == BI_RGB) {
                int bpp = pvi->bmiHeader.biBitCount;
                GUID subtype = pmt->subtype;

                if ((bpp == 32 && subtype == MEDIASUBTYPE_RGB32)
                    || (bpp == 24 && subtype == MEDIASUBTYPE_RGB24)) {
                    isRGB = true;
                    thisWidth = pvi->bmiHeader.biWidth;
                    thisHeight = std::abs(pvi->bmiHeader.biHeight);
                }
            }
        } else if (pmt->formattype == FORMAT_VideoInfo2) {
            auto pvi2 = reinterpret_cast<VIDEOINFOHEADER2 *>(pmt->pbFormat);

            if (pvi2->bmiHeader.biCompression == BI_RGB) {
                int bpp = pvi2->bmiHeader.biBitCount;
                GUID subtype = pmt->subtype;

                if ((bpp == 32 && subtype == MEDIASUBTYPE_RGB32)
                    || (bpp == 24 && subtype == MEDIASUBTYPE_RGB24)) {
                    isRGB = true;
                    thisWidth = pvi2->bmiHeader.biWidth;
                    thisHeight = std::abs(pvi2->bmiHeader.biHeight);
                }
            }
        }

        if (isRGB && thisWidth > 0 && thisHeight > 0) {
            long thisPixels = (long) thisWidth * thisHeight;
            long diff = std::labs(thisPixels - targetPixels);

            if (diff < minDiff) {
                minDiff = diff;
                bestIndex = i;
            }
        }

        AkVCam::deleteMediaType(&pmt);
    }

    if (bestIndex == ULONG_MAX) {
        pConfig->Release();

        if (connectedPin)
            connectedPin->Release();

        return E_FAIL;
    }

    // Get the best media type
    AM_MEDIA_TYPE *bestType = nullptr;
    hr = pConfig->GetStreamCaps(bestIndex, &bestType, pSCC.data());

    if (FAILED(hr) || !bestType) {
        pConfig->Release();

        if (connectedPin)
            connectedPin->Release();

        return hr;
    }

    // Set the format
    hr = pConfig->SetFormat(bestType);
    AkVCam::deleteMediaType(&bestType);
    pConfig->Release();

    if (connectedPin)
        connectedPin->Release();

    return hr;
}
