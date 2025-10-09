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

#include <vector>
#include <combaseapi.h>
#include <winreg.h>
#include <uuids.h>

#include "plugininterface.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

#define ROOT_HKEY HKEY_CLASSES_ROOT
#define SUBKEY_PREFIX "CLSID"

namespace AkVCam
{
    class PluginInterfacePrivate
    {
        public:
            HINSTANCE m_pluginHinstance {nullptr};

            bool registerServer(const std::string &deviceId,
                                const std::string &description) const;
            void unregisterServer(const std::string &deviceId) const;
            void unregisterServer(const CLSID &clsid) const;
            bool registerFilter(const std::string &deviceId,
                                const std::string &description) const;
            void unregisterFilter(const std::string &deviceId) const;
            void unregisterFilter(const CLSID &clsid) const;
            bool setDeviceId(const std::string &deviceId) const;
    };
}

AkVCam::PluginInterface::PluginInterface()
{
    this->d = new PluginInterfacePrivate;
}

AkVCam::PluginInterface::~PluginInterface()
{
    delete this->d;
}

void AkVCam::PluginInterface::setPluginHinstance(HINSTANCE instance)
{
    this->d->m_pluginHinstance = instance;
}

bool AkVCam::PluginInterface::createDevice(const std::string &deviceId,
                                           const std::string &description)
{
    AkLogFunction();

    if (!this->d->registerServer(deviceId, description))
        goto createDevice_failed;

    if (!this->d->registerFilter(deviceId, description))
        goto createDevice_failed;

    if (!this->d->setDeviceId(deviceId))
        goto createDevice_failed;

    return true;

createDevice_failed:
    this->destroyDevice(deviceId);

    return false;
}

void AkVCam::PluginInterface::destroyDevice(const std::string &deviceId)
{
    AkLogFunction();

    this->d->unregisterFilter(deviceId);
    this->d->unregisterServer(deviceId);
}

void AkVCam::PluginInterface::destroyDevice(const CLSID &clsid)
{
    AkLogFunction();

    this->d->unregisterFilter(clsid);
    this->d->unregisterServer(clsid);
}

void AkVCam::PluginInterface::initializeLogger() const
{
    static bool loggerReady = false;

    if (loggerReady)
        return;

    auto loglevel = Preferences::logLevel();
    Logger::setLogLevel(loglevel);

    if (loglevel > AKVCAM_LOGLEVEL_DEFAULT) {
        // Turn on lights
#ifdef _WIN32
        FILE *fp;
        freopen_s(&fp, "CONOUT$", "a", stdout);
        freopen_s(&fp, "CONOUT$", "a", stderr);
        setvbuf(stdout, nullptr, _IONBF, 0);
#else
        freopen("CONOUT$", "a", stdout);
        freopen("CONOUT$", "a", stderr);
        setbuf(stdout, nullptr);
#endif
    }

    logSetup();
    loggerReady = true;
}

bool AkVCam::PluginInterfacePrivate::registerServer(const std::string &deviceId,
                                                    const std::string &description) const
{
    AkLogFunction();

    // Define the layout in registry of the filter.

    auto clsid = createClsidStrFromStr(deviceId);
    auto fileName = moduleFileName(this->m_pluginHinstance);
    std::string threadingModel = "Both";

    AkLogInfo() << "CLSID: " << clsid << std::endl;
    AkLogInfo() << "Description: " << description << std::endl;
    AkLogInfo() << "Filename: " << fileName << std::endl;

    auto subkey = SUBKEY_PREFIX "\\" + clsid;

    HKEY keyCLSID = nullptr;
    HKEY keyServerType = nullptr;
    LONG result = RegCreateKeyA(ROOT_HKEY, subkey.c_str(), &keyCLSID);
    bool ok = false;

    if (result != ERROR_SUCCESS)
        goto registerServer_failed;

    result =
            RegSetValueA(keyCLSID,
                         nullptr,
                         REG_SZ,
                         description.c_str(),
                         DWORD(description.size()));

    if (result != ERROR_SUCCESS)
        goto registerServer_failed;

    result = RegCreateKey(keyCLSID, TEXT("InprocServer32"), &keyServerType);

    if (result != ERROR_SUCCESS)
        goto registerServer_failed;

    result =
            RegSetValueA(keyServerType,
                         nullptr,
                         REG_SZ,
                         fileName.c_str(),
                         DWORD(fileName.size()));

    if (result != ERROR_SUCCESS)
        goto registerServer_failed;

    result =
            RegSetValueExA(keyServerType,
                           "ThreadingModel",
                           0L,
                           REG_SZ,
                           reinterpret_cast<const BYTE *>(threadingModel.c_str()),
                           DWORD(threadingModel.size() + 1));

    ok = true;

registerServer_failed:
    if (keyServerType)
        RegCloseKey(keyServerType);

    if (keyCLSID)
        RegCloseKey(keyCLSID);

    AkLogInfo() << "Result: " << stringFromResult(result) << std::endl;

    return ok;
}

void AkVCam::PluginInterfacePrivate::unregisterServer(const std::string &deviceId) const
{
    AkLogFunction();

    this->unregisterServer(createClsidFromStr(deviceId));
}

void AkVCam::PluginInterfacePrivate::unregisterServer(const CLSID &clsid) const
{
    AkLogFunction();

    auto clsidStr = stringFromIid(clsid);
    AkLogInfo() << "CLSID: " << clsidStr << std::endl;
    auto subkey = SUBKEY_PREFIX "\\" + clsidStr;
    deleteTree(ROOT_HKEY, subkey.c_str(), 0);
}

bool AkVCam::PluginInterfacePrivate::registerFilter(const std::string &deviceId,
                                                    const std::string &description) const
{
    AkLogFunction();

    auto clsid = AkVCam::createClsidFromStr(deviceId);
    IFilterMapper2 *filterMapper = nullptr;
    IMoniker *pMoniker = nullptr;
    std::vector<REGPINTYPES> pinTypes {
        {&MEDIATYPE_Video, &MEDIASUBTYPE_NULL}
    };
    std::vector<REGPINMEDIUM> mediums;
    std::vector<REGFILTERPINS2> pins {
        {
            REG_PINFLAG_B_OUTPUT,
            1,
            UINT(pinTypes.size()),
            pinTypes.data(),
            UINT(mediums.size()),
            mediums.data(),
            &PIN_CATEGORY_CAPTURE
        }
    };
    REGFILTER2 regFilter;
    regFilter.dwVersion = 2;
    regFilter.dwMerit = MERIT_DO_NOT_USE;
    regFilter.cPins2 = ULONG(pins.size());
    regFilter.rgPins2 = pins.data();

    auto result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool ok = false;
    LPWSTR wdescription = nullptr;

    if (FAILED(result)) {
        AkLogError() << "Failed to initialize the COM library." << std::endl;

        goto registerFilter_failed;
    }

    result = CoCreateInstance(CLSID_FilterMapper2,
                              nullptr,
                              CLSCTX_INPROC_SERVER,
                              IID_IFilterMapper2,
                              reinterpret_cast<void **>(&filterMapper));

    if (FAILED(result)) {
        AkLogError() << "Can't create instance for IFilterMapper2." << std::endl;

        goto registerFilter_failed;
    }

    wdescription = wstrFromString(description);
    result = filterMapper->RegisterFilter(clsid,
                                          wdescription,
                                          &pMoniker,
                                          &CLSID_VideoInputDeviceCategory,
                                          nullptr,
                                          &regFilter);
    CoTaskMemFree(wdescription);
    ok = true;

registerFilter_failed:

    if (filterMapper)
        filterMapper->Release();

    CoUninitialize();

    AkLogInfo() << "Result: " << stringFromResult(result) << std::endl;

    return ok;
}

void AkVCam::PluginInterfacePrivate::unregisterFilter(const std::string &deviceId) const
{
    AkLogFunction();

    this->unregisterFilter(AkVCam::createClsidFromStr(deviceId));
}

void AkVCam::PluginInterfacePrivate::unregisterFilter(const CLSID &clsid) const
{
    AkLogFunction();
    IFilterMapper2 *filterMapper = nullptr;
    auto result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    if (FAILED(result))
        goto unregisterFilter_failed;

    result = CoCreateInstance(CLSID_FilterMapper2,
                              nullptr,
                              CLSCTX_INPROC_SERVER,
                              IID_IFilterMapper2,
                              reinterpret_cast<void **>(&filterMapper));

    if (FAILED(result))
        goto unregisterFilter_failed;

    result = filterMapper->UnregisterFilter(&CLSID_VideoInputDeviceCategory,
                                            nullptr,
                                            clsid);

unregisterFilter_failed:

    if (filterMapper)
        filterMapper->Release();

    CoUninitialize();
    AkLogInfo() << "Result: " << stringFromResult(result) << std::endl;
}

bool AkVCam::PluginInterfacePrivate::setDeviceId(const std::string &deviceId) const
{
    AkLogFunction();

    std::string subKey =
            SUBKEY_PREFIX "\\"
            + stringFromIid(CLSID_VideoInputDeviceCategory)
            + "\\Instance\\"
            + createClsidStrFromStr(deviceId);
    AkLogInfo() << "SubKey: " << subKey << std::endl;

    HKEY hKey = nullptr;
    auto result = RegOpenKeyExA(ROOT_HKEY,
                                subKey.c_str(),
                                0,
                                KEY_ALL_ACCESS,
                                &hKey);
    bool ok = false;

    if (result != ERROR_SUCCESS)
        goto setDeviceId_failed;

    result = RegSetValueExA(hKey,
                            "DevicePath",
                            0,
                            REG_SZ,
                            reinterpret_cast<const BYTE *>(deviceId.c_str()),
                            DWORD(deviceId.size() + 1));

    if (result != ERROR_SUCCESS)
        goto setDeviceId_failed;

    ok = true;

setDeviceId_failed:
    if (hKey)
        RegCloseKey(hKey);

    AkLogInfo() << "Result: " << stringFromResult(result) << std::endl;

    return ok;
}
