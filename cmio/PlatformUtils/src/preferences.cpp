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
#include <cstring>
#include <CoreFoundation/CoreFoundation.h>

#include "preferences.h"
#include "utils.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/logger.h"

#define PREFERENCES_ID CFSTR(CMIO_ASSISTANT_NAME)
#define AKVCAM_SERVICETIMEOUT_DEFAULT 10

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
            std::string key;
            auto cfKey = CFStringRef(CFArrayGetValueAtIndex(cfKeys, i));
            auto len = size_t(CFStringGetLength(cfKey));
            auto data = CFStringGetCStringPtr(cfKey, kCFStringEncodingUTF8);

            if (data) {
                key = std::string(data, len);
            } else {
                auto maxLen =
                        CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8)  + 1;
                auto cstr = new char[maxLen];
                memset(cstr, 0, maxLen);

                if (CFStringGetCString(CFStringRef(cfKey),
                                       cstr,
                                       maxLen,
                                       kCFStringEncodingUTF8)) {
                    key = std::string(cstr, len);
                }

                delete [] cstr;
            }

            if (!key.empty())
                keys.push_back(key);
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
                                const std::string &value)
{
    AkLogFunction();
    AkLogInfo() << "Writing: " << key << " = " << value << std::endl;
    auto cfKey = CFStringCreateWithCString(kCFAllocatorDefault,
                                           key.c_str(),
                                           kCFStringEncodingUTF8);
    auto cfValue = CFStringCreateWithCString(kCFAllocatorDefault,
                                             value.c_str(),
                                             kCFStringEncodingUTF8);
    CFPreferencesSetValue(cfKey,
                          CFTypeRef(cfValue),
                          PREFERENCES_ID,
                          kCFPreferencesCurrentUser,
                          kCFPreferencesAnyHost);
    CFRelease(cfValue);
    CFRelease(cfKey);
}

void AkVCam::Preferences::write(const std::string &key, int value)
{
    AkLogFunction();
    AkLogInfo() << "Writing: " << key << " = " << value << std::endl;
    auto cfKey = CFStringCreateWithCString(kCFAllocatorDefault,
                                           key.c_str(),
                                           kCFStringEncodingUTF8);
    auto cfValue = CFNumberCreate(kCFAllocatorDefault,
                                  kCFNumberIntType,
                                  &value);
    CFPreferencesSetValue(cfKey,
                          CFTypeRef(cfValue),
                          PREFERENCES_ID,
                          kCFPreferencesCurrentUser,
                          kCFPreferencesAnyHost);
    CFRelease(cfValue);
    CFRelease(cfKey);
}

void AkVCam::Preferences::write(const std::string &key, double value)
{
    AkLogFunction();
    AkLogInfo() << "Writing: " << key << " = " << value << std::endl;
    auto cfKey = CFStringCreateWithCString(kCFAllocatorDefault,
                                           key.c_str(),
                                           kCFStringEncodingUTF8);
    auto cfValue = CFNumberCreate(kCFAllocatorDefault,
                                  kCFNumberDoubleType,
                                  &value);
    CFPreferencesSetValue(cfKey,
                          CFTypeRef(cfValue),
                          PREFERENCES_ID,
                          kCFPreferencesCurrentUser,
                          kCFPreferencesAnyHost);
    CFRelease(cfValue);
    CFRelease(cfKey);
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
    auto cfKey = CFStringCreateWithCString(kCFAllocatorDefault,
                                           key.c_str(),
                                           kCFStringEncodingUTF8);
    auto cfValue = CFPreferencesCopyValue(cfKey,
                                          PREFERENCES_ID,
                                          kCFPreferencesCurrentUser,
                                          kCFPreferencesAnyHost);
    auto value = defaultValue;

    if (cfValue) {
        auto len = size_t(CFStringGetLength(CFStringRef(cfValue)));
        auto data = CFStringGetCStringPtr(CFStringRef(cfValue), kCFStringEncodingUTF8);

        if (data) {
            value = std::string(data, len);
        } else {
            auto maxLen =
                    CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8)  + 1;
            auto cstr = new char[maxLen];
            memset(cstr, 0, maxLen);

            if (CFStringGetCString(CFStringRef(cfValue),
                                   cstr,
                                   maxLen,
                                   kCFStringEncodingUTF8)) {
                value = std::string(cstr, len);
            }

            delete [] cstr;
        }

        CFRelease(cfValue);
    }

    CFRelease(cfKey);

    return value;
}

int AkVCam::Preferences::readInt(const std::string &key, int defaultValue)
{
    AkLogFunction();
    auto cfKey = CFStringCreateWithCString(kCFAllocatorDefault,
                                           key.c_str(),
                                           kCFStringEncodingUTF8);
    auto cfValue = CFPreferencesCopyValue(cfKey,
                                          PREFERENCES_ID,
                                          kCFPreferencesCurrentUser,
                                          kCFPreferencesAnyHost);
    auto value = defaultValue;

    if (cfValue) {
        CFNumberGetValue(CFNumberRef(cfValue), kCFNumberIntType, &value);
        CFRelease(cfValue);
    }

    CFRelease(cfKey);

    return value;
}

double AkVCam::Preferences::readDouble(const std::string &key,
                                       double defaultValue)
{
    AkLogFunction();
    auto cfKey = CFStringCreateWithCString(kCFAllocatorDefault,
                                           key.c_str(),
                                           kCFStringEncodingUTF8);
    auto cfValue = CFPreferencesCopyValue(cfKey,
                                          PREFERENCES_ID,
                                          kCFPreferencesCurrentUser,
                                          kCFPreferencesAnyHost);
    auto value = defaultValue;

    if (cfValue) {
        CFNumberGetValue(CFNumberRef(cfValue), kCFNumberDoubleType, &value);
        CFRelease(cfValue);
    }

    CFRelease(cfKey);

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
    auto cfKey = CFStringCreateWithCString(kCFAllocatorDefault,
                                           key.c_str(),
                                           kCFStringEncodingUTF8);
    CFPreferencesSetValue(cfKey,
                          nullptr,
                          PREFERENCES_ID,
                          kCFPreferencesCurrentUser,
                          kCFPreferencesAnyHost);
    CFRelease(cfKey);
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
    auto cfKeyFrom = CFStringCreateWithCString(kCFAllocatorDefault,
                                               keyFrom.c_str(),
                                               kCFStringEncodingUTF8);
    auto cfValue = CFPreferencesCopyValue(cfKeyFrom,
                                          PREFERENCES_ID,
                                          kCFPreferencesCurrentUser,
                                          kCFPreferencesAnyHost);

    if (cfValue) {
        auto cfKeyTo = CFStringCreateWithCString(kCFAllocatorDefault,
                                                 keyTo.c_str(),
                                                 kCFStringEncodingUTF8);
        CFPreferencesSetValue(cfKeyTo,
                              cfValue,
                              PREFERENCES_ID,
                              kCFPreferencesCurrentUser,
                              kCFPreferencesAnyHost);
        CFRelease(cfKeyTo);
        CFRelease(cfValue);
    }

    CFRelease(cfKeyFrom);
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
#ifndef FAKE_APPLE
    AkLogFunction();
    CFPreferencesSynchronize(PREFERENCES_ID,
                             kCFPreferencesCurrentUser,
                             kCFPreferencesAnyHost);
#endif
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

bool AkVCam::Preferences::removeCamera(const std::string &deviceId)
{
    AkLogFunction();
    AkLogInfo() << "Device: " << deviceId << std::endl;
    int cameraIndex = cameraFromId(deviceId);

    if (cameraIndex < 0)
        return false;

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

    return true;
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
        auto id = AKVCAM_DEVICE_PREFIX + std::to_string(i);

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

bool AkVCam::Preferences::cameraSetDescription(size_t cameraIndex,
                                               const std::string &description)
{
    if (cameraIndex >= camerasCount())
        return false;

    write("cameras." + std::to_string(cameraIndex) + ".description",
          description);
    sync();

    return true;
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

bool AkVCam::Preferences::cameraSetFormats(size_t cameraIndex,
                                           const std::vector<AkVCam::VideoFormat> &formats)
{
    AkLogFunction();

    if (cameraIndex >= camerasCount())
        return false;

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

    return true;
}

bool AkVCam::Preferences::cameraAddFormat(size_t cameraIndex,
                                          const AkVCam::VideoFormat &format,
                                          int index)
{
    AkLogFunction();
    auto formats = cameraFormats(cameraIndex);

    if ((index < 0) || (index > int(formats.size())))
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

    return false;
}

bool AkVCam::Preferences::cameraRemoveFormat(size_t cameraIndex, int index)
{
    AkLogFunction();
    auto formats = cameraFormats(cameraIndex);

    if (index < 0 || index >= int(formats.size()))
        return false;

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

    return true;
}

int AkVCam::Preferences::cameraControlValue(size_t cameraIndex,
                                            const std::string &key)
{
    return readInt("cameras." + std::to_string(cameraIndex) + ".controls." + key);
}

bool AkVCam::Preferences::cameraSetControlValue(size_t cameraIndex,
                                                const std::string &key,
                                                int value)
{
    write("cameras." + std::to_string(cameraIndex) + ".controls." + key, value);
    sync();

    return true;
}

std::string AkVCam::Preferences::picture()
{
    return readString("picture");
}

bool AkVCam::Preferences::setPicture(const std::string &picture)
{
    write("picture", picture);
    sync();

    return true;
}

int AkVCam::Preferences::logLevel()
{
    return readInt("loglevel", AKVCAM_LOGLEVEL_DEFAULT);
}

bool AkVCam::Preferences::setLogLevel(int logLevel)
{
    write("loglevel", logLevel);
    sync();

    return true;
}

int AkVCam::Preferences::servicePort()
{
    return readInt("servicePort", atoi(AKVCAM_SERVICEPORT_DEFAULT));
}

bool AkVCam::Preferences::setServicePort(int servicePort)
{
    write("servicePort", servicePort);
    sync();

    return true;
}

int AkVCam::Preferences::serviceTimeout()
{
    return readInt("serviceTimeout", AKVCAM_SERVICETIMEOUT_DEFAULT);
}

bool AkVCam::Preferences::setServiceTimeout(int timeoutSecs)
{
    write("serviceTimeout", timeoutSecs);
    sync();

    return true;
}
