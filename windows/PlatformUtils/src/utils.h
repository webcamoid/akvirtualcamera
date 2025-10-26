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
#include <vector>
#include <strmif.h>

#include "VCamUtils/src/videoformattypes.h"
#include "VCamUtils/src/logger.h"

#define AkLogInterface(interface, instance) \
    AkLogDebug("Returning %s (%p)", #interface, instance)

namespace AkVCam
{
    class VideoFormat;
    class VideoFrame;

    std::string locateManagerPath();
    std::string locateServicePath();
    std::string locateMFServicePath();
    std::string locatePluginPath();
    std::string locateMFPluginPath();
    std::string locateAltManagerPath();
    std::string locateAltServicePath();
    std::string locateAltMFServicePath();
    std::string locateAltPluginPath();
    std::string locateAltMFPluginPath();
    bool supportsMediaFoundationVCam();
    std::string tempPath();
    std::string moduleFileName(HINSTANCE hinstDLL);
    std::string dirname(const std::string &path);
    bool fileExists(const std::string &path);
    std::string realPath(const std::string &path);
    std::string stringFromError(DWORD errorCode);
    CLSID createClsidFromStr(const std::string &str);
    std::string createClsidStrFromStr(const std::string &str);
    std::string stringFromIid(const IID &iid);
    std::string stringFromResult(HRESULT result);
    std::string stringFromClsid(const CLSID &clsid);
    std::string stringFromWSTR(LPCWSTR wstr);
    LPWSTR wstrFromString(const std::string &str);
    std::string stringFromTSTR(LPCTSTR tstr);
    LPTSTR tstrFromString(const std::string &str);
    PixelFormat formatFromGuid(const GUID &guid);
    const GUID &guidFromFormat(PixelFormat format);
    DWORD compressionFromFormat(PixelFormat format);
    PixelFormat pixelFormatFromCommonString(const std::string &format);
    std::string pixelFormatToCommonString(PixelFormat format);
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
    std::string stringFromMajorType(const GUID &majorType);
    std::string stringFromSubType(const GUID &subType);
    std::string stringFromFormatType(const GUID &formatType);
    std::string stringFromMediaType(const AM_MEDIA_TYPE *mediaType);
    std::string stringFromMediaSample(IMediaSample *mediaSample);
    LSTATUS deleteTree(HKEY key, LPCSTR subkey, REGSAM samFlags);
    LSTATUS copyTree(HKEY src, LPCSTR subkey, HKEY dst, REGSAM samFlags);
    VideoFrame loadPicture(const std::string &fileName);
    std::string logPath(const std::string &logName);
    void logSetup(const std::string &context={});
    bool isDeviceIdTaken(const std::string &deviceId);
    std::string createDeviceId();
    std::string cameraIdFromCLSID(const CLSID &clsid);
    std::vector<CLSID> listAllCameras();
    std::vector<CLSID> listRegisteredCameras();
    std::vector<uint64_t> systemProcesses();
    uint64_t currentPid();
    std::string exePath(uint64_t pid);
    std::string currentBinaryPath();
    bool isServiceRunning();
    bool isServicePortUp();
    int exec(const std::vector<std::string> &parameters,
             const std::string &directory={},
             bool show=false);
    bool execDetached(const std::vector<std::string> &parameters,
                      const std::string &directory={},
                      bool show=false);
    bool needsRoot(const std::string &task);
    int sudo(const std::vector<std::string> &parameters);
}

#endif // PLATFORM_UTILS_H
