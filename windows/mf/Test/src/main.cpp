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
#include "MFUtils/src/utils.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HRESULT RenderFrame(HWND hwnd, IMFSourceReader *pReader);

int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    MFStartup(MF_VERSION, MFSTARTUP_FULL);

    auto cameras = AkVCam::listRegisteredMFCameras();

    if (cameras.empty()) {
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
        MFShutdown();
        CoUninitialize();

        return -1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Inicialize IMFMediaSource and IMFSourceReader
    auto pMediaSource = new AkVCam::MediaSource(cameras.front());
    IMFSourceReader *pReader = nullptr;
    auto hr = MFCreateSourceReaderFromMediaSource(pMediaSource, nullptr, &pReader);

    if (FAILED(hr)) {
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

    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

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
            BITMAPINFO bmi;
            memset(&bmi, 0, sizeof(BITMAPINFOHEADER));
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = width;
            bmi.bmiHeader.biHeight = -static_cast<int>(height);
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            StretchDIBits(hdc,
                          0,
                          0,
                          width,
                          height,
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
