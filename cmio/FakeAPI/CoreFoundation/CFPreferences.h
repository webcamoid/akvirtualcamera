/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#ifndef CFPREFERENCES_H
#define CFPREFERENCES_H

#include <fstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

#include "CFArray.h"
#include "CFString.h"
#include "CFNumber.h"
#include "../cmio/PlatformUtils/src/utils.h"
#include "../VCamUtils/src/utils.h"

static const char *kCFPreferencesCurrentUser = "kCFPreferencesCurrentUser";
static const char *kCFPreferencesAnyHost = "kCFPreferencesAnyHost";

inline CFArrayRef CFPreferencesCopyKeyList(const char *applicationID,
                                           const char *userName,
                                           const char *hostName)
{
    std::vector<void *> allKeys;
    char fname[4096];
    snprintf(fname, 4096, "%s/.config/%s/%s.conf", getenv("HOME"), AKVCAM_PLUGIN_NAME, applicationID);

    std::ifstream confFile(fname);

    if (!confFile.is_open())
        return nullptr;

    std::string line;

    while (std::getline(confFile, line)) {
        line = AkVCam::trimmed(line);

        if (line.empty()
            || line[0] == '#'
            || line[0] == '[') {
            continue;
        }

        auto pos = line.find("=");

        if (pos == std::string::npos)
            continue;

        auto key = AkVCam::trimmed(line.substr(0, pos));
        auto cfStr = CFStringCreateWithCString(kCFAllocatorDefault,
                                               key.c_str(),
                                               kCFStringEncodingUTF8);
        allKeys.push_back(const_cast<void *>(reinterpret_cast<const void *>(cfStr)));
    }

    CFArrayCallBacks callbacks;
    memset(&callbacks, 0, sizeof(CFArrayCallBacks));
    callbacks.release = [] (CFAllocatorRef allocator, const void *value) {
        CFRelease(CFStringRef(value));
    };
    auto array = CFArrayCreate(kCFAllocatorDefault,
                               const_cast<const void **>(allKeys.data()),
                               allKeys.size(),
                               &callbacks);

    return array;
}

inline void CFPreferencesSetValue(CFStringRef key,
                                  CFTypeRef value,
                                  const char *applicationID,
                                  const char *userName,
                                  const char *hostName)
{
    char confPath[4096];
    snprintf(confPath, 4096, "%s/.config/%s", getenv("HOME"), AKVCAM_PLUGIN_NAME);
    mkdir(confPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

    char fname[4096];
    snprintf(fname, 4096, "%s/%s.conf", confPath, applicationID);

    char tempfname[4096];
    snprintf(tempfname, 4096, "%s.tmp", fname);

    std::ifstream confFile(fname);
    std::ofstream tmpFile(tempfname);
    bool keyWritten = false;

    if (confFile.is_open()) {
        std::string line;

        while (std::getline(confFile, line)) {
            line = AkVCam::trimmed(line);

            if (line.empty()
                || line[0] == '#'
                || line[0] == '[') {
                tmpFile << line << std::endl;
            } else {
                auto pos = line.find("=");

                if (pos != std::string::npos) {
                    auto ckey = AkVCam::trimmed(line.substr(0, pos));

                    if (ckey == CFStringGetCStringPtr(key, kCFStringEncodingUTF8)) {
                        if (value->type == CFNumberGetTypeID()) {
                            double dvalue = 0.0;
                            CFNumberGetValue(value, kCFNumberDoubleType, &dvalue);
                            tmpFile << ckey << " = " << dvalue << std::endl;
                            keyWritten = true;
                        } else if (value->type == CFStringGetTypeID()) {
                            tmpFile << ckey << " = " << CFStringGetCStringPtr(value, kCFStringEncodingUTF8) << std::endl;
                            keyWritten = true;
                        } else {
                            tmpFile << ckey << " = " << std::endl;
                            keyWritten = true;
                        }
                    } else {
                        tmpFile << line << std::endl;
                    }
                } else {
                    tmpFile << line << std::endl;
                }
            }
        }
    }

    if (!keyWritten) {
        auto ckey = CFStringGetCStringPtr(key, kCFStringEncodingUTF8);

        if (value->type == CFNumberGetTypeID()) {
            double dvalue = 0.0;
            CFNumberGetValue(value, kCFNumberDoubleType, &dvalue);
            tmpFile << ckey << " = " << dvalue << std::endl;
        } else if (value->type == CFStringGetTypeID()) {
            tmpFile << ckey << " = " << CFStringGetCStringPtr(value, kCFStringEncodingUTF8) << std::endl;
        } else {
            tmpFile << ckey << " = " << std::endl;
        }
    }

    tmpFile.close();
    AkVCam::move(tempfname, fname);
}

inline CFTypeRef CFPreferencesCopyValue(CFStringRef key,
                                        const char *applicationID,
                                        const char *userName,
                                        const char *hostName)
{
    char fname[4096];
    snprintf(fname, 4096, "%s/.config/%s/%s.conf", getenv("HOME"), AKVCAM_PLUGIN_NAME, applicationID);

    std::ifstream confFile(fname);

    if (!confFile.is_open())
        return nullptr;

    std::string line;

    while (std::getline(confFile, line)) {
        line = AkVCam::trimmed(line);

        if (line.empty()
            || line[0] == '#'
            || line[0] == '[') {
            continue;
        }

        auto pos = line.find("=");

        if (pos == std::string::npos)
            continue;

        auto ckey = AkVCam::trimmed(line.substr(0, pos));

        if (ckey == CFStringGetCStringPtr(key, kCFStringEncodingUTF8)) {
            auto value = AkVCam::trimmed(line.substr(pos + 1, line.size() - pos - 1));

            size_t index = 0;
            auto dvalue = std::stod(value, &index);

            if (index == value.size())
                return CFNumberCreate(kCFAllocatorDefault,
                                      kCFNumberDoubleType,
                                      &dvalue);

            auto ivalue = std::stoi(value, &index);

            if (index == value.size())
                return CFNumberCreate(kCFAllocatorDefault,
                                      kCFNumberIntType,
                                      &ivalue);

            return CFStringCreateWithCString(kCFAllocatorDefault,
                                             value.c_str(),
                                             kCFStringEncodingUTF8);
        }
    }

    return nullptr;
}

#endif // CFPREFERENCES_H
