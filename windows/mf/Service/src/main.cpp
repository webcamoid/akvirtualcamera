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

#include <algorithm>
#include <iostream>
#include <csignal>
#include <memory>
#include <mutex>
#include <thread>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>

#include "mfvcam.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/sharedmemory.h"
#include "MFUtils/src/utils.h"

struct VirtualCamera
{
    std::string m_description;
    std::shared_ptr<IMFVCam> m_vcam;
    std::thread m_thread;

    VirtualCamera();
    VirtualCamera(const std::string &description,
                  std::shared_ptr<IMFVCam> vcam);
    ~VirtualCamera();
    void start();
    void run();
};

using MFCreateVirtualCameraType = HRESULT (*)(MFVCamType type,
                                              MFVCamLifetime lifetime,
                                              MFVCamAccess access,
                                              LPCWSTR friendlyName,
                                              LPCWSTR sourceId,
                                              const GUID *categories,
                                              ULONG categoryCount,
                                              IMFVCam **virtualCamera);
using VirtualCameras = std::map<CLSID, std::shared_ptr<VirtualCamera>>;

HMODULE mfsensorgroupHnd;
MFCreateVirtualCameraType mfCreateVirtualCamera;
AkVCam::IpcBridge *ipcBridgePtr;
std::mutex *mtxPtr;
VirtualCameras *camerasPtr;

void updateCameras(void *, const std::vector<std::string> &);
inline bool operator <(const GUID &a, const GUID &b)
{
    return AkVCam::stringFromIid(a) < AkVCam::stringFromIid(b);
}

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

    VirtualCameras cameras;
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
    auto cameras = AkVCam::listRegisteredMFCameras();

    // Append new devices if any.

    for (auto &clsid: cameras) {
        auto cameraIndex = AkVCam::Preferences::cameraFromCLSID(clsid);

        if (cameraIndex < 0)
            continue;

        auto description = AkVCam::Preferences::cameraDescription(cameraIndex);

        if (description.empty())
            continue;

        auto clsidStr = AkVCam::stringFromIid(clsid);
        auto clsidWStr = AkVCam::wstrFromString(clsidStr);

        if (!clsidWStr)
            continue;

        auto descriptionWStr = AkVCam::wstrFromString(description);

        if (!descriptionWStr) {
            CoTaskMemFree(clsidWStr);

            continue;
        }

        if (!camerasPtr->contains(clsid)) {
            auto deviceId = AkVCam::Preferences::cameraId(cameraIndex);
            AkPrintOut("Registering device '%s' (%s, %s)",
                       description.c_str(),
                       deviceId.c_str(),
                       clsidStr.c_str());

            if (!AkVCam::isDeviceIdMFTaken(deviceId)) {
                AkPrintErr("WARNING: The device is not registered");
                CoTaskMemFree(clsidWStr);
                CoTaskMemFree(descriptionWStr);

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

            AkPrintOut("Appending '%s' to the virtual cameras list", description.c_str());
            auto vcamPtr = std::shared_ptr<IMFVCam>(vcam, [] (IMFVCam *vcam) {
                               vcam->Remove();
                               vcam->Release();
                           });
            camerasPtr->emplace(clsid, std::make_shared<VirtualCamera>(description, vcamPtr));

            AkPrintOut("Starting '%s'", description.c_str());
            camerasPtr->at(clsid)->start();
        }

        CoTaskMemFree(clsidWStr);
        CoTaskMemFree(descriptionWStr);
    }

    // Remove old devices.

    std::vector<CLSID> camerasToRemove;

    for (auto it = camerasPtr->begin(); it != camerasPtr->end(); it++) {
        auto cit = std::find_if(cameras.begin(),
                                cameras.end(),
                                [&it] (const CLSID &clsid) {
            return clsid == it->first;
        });

        if (cit == cameras.end())
            camerasToRemove.push_back(it->first);
    }

    for (auto &camera: camerasToRemove)
        camerasPtr->erase(camera);
}

VirtualCamera::VirtualCamera()
{

}

VirtualCamera::VirtualCamera(const std::string &description,
                             std::shared_ptr<IMFVCam> vcam):
    m_description(description),
    m_vcam(vcam)
{

}

VirtualCamera::~VirtualCamera()
{
    this->m_vcam->Stop();

    if (this->m_thread.joinable())
        this->m_thread.join();
}

void VirtualCamera::start()
{
    this->m_thread = std::thread(&VirtualCamera::run, this);
}

void VirtualCamera::run()
{
    auto hr = this->m_vcam->Start(nullptr);

    if (SUCCEEDED(hr))
        AkPrintOut("'%s' started", this->m_description.c_str());
    else
        AkPrintErr("Error starting the virtual camera: 0x%x", hr);
}
