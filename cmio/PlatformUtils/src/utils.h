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
#include <CoreMediaIO/CMIOHardwarePlugIn.h>
#include <CoreMedia/CMFormatDescription.h>

#include "VCamUtils/src/videoformattypes.h"

namespace AkVCam
{
    class VideoFrame;

    bool uuidEqual(const REFIID &uuid1, const CFUUIDRef uuid2);
    std::string enumToString(UInt32 value);
    FourCharCode formatToCM(PixelFormat format);
    PixelFormat formatFromCM(FourCharCode format);
    std::shared_ptr<CFTypeRef> cfTypeFromStd(const std::string &str);
    std::shared_ptr<CFTypeRef> cfTypeFromStd(int num);
    std::shared_ptr<CFTypeRef> cfTypeFromStd(double num);
    std::string stringFromCFType(CFTypeRef cfType);
    std::string realPath(const std::string &path);
    VideoFrame loadPicture(const std::string &fileName);
}

#endif // PLATFORM_UTILS_H
