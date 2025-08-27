/* Webcamoid, camera capture application.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#include <windows.h>
#include <shlwapi.h>
#include <cstdint>
#include <iostream>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mfobjects.h>
#include <mfreadwrite.h>

#include "mediasource.h"
#include "PlatformUtils/src/utils.h"
#include "PlatformUtils/src/preferences.h"
#include "VCamUtils/src/logger.h"
#include "MFUtils/src/utils.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT RenderFrame(HWND hwnd, IMFSourceReader *pReader);

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    auto loglevel = AkVCam::Preferences::logLevel();
    AkVCam::Logger::setLogLevel(loglevel);
    auto defaultLogFile = AkVCam::tempPath()
                          + "/"
                          + AkVCam::basename(exePath)
                          + ".log";
    auto logFile = AkVCam::Preferences::readString("logfile", defaultLogFile);
    AkVCam::Logger::setLogFile(logFile);
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    MFStartup(MF_VERSION, MFSTARTUP_FULL);

    auto cameras = AkVCam::listRegisteredMFCameras();

    if (cameras.empty()) {
        MessageBox(nullptr,
                   TEXT("No cameras defined. Please, create at least one camera using the manager."),
                   TEXT("Error"),
                   MB_OK | MB_ICONERROR);
        MFShutdown();
        CoUninitialize();

        return -1;
    }

    // Register the window class
    WNDCLASS wc;
    memset(&wc, 0, sizeof(WNDCLASS));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("VideoWindowClass");
    RegisterClass(&wc);

    // Create the window
    auto hwnd =
            CreateWindow(wc.lpszClassName,
                         TEXT("Video Player"),
                         WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         800,
                         600,
                         NULL,
                         NULL,
                         hInstance,
                         NULL);

    if (!hwnd) {
        MessageBox(nullptr,
                   TEXT("Failed to create the window."),
                   TEXT("Error"),
                   MB_OK | MB_ICONERROR);
        MFShutdown();
        CoUninitialize();

        return -1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Inicialize IMFMediaSource and IMFSourceReader
    auto pMediaSource = new AkVCam::MediaSource(cameras.front());
    IMFSourceReader *pReader = nullptr;
    auto hr = MFCreateSourceReaderFromMediaSource(pMediaSource,
                                                  nullptr,
                                                  &pReader);

    if (FAILED(hr)) {
        MessageBox(nullptr,
                   TEXT("Failed to create the media source reader."),
                   TEXT("Error"),
                   MB_OK | MB_ICONERROR);
        pMediaSource->Release();
        MFShutdown();
        CoUninitialize();

        return -1;
    }

    // Configure the stream format
    IMFMediaType *pMediaType = nullptr;
    hr = MFCreateMediaType(&pMediaType);

    if (SUCCEEDED(hr)) {
        pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        pMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                     nullptr,
                                     pMediaType);
        pMediaType->Release();
    }

    // Select the video stream
    pReader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
    MSG msg;
    memset(&msg, 0, sizeof(MSG));
    bool running = true;

    while (running) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;

                break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        RenderFrame(hwnd, pReader);
    }

    pReader->Release();
    pMediaSource->Release();
    MFShutdown();
    CoUninitialize();

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

HRESULT RenderFrame(HWND hwnd, IMFSourceReader *pReader)
{
    DWORD streamIndex = 0;
    DWORD flags = 0;
    LONGLONG timeStamp = 0;
    IMFSample *pSample = nullptr;
    auto hr =
            pReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                0,
                                &streamIndex,
                                &flags,
                                &timeStamp,
                                &pSample);

    if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) {
        if (pSample)
            pSample->Release();

        return hr;
    }

    // Get the frame buffer
    IMFMediaBuffer *pBuffer = nullptr;
    hr = pSample->ConvertToContiguousBuffer(&pBuffer);

    if (SUCCEEDED(hr)) {
        BYTE *pData = nullptr;
        DWORD length;
        hr = pBuffer->Lock(&pData, nullptr, &length);

        if (SUCCEEDED(hr)) {
            IMFMediaType *pType = nullptr;
            pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                         &pType);
            UINT32 width = 0;
            UINT32 height = 0;
            MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);
            pType->Release();

            // Render the frame

            auto hdc = GetDC(hwnd);

            RECT rect;
            GetClientRect(hwnd, &rect);
            int winWidth = rect.right - rect.left;
            int winHeight = rect.bottom - rect.top;

            double frameAspect =
                    static_cast<double>(width) / static_cast<double>(height);
            double winAspect =
                    static_cast<double>(winWidth) / static_cast<double>(winHeight);

            int dstWidth;
            int dstHeight;
            int dstX;
            int dstY;

            if (winAspect > frameAspect) {
                dstHeight = winHeight;
                dstWidth = static_cast<int>(dstHeight * frameAspect);
                dstX = (winWidth - dstWidth) / 2;
                dstY = 0;
            } else {
                dstWidth = winWidth;
                dstHeight = static_cast<int>(dstWidth / frameAspect);
                dstX = 0;
                dstY = (winHeight - dstHeight) / 2;
            }

            BITMAPINFO bmi;
            memset(&bmi, 0, sizeof(BITMAPINFOHEADER));
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = width;
            bmi.bmiHeader.biHeight = -static_cast<int>(height);
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            // Clean the window
            auto hBrush = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
            FillRect(hdc, &rect, hBrush);

            // Draw the frame
            StretchDIBits(hdc,
                          dstX,
                          dstY,
                          dstWidth,
                          dstHeight,
                          0,
                          0,
                          width,
                          height,
                          pData,
                          &bmi,
                          DIB_RGB_COLORS,
                          SRCCOPY);
            ReleaseDC(hwnd, hdc);

            pBuffer->Unlock();
        }

        pBuffer->Release();
    }

    pSample->Release();

    return hr;
}
