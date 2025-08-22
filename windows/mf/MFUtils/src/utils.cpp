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
#include <mfapi.h>
#include <winreg.h>

#include "utils.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/logger.h"
#include "VCamUtils/src/videoformat.h"

namespace AkVCam
{
    struct AkPixelFormatMF
    {
        FourCC format;
        GUID mfFormat;

        static const AkPixelFormatMF *table()
        {
            static const AkPixelFormatMF akPixelFormatMFTable[] = {
                {PixelFormatRGB32    , MFVideoFormat_RGB32 },
                {PixelFormatRGB24    , MFVideoFormat_RGB24 },
                {PixelFormatRGB16    , MFVideoFormat_RGB565},
                {PixelFormatRGB15    , MFVideoFormat_RGB555},
                {PixelFormatUYVY     , MFVideoFormat_UYVY  },
                {PixelFormatYUY2     , MFVideoFormat_YUY2  },
                {PixelFormatNV12     , MFVideoFormat_NV12  },
                {MKFOURCC(0, 0, 0, 0), GUID_NULL           },
            };

            return akPixelFormatMFTable;
        }

        inline static const AkPixelFormatMF *byFormat(FourCC format)
        {
            auto it = table();

            for (; it->format != MKFOURCC(0, 0, 0, 0); ++it)
                if (it->format == format)
                    return it;

            return it;
        }

        inline static const AkPixelFormatMF *byMFFormat(const GUID &mfFormat)
        {
            auto it = table();

            for (; it->format != MKFOURCC(0, 0, 0, 0); ++it)
                if (IsEqualGUID(it->mfFormat, mfFormat))
                    return it;

            return it;
        }
    };
}

inline bool operator <(const CLSID &a, const CLSID &b)
{
    return AkVCam::stringFromIid(a) < AkVCam::stringFromIid(b);
}

bool AkVCam::isDeviceIdMFTaken(const std::string &deviceId)
{
    AkLogFunction();
    CLSID clsid = createClsidFromStr(deviceId);

    if (IsEqualCLSID(clsid, CLSID_NULL))
        return false;

    std::string clsidStr = stringFromClsid(clsid);

    if (clsidStr.empty())
        return false;

    AkLogDebug() << "Checking CLSID: " << clsidStr << std::endl;

    const char *subkeyPrefix = "Software\\Classes\\CLSID";
    std::string subkey = std::string(subkeyPrefix) + "\\" + clsidStr;

    // Try opening the sub-key
    HKEY keyCLSID = nullptr;
    LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                subkey.c_str(),
                                0,
                                KEY_READ,
                                &keyCLSID);
    bool taken = (result == ERROR_SUCCESS);

    if (keyCLSID)
        RegCloseKey(keyCLSID);

    AkLogDebug() << "CLSID "
                 << clsidStr
                 << (taken? " is taken": " is not taken")
                 << std::endl;

    return taken;
}

std::string AkVCam::createDeviceIdMF()
{
    AkLogFunction();

    // List device CLSIDs in use.
    const int maxId = 64;

    for (int i = 0; i < maxId; i++) {
        /* There are no rules for device IDs in Windows. Just append an
         * incremental index to a common prefix.
         */
        auto id = AKVCAM_DEVICE_PREFIX + std::to_string(i);

        // Check if the ID is being used, if not return it.
        if (!isDeviceIdMFTaken(id))
            return id;
    }

    return {};
}

std::vector<CLSID> AkVCam::listRegisteredMFCameras()
{
    AkLogFunction();
    auto pluginFolder = locatePluginPath();
    AkLogDebug() << "Plugin path: " << pluginFolder << std::endl;

    if (pluginFolder.empty())
        return {};

    auto pluginPath = pluginFolder + "\\" AKVCAM_PLUGIN_NAME ".dll";
    AkLogDebug() << "Plugin binary: " << pluginPath << std::endl;

    if (!fileExists(pluginPath)) {
        AkLogError() << "Plugin binary not found: " << pluginPath << std::endl;

        return {};
    }

    std::vector<CLSID> cameras;

    // Open HKLM\Software\Classes\CLSID

    HKEY keyCLSID = nullptr;
    const char *subkeyPrefix = "Software\\Classes\\CLSID";
    LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                subkeyPrefix,
                                0,
                                KEY_READ | KEY_ENUMERATE_SUB_KEYS,
                                &keyCLSID);

    if (result != ERROR_SUCCESS) {
        AkLogError() << "Failed to open CLSID registry key: " << result << std::endl;

        return cameras;
    }

    // Get sub-keys info
    DWORD subkeys = 0;
    result = RegQueryInfoKeyA(keyCLSID,
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
        AkLogError() << "Failed to query CLSID key info: " << result << std::endl;
        RegCloseKey(keyCLSID);

        return cameras;
    }

    // Iterate CLSIDs
    for (DWORD i = 0; i < subkeys; ++i) {
        char subKeyName[MAX_PATH];
        DWORD subKeyLen = MAX_PATH;
        result = RegEnumKeyExA(keyCLSID,
                               i,
                               subKeyName,
                               &subKeyLen,
                               nullptr,
                               nullptr,
                               nullptr,
                               nullptr);

        if (result != ERROR_SUCCESS)
            continue;

        // Get CLSID from the key

        CLSID clsid = createClsidFromStr(subKeyName);

        if (IsEqualCLSID(clsid, CLSID_NULL))
            continue;

        // Open InprocServer32 sub-key for checking the binary file associated

        HKEY keyInproc = nullptr;
        auto inprocSubkey = std::string(subkeyPrefix)
                            + "\\"
                            + subKeyName
                            + "\\InprocServer32";
        result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                               inprocSubkey.c_str(),
                               0, KEY_READ,
                               &keyInproc);

        if (result != ERROR_SUCCESS)
            continue;

        // Read the default value (name of the DLL file)
        char dllPath[MAX_PATH];
        DWORD dllPathLen = MAX_PATH;
        result = RegQueryValueExA(keyInproc,
                                  nullptr,
                                  nullptr,
                                  nullptr,
                                  reinterpret_cast<LPBYTE>(dllPath),
                                  &dllPathLen);
        RegCloseKey(keyInproc);

        if (result != ERROR_SUCCESS)
            continue;

        // Compare the DLL path with pluginPath
        if (pluginPath == dllPath) {
            AkLogDebug() << "Found matching camera CLSID: " << subKeyName << std::endl;
            cameras.push_back(clsid);
        }
    }

    RegCloseKey(keyCLSID);

    AkLogDebug() << "Found " << cameras.size() << " registered cameras" << std::endl;

    return cameras;
}

AkVCam::FourCC AkVCam::pixelFormatFromMediaFormat(const GUID &mfFormat)
{
    return AkPixelFormatMF::byMFFormat(mfFormat)->format;
}

GUID AkVCam::mediaFormatFromPixelFormat(FourCC format)
{
    return AkPixelFormatMF::byFormat(format)->mfFormat;
}

IMFMediaType *AkVCam::mfMediaTypeFromFormat(const VideoFormat &videoFormat)
{
    auto formatGUID = mediaFormatFromPixelFormat(videoFormat.fourcc());

    if (IsEqualGUID(formatGUID, GUID_NULL))
        return nullptr;

    IMFMediaType *mediaType = nullptr;

    if (FAILED(MFCreateMediaType(&mediaType)))
        return nullptr;

    if (FAILED(mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video))) {
        mediaType->Release();

        return nullptr;
    }

    if (FAILED(mediaType->SetGUID(MF_MT_SUBTYPE, formatGUID))) {
        mediaType->Release();

        return nullptr;
    }

    auto hr = MFSetAttributeSize(mediaType,
                                 MF_MT_FRAME_SIZE,
                                 videoFormat.width(),
                                 videoFormat.height());

    if (FAILED(hr)) {
        mediaType->Release();

        return nullptr;
    }

    auto fps = videoFormat.minimumFrameRate();
    hr = MFSetAttributeRatio(mediaType, MF_MT_FRAME_RATE, fps.num(), fps.den());

    if (FAILED(hr)) {
        mediaType->Release();

        return nullptr;
    }

    hr = MFSetAttributeRatio(mediaType, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

    if (FAILED(hr)) {
        mediaType->Release();

        return nullptr;
    }

    //pMediaType->SetUINT32(MF_MT_DEFAULT_STRIDE, 640 * 4);
    //pMediaType->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE);

    return mediaType;
}

AkVCam::VideoFormat AkVCam::formatFromMFMediaType(IMFMediaType *mediaType)
{
    if (!mediaType)
        return {};

    GUID majorType;

    if (FAILED(mediaType->GetGUID(MF_MT_MAJOR_TYPE, &majorType))
        || !IsEqualGUID(majorType, MFMediaType_Video))
        return {};

    GUID subType;

    if (FAILED(mediaType->GetGUID(MF_MT_SUBTYPE, &subType)))
        return {};

    auto format = AkPixelFormatMF::byMFFormat(subType)->format;

    if (format == MKFOURCC(0, 0, 0, 0))
        return {};

    UINT32 width = 0;
    UINT32 height = 0;
    auto hr = MFGetAttributeSize(mediaType,
                                 MF_MT_FRAME_SIZE,
                                 &width,
                                 &height);

    if (FAILED(hr) || width < 1 || height < 1)
        return {};

    UINT32 fpsNum = 0;
    UINT32 fpsDen = 0;
    hr = MFGetAttributeRatio(mediaType, MF_MT_FRAME_RATE, &fpsNum, &fpsDen);

    if (FAILED(hr) || fpsNum < 1 || fpsDen < 1)
        return {};

    return VideoFormat(format, width, height, {Fraction(fpsNum, fpsDen)});
}
