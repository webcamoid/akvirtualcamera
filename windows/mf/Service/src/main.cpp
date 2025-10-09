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

#include <iostream>
#include <csignal>
#include <memory>
#include <mutex>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>

#include "mfvcam.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/ipcbridge.h"
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
MFCreateVirtualCameraType mfCreateVirtualCamera;
AkVCam::IpcBridge *ipcBridgePtr;
std::mutex *mtxPtr;
CamerasList *camerasPtr;

void updateCameras(void *, const std::vector<std::string> &);

int main(int argc, char **argv)
{
    AkVCam::logSetup();
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
    mfCreateVirtualCamera =
        reinterpret_cast<MFCreateVirtualCameraType>(GetProcAddress(mfsensorgroupHnd,
                                                                   "MFCreateVirtualCamera"));

    if (!mfCreateVirtualCamera) {
        std::cerr << "'MFCreateVirtualCamera' funtion not found." << std::endl;
        FreeLibrary(mfsensorgroupHnd);
        MFShutdown();

        return -1;
    }

    std::mutex mtx;
    mtxPtr = &mtx;

    // Create the cameras

    CamerasList cameras;
    camerasPtr = &cameras;
    updateCameras(nullptr, {});

    // Subscribe for the virtual cameras updated event

    AkVCam::IpcBridge ipcBridge;
    ipcBridgePtr = &ipcBridge;
    ipcBridge.connectDevicesChanged(nullptr, updateCameras);

    // Stop the virtual camera on Ctrl + C

    signal(SIGTERM, [] (int) {
        ipcBridgePtr->disconnectDevicesChanged(nullptr, updateCameras);
        mtxPtr->lock();
        camerasPtr->clear();
        mtxPtr->unlock();
        FreeLibrary(mfsensorgroupHnd);
        MFShutdown();
    });

    // Run the virtual camera.

    std::cout << "Virtual camera service started. Press Enter to stop it..." << std::endl;
    getchar();

    ipcBridge.disconnectDevicesChanged(nullptr, updateCameras);
    mtx.lock();
    cameras.clear();
    mtx.unlock();
    FreeLibrary(mfsensorgroupHnd);
    MFShutdown();

    return 0;
}

void updateCameras(void *, const std::vector<std::string> &)
{
    std::lock_guard<std::mutex> lock(*mtxPtr);
    camerasPtr->clear();

    for (auto &clsid: AkVCam::listRegisteredMFCameras()) {
        auto cameraIndex = AkVCam::Preferences::cameraFromCLSID(clsid);

        if (cameraIndex < 0)
            continue;

        auto description = AkVCam::Preferences::cameraDescription(cameraIndex);

        if (description.empty())
            continue;

        auto clsidStr = AkVCam::stringFromClsid(clsid);
        auto clsidWStr = AkVCam::wstrFromString(clsidStr);

        if (!clsidWStr)
            continue;

        auto descriptionWStr = AkVCam::wstrFromString(description);

        if (!descriptionWStr) {
            CoTaskMemFree(clsidWStr);

            continue;
        }

        // For creating the virtual camera, the MediaSource must be registered

        auto deviceId = AkVCam::Preferences::cameraId(cameraIndex);
        std::cout << "Registering device '" << description << "' (" << deviceId << ", " << clsidStr << ")" << std::endl;

        if (!AkVCam::isDeviceIdMFTaken(deviceId)) {
            std::cerr << "WARNING: The device is not registered" << std::endl;
            CoTaskMemFree(clsidWStr);

            continue;
        }

        IMFVCam *vcam = nullptr;
        auto hr = mfCreateVirtualCamera(MFVCamType_SoftwareCameraSource,
                                        MFVCamLifetime_Session,
                                        MFVCamAccess_CurrentUser,
                                        descriptionWStr,
                                        clsidWStr,
                                        nullptr,
                                        0,
                                        &vcam);

        if (FAILED(hr) || !vcam) {
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

        camerasPtr->push_back(std::shared_ptr<IMFVCam>(vcam, [] (IMFVCam *vcam) {
            vcam->Remove();
            vcam->Shutdown();
            vcam->Release();
        }));
    }
}
