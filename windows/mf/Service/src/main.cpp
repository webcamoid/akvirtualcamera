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
#include "VCamUtils/src/sharedmemory.h"
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
    // Only allow one instance
    AkVCam::SharedMemory instanceLock;
    instanceLock.setName(AKVCAM_SERVICE_MF_NAME "_Lock");

    if (!instanceLock.open(1024, AkVCam::SharedMemory::OpenModeWrite))
        return 0;

    AkVCam::logSetup();
    AkPrintOut("Starting the virtual camera service.");
    auto hr = MFStartup(MF_VERSION);

    if (FAILED(hr))
        return -1;

    mfsensorgroupHnd = LoadLibrary(TEXT("mfsensorgroup.dll"));

    if (!mfsensorgroupHnd) {
        AkPrintErr("mfsensorgroup.dll is missing. This virtual camera only works in Windows 11+.");
        MFShutdown();

        return -1;
    }

    // Get a reference to MFCreateVirtualCamera
    mfCreateVirtualCamera =
        reinterpret_cast<MFCreateVirtualCameraType>(GetProcAddress(mfsensorgroupHnd,
                                                                   "MFCreateVirtualCamera"));

    if (!mfCreateVirtualCamera) {
        AkPrintErr("'MFCreateVirtualCamera' funtion not found.");
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

    AkPrintOut("Virtual camera service started. Press Enter to stop it...");
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

        auto clsidStr = AkVCam::stringFromClsidMF(clsid);
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
        AkPrintOut("Registering device '%s' (%s, %s)",
                   description.c_str(),
                   deviceId.c_str(),
                   clsidStr.c_str());

        if (!AkVCam::isDeviceIdMFTaken(deviceId)) {
            AkPrintErr("WARNING: The device is not registered");
            CoTaskMemFree(clsidWStr);

            continue;
        }

        AkPrintOut("Creating '%s'", description.c_str());

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
            AkPrintErr("Error creating the virtual camera: 0x%x", hr);
            CoTaskMemFree(clsidWStr);
            CoTaskMemFree(descriptionWStr);

            continue;
        }

        AkPrintOut("Starting '%s'", description.c_str());

        CoTaskMemFree(clsidWStr);
        CoTaskMemFree(descriptionWStr);
        hr = vcam->Start(nullptr);

        if (FAILED(hr)) {
            AkPrintErr("Error starting the virtual camera: 0x%x", hr);
            vcam->Shutdown();
            vcam->Release();

            continue;
        }

        AkPrintOut("Appending '%s' to the virtual cameras list", description);

        camerasPtr->push_back(std::shared_ptr<IMFVCam>(vcam, [] (IMFVCam *vcam) {
            vcam->Remove();
            vcam->Shutdown();
            vcam->Release();
        }));
    }
}
