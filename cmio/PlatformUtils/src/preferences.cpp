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

#include "preferences.h"
#include "utils.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/logger.h"

#define PREFERENCES_ID CFSTR(CMIO_ASSISTANT_NAME)

std::vector<std::string> AkVCam::Preferences::keys()
{
    AkLogFunction();
    std::vector<std::string> keys;

    auto cfKeys = CFPreferencesCopyKeyList(PREFERENCES_ID,
                                           kCFPreferencesCurrentUser,
                                           kCFPreferencesAnyHost);

    if (cfKeys) {
        auto size = CFArrayGetCount(cfKeys);

        for (CFIndex i = 0; i < size; i++) {
            auto key = CFStringRef(CFArrayGetValueAtIndex(cfKeys, i));
            keys.push_back(stringFromCFType(key));
        }

        CFRelease(cfKeys);
    }

    AkLogInfo() << "Keys: " << keys.size() << std::endl;
    std::sort(keys.begin(), keys.end());

    for (auto &key: keys)
        AkLogInfo() << "    " << key << std::endl;

    return keys;
}

void AkVCam::Preferences::write(const std::string &key,
                                const std::shared_ptr<CFTypeRef> &value)
{
    AkLogFunction();
    AkLogInfo() << "Writing: " << key << " = " << *value << std::endl;
    auto cfKey = cfTypeFromStd(key);
    CFPreferencesSetValue(CFStringRef(*cfKey),
                          *value,
                          PREFERENCES_ID,
                          kCFPreferencesCurrentUser,
                          kCFPreferencesAnyHost);
}

void AkVCam::Preferences::write(const std::string &key,
                                const std::string &value)
{
    AkLogFunction();
    AkLogInfo() << "Writing: " << key << " = " << value << std::endl;
    auto cfKey = cfTypeFromStd(key);
    auto cfValue = cfTypeFromStd(value);
    CFPreferencesSetValue(CFStringRef(*cfKey),
                          *cfValue,
                          PREFERENCES_ID,
                          kCFPreferencesCurrentUser,
                          kCFPreferencesAnyHost);
}

void AkVCam::Preferences::write(const std::string &key, int value)
{
    AkLogFunction();
    AkLogInfo() << "Writing: " << key << " = " << value << std::endl;
    auto cfKey = cfTypeFromStd(key);
    auto cfValue = cfTypeFromStd(value);
    CFPreferencesSetValue(CFStringRef(*cfKey),
                          *cfValue,
                          PREFERENCES_ID,
                          kCFPreferencesCurrentUser,
                          kCFPreferencesAnyHost);
}

void AkVCam::Preferences::write(const std::string &key, double value)
{
    AkLogFunction();
    AkLogInfo() << "Writing: " << key << " = " << value << std::endl;
    auto cfKey = cfTypeFromStd(key);
    auto cfValue = cfTypeFromStd(value);
    CFPreferencesSetValue(CFStringRef(*cfKey),
                          *cfValue,
                          PREFERENCES_ID,
                          kCFPreferencesCurrentUser,
                          kCFPreferencesAnyHost);
}

void AkVCam::Preferences::write(const std::string &key,
                                std::vector<std::string> &value)
{
    AkLogFunction();
    write(key, join(value, ","));
}

std::shared_ptr<CFTypeRef> AkVCam::Preferences::read(const std::string &key)
{
    AkLogFunction();
    auto cfKey = cfTypeFromStd(key);
    auto cfValue = CFTypeRef(CFPreferencesCopyValue(CFStringRef(*cfKey),
                                                    PREFERENCES_ID,
                                                    kCFPreferencesCurrentUser,
                                                    kCFPreferencesAnyHost));

    if (!cfValue)
        return {};

    return std::shared_ptr<CFTypeRef>(new CFTypeRef(cfValue),
                                      [] (CFTypeRef *ptr) {
                                          CFRelease(*ptr);
                                          delete ptr;
                                      });
}

std::string AkVCam::Preferences::readString(const std::string &key,
                                            const std::string &defaultValue)
{
    AkLogFunction();
    auto cfKey = cfTypeFromStd(key);
    auto cfValue =
            CFStringRef(CFPreferencesCopyValue(CFStringRef(*cfKey),
                                               PREFERENCES_ID,
                                               kCFPreferencesCurrentUser,
                                               kCFPreferencesAnyHost));
    auto value = defaultValue;

    if (cfValue) {
        value = stringFromCFType(cfValue);
        CFRelease(cfValue);
    }

    return value;
}

int AkVCam::Preferences::readInt(const std::string &key, int defaultValue)
{
    AkLogFunction();
    auto cfKey = cfTypeFromStd(key);
    auto cfValue =
            CFNumberRef(CFPreferencesCopyValue(CFStringRef(*cfKey),
                                               PREFERENCES_ID,
                                               kCFPreferencesCurrentUser,
                                               kCFPreferencesAnyHost));
    auto value = defaultValue;

    if (cfValue) {
        CFNumberGetValue(cfValue, kCFNumberIntType, &value);
        CFRelease(cfValue);
    }

    return value;
}

double AkVCam::Preferences::readDouble(const std::string &key,
                                       double defaultValue)
{
    AkLogFunction();
    auto cfKey = cfTypeFromStd(key);
    auto cfValue =
            CFNumberRef(CFPreferencesCopyValue(CFStringRef(*cfKey),
                                               PREFERENCES_ID,
                                               kCFPreferencesCurrentUser,
                                               kCFPreferencesAnyHost));
    auto value = defaultValue;

    if (cfValue) {
        CFNumberGetValue(cfValue, kCFNumberDoubleType, &value);
        CFRelease(cfValue);
    }

    return value;
}

bool AkVCam::Preferences::readBool(const std::string &key, bool defaultValue)
{
    AkLogFunction();
    auto cfKey = cfTypeFromStd(key);
    auto cfValue =
            CFBooleanRef(CFPreferencesCopyValue(CFStringRef(*cfKey),
                                                PREFERENCES_ID,
                                                kCFPreferencesCurrentUser,
                                                kCFPreferencesAnyHost));
    auto value = defaultValue;

    if (cfValue) {
        value = CFBooleanGetValue(cfValue);
        CFRelease(cfValue);
    }

    return value;
}

std::vector<std::string> AkVCam::Preferences::readStringList(const std::string &key,
                                                             const std::vector<std::string> &defaultValue)
{
    auto value = defaultValue;

    for (auto &str: split(readString(key), ','))
        value.push_back(trimmed(str));

    return value;
}

void AkVCam::Preferences::deleteKey(const std::string &key)
{
    AkLogFunction();
    AkLogInfo() << "Deleting " << key << std::endl;
    auto cfKey = cfTypeFromStd(key);
    CFPreferencesSetValue(CFStringRef(*cfKey),
                          nullptr,
                          PREFERENCES_ID,
                          kCFPreferencesCurrentUser,
                          kCFPreferencesAnyHost);
}

void AkVCam::Preferences::deleteAllKeys(const std::string &key)
{
    AkLogFunction();
    AkLogInfo() << "Key: " << key << std::endl;

    for (auto &key_: keys())
        if (key_.size() >= key.size() && key_.substr(0, key.size()) == key)
            deleteKey(key_);
}

void AkVCam::Preferences::move(const std::string &keyFrom,
                               const std::string &keyTo)
{
    AkLogFunction();
    AkLogInfo() << "From: " << keyFrom << std::endl;
    AkLogInfo() << "To: " << keyTo << std::endl;
    auto value = read(keyFrom);

    if (!value)
        return;

    write(keyTo, value);
    deleteKey(keyFrom);
}

void AkVCam::Preferences::moveAll(const std::string &keyFrom,
                                  const std::string &keyTo)
{
    AkLogFunction();
    AkLogInfo() << "From: " << keyFrom << std::endl;
    AkLogInfo() << "To: " << keyTo << std::endl;

    for (auto &key: keys())
        if (key.size() >= keyFrom.size()
            && key.substr(0, keyFrom.size()) == keyFrom) {
            if (key.size() == keyFrom.size())
                move(key, keyTo);
            else
                move(key, keyTo + key.substr(keyFrom.size()));
        }
}

void AkVCam::Preferences::sync()
{
    AkLogFunction();
    CFPreferencesSynchronize(PREFERENCES_ID,
                             kCFPreferencesCurrentUser,
                             kCFPreferencesAnyHost);
}

std::string AkVCam::Preferences::addDevice(const std::string &description,
                                           const std::string &deviceId)
{
    AkLogFunction();
    std::string id;

    if (deviceId.empty())
        id = createDeviceId();
    else if (!idDeviceIdTaken(deviceId))
        id = deviceId;

    if (id.empty())
        return {};

    int cameraIndex = readInt("cameras");
    write("cameras", cameraIndex + 1);
    write("cameras."
          + std::to_string(cameraIndex)
          + ".description",
          description);
    write("cameras."
          + std::to_string(cameraIndex)
          + ".id",
          id);
    sync();

    return id;
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
    int cameraIndex = readInt("cameras");
    write("cameras", cameraIndex + 1);
    write("cameras."
          + std::to_string(cameraIndex)
          + ".description",
          description);
    write("cameras."
          + std::to_string(cameraIndex)
          + ".id",
          id);
    write("cameras."
          + std::to_string(cameraIndex)
          + ".formats",
          int(formats.size()));

    for (size_t i = 0; i < formats.size(); i++) {
        auto &format = formats[i];
        auto prefix = "cameras."
                    + std::to_string(cameraIndex)
                    + ".formats."
                    + std::to_string(i);
        auto formatStr = VideoFormat::stringFromFourcc(format.fourcc());
        write(prefix + ".format", formatStr);
        write(prefix + ".width", format.width());
        write(prefix + ".height", format.height());
        write(prefix + ".fps", format.minimumFrameRate().toString());
    }

    sync();

    return id;
}

void AkVCam::Preferences::removeCamera(const std::string &deviceId)
{
    AkLogFunction();
    AkLogInfo() << "Device: " << deviceId << std::endl;
    int cameraIndex = cameraFromId(deviceId);

    if (cameraIndex < 0)
        return;

    cameraSetFormats(size_t(cameraIndex), {});

    auto nCameras = camerasCount();
    deleteAllKeys("cameras." + std::to_string(cameraIndex));

    for (auto i = size_t(cameraIndex + 1); i < nCameras; i++)
        moveAll("cameras." + std::to_string(i),
                           "cameras." + std::to_string(i - 1));

    if (nCameras > 1)
        write("cameras", int(nCameras - 1));
    else
        deleteKey("cameras");

    sync();
}

size_t AkVCam::Preferences::camerasCount()
{
    AkLogFunction();
    int nCameras = readInt("cameras");
    AkLogInfo() << "Cameras: " << nCameras << std::endl;

    return size_t(nCameras);
}

bool AkVCam::Preferences::idDeviceIdTaken(const std::string &deviceId)
{
    AkLogFunction();

    // List device IDs in use.
    std::vector<std::string> cameraIds;

    for (size_t i = 0; i < camerasCount(); i++)
        cameraIds.push_back(cameraId(i));

    return std::find(cameraIds.begin(),
                     cameraIds.end(),
                     deviceId) != cameraIds.end();
}

std::string AkVCam::Preferences::createDeviceId()
{
    AkLogFunction();

    // List device IDs in use.
    std::vector<std::string> cameraIds;

    for (size_t i = 0; i < camerasCount(); i++)
        cameraIds.push_back(cameraId(i));

    const int maxId = 64;

    for (int i = 0; i < maxId; i++) {
        /* There are no rules for device IDs in Mac. Just append an
         * incremental index to a common prefix.
         */
        auto id = CMIO_PLUGIN_DEVICE_PREFIX + std::to_string(i);

        // Check if the ID is being used, if not return it.
        if (std::find(cameraIds.begin(),
                      cameraIds.end(),
                      id) == cameraIds.end())
            return id;
    }

    return {};
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
    for (size_t i = 0; i < camerasCount(); i++)
        if (cameraId(i) == deviceId)
            return true;

    return false;
}

std::string AkVCam::Preferences::cameraDescription(size_t cameraIndex)
{
    if (cameraIndex >= camerasCount())
        return {};

    return readString("cameras."
                      + std::to_string(cameraIndex)
                      + ".description");
}

void AkVCam::Preferences::cameraSetDescription(size_t cameraIndex,
                                               const std::string &description)
{
    if (cameraIndex >= camerasCount())
        return;

    write("cameras." + std::to_string(cameraIndex) + ".description",
          description);
    sync();
}

std::string AkVCam::Preferences::cameraId(size_t cameraIndex)
{
    return readString("cameras."
                      + std::to_string(cameraIndex)
                      + ".id");
}

size_t AkVCam::Preferences::formatsCount(size_t cameraIndex)
{
    return size_t(readInt("cameras."
                          + std::to_string(cameraIndex)
                          + ".formats"));
}

AkVCam::VideoFormat AkVCam::Preferences::cameraFormat(size_t cameraIndex,
                                                      size_t formatIndex)
{
    AkLogFunction();
    auto prefix = "cameras."
                + std::to_string(cameraIndex)
                + ".formats."
                + std::to_string(formatIndex);
    auto format = readString(prefix + ".format");
    auto fourcc = VideoFormat::fourccFromString(format);
    int width = readInt(prefix + ".width");
    int height = readInt(prefix + ".height");
    auto fps = Fraction(readString(prefix + ".fps"));

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

    write("cameras."
              + std::to_string(cameraIndex)
              + ".formats",
          int(formats.size()));

    for (size_t i = 0; i < formats.size(); i++) {
        auto &format = formats[i];
        auto prefix = "cameras."
                      + std::to_string(cameraIndex)
                      + ".formats."
                      + std::to_string(i);
        auto formatStr = VideoFormat::stringFromFourcc(format.fourcc());
        write(prefix + ".format", formatStr);
        write(prefix + ".width", format.width());
        write(prefix + ".height", format.height());
        write(prefix + ".fps", format.minimumFrameRate().toString());
    }

    sync();
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
    write("cameras."
          + std::to_string(cameraIndex)
          + ".formats",
          int(formats.size()));

    for (size_t i = 0; i < formats.size(); i++) {
        auto &format = formats[i];
        auto prefix = "cameras."
                    + std::to_string(cameraIndex)
                    + ".formats."
                    + std::to_string(i);
        auto formatStr = VideoFormat::stringFromFourcc(format.fourcc());
        write(prefix + ".format", formatStr);
        write(prefix + ".width", format.width());
        write(prefix + ".height", format.height());
        write(prefix + ".fps", format.minimumFrameRate().toString());
    }

    sync();
}

void AkVCam::Preferences::cameraRemoveFormat(size_t cameraIndex, int index)
{
    AkLogFunction();
    auto formats = cameraFormats(cameraIndex);

    if (index < 0 || index >= int(formats.size()))
        return;

    formats.erase(formats.begin() + index);

    write("cameras."
          + std::to_string(cameraIndex)
          + ".formats",
          int(formats.size()));

    for (size_t i = 0; i < formats.size(); i++) {
        auto &format = formats[i];
        auto prefix = "cameras."
                    + std::to_string(cameraIndex)
                    + ".formats."
                    + std::to_string(i);
        auto formatStr = VideoFormat::stringFromFourcc(format.fourcc());
        write(prefix + ".format", formatStr);
        write(prefix + ".width", format.width());
        write(prefix + ".height", format.height());
        write(prefix + ".fps", format.minimumFrameRate().toString());
    }

    sync();
}

int AkVCam::Preferences::cameraControlValue(size_t cameraIndex,
                                            const std::string &key)
{
    return readInt("cameras." + std::to_string(cameraIndex) + ".controls." + key);
}

void AkVCam::Preferences::cameraSetControlValue(size_t cameraIndex,
                                                const std::string &key,
                                                int value)
{
    write("cameras." + std::to_string(cameraIndex) + ".controls." + key, value);
    sync();
}

std::string AkVCam::Preferences::picture()
{
    return readString("picture");
}

void AkVCam::Preferences::setPicture(const std::string &picture)
{
    write("picture", picture);
    sync();
}

int AkVCam::Preferences::logLevel()
{
    return readInt("loglevel", AKVCAM_LOGLEVEL_DEFAULT);
}

void AkVCam::Preferences::setLogLevel(int logLevel)
{
    write("loglevel", logLevel);
    sync();
}
