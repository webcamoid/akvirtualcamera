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

#include <string>
#include <strmif.h>

#include "VCamUtils/src/image/videoformattypes.h"
#include "VCamUtils/src/logger.h"

#define AkLogInterface(interface, instance) \
    AkLogInfo() << "Returning " << #interface << "(" << instance << ")" << std::endl

namespace AkVCam
{
    class VideoFormat;
    class VideoFrame;

    std::string tempPath();
    std::string moduleFileName(HINSTANCE hinstDLL);
    std::string errorToString(DWORD errorCode);
    CLSID createClsidFromStr(const std::string &str);
    std::string createClsidStrFromStr(const std::string &str);
    std::string stringFromIid(const IID &iid);
    std::string stringFromResult(HRESULT result);
    std::string stringFromClsid(const CLSID &clsid);
    std::string stringFromWSTR(LPCWSTR wstr);
    LPWSTR stringToWSTR(const std::string &str);
    FourCC formatFromGuid(const GUID &guid);
    const GUID &guidFromFormat(FourCC format);
    DWORD compressionFromFormat(FourCC format);
    bool isSubTypeSupported(const GUID &subType);
    AM_MEDIA_TYPE *mediaTypeFromFormat(const VideoFormat &format);
    VideoFormat formatFromMediaType(const AM_MEDIA_TYPE *mediaType);
    bool isEqualMediaType(const AM_MEDIA_TYPE *mediaType1,
                          const AM_MEDIA_TYPE *mediaType2,
                          bool exact=false);
    bool copyMediaType(AM_MEDIA_TYPE *dstMediaType,
                       const AM_MEDIA_TYPE *srcMediaType);
    AM_MEDIA_TYPE *createMediaType(const AM_MEDIA_TYPE *mediaType);
    void deleteMediaType(AM_MEDIA_TYPE **mediaType);
    bool containsMediaType(const AM_MEDIA_TYPE *mediaType,
                           IEnumMediaTypes *mediaTypes);
    std::string stringFromMajorType(const GUID &majorType);
    std::string stringFromSubType(const GUID &subType);
    std::string stringFromFormatType(const GUID &formatType);
    std::string stringFromMediaType(const AM_MEDIA_TYPE *mediaType);
    std::string stringFromMediaSample(IMediaSample *mediaSample);
    LSTATUS deleteTree(HKEY key, LPCSTR subkey, REGSAM samFlags);
    LSTATUS copyTree(HKEY src, LPCSTR subkey, HKEY dst, REGSAM samFlags);
    VideoFrame loadPicture(const std::string &fileName);
}

#endif // PLATFORM_UTILS_H
