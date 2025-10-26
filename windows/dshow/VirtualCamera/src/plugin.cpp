/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

#include "plugin.h"
#include "plugininterface.h"
#include "classfactory.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

inline AkVCam::PluginInterface *pluginInterface()
{
    static AkVCam::PluginInterface pluginInterface;

    return &pluginInterface;
}

// Filter entry point
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    UNUSED(lpvReserved);
    pluginInterface()->initializeLogger();
    AkLogFunction();

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            AkLogInfo("Reason Attach");
            AkLogInfo("Module file name: %s",
                      AkVCam::moduleFileName(hinstDLL).c_str());
            DisableThreadLibraryCalls(hinstDLL);
            pluginInterface()->setPluginHinstance(hinstDLL);

            break;

        case DLL_PROCESS_DETACH:
            AkLogInfo("Reason Detach");

            break;

        default:
            AkLogInfo("Reason Unknown: %d", fdwReason);

            break;
    }

    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    AkLogFunction();
    AkLogDebug("CLSID: %s", AkVCam::stringFromClsid(rclsid).c_str());
    AkLogDebug("IID: %s", AkVCam::stringFromClsid(riid).c_str());

    if (AkVCam::Preferences::cameraFromCLSID(rclsid) < 0)
        return CLASS_E_CLASSNOTAVAILABLE;

    auto classFactory = new AkVCam::ClassFactory(rclsid);
    auto hr = classFactory->QueryInterface(riid, ppv);
    classFactory->Release();

    return hr;
}

STDAPI DllCanUnloadNow()
{
    AkLogFunction();

    return AkVCam::ClassFactory::locked()? S_FALSE: S_OK;
}

STDAPI DllRegisterServer()
{
    pluginInterface()->initializeLogger();
    AkLogFunction();
    DllUnregisterServer();

    bool ok = true;

    for (size_t i = 0; i < AkVCam::Preferences::camerasCount(); i++) {
        auto description = AkVCam::Preferences::cameraDescription(i);
        auto deviceId = AkVCam::Preferences::cameraId(i);
        auto clsid = AkVCam::createClsidFromStr(deviceId);

        AkLogInfo("Creating Camera");
        AkLogInfo("    Description: %s", description.c_str());
        AkLogInfo("    ID: %s", deviceId.c_str());
        AkLogInfo("    CLSID: %s", AkVCam::stringFromIid(clsid).c_str());

        ok &= pluginInterface()->createDevice(deviceId, description);
    }

    return ok? S_OK: E_UNEXPECTED;
}

STDAPI DllUnregisterServer()
{
    pluginInterface()->initializeLogger();
    AkLogFunction();
    auto cameras = AkVCam::listRegisteredCameras();

    for (auto camera: cameras) {
        AkLogInfo("Deleting %s", AkVCam::stringFromClsid(camera).c_str());
        pluginInterface()->destroyDevice(camera);
    }

    return S_OK;
}
