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

#include "preferences.h"
#include "utils.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/logger/logger.h"

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
    CFPreferencesSetAppValue(CFStringRef(*cfKey), *value, PREFERENCES_ID);
}

void AkVCam::Preferences::write(const std::string &key,
                                const std::string &value)
{
    AkLogFunction();
    AkLogInfo() << "Writing: " << key << " = " << value << std::endl;
    auto cfKey = cfTypeFromStd(key);
    auto cfValue = cfTypeFromStd(value);
    CFPreferencesSetAppValue(CFStringRef(*cfKey), *cfValue, PREFERENCES_ID);
}

void AkVCam::Preferences::write(const std::string &key,
                                const std::wstring &value)
{
    AkLogFunction();
    AkLogInfo() << "Writing: "
                << key
                << " = "
                << std::string(value.begin(), value.end()) << std::endl;
    auto cfKey = cfTypeFromStd(key);
    auto cfValue = cfTypeFromStd(value);
    CFPreferencesSetAppValue(CFStringRef(*cfKey), *cfValue, PREFERENCES_ID);
}

void AkVCam::Preferences::write(const std::string &key, int value)
{
    AkLogFunction();
    AkLogInfo() << "Writing: " << key << " = " << value << std::endl;
    auto cfKey = cfTypeFromStd(key);
    auto cfValue = cfTypeFromStd(value);
    CFPreferencesSetAppValue(CFStringRef(*cfKey), *cfValue, PREFERENCES_ID);
}

void AkVCam::Preferences::write(const std::string &key, double value)
{
    AkLogFunction();
    AkLogInfo() << "Writing: " << key << " = " << value << std::endl;
    auto cfKey = cfTypeFromStd(key);
    auto cfValue = cfTypeFromStd(value);
    CFPreferencesSetAppValue(CFStringRef(*cfKey), *cfValue, PREFERENCES_ID);
}

std::shared_ptr<CFTypeRef> AkVCam::Preferences::read(const std::string &key)
{
    AkLogFunction();
    auto cfKey = cfTypeFromStd(key);
    auto cfValue = CFTypeRef(CFPreferencesCopyAppValue(CFStringRef(*cfKey),
                                                       PREFERENCES_ID));

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
            CFStringRef(CFPreferencesCopyAppValue(CFStringRef(*cfKey),
                                                  PREFERENCES_ID));
    auto value = defaultValue;

    if (cfValue) {
        value = stringFromCFType(cfValue);
        CFRelease(cfValue);
    }

    return value;
}

std::wstring AkVCam::Preferences::readWString(const std::string &key,
                                              const std::wstring &defaultValue)
{
    AkLogFunction();
    auto cfKey = cfTypeFromStd(key);
    auto cfValue =
            CFStringRef(CFPreferencesCopyAppValue(CFStringRef(*cfKey),
                                                  PREFERENCES_ID));
    auto value = defaultValue;

    if (cfValue) {
        value = wstringFromCFType(cfValue);
        CFRelease(cfValue);
    }

    return value;
}

int AkVCam::Preferences::readInt(const std::string &key, int defaultValue)
{
    AkLogFunction();
    auto cfKey = cfTypeFromStd(key);
    auto cfValue =
            CFNumberRef(CFPreferencesCopyAppValue(CFStringRef(*cfKey),
                                                  PREFERENCES_ID));
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
            CFNumberRef(CFPreferencesCopyAppValue(CFStringRef(*cfKey),
                                                  PREFERENCES_ID));
    auto value = defaultValue;

    if (cfValue) {
        CFNumberGetValue(cfValue, kCFNumberDoubleType, &value);
        CFRelease(cfValue);
    }

    return value;
}

void AkVCam::Preferences::deleteKey(const std::string &key)
{
    AkLogFunction();
    AkLogInfo() << "Deleting " << key << std::endl;
    auto cfKey = cfTypeFromStd(key);
    CFPreferencesSetAppValue(CFStringRef(*cfKey), nullptr, PREFERENCES_ID);
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
    CFPreferencesAppSynchronize(PREFERENCES_ID);
}

std::string AkVCam::Preferences::addCamera(const std::wstring &description,
                                           const std::vector<VideoFormat> &formats)
{
    return addCamera("", description, formats);
}

std::string AkVCam::Preferences::addCamera(const std::string &path,
                                           const std::wstring &description,
                                           const std::vector<VideoFormat> &formats)
{
    AkLogFunction();

    if (!path.empty() && cameraExists(path))
        return {};

    auto path_ = path.empty()? createDevicePath(): path;
    int cameraIndex = readInt("cameras");
    write("cameras", cameraIndex + 1);

    write("cameras."
          + std::to_string(cameraIndex)
          + ".description",
          description);
    write("cameras."
          + std::to_string(cameraIndex)
          + ".path",
          path_);
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

    return path_;
}

void AkVCam::Preferences::removeCamera(const std::string &path)
{
    AkLogFunction();
    AkLogInfo() << "Device: " << path << std::endl;
    int cameraIndex = cameraFromPath(path);

    if (cameraIndex < 0)
        return;

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
        auto path = CMIO_PLUGIN_DEVICE_PREFIX + std::to_string(i);

        // Check if the path is being used, if not return it.
        if (std::find(cameraPaths.begin(),
                      cameraPaths.end(),
                      path) == cameraPaths.end())
            return path;
    }

    return {};
}

int AkVCam::Preferences::cameraFromPath(const std::string &path)
{
    for (size_t i = 0; i < camerasCount(); i++)
        if (cameraPath(i) == path && !cameraFormats(i).empty())
            return int(i);

    return -1;
}

bool AkVCam::Preferences::cameraExists(const std::string &path)
{
    for (size_t i = 0; i < camerasCount(); i++)
        if (cameraPath(i) == path)
            return true;

    return false;
}

std::wstring AkVCam::Preferences::cameraDescription(size_t cameraIndex)
{
    return readWString("cameras."
                       + std::to_string(cameraIndex)
                       + ".description");
}

std::string AkVCam::Preferences::cameraPath(size_t cameraIndex)
{
    return readString("cameras."
                      + std::to_string(cameraIndex)
                      + ".path");
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
