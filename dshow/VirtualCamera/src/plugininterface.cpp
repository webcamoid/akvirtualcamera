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
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class PluginInterfacePrivate
    {
        public:
            HINSTANCE m_pluginHinstance {nullptr};
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

HINSTANCE AkVCam::PluginInterface::pluginHinstance() const
{
    return this->d->m_pluginHinstance;
}

HINSTANCE &AkVCam::PluginInterface::pluginHinstance()
{
    return this->d->m_pluginHinstance;
}

bool AkVCam::PluginInterface::registerServer(const std::string &deviceId,
                                             const std::wstring &description) const
{
    AkLogFunction();

    // Define the layout in registry of the filter.

    auto clsid = createClsidWStrFromStr(deviceId);
    auto fileName = AkVCam::moduleFileNameW(this->d->m_pluginHinstance);
    std::wstring threadingModel = L"Both";

    AkLogInfo() << "CLSID: " << std::string(clsid.begin(), clsid.end()) << std::endl;
    AkLogInfo() << "Description: " << std::string(description.begin(), description.end()) << std::endl;
    AkLogInfo() << "Filename: " << std::string(fileName.begin(), fileName.end()) << std::endl;

    auto subkey = L"CLSID\\" + clsid;

    HKEY keyCLSID = nullptr;
    HKEY keyServerType = nullptr;
    LONG result = RegCreateKey(HKEY_CLASSES_ROOT, subkey.c_str(), &keyCLSID);
    bool ok = false;

    if (result != ERROR_SUCCESS)
        goto registerServer_failed;

    result =
            RegSetValue(keyCLSID,
                        nullptr,
                        REG_SZ,
                        description.c_str(),
                        DWORD(description.size()));

    if (result != ERROR_SUCCESS)
        goto registerServer_failed;

    result = RegCreateKey(keyCLSID, L"InprocServer32", &keyServerType);

    if (result != ERROR_SUCCESS)
        goto registerServer_failed;

    result =
            RegSetValue(keyServerType,
                        nullptr,
                        REG_SZ,
                        fileName.c_str(),
                        DWORD(fileName.size()));

    if (result != ERROR_SUCCESS)
        goto registerServer_failed;

    result =
            RegSetValueEx(keyServerType,
                          L"ThreadingModel",
                          0L,
                          REG_SZ,
                          reinterpret_cast<const BYTE *>(threadingModel.c_str()),
                          DWORD((threadingModel.size() + 1) * sizeof(wchar_t)));

    ok = true;

registerServer_failed:
    if (keyServerType)
        RegCloseKey(keyServerType);

    if (keyCLSID)
        RegCloseKey(keyCLSID);

    AkLogInfo() << "Result: " << stringFromResult(result) << std::endl;

    return ok;
}

void AkVCam::PluginInterface::unregisterServer(const std::string &deviceId) const
{
    AkLogFunction();

    this->unregisterServer(createClsidFromStr(deviceId));
}

void AkVCam::PluginInterface::unregisterServer(const std::wstring &deviceId) const
{
    AkLogFunction();

    this->unregisterServer(createClsidFromStr(deviceId));
}

void AkVCam::PluginInterface::unregisterServer(const CLSID &clsid) const
{
    AkLogFunction();

    auto clsidStr = stringFromClsid(clsid);
    AkLogInfo() << "CLSID: " << clsidStr << std::endl;
    auto subkey = "CLSID\\" + clsidStr;
    deleteTree(HKEY_CLASSES_ROOT, subkey.c_str(), 0);
}

bool AkVCam::PluginInterface::registerFilter(const std::string &deviceId,
                                             const std::wstring &description) const
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

    auto result = CoInitialize(nullptr);
    bool ok = false;

    if (FAILED(result))
        goto registerFilter_failed;

    result = CoCreateInstance(CLSID_FilterMapper2,
                              nullptr,
                              CLSCTX_INPROC_SERVER,
                              IID_IFilterMapper2,
                              reinterpret_cast<void **>(&filterMapper));

    if (FAILED(result))
        goto registerFilter_failed;

    result = filterMapper->RegisterFilter(clsid,
                                          description.c_str(),
                                          &pMoniker,
                                          &CLSID_VideoInputDeviceCategory,
                                          nullptr,
                                          &regFilter);

    ok = true;

registerFilter_failed:

    if (filterMapper)
        filterMapper->Release();

    CoUninitialize();

    AkLogInfo() << "Result: " << stringFromResult(result) << std::endl;

    return ok;
}

void AkVCam::PluginInterface::unregisterFilter(const std::string &deviceId) const
{
    AkLogFunction();

    this->unregisterFilter(AkVCam::createClsidFromStr(deviceId));
}

void AkVCam::PluginInterface::unregisterFilter(const std::wstring &deviceId) const
{
    AkLogFunction();

    this->unregisterFilter(AkVCam::createClsidFromStr(deviceId));
}

void AkVCam::PluginInterface::unregisterFilter(const CLSID &clsid) const
{
    AkLogFunction();
    IFilterMapper2 *filterMapper = nullptr;
    auto result = CoInitialize(nullptr);

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

bool AkVCam::PluginInterface::setDevicePath(const std::string &deviceId) const
{
    AkLogFunction();

    std::wstring subKey =
            L"CLSID\\"
            + wstringFromIid(CLSID_VideoInputDeviceCategory)
            + L"\\Instance\\"
            + createClsidWStrFromStr(deviceId);
    AkLogInfo() << "Key: HKEY_CLASSES_ROOT" << std::endl;
    AkLogInfo() << "SubKey: " << std::string(subKey.begin(), subKey.end()) << std::endl;

    HKEY hKey = nullptr;
    auto result = RegOpenKeyEx(HKEY_CLASSES_ROOT,
                               subKey.c_str(),
                               0,
                               KEY_ALL_ACCESS,
                               &hKey);
    bool ok = false;

    if (result != ERROR_SUCCESS)
        goto setDevicePath_failed;

    result = RegSetValueExA(hKey,
                            "DevicePath",
                            0,
                            REG_SZ,
                            reinterpret_cast<const BYTE *>(deviceId.c_str()),
                            DWORD((deviceId.size() + 1) * sizeof(wchar_t)));

    if (result != ERROR_SUCCESS)
        goto setDevicePath_failed;

    ok = true;

setDevicePath_failed:
    if (hKey)
        RegCloseKey(hKey);

    AkLogInfo() << "Result: " << stringFromResult(result) << std::endl;

    return ok;
}

bool AkVCam::PluginInterface::createDevice(const std::string &deviceId,
                                           const std::wstring &description)
{
    AkLogFunction();

    if (!this->registerServer(deviceId, description))
        goto createDevice_failed;

    if (!this->registerFilter(deviceId, description))
        goto createDevice_failed;

    if (!this->setDevicePath(deviceId))
        goto createDevice_failed;

    return true;

createDevice_failed:
    this->destroyDevice(deviceId);

    return false;
}

void AkVCam::PluginInterface::destroyDevice(const std::string &deviceId)
{
    AkLogFunction();

    this->unregisterFilter(deviceId);
    this->unregisterServer(deviceId);
}

void AkVCam::PluginInterface::destroyDevice(const std::wstring &deviceId)
{
    AkLogFunction();

    this->unregisterFilter(deviceId);
    this->unregisterServer(deviceId);
}

void AkVCam::PluginInterface::destroyDevice(const CLSID &clsid)
{
    AkLogFunction();

    this->unregisterFilter(clsid);
    this->unregisterServer(clsid);
}
