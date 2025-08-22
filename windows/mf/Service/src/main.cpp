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

#include <iostream>
#include <csignal>
#include <memory>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>

#include "mfvcam.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "MFUtils/src/utils.h"

using MFCreateVirtualCameraType = HRESULT (*)(MFVCamType type,
                                              MFVCamLifetime lifetime,
                                              MFVCamAccess access,
                                              LPCWSTR friendlyName,
                                              LPCWSTR sourceId,
                                              const GUID *categories,
                                              ULONG categoryCount,
                                              IMFVCam **virtualCamera);
using CamerasList = std::vector<std::shared_ptr<IMFVCam>>;

HMODULE mfsensorgroupHnd;
CamerasList *camerasPtr;

int main(int argc, char **argv)
{
    std::cout << "Starting the virtual camera service." << std::endl;
    auto hr = MFStartup(MF_VERSION);

    if (FAILED(hr))
        return -1;

    mfsensorgroupHnd = LoadLibrary(TEXT("mfsensorgroup.dll"));

    if (!mfsensorgroupHnd) {
        std::cerr << "mfsensorgroup.dll is missing. This virtual camera only works in Windows 11+." << std::endl;
        MFShutdown();

        return -1;
    }

    // Get a reference to MFCreateVirtualCamera
    auto mfCreateVirtualCamera =
        reinterpret_cast<MFCreateVirtualCameraType>(GetProcAddress(mfsensorgroupHnd,
                                                                   "MFCreateVirtualCamera"));

    if (!mfCreateVirtualCamera) {
        std::cerr << "'MFCreateVirtualCamera' funtion not found." << std::endl;
        FreeLibrary(mfsensorgroupHnd);
        MFShutdown();

        return -1;
    }

    CamerasList cameras;
    camerasPtr = &cameras;

    for (auto &clsid: AkVCam::listRegisteredMFCameras()) {
        auto cameraIndex = AkVCam::Preferences::cameraFromCLSID(clsid);

        if (cameraIndex < 0)
            continue;

        auto description = AkVCam::Preferences::cameraDescription(cameraIndex);

        if (description.empty())
            continue;

        auto clsidWStr = AkVCam::stringToWSTR(AkVCam::stringFromClsid(clsid));
        auto descriptionWStr = AkVCam::stringToWSTR(description);

        // For creating the virtual camera, the MediaSource must be registered

        IMFVCam *vcam = nullptr;
        hr = mfCreateVirtualCamera(MFVCamType_SoftwareCameraSource,
                                   MFVCamLifetime_System,
                                   MFVCamAccess_AllUsers,
                                   descriptionWStr,
                                   clsidWStr,
                                   nullptr,
                                   0,
                                   &vcam);

        if (FAILED(hr)) {
            std::cerr << "Error creating the virtual camera: " << hr << std::endl;
            CoTaskMemFree(clsidWStr);
            CoTaskMemFree(descriptionWStr);

            continue;
        }

        CoTaskMemFree(clsidWStr);
        CoTaskMemFree(descriptionWStr);
        hr = vcam->Start(nullptr);

        if (FAILED(hr)) {
            std::cerr << "Error starting the virtual camera: " << hr << std::endl;
            vcam->Shutdown();
            vcam->Release();

            continue;
        }

        cameras.push_back(std::shared_ptr<IMFVCam>(vcam, [] (IMFVCam *vcam) {
            vcam->Stop();
            vcam->Shutdown();
            vcam->Release();
        }));
    }

    // Stop the virtual camera on Ctrl + C

    signal(SIGTERM, [] (int) {
        camerasPtr->clear();
        FreeLibrary(mfsensorgroupHnd);
        MFShutdown();
    });

    // Run the virtual camera.

    std::cout << "Virtual camera service started. Press Enter to stop it..." << std::endl;
    getchar();

    cameras.clear();
    FreeLibrary(mfsensorgroupHnd);
    MFShutdown();

    return 0;
}
