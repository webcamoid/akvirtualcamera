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
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/logger.h"

#define REG_PREFIX "SOFTWARE\\Webcamoid\\VirtualCamera\\"

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
                       LPDWORD dataSize);
        void setValue(const std::string &key,
                      DWORD dataType,
                      LPCSTR data,
                      DWORD dataSize);
    }
}

void AkVCam::Preferences::write(const std::string &key,
                                const std::string &value)
{
    AkLogFunction();
    AkLogInfo() << "Writing: " << key << " = " << value << std::endl;
    setValue(key, REG_SZ, value.c_str(), DWORD(value.size()));
}

void AkVCam::Preferences::write(const std::string &key, int value)
{
    AkLogFunction();
    AkLogInfo() << "Writing: " << key << " = " << value << std::endl;
    setValue(key,
             REG_DWORD,
             reinterpret_cast<const char *>(&value),
             DWORD(sizeof(int)));
}

void AkVCam::Preferences::write(const std::string &key, double value)
{
    AkLogFunction();
    AkLogInfo() << "Writing: " << key << " = " << value << std::endl;
    auto val = std::to_string(value);
    setValue(key,
             REG_SZ,
             val.c_str(),
             DWORD(val.size()));
}

void AkVCam::Preferences::write(const std::string &key,
                                std::vector<std::string> &value)
{
    AkLogFunction();
    write(key, join(value, ","));
}

std::string AkVCam::Preferences::readString(const std::string &key,
                                            const std::string &defaultValue)
{
    AkLogFunction();
    char value[MAX_PATH];
    memset(value, 0, MAX_PATH * sizeof(char));
    DWORD valueSize = MAX_PATH;

    if (!readValue(key, RRF_RT_REG_SZ, &value, &valueSize))
        return defaultValue;

    return {value};
}

int AkVCam::Preferences::readInt(const std::string &key, int defaultValue)
{
    AkLogFunction();
    DWORD value = 0;
    DWORD valueSize = sizeof(DWORD);

    if (!readValue(key, RRF_RT_REG_DWORD, &value, &valueSize))
        return defaultValue;

    return int(value);
}

double AkVCam::Preferences::readDouble(const std::string &key,
                                       double defaultValue)
{
    AkLogFunction();
    auto value = readString(key, std::to_string(defaultValue));
    std::string::size_type sz;

    return std::stod(value, &sz);
}

bool AkVCam::Preferences::readBool(const std::string &key, bool defaultValue)
{
    AkLogFunction();

    return readInt(key, defaultValue) != 0;
}

std::vector<CLSID> AkVCam::Preferences::listRegisteredCameras(HINSTANCE hinstDLL)
{
    WCHAR *strIID = nullptr;
    StringFromIID(CLSID_VideoInputDeviceCategory, &strIID);

    std::wstringstream ss;
    ss << L"CLSID\\"
       << strIID
       << L"\\Instance";
    CoTaskMemFree(strIID);

    HKEY key = nullptr;
    auto result = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                                ss.str().c_str(),
                                0,
                                MAXIMUM_ALLOWED,
                                &key);

    if (result != ERROR_SUCCESS)
        return {};

    DWORD subkeys = 0;

    result = RegQueryInfoKey(key,
                             nullptr,
                             nullptr,
                             nullptr,
                             &subkeys,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr,
                             nullptr);

    if (result != ERROR_SUCCESS) {
        RegCloseKey(key);

        return {};
    }

    std::vector<CLSID> cameras;
    FILETIME lastWrite;

    for (DWORD i = 0; i < subkeys; i++) {
        WCHAR subKey[MAX_PATH];
        memset(subKey, 0, MAX_PATH * sizeof(WCHAR));
        DWORD subKeyLen = MAX_PATH;
        result = RegEnumKeyExW(key,
                               i,
                               subKey,
                               &subKeyLen,
                               nullptr,
                               nullptr,
                               nullptr,
                               &lastWrite);

        if (result != ERROR_SUCCESS)
            continue;

        std::wstringstream ss;
        ss << L"CLSID\\" << subKey << L"\\InprocServer32";
        WCHAR path[MAX_PATH];
        memset(path, 0, MAX_PATH * sizeof(WCHAR));
        DWORD pathSize = MAX_PATH;

        if (RegGetValueW(HKEY_CLASSES_ROOT,
                         ss.str().c_str(),
                         nullptr,
                         RRF_RT_REG_SZ,
                         nullptr,
                         path,
                         &pathSize) == ERROR_SUCCESS) {
            WCHAR modulePath[MAX_PATH];
            memset(modulePath, 0, MAX_PATH * sizeof(WCHAR));
            GetModuleFileNameW(hinstDLL, modulePath, MAX_PATH);

            if (!lstrcmpiW(path, modulePath)) {
                CLSID clsid;
                memset(&clsid, 0, sizeof(CLSID));
                CLSIDFromString(subKey, &clsid);
                cameras.push_back(clsid);
            }
        }
    }

    RegCloseKey(key);

    return cameras;
}

void AkVCam::Preferences::deleteKey(const std::string &key)
{
    AkLogFunction();
    AkLogInfo() << "Deleting " << key << std::endl;
    std::string subKey;
    std::string val;
    splitSubKey(key, subKey, val);

    if (val.empty()) {
        deleteTree(HKEY_LOCAL_MACHINE, subKey.c_str(), KEY_WOW64_64KEY);
    } else {
        HKEY hkey = nullptr;
        auto result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                    subKey.c_str(),
                                    0,
                                    KEY_ALL_ACCESS | KEY_WOW64_64KEY,
                                    &hkey);

        if (result == ERROR_SUCCESS) {
            RegDeleteValueA(hkey, val.c_str());
            RegCloseKey(hkey);
        }
    }
}

void AkVCam::Preferences::move(const std::string &keyFrom,
                               const std::string &keyTo)
{
    AkLogFunction();
    AkLogInfo() << "From: " << keyFrom << std::endl;
    AkLogInfo() << "To: " << keyTo << std::endl;

    std::string subKeyFrom = REG_PREFIX "\\" + keyFrom;
    HKEY hkeyFrom = nullptr;
    auto result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                subKeyFrom.c_str(),
                                0,
                                KEY_READ | KEY_WOW64_64KEY,
                                &hkeyFrom);

    if (result == ERROR_SUCCESS) {
        std::string subKeyTo = REG_PREFIX "\\" + keyTo;
        HKEY hkeyTo = nullptr;
        result = RegCreateKeyExA(HKEY_LOCAL_MACHINE,
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
                deleteKey(keyFrom);

            RegCloseKey(hkeyTo);
        }

        RegCloseKey(hkeyFrom);
    }
}

std::string AkVCam::Preferences::addDevice(const std::string &description)
{
    AkLogFunction();
    auto path = createDevicePath();
    int cameraIndex = readInt("Cameras\\size") + 1;
    write("Cameras\\size", cameraIndex);
    write("Cameras\\" + std::to_string(cameraIndex) + "\\description",
          description);
    write("Cameras\\" + std::to_string(cameraIndex) + "\\path", path);

    return path;
}

std::string AkVCam::Preferences::addCamera(const std::string &description,
                                           const std::vector<VideoFormat> &formats)
{
    return addCamera("", description, formats);
}

std::string AkVCam::Preferences::addCamera(const std::string &path,
                                           const std::string &description,
                                           const std::vector<VideoFormat> &formats)
{
    AkLogFunction();

    if (!path.empty() && cameraExists(path))
        return {};

    auto path_ = path.empty()? createDevicePath(): path;
    int cameraIndex = readInt("Cameras\\") + 1;
    write("Cameras\\size", cameraIndex);
    write("Cameras\\"
          + std::to_string(cameraIndex)
          + "\\description",
          description);
    write("Cameras\\"
          + std::to_string(cameraIndex)
          + "\\path",
          path_);
    write("Cameras\\"
          + std::to_string(cameraIndex)
          + "\\Formats\\size",
          int(formats.size()));

    for (size_t i = 0; i < formats.size(); i++) {
        auto &format = formats[i];
        auto prefix = "Cameras\\"
                    + std::to_string(cameraIndex)
                    + "\\Formats\\"
                    + std::to_string(i + 1);
        auto formatStr = VideoFormat::stringFromFourcc(format.fourcc());
        write(prefix + "\\format", formatStr);
        write(prefix + "\\width", format.width());
        write(prefix + "\\height", format.height());
        write(prefix + "\\fps", format.minimumFrameRate().toString());
    }

    return path_;
}

void AkVCam::Preferences::removeCamera(const std::string &path)
{
    AkLogFunction();
    AkLogInfo() << "Device: " << path << std::endl;
    int cameraIndex = cameraFromPath(path);

    if (cameraIndex < 0)
        return;

    cameraSetFormats(size_t(cameraIndex), {});

    auto nCameras = camerasCount();
    deleteKey("Cameras\\" + std::to_string(cameraIndex + 1) + '\\');

    for (auto i = size_t(cameraIndex + 1); i < nCameras; i++)
        move("Cameras\\" + std::to_string(i + 1),
             "Cameras\\" + std::to_string(i));

    if (nCameras > 1)
        write("Cameras\\size", int(nCameras - 1));
    else
        deleteKey("Cameras\\");
}

size_t AkVCam::Preferences::camerasCount()
{
    AkLogFunction();
    int nCameras = readInt("Cameras\\size");
    AkLogInfo() << "Cameras: " << nCameras << std::endl;

    return size_t(nCameras);
}

std::string AkVCam::Preferences::createDevicePath()
{
    AkLogFunction();

    // List device paths in use.
    std::vector<std::string> cameraPaths;

    for (size_t i = 0; i < camerasCount(); i++)
        cameraPaths.push_back(cameraPath(i));

    const int maxId = 64;

    for (int i = 0; i < maxId; i++) {
        /* There are no rules for device paths in Windows. Just append an
         * incremental index to a common prefix.
         */
        auto path = DSHOW_PLUGIN_DEVICE_PREFIX + std::to_string(i);

        // Check if the path is being used, if not return it.
        if (std::find(cameraPaths.begin(),
                      cameraPaths.end(),
                      path) == cameraPaths.end())
            return path;
    }

    return {};
}

int AkVCam::Preferences::cameraFromCLSID(const CLSID &clsid)
{
    for (DWORD i = 0; i < camerasCount(); i++) {
        auto cameraClsid = createClsidFromStr(cameraPath(i));

        if (IsEqualCLSID(cameraClsid, clsid))
            return int(i);
    }

    return -1;
}

int AkVCam::Preferences::cameraFromPath(const std::string &path)
{
    for (size_t i = 0; i < camerasCount(); i++)
        if (cameraPath(i) == path)
            return int(i);

    return -1;
}

bool AkVCam::Preferences::cameraExists(const std::string &path)
{
    for (DWORD i = 0; i < camerasCount(); i++)
        if (cameraPath(i) == path)
            return true;

    return false;
}

std::string AkVCam::Preferences::cameraDescription(size_t cameraIndex)
{
    if (cameraIndex >= camerasCount())
        return {};

    return readString("Cameras\\"
                      + std::to_string(cameraIndex + 1)
                      + "\\description");
}

void AkVCam::Preferences::cameraSetDescription(size_t cameraIndex,
                                               const std::string &description)
{
    if (cameraIndex >= camerasCount())
        return;

    write("Cameras\\" + std::to_string(cameraIndex + 1) + "\\description",
          description);
}

std::string AkVCam::Preferences::cameraPath(size_t cameraIndex)
{
    return readString("Cameras\\"
                      + std::to_string(cameraIndex + 1)
                      + "\\path");
}

size_t AkVCam::Preferences::formatsCount(size_t cameraIndex)
{
    return size_t(readInt("Cameras\\"
                          + std::to_string(cameraIndex + 1)
                          + "\\Formats\\size"));
}

AkVCam::VideoFormat AkVCam::Preferences::cameraFormat(size_t cameraIndex,
                                                      size_t formatIndex)
{
    AkLogFunction();
    auto prefix = "Cameras\\"
                + std::to_string(cameraIndex + 1)
                + "\\Formats\\"
                + std::to_string(formatIndex + 1);
    auto format = readString(prefix + "\\format");
    auto fourcc = VideoFormat::fourccFromString(format);
    int width = readInt(prefix + "\\width");
    int height = readInt(prefix + "\\height");
    auto fps = Fraction(readString(prefix + "\\fps"));

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

void AkVCam::Preferences::cameraSetFormats(size_t cameraIndex,
                                           const std::vector<AkVCam::VideoFormat> &formats)
{
    AkLogFunction();

    if (cameraIndex >= camerasCount())
        return;

    write("Cameras\\"
          + std::to_string(cameraIndex + 1)
          + "\\Formats\\size",
          int(formats.size()));

    for (size_t i = 0; i < formats.size(); i++) {
        auto &format = formats[i];
        auto prefix = "Cameras\\"
                      + std::to_string(cameraIndex + 1)
                      + "\\Formats\\"
                      + std::to_string(i + 1);
        auto formatStr = VideoFormat::stringFromFourcc(format.fourcc());
        write(prefix + "\\format", formatStr);
        write(prefix + "\\width", format.width());
        write(prefix + "\\height", format.height());
        write(prefix + "\\fps", format.minimumFrameRate().toString());
    }
}

void AkVCam::Preferences::cameraAddFormat(size_t cameraIndex,
                                          const AkVCam::VideoFormat &format,
                                          int index)
{
    AkLogFunction();
    auto formats = cameraFormats(cameraIndex);

    if (index < 0 || index > int(formats.size()))
        index = int(formats.size());

    formats.insert(formats.begin() + index, format);
    write("Cameras\\"
          + std::to_string(cameraIndex + 1)
          + "\\Formats\\size",
          int(formats.size()));

    for (size_t i = 0; i < formats.size(); i++) {
        auto &format = formats[i];
        auto prefix = "Cameras\\"
                    + std::to_string(cameraIndex + 1)
                    + "\\Formats\\"
                    + std::to_string(i + 1);
        auto formatStr = VideoFormat::stringFromFourcc(format.fourcc());
        write(prefix + "\\format", formatStr);
        write(prefix + "\\width", format.width());
        write(prefix + "\\height", format.height());
        write(prefix + "\\fps", format.minimumFrameRate().toString());
    }
}

void AkVCam::Preferences::cameraRemoveFormat(size_t cameraIndex, int index)
{
    AkLogFunction();
    auto formats = cameraFormats(cameraIndex);

    if (index < 0 || index >= int(formats.size()))
        return;

    formats.erase(formats.begin() + index);
    write("Cameras\\"
          + std::to_string(cameraIndex + 1)
          + "\\Formats\\size",
          int(formats.size()));

    for (size_t i = 0; i < formats.size(); i++) {
        auto &format = formats[i];
        auto prefix = "Cameras\\"
                    + std::to_string(cameraIndex)
                    + "\\Formats\\"
                    + std::to_string(i + 1);
        auto formatStr = VideoFormat::stringFromFourcc(format.fourcc());
        write(prefix + "\\format", formatStr);
        write(prefix + "\\width", format.width());
        write(prefix + "\\height", format.height());
        write(prefix + "\\fps", format.minimumFrameRate().toString());
    }
}

int AkVCam::Preferences::cameraControlValue(size_t cameraIndex,
                                            const std::string &key)
{
    return readInt("Cameras\\"
                   + std::to_string(cameraIndex + 1)
                   + "\\Controls\\"
                   + key);
}

void AkVCam::Preferences::cameraSetControlValue(size_t cameraIndex,
                                                const std::string &key,
                                                int value)
{
    write("Cameras\\"
          + std::to_string(cameraIndex + 1)
          + "\\Controls\\"
          + key,
          value);
}

std::string AkVCam::Preferences::picture()
{
    return readString("picture");
}

void AkVCam::Preferences::setPicture(const std::string &picture)
{
    write("picture", picture);
}

int AkVCam::Preferences::logLevel()
{
    return readInt("loglevel", AKVCAM_LOGLEVEL_DEFAULT);
}

void AkVCam::Preferences::setLogLevel(int logLevel)
{
    write("loglevel", logLevel);
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
                                    LPDWORD dataSize)
{
    std::string subKey;
    std::string val;
    splitSubKey(key, subKey, val);
    HKEY hkey = nullptr;
    auto result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
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

void AkVCam::Preferences::setValue(const std::string &key,
                                   DWORD dataType,
                                   LPCSTR data,
                                   DWORD dataSize)
{
    std::string subKey;
    std::string val;
    splitSubKey(key, subKey, val);
    HKEY hkey = nullptr;
    LONG result = RegCreateKeyExA(HKEY_LOCAL_MACHINE,
                                  subKey.c_str(),
                                  0,
                                  nullptr,
                                  REG_OPTION_NON_VOLATILE,
                                  KEY_WRITE | KEY_WOW64_64KEY,
                                  nullptr,
                                  &hkey,
                                  nullptr);

    if (result != ERROR_SUCCESS)
        return;

    RegSetValueA(hkey,
                 val.c_str(),
                 dataType,
                 data,
                 dataSize);
    RegCloseKey(hkey);
}
