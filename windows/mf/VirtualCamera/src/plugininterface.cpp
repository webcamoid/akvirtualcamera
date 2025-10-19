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

#include <vector>
#include <combaseapi.h>
#include <winreg.h>
#include <uuids.h>

#include "plugininterface.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

#define ROOT_HKEY HKEY_LOCAL_MACHINE
#define SUBKEY_PREFIX "Software\\Classes\\CLSID"

namespace AkVCam
{
    class PluginInterfacePrivate
    {
        public:
            HINSTANCE m_pluginHinstance {nullptr};

            bool registerMediaSource(const std::string &deviceId,
                                const std::string &description) const;
            void unregisterMediaSource(const std::string &deviceId) const;
            void unregisterMediaSource(const CLSID &clsid) const;
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

    return this->d->registerMediaSource(deviceId, description);
}

void AkVCam::PluginInterface::destroyDevice(const std::string &deviceId)
{
    AkLogFunction();

    this->d->unregisterMediaSource(deviceId);
}

void AkVCam::PluginInterface::destroyDevice(const CLSID &clsid)
{
    AkLogFunction();

    this->d->unregisterMediaSource(clsid);
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

    AkVCam::logSetup();
    loggerReady = true;
}

bool AkVCam::PluginInterfacePrivate::registerMediaSource(const std::string &deviceId,
                                                         const std::string &description) const
{
    AkLogFunction();

    // Define the layout in registry of the filter.

    auto clsid = createClsidStrFromStr(deviceId);
    auto fileName = moduleFileName(this->m_pluginHinstance);
    std::string threadingModel = "Both";

    AkLogInfo("CLSID: %s", clsid.c_str());
    AkLogInfo("Description: %s", description.c_str());
    AkLogInfo("Filename: %s", fileName.c_str());

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

    AkLogInfo("Result: %s", stringFromResult(result).c_str());

    return ok;
}

void AkVCam::PluginInterfacePrivate::unregisterMediaSource(const std::string &deviceId) const
{
    AkLogFunction();

    this->unregisterMediaSource(createClsidFromStr(deviceId));
}

void AkVCam::PluginInterfacePrivate::unregisterMediaSource(const CLSID &clsid) const
{
    AkLogFunction();

    auto clsidStr = stringFromIid(clsid);
    AkLogInfo("CLSID: %s", clsidStr.c_str());
    auto subkey = SUBKEY_PREFIX "\\" + clsidStr;
    deleteTree(ROOT_HKEY, subkey.c_str(), 0);
}
