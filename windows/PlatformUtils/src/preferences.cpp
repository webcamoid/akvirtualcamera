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

#include <algorithm>
#include <codecvt>
#include <locale>
#include <sstream>
#include <windows.h>
#include <winreg.h>
#include <uuids.h>

#include "preferences.h"
#include "utils.h"
#include "VCamUtils/src/fraction.h"
#include "VCamUtils/src/logger.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/utils.h"

#define REG_PREFIX "SOFTWARE\\Webcamoid\\VirtualCamera"
#define AKVCAM_SERVICETIMEOUT_DEFAULT 10

namespace AkVCam
{
    namespace Preferences
    {
        void splitSubKey(const std::string &key,
                         std::string &subKey,
                         std::string &value);
        bool readValue(const std::string &key,
                       DWORD dataTypeFlags,
                       PVOID data,
                       LPDWORD dataSize,
                       bool global);
        bool setValue(const std::string &key,
                      DWORD dataType,
                      LPCSTR data,
                      DWORD dataSize,
                      bool global);
    }
}

bool AkVCam::Preferences::write(const std::string &key,
                                const std::string &value,
                                bool global)
{
    AkLogFunction();
    AkLogDebug("Writing: %s = %s", key.c_str(), value.c_str());

    return setValue(key, REG_SZ, value.c_str(), DWORD(value.size()), global);
}

bool AkVCam::Preferences::write(const std::string &key, int value, bool global)
{
    AkLogFunction();
    AkLogDebug("Writing: %s = %d", key.c_str(), value);

    return setValue(key,
                    REG_DWORD,
                    reinterpret_cast<const char *>(&value),
                    DWORD(sizeof(int)),
                    global);
}

bool AkVCam::Preferences::write(const std::string &key,
                                int64_t value,
                                bool global)
{
    AkLogFunction();
    AkLogDebug("Writing: %s = %lld", key.c_str(), value);

    return setValue(key,
                    REG_QWORD,
                    reinterpret_cast<const char *>(&value),
                    DWORD(sizeof(int64_t)),
                    global);
}

bool AkVCam::Preferences::write(const std::string &key,
                                double value,
                                bool global)
{
    AkLogFunction();
    AkLogDebug("Writing: %s = %f", key.c_str(), value);
    auto val = std::to_string(value);

    return setValue(key,
                    REG_SZ,
                    val.c_str(),
                    DWORD(val.size()),
                    global);
}

bool AkVCam::Preferences::write(const std::string &key,
                                std::vector<std::string> &value,
                                bool global)
{
    AkLogFunction();

    return write(key, join(value, ","), global);
}

std::string AkVCam::Preferences::readString(const std::string &key,
                                            const std::string &defaultValue,
                                            bool global)
{
    AkLogFunction();
    char value[MAX_PATH];
    memset(value, 0, MAX_PATH * sizeof(char));
    DWORD valueSize = MAX_PATH;

    if (!readValue(key, RRF_RT_REG_SZ, &value, &valueSize, global))
        return defaultValue;

    if (strnlen(value, MAX_PATH) < 1)
        return defaultValue;

    return {value};
}

int AkVCam::Preferences::readInt(const std::string &key,
                                 int defaultValue,
                                 bool global)
{
    AkLogFunction();
    DWORD value = 0;
    DWORD valueSize = sizeof(DWORD);

    if (!readValue(key, RRF_RT_REG_DWORD, &value, &valueSize, global))
        return defaultValue;

    return int(value);
}

int64_t AkVCam::Preferences::readInt64(const std::string &key,
                                       int64_t defaultValue,
                                       bool global)
{
    AkLogFunction();
    DWORD64 value = 0;
    DWORD valueSize = sizeof(DWORD64);

    if (!readValue(key, RRF_RT_REG_QWORD, &value, &valueSize, global))
        return defaultValue;

    return int(value);
}

double AkVCam::Preferences::readDouble(const std::string &key,
                                       double defaultValue,
                                       bool global)
{
    AkLogFunction();
    auto value = readString(key, std::to_string(defaultValue), global);
    std::string::size_type sz;

    return std::stod(value, &sz);
}

bool AkVCam::Preferences::deleteKey(const std::string &key, bool global)
{
    AkLogFunction();
    AkLogDebug("Deleting %s", key.c_str());
    HKEY rootKey = global? HKEY_LOCAL_MACHINE: HKEY_CURRENT_USER;
    std::string subKey;
    std::string val;
    splitSubKey(key, subKey, val);
    bool ok = false;

    if (val.empty()) {
        ok = deleteTree(rootKey,
                        subKey.c_str(),
                        KEY_WOW64_64KEY) == ERROR_SUCCESS;
    } else {
        HKEY hkey = nullptr;
        auto result = RegOpenKeyExA(rootKey,
                                    subKey.c_str(),
                                    0,
                                    KEY_ALL_ACCESS | KEY_WOW64_64KEY,
                                    &hkey);

        if (result == ERROR_SUCCESS) {
            ok = RegDeleteValueA(hkey, val.c_str()) == ERROR_SUCCESS;
            RegCloseKey(hkey);
        }
    }

    return ok;
}

bool AkVCam::Preferences::move(const std::string &keyFrom,
                               const std::string &keyTo,
                               bool global)
{
    AkLogFunction();
    AkLogDebug("From: %s", keyFrom.c_str());
    AkLogDebug("To: %s", keyTo.c_str());
    HKEY rootKey = global? HKEY_LOCAL_MACHINE: HKEY_CURRENT_USER;
    bool ok = false;
    std::string subKeyFrom = REG_PREFIX "\\" + keyFrom;
    HKEY hkeyFrom = nullptr;
    auto result = RegOpenKeyExA(rootKey,
                                subKeyFrom.c_str(),
                                0,
                                KEY_READ | KEY_WOW64_64KEY,
                                &hkeyFrom);

    if (result == ERROR_SUCCESS) {
        std::string subKeyTo = REG_PREFIX "\\" + keyTo;
        HKEY hkeyTo = nullptr;
        result = RegCreateKeyExA(rootKey,
                                 subKeyTo.c_str(),
                                 0,
                                 nullptr,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_WRITE | KEY_WOW64_64KEY,
                                 nullptr,
                                 &hkeyTo,
                                 nullptr);

        if (result == ERROR_SUCCESS) {
            result = copyTree(hkeyFrom, nullptr, hkeyTo, KEY_WOW64_64KEY);

            if (result == ERROR_SUCCESS)
                ok = deleteKey(keyFrom, global);

            RegCloseKey(hkeyTo);
        }

        RegCloseKey(hkeyFrom);
    }

    return ok;
}

std::string AkVCam::Preferences::addDevice(const std::string &description,
                                           const std::string &deviceId)
{
    AkLogFunction();
    std::string id;

    if (deviceId.empty())
        id = createDeviceId();
    else if (!isDeviceIdTaken(deviceId))
        id = deviceId;

    if (id.empty())
        return {};

    bool ok = true;
    int cameraIndex = readInt("Cameras\\size", 0, true) + 1;
    ok &= write("Cameras\\size", cameraIndex, true);
    ok &= write("Cameras\\" + std::to_string(cameraIndex) + "\\description",
                description,
                true);
    ok &= write("Cameras\\" + std::to_string(cameraIndex) + "\\id", id, true);

    return ok? id: std::string();
}

std::string AkVCam::Preferences::addCamera(const std::string &description,
                                           const std::vector<VideoFormat> &formats)
{
    return addCamera("", description, formats);
}

std::string AkVCam::Preferences::addCamera(const std::string &deviceId,
                                           const std::string &description,
                                           const std::vector<VideoFormat> &formats)
{
    AkLogFunction();

    if (!deviceId.empty() && cameraExists(deviceId))
        return {};

    auto id = deviceId.empty()? createDeviceId(): deviceId;

    if (id.empty())
        return {};

    bool ok = true;
    int cameraIndex = readInt("Cameras\\", 0, true) + 1;
    ok &= write("Cameras\\size", cameraIndex, true);
    ok &= write("Cameras\\"
                + std::to_string(cameraIndex)
                + "\\description",
                description,
                true);
    ok &= write("Cameras\\"
                + std::to_string(cameraIndex)
                + "\\id",
                id,
                true);
    ok &= write("Cameras\\"
                + std::to_string(cameraIndex)
                + "\\Formats\\size",
                int(formats.size()),
                true);

    for (size_t i = 0; i < formats.size(); i++) {
        auto &format = formats[i];
        auto prefix = "Cameras\\"
                    + std::to_string(cameraIndex)
                    + "\\Formats\\"
                    + std::to_string(i + 1);
        auto formatStr = pixelFormatToCommonString(format.format());
        ok &= write(prefix + "\\format", formatStr, true);
        ok &= write(prefix + "\\width", format.width(), true);
        ok &= write(prefix + "\\height", format.height(), true);
        ok &= write(prefix + "\\fps",
                    format.fps().toString(),
                    true);
    }

    return ok? id: std::string();
}

bool AkVCam::Preferences::removeCamera(const std::string &deviceId)
{
    AkLogFunction();
    AkLogDebug("Device: %s", deviceId.c_str());
    int cameraIndex = cameraFromId(deviceId);

    if (cameraIndex < 0)
        return false;

    auto nCameras = camerasCount();
    bool ok = true;
    ok &= deleteKey("Cameras\\" + std::to_string(cameraIndex + 1) + '\\', true);

    for (auto i = size_t(cameraIndex + 1); i < nCameras; i++)
        ok &= move("Cameras\\" + std::to_string(i + 1),
                   "Cameras\\" + std::to_string(i),
                   true);

    ok &= deleteKey("Cameras\\" + std::to_string(nCameras) + '\\', true);

    if (nCameras > 1)
        ok &= write("Cameras\\size", int(nCameras - 1), true);
    else
        ok &= deleteKey("Cameras\\", true);

    return ok;
}

size_t AkVCam::Preferences::camerasCount()
{
    AkLogFunction();
    int nCameras = readInt("Cameras\\size", 0, true);
    AkLogDebug("Cameras: %d", nCameras);

    return size_t(nCameras);
}

int AkVCam::Preferences::cameraFromCLSID(const CLSID &clsid)
{
    AkLogFunction();
    AkLogDebug("CLSID: %s", stringFromIid(clsid).c_str());

    for (size_t i = 0; i < camerasCount(); i++) {
        auto cameraClsid = createClsidFromStr(cameraId(i));

        if (IsEqualCLSID(cameraClsid, clsid))
            return int(i);
    }

    return -1;
}

int AkVCam::Preferences::cameraFromId(const std::string &deviceId)
{
    for (size_t i = 0; i < camerasCount(); i++)
        if (cameraId(i) == deviceId)
            return int(i);

    return -1;
}

bool AkVCam::Preferences::cameraExists(const std::string &deviceId)
{
    for (DWORD i = 0; i < camerasCount(); i++)
        if (cameraId(i) == deviceId)
            return true;

    return false;
}

std::string AkVCam::Preferences::cameraDescription(size_t cameraIndex)
{
    if (cameraIndex >= camerasCount())
        return {};

    return readString("Cameras\\"
                      + std::to_string(cameraIndex + 1)
                      + "\\description",
                      {},
                      true);
}

bool AkVCam::Preferences::cameraSetDescription(size_t cameraIndex,
                                               const std::string &description)
{
    if (cameraIndex >= camerasCount())
        return false;

    return write("Cameras\\" + std::to_string(cameraIndex + 1) + "\\description",
                 description,
                 true);
}

std::string AkVCam::Preferences::cameraId(size_t cameraIndex)
{
    return readString("Cameras\\"
                      + std::to_string(cameraIndex + 1)
                      + "\\id",
                      {},
                      true);
}

size_t AkVCam::Preferences::formatsCount(size_t cameraIndex)
{
    return size_t(readInt("Cameras\\"
                          + std::to_string(cameraIndex + 1)
                          + "\\Formats\\size",
                          0,
                          true));
}

AkVCam::VideoFormat AkVCam::Preferences::cameraFormat(size_t cameraIndex,
                                                      size_t formatIndex)
{
    AkLogFunction();
    auto prefix = "Cameras\\"
                + std::to_string(cameraIndex + 1)
                + "\\Formats\\"
                + std::to_string(formatIndex + 1);
    auto format = readString(prefix + "\\format", {}, true);
    auto fourcc = pixelFormatFromCommonString(format);
    int width = readInt(prefix + "\\width", 0, true);
    int height = readInt(prefix + "\\height", 0, true);
    auto fps = Fraction(readString(prefix + "\\fps", {}, true));

    return VideoFormat(fourcc, width, height, {fps});
}

std::vector<AkVCam::VideoFormat> AkVCam::Preferences::cameraFormats(size_t cameraIndex)
{
    AkLogFunction();
    std::vector<AkVCam::VideoFormat> formats;

    for (size_t i = 0; i < formatsCount(cameraIndex); i++) {
        auto videoFormat = cameraFormat(cameraIndex, i);

        if (videoFormat)
            formats.push_back(videoFormat);
    }

    return formats;
}

bool AkVCam::Preferences::cameraSetFormats(size_t cameraIndex,
                                           const std::vector<AkVCam::VideoFormat> &formats)
{
    AkLogFunction();

    if (cameraIndex >= camerasCount())
        return false;

    bool ok = true;
    ok &= deleteKey("Cameras\\" + std::to_string(cameraIndex + 1) + "\\Formats\\",
                    true);
    ok &= write("Cameras\\"
                + std::to_string(cameraIndex + 1)
                + "\\Formats\\size",
                int(formats.size()),
                true);

    for (size_t i = 0; i < formats.size(); i++) {
        auto &format = formats[i];
        auto prefix = "Cameras\\"
                      + std::to_string(cameraIndex + 1)
                      + "\\Formats\\"
                      + std::to_string(i + 1);
        auto formatStr = pixelFormatToCommonString(format.format());
        ok &= write(prefix + "\\format", formatStr, true);
        ok &= write(prefix + "\\width", format.width(), true);
        ok &= write(prefix + "\\height", format.height(), true);
        ok &= write(prefix + "\\fps", format.fps().toString(), true);
    }

    return ok;
}

bool AkVCam::Preferences::cameraAddFormat(size_t cameraIndex,
                                          const AkVCam::VideoFormat &format,
                                          int index)
{
    AkLogFunction();
    auto formats = cameraFormats(cameraIndex);

    if ((index < 0) || (index > int(formats.size())))
        index = int(formats.size());

    bool ok = true;
    formats.insert(formats.begin() + index, format);
    ok &= write("Cameras\\"
                + std::to_string(cameraIndex + 1)
                + "\\Formats\\size",
                int(formats.size()),
                true);

    for (size_t i = 0; i < formats.size(); i++) {
        auto &format = formats[i];
        auto prefix = "Cameras\\"
                    + std::to_string(cameraIndex + 1)
                    + "\\Formats\\"
                    + std::to_string(i + 1);
        auto formatStr = pixelFormatToCommonString(format.format());
        ok &= write(prefix + "\\format", formatStr, true);
        ok &= write(prefix + "\\width", format.width(), true);
        ok &= write(prefix + "\\height", format.height(), true);
        ok &= write(prefix + "\\fps", format.fps().toString(), true);
    }

    return ok;
}

bool AkVCam::Preferences::cameraRemoveFormat(size_t cameraIndex, int index)
{
    AkLogFunction();
    auto formats = cameraFormats(cameraIndex);

    if (index < 0 || index >= int(formats.size()))
        return false;

    bool ok = true;
    formats.erase(formats.begin() + index);
    ok &= write("Cameras\\"
                + std::to_string(cameraIndex + 1)
                + "\\Formats\\size",
                int(formats.size()),
                true);

    for (size_t i = 0; i < formats.size(); i++) {
        auto &format = formats[i];
        auto prefix = "Cameras\\"
                    + std::to_string(cameraIndex)
                    + "\\Formats\\"
                    + std::to_string(i + 1);
        auto formatStr = pixelFormatToCommonString(format.format());
        ok &= write(prefix + "\\format", formatStr, true);
        ok &= write(prefix + "\\width", format.width(), true);
        ok &= write(prefix + "\\height", format.height(), true);
        ok &= write(prefix + "\\fps",
                    format.fps().toString(),
                    true);
    }

    return ok;
}

int AkVCam::Preferences::cameraControlValue(size_t cameraIndex,
                                            const std::string &key)
{
    return readInt("Cameras\\"
                   + std::to_string(cameraIndex + 1)
                   + "\\Controls\\"
                   + key);
}

bool AkVCam::Preferences::cameraSetControlValue(size_t cameraIndex,
                                                const std::string &key,
                                                int value)
{
    return write("Cameras\\"
                 + std::to_string(cameraIndex + 1)
                 + "\\Controls\\"
                 + key,
                 value);
}

bool AkVCam::Preferences::cameraDirectMode(const std::string &deviceId)
{
    int cameraIndex = cameraFromId(deviceId);

    if (cameraIndex < 0)
        return false;

    return cameraDirectMode(cameraIndex);
}

bool AkVCam::Preferences::cameraDirectMode(size_t cameraIndex)
{
    return readInt("Cameras\\"
                   + std::to_string(cameraIndex + 1)
                   + "\\directMode") > 0;
}

bool AkVCam::Preferences::setCameraDirectMode(const std::string &deviceId,
                                              bool directMode)
{
    int cameraIndex = cameraFromId(deviceId);

    if (cameraIndex < 0)
        return false;

    return write("Cameras\\"
                 + std::to_string(cameraIndex + 1)
                 + "\\directMode",
                 directMode? 1: 0);
}

std::string AkVCam::Preferences::picture()
{
    return readString("picture");
}

bool AkVCam::Preferences::setPicture(const std::string &picture)
{
    return write("picture", picture);
}

int AkVCam::Preferences::logLevel()
{
    return readInt("loglevel", AKVCAM_LOGLEVEL_DEFAULT, true);
}

bool AkVCam::Preferences::setLogLevel(int logLevel)
{
    return write("loglevel", logLevel, true);
}

int AkVCam::Preferences::servicePort()
{
    return readInt("servicePort", atoi(AKVCAM_SERVICEPORT_DEFAULT), true);
}

bool AkVCam::Preferences::setServicePort(int servicePort)
{
    return write("servicePort", servicePort, true);
}

int AkVCam::Preferences::serviceTimeout()
{
    return readInt("serviceTimeout", AKVCAM_SERVICETIMEOUT_DEFAULT, true);
}

bool AkVCam::Preferences::setServiceTimeout(int timeoutSecs)
{
    return write("serviceTimeout", timeoutSecs, true);
}

AkVCam::DataMode AkVCam::Preferences::dataMode()
{
    auto dataMode = readString("dataMode", "mmap", true);

    return dataMode == "sockets"? DataMode_Sockets: DataMode_SharedMemory;
}

bool AkVCam::Preferences::setDataMode(DataMode dataMode)
{
    return write("dataMode",
                 dataMode == DataMode_Sockets? "sockets": "mmap",
                 true);
}

size_t AkVCam::Preferences::pageSize()
{
    static const int64_t defaultPageSize = 4L * 1920L * 1280L;

    return readInt64("pageSize", defaultPageSize, true);
}

bool AkVCam::Preferences::setPageSize(size_t pageSize)
{
    return write("pageSize", static_cast<int64_t>(pageSize));
}

void AkVCam::Preferences::splitSubKey(const std::string &key,
                                      std::string &subKey,
                                      std::string &value)
{
    subKey = REG_PREFIX;
    auto separator = key.rfind('\\');

    if (separator == std::string::npos) {
        value = key;
    } else {
        subKey += '\\' + key.substr(0, separator);

        if (separator + 1 < key.size())
            value = key.substr(separator + 1);
    }
}

bool AkVCam::Preferences::readValue(const std::string &key,
                                    DWORD dataTypeFlags,
                                    PVOID data,
                                    LPDWORD dataSize,
                                    bool global)
{
    AkLogFunction();
    HKEY rootKey = global? HKEY_LOCAL_MACHINE: HKEY_CURRENT_USER;
    std::string subKey;
    std::string val;
    splitSubKey(key, subKey, val);
    AkLogDebug("SubKey: %s", subKey.c_str());
    AkLogDebug("Value: %s", val.c_str());
    HKEY hkey = nullptr;
    auto result = RegOpenKeyExA(rootKey,
                                subKey.c_str(),
                                0,
                                KEY_READ | KEY_WOW64_64KEY,
                                &hkey);

    if (result != ERROR_SUCCESS)
        return false;

    result = RegGetValueA(hkey,
                          nullptr,
                          val.c_str(),
                          dataTypeFlags,
                          nullptr,
                          data,
                          dataSize);
    RegCloseKey(hkey);

    return result == ERROR_SUCCESS;
}

bool AkVCam::Preferences::setValue(const std::string &key,
                                   DWORD dataType,
                                   LPCSTR data,
                                   DWORD dataSize,
                                   bool global)
{
    AkLogFunction();
    HKEY rootKey = global? HKEY_LOCAL_MACHINE: HKEY_CURRENT_USER;
    std::string subKey;
    std::string val;
    splitSubKey(key, subKey, val);
    AkLogDebug("SubKey: %s", subKey.c_str());
    AkLogDebug("Value: %s", val.c_str());
    HKEY hkey = nullptr;
    LONG result = RegCreateKeyExA(rootKey,
                                  subKey.c_str(),
                                  0,
                                  nullptr,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_WRITE | KEY_WOW64_64KEY,
                                  nullptr,
                                  &hkey,
                                  nullptr);

    if (result != ERROR_SUCCESS)
        return false;

    result = RegSetValueExA(hkey,
                            val.c_str(),
                            0,
                            dataType,
                            reinterpret_cast<CONST BYTE *>(data),
                            dataSize);
    RegCloseKey(hkey);

    return result == ERROR_SUCCESS;
}
