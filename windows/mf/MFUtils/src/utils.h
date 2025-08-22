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

#ifndef MFUTILS_H
#define MFUTILS_H

#include <string>
#include <vector>
#include <mfobjects.h>

#include "VCamUtils/src/videoformattypes.h"

namespace AkVCam
{
    class VideoFormat;

    bool isDeviceIdMFTaken(const std::string &deviceId);
    std::string createDeviceIdMF();
    std::vector<CLSID> listRegisteredMFCameras();
    FourCC pixelFormatFromMediaFormat(const GUID &mfFormat);
    GUID mediaFormatFromPixelFormat(FourCC format);
    IMFMediaType *mfMediaTypeFromFormat(const VideoFormat &videoFormat);
    VideoFormat formatFromMFMediaType(IMFMediaType *mediaType);
}

#endif // MFUTILS_H
