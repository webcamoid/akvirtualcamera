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

#ifndef PLATFORM_UTILS_H
#define PLATFORM_UTILS_H

#include <memory>
#include <string>
#include <vector>
#include <CoreMediaIO/CMIOHardwarePlugIn.h>
#include <CoreMedia/CMFormatDescription.h>

#include "VCamUtils/src/videoformattypes.h"

namespace AkVCam
{
    class VideoFrame;

    std::string locateManagerPath();
    std::string locateServicePath();
    std::string locatePluginPath();
    std::string tempPath();
    bool uuidEqual(const REFIID &uuid1, const CFUUIDRef uuid2);
    std::string enumToString(uint32_t value);
    FourCharCode formatToCM(PixelFormat format);
    PixelFormat formatFromCM(FourCharCode format);
    PixelFormat pixelFormatFromCommonString(const std::string &format);
    std::string pixelFormatToCommonString(PixelFormat format);
    std::string dirname(const std::string &path);
    std::string realPath(const std::string &path);
    VideoFrame loadPicture(const std::string &fileName);
    bool fileExists(const std::string &path);
    bool readEntitlements(const std::string &app,
                          const std::string &output);
    std::string logPath(const std::string &logName);
    void logSetup(const std::string &context={});
    bool isDeviceIdTaken(const std::string &deviceId);
    std::string createDeviceId();
    std::vector<uint64_t> systemProcesses();
    uint64_t currentPid();
    std::string exePath(uint64_t pid);
    std::string currentBinaryPath();
    bool isServiceRunning();
    bool isServicePortUp();
    bool needsRoot(const std::string &task);
    int sudo(const std::vector<std::string> &parameters);
}

#endif // PLATFORM_UTILS_H
