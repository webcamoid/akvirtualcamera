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
#include <map>
#include <sstream>
#include <dshow.h>
#include <dvdmedia.h>
#include <comdef.h>
#include <psapi.h>
#include <shlobj.h>
#include <wincodec.h>

#include "utils.h"
#include "preferences.h"
#include "VCamUtils/src/fraction.h"
#include "VCamUtils/src/messageclient.h"
#include "VCamUtils/src/utils.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"

#define TIME_BASE 1.0e7

namespace AkVCam
{
    class VideoFormatSpecsPrivate
    {
        public:
            PixelFormat pixelFormat;
            const char *name;
            DWORD compression;
            GUID guid;
            const DWORD *masks;

            static const VideoFormatSpecsPrivate *formats();
            static inline const VideoFormatSpecsPrivate *byGuid(const GUID &guid);
            static inline const VideoFormatSpecsPrivate *byPixelFormat(PixelFormat pixelFormat);
            static inline const VideoFormatSpecsPrivate *byName(const char *name);
    };

    std::string currentArchitecture();
    std::string altArchitecture();
    std::string pluginInstallPath();
}

inline bool operator <(const CLSID &a, const CLSID &b)
{
    return AkVCam::stringFromIid(a) < AkVCam::stringFromIid(b);
}

std::string AkVCam::locateManagerPath()
{
    AkLogFunction();

    auto file = pluginInstallPath()
                + "\\" + currentArchitecture()
                + "\\" AKVCAM_MANAGER_NAME ".exe";

    /* If for whatever reason the program for the current architecture is not
     * available, try using the alternative version if available.
     */

    return fileExists(file)? file: locateAltServicePath();
}

std::string AkVCam::locateServicePath()
{
    AkLogFunction();

    auto file = pluginInstallPath()
                + "\\" + currentArchitecture()
                + "\\" AKVCAM_SERVICE_NAME ".exe";

    // Same as above.

    return fileExists(file)? file: locateAltServicePath();
}

std::string AkVCam::locateMFServicePath()
{
    AkLogFunction();

    auto file = pluginInstallPath()
                + "\\" + currentArchitecture()
                + "\\" AKVCAM_SERVICE_MF_NAME ".exe";

    // Same as above.

    return fileExists(file)? file: locateAltServicePath();
}

std::string AkVCam::locatePluginPath()
{
    AkLogFunction();

    auto file = pluginInstallPath()
                + "\\" + currentArchitecture()
                + "\\" AKVCAM_PLUGIN_NAME ".dll";

    // We can't use the alt version here.

    return fileExists(file)? file: std::string();
}

std::string AkVCam::locateMFPluginPath()
{
    AkLogFunction();

    auto file = pluginInstallPath()
                + "\\" + currentArchitecture()
                + "\\" AKVCAM_PLUGIN_MF_NAME ".dll";

    // We can't use the alt version here.

    return fileExists(file)? file: std::string();
}

std::string AkVCam::locateAltManagerPath()
{
    auto file = pluginInstallPath()
                + "\\" + altArchitecture()
                + "\\" AKVCAM_MANAGER_NAME ".exe";

    return fileExists(file)? file: std::string();
}

std::string AkVCam::locateAltServicePath()
{
    auto file = pluginInstallPath()
                + "\\" + altArchitecture()
                + "\\" AKVCAM_SERVICE_NAME ".exe";

    return fileExists(file)? file: std::string();
}

std::string AkVCam::locateAltMFServicePath()
{
    auto file = pluginInstallPath()
                + "\\" + altArchitecture()
                + "\\" AKVCAM_SERVICE_MF_NAME ".exe";

    return fileExists(file)? file: std::string();
}

std::string AkVCam::locateAltPluginPath()
{
    auto file = pluginInstallPath()
                + "\\" + altArchitecture()
                + "\\" AKVCAM_PLUGIN_NAME ".dll";

    return fileExists(file)? file: std::string();
}

std::string AkVCam::locateAltMFPluginPath()
{
    auto file = pluginInstallPath()
                + "\\" + altArchitecture()
                + "\\" AKVCAM_PLUGIN_MF_NAME ".dll";

    return fileExists(file)? file: std::string();
}

bool AkVCam::supportsMediaFoundationVCam()
{
    auto servicePath = locateMFServicePath();

    if (servicePath.empty())
        return false;

    auto pluginPath = locateMFPluginPath();

    if (pluginPath.empty())
        return false;

    auto mfsensorgroupHnd = LoadLibrary(TEXT("mfsensorgroup.dll"));

    if (!mfsensorgroupHnd)
        return false;

    bool supported = GetProcAddress(mfsensorgroupHnd, "MFCreateVirtualCamera") != nullptr;
    FreeLibrary(mfsensorgroupHnd);

    return supported;
}

std::string AkVCam::tempPath()
{
    CHAR tempPath[MAX_PATH];
    memset(tempPath, 0, MAX_PATH * sizeof(CHAR));
    GetTempPathA(MAX_PATH, tempPath);

    return std::string(tempPath);
}

std::string AkVCam::moduleFileName(HINSTANCE hinstDLL)
{
    CHAR fileName[MAX_PATH];
    memset(fileName, 0, MAX_PATH * sizeof(CHAR));
    GetModuleFileNameA(hinstDLL, fileName, MAX_PATH);

    return std::string(fileName);
}

std::string AkVCam::dirname(const std::string &path)
{
    return path.substr(0, path.rfind("\\"));
}

bool AkVCam::fileExists(const std::string &path)
{
    return GetFileAttributesA(path.c_str()) & FILE_ATTRIBUTE_ARCHIVE;
}

std::string AkVCam::realPath(const std::string &path)
{
    char rpath[MAX_PATH];
    memset(rpath, 0, MAX_PATH);
    GetFullPathNameA(path.c_str(), MAX_PATH, rpath, nullptr);

    return std::string(rpath);
}

std::string AkVCam::stringFromError(DWORD errorCode)
{
    CHAR *errorStr = nullptr;
    auto size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER
                               | FORMAT_MESSAGE_FROM_SYSTEM
                               | FORMAT_MESSAGE_IGNORE_INSERTS,
                               nullptr,
                               errorCode,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                               reinterpret_cast<LPSTR>(&errorStr),
                               0,
                               nullptr);
    std::string error(errorStr, size);
    LocalFree(errorStr);

    return error;
}

// Converts a human redable string to a CLSID using MD5 hash.
CLSID AkVCam::createClsidFromStr(const std::string &str)
{
    AkLogFunction();
    AkLogDebug() << "String: " << str << std::endl;
    HCRYPTPROV provider = 0;
    HCRYPTHASH hash = 0;
    CLSID clsid;
    DWORD clsidLen = sizeof(CLSID);
    memset(&clsid, 0, sizeof(CLSID));

    if (!CryptAcquireContext(&provider,
                             nullptr,
                             nullptr,
                             PROV_RSA_FULL,
                             CRYPT_VERIFYCONTEXT))
        goto clsidFromStr_failed;

    if (!CryptCreateHash(provider, CALG_MD5, 0, 0, &hash))
        goto clsidFromStr_failed;

    if (!CryptHashData(hash,
                       reinterpret_cast<const BYTE *>(str.c_str()),
                       DWORD(str.size()),
                       0))
        goto clsidFromStr_failed;

    CryptGetHashParam(hash,
                      HP_HASHVAL,
                      reinterpret_cast<BYTE *>(&clsid),
                      &clsidLen,
                      0);

clsidFromStr_failed:
    if (hash)
        CryptDestroyHash(hash);

    if (provider)
        CryptReleaseContext(provider, 0);

    AkLogDebug() << "CLSID: " << stringFromIid(clsid) << std::endl;

    return clsid;
}

std::string AkVCam::createClsidStrFromStr(const std::string &str)
{
    return stringFromIid(createClsidFromStr(str));
}

std::string AkVCam::stringFromIid(const IID &iid)
{
    LPWSTR strIID = nullptr;
    StringFromIID(iid, &strIID);
    auto str = stringFromWSTR(strIID);
    CoTaskMemFree(strIID);

    return str;
}

std::string AkVCam::stringFromResult(HRESULT result)
{
    auto msg = stringFromWSTR(_com_error(result).ErrorMessage());

    return msg;
}

std::string AkVCam::stringFromClsid(const CLSID &clsid)
{
    static const std::map<CLSID, std::string> clsidToString {
        {IID_IAgileObject         , "IAgileObject"         },
        {IID_IAMAnalogVideoDecoder, "IAMAnalogVideoDecoder"},
        {IID_IAMAudioInputMixer   , "IAMAudioInputMixer"   },
        {IID_IAMAudioRendererStats, "IAMAudioRendererStats"},
        {IID_IAMBufferNegotiation , "IAMBufferNegotiation" },
        {IID_IAMCameraControl     , "IAMCameraControl"     },
        {IID_IAMClockAdjust       , "IAMClockAdjust"       },
        {IID_IAMCrossbar          , "IAMCrossbar"          },
        {IID_IAMDeviceRemoval     , "IAMDeviceRemoval"     },
        {IID_IAMExtDevice         , "IAMExtDevice"         },
        {IID_IAMFilterMiscFlags   , "IAMFilterMiscFlags"   },
        {IID_IAMOpenProgress      , "IAMOpenProgress"      },
        {IID_IAMPushSource        , "IAMPushSource"        },
        {IID_IAMStreamConfig      , "IAMStreamConfig"      },
        {IID_IAMTVTuner           , "IAMTVTuner"           },
        {IID_IAMVfwCaptureDialogs , "IAMVfwCaptureDialogs" },
        {IID_IAMVfwCompressDialogs, "IAMVfwCompressDialogs"},
        {IID_IAMVideoCompression  , "IAMVideoCompression"  },
        {IID_IAMVideoControl      , "IAMVideoControl"      },
        {IID_IAMVideoProcAmp      , "IAMVideoProcAmp"      },
        {IID_IBaseFilter          , "IBaseFilter"          },
        {IID_IBasicAudio          , "IBasicAudio"          },
        {IID_IBasicVideo          , "IBasicVideo"          },
        {IID_IClassFactory        , "IClassFactory"        },
        {IID_IEnumMediaTypes      , "IEnumMediaTypes"      },
        {IID_IEnumPins            , "IEnumPins"            },
        {IID_IFileSinkFilter      , "IFileSinkFilter"      },
        {IID_IFileSinkFilter2     , "IFileSinkFilter2"     },
        {IID_IFileSourceFilter    , "IFileSourceFilter"    },
        {IID_IKsPropertySet       , "IKsPropertySet"       },
        {IID_IMarshal             , "IMarshal"             },
        {IID_IMediaControl        , "IMediaControl"        },
        {IID_IMediaFilter         , "IMediaFilter"         },
        {IID_IMediaPosition       , "IMediaPosition"       },
        {IID_IMediaSample         , "IMediaSample"         },
        {IID_IMediaSample2        , "IMediaSample2"        },
        {IID_IMediaSeeking        , "IMediaSeeking"        },
        {IID_IMediaEventSink      , "IMediaEventSink"      },
        {IID_IMemAllocator        , "IMemAllocator"        },
        {IID_INoMarshal           , "INoMarshal"           },
        {IID_IPersist             , "IPersist"             },
        {IID_IPersistPropertyBag  , "IPersistPropertyBag"  },
        {IID_IPin                 , "IPin"                 },
        {IID_IProvideClassInfo    , "IProvideClassInfo"    },
        {IID_IQualityControl      , "IQualityControl"      },
        {IID_IReferenceClock      , "IReferenceClock"      },
        {IID_IRpcOptions          , "IRpcOptions"          },
        {IID_ISpecifyPropertyPages, "ISpecifyPropertyPages"},
        {IID_IVideoWindow         , "IVideoWindow"         },
        {IID_IUnknown             , "IUnknown"             },
    };

    for (auto &id: clsidToString)
        if (IsEqualCLSID(id.first, clsid))
            return id.second;

    return stringFromIid(clsid);
}

std::string AkVCam::stringFromWSTR(LPCWSTR wstr)
{
    auto len = WideCharToMultiByte(CP_ACP,
                                   0,
                                   wstr,
                                   -1,
                                   nullptr,
                                   0,
                                   nullptr,
                                   nullptr);

    if (len < 1)
        return {};

    auto cstr = new CHAR[len + 1];

    if (!cstr)
        return {};

    memset(cstr, 0, size_t(len + 1) * sizeof(CHAR));
    WideCharToMultiByte(CP_ACP,
                        0,
                        wstr,
                        -1,
                        cstr,
                        len,
                        nullptr,
                        nullptr);
    std::string str(cstr);
    delete [] cstr;

    return str;
}

LPWSTR AkVCam::wstrFromString(const std::string &str)
{
    auto len = MultiByteToWideChar(CP_ACP,
                                   0,
                                   str.c_str(),
                                   -1,
                                   nullptr,
                                   0);

    if (len < 1)
        return nullptr;

    auto wstr = reinterpret_cast<LPWSTR>(CoTaskMemAlloc(size_t(len + 1)
                                                        * sizeof(WCHAR)));

    if (!wstr)
        return nullptr;

    memset(wstr, 0, size_t(len + 1) * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP,
                        0,
                        str.c_str(),
                        -1,
                        wstr,
                        len);

    return wstr;
}

std::string AkVCam::stringFromTSTR(LPCTSTR tstr)
{
    if (!tstr)
         return {};

#ifdef UNICODE
     int required = WideCharToMultiByte(CP_UTF8, 0,
                                        tstr, -1,
                                        nullptr, 0,
                                        nullptr, nullptr);

     if (required <= 0)
         return {};

     std::string result(required - 1, 0);
     WideCharToMultiByte(CP_UTF8, 0,
                         tstr, -1,
                         &result[0], required - 1,
                         nullptr, nullptr);

     return result;
#else
    return {tstr};
#endif
}

LPTSTR AkVCam::tstrFromString(const std::string &str)
{
    if (str.empty())
        return nullptr;

#ifdef UNICODE
    int required = MultiByteToWideChar(CP_UTF8, 0,
                                       str.c_str(),
                                       static_cast<int>(str.size()),
                                       nullptr, 0);

    if (required <= 0)
        return nullptr;

    auto wbuffer =
            reinterpret_cast<LPWSTR>(CoTaskMemAlloc((required + 1)
                                                    * sizeof(WCHAR)));

    if (!wbuffer)
        return nullptr;

    MultiByteToWideChar(CP_UTF8, 0,
                        str.c_str(), (int)str.size(),
                        wbuffer, required);

    wbuffer[required] = L'\0';

    return wbuffer;
#else
    auto buffer = reinterpret_cast<LPTSTR>(CoTaskMemAlloc(str.size() + 1));

    if (!buffer)
        return nullptr;

    strcpy_s(buffer, str.size() + 1, str.c_str());

    return buffer;
#endif
}

AkVCam::PixelFormat AkVCam::formatFromGuid(const GUID &guid)
{
    auto formatSpec = VideoFormatSpecsPrivate::byGuid(guid);

    if (!formatSpec)
        return PixelFormat_none;

    return formatSpec->pixelFormat;
}

const GUID &AkVCam::guidFromFormat(PixelFormat format)
{
    auto formatSpec = VideoFormatSpecsPrivate::byPixelFormat(format);

    if (!formatSpec)
        return GUID_NULL;

    return formatSpec->guid;
}

DWORD AkVCam::compressionFromFormat(PixelFormat format)
{
    auto formatSpec = VideoFormatSpecsPrivate::byPixelFormat(format);

    if (!formatSpec)
        return 0;

    return formatSpec->compression;
}

AkVCam::PixelFormat AkVCam::pixelFormatFromCommonString(const std::string &format)
{
    auto pixelFormat = VideoFormatSpecsPrivate::byName(format.c_str())->pixelFormat;

    if (pixelFormat != PixelFormat_none)
        return pixelFormat;

    return VideoFormat::pixelFormatFromString(format);
}

std::string AkVCam::pixelFormatToCommonString(PixelFormat format)
{
    auto name = std::string(VideoFormatSpecsPrivate::byPixelFormat(format)->name);

    if (!name.empty())
        return name;

    return VideoFormat::pixelFormatToString(format);
}

bool AkVCam::isSubTypeSupported(const GUID &subType)
{
    for (auto it = VideoFormatSpecsPrivate::formats(); it->pixelFormat != PixelFormat_none; ++it)
        if (IsEqualGUID(it->guid, subType))
            return true;

    return false;
}

AM_MEDIA_TYPE *AkVCam::mediaTypeFromFormat(const AkVCam::VideoFormat &format)
{
    auto subtype = guidFromFormat(format.format());

    if (IsEqualGUID(subtype, GUID_NULL))
        return nullptr;

    auto frameSize = format.dataSize();

    if (!frameSize)
        return nullptr;

    auto videoInfo =
            reinterpret_cast<VIDEOINFO *>(CoTaskMemAlloc(sizeof(VIDEOINFO)));
    memset(videoInfo, 0, sizeof(VIDEOINFO));
    auto fps = format.fps();


    // Initialize info header.
    videoInfo->rcSource = {0, 0, 0, 0};
    videoInfo->rcTarget = videoInfo->rcSource;
    videoInfo->dwBitRate = DWORD(8
                                 * frameSize
                                 * size_t(fps.num())
                                 / size_t(fps.den()));
    videoInfo->AvgTimePerFrame = REFERENCE_TIME(TIME_BASE / fps.value());

    // Initialize bitmap header.
    videoInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    videoInfo->bmiHeader.biWidth = format.width();
    videoInfo->bmiHeader.biHeight = format.height();
    videoInfo->bmiHeader.biPlanes = 1;
    videoInfo->bmiHeader.biBitCount = WORD(format.bpp());
    videoInfo->bmiHeader.biCompression = compressionFromFormat(format.format());
    videoInfo->bmiHeader.biSizeImage = DWORD(format.dataSize());

    switch (videoInfo->bmiHeader.biCompression) {
    case BI_RGB:
        if (videoInfo->bmiHeader.biBitCount == 8) {
            videoInfo->bmiHeader.biClrUsed = iPALETTE_COLORS;

            if (HDC hdc = GetDC(nullptr)) {
                PALETTEENTRY palette[iPALETTE_COLORS];

                if (GetSystemPaletteEntries(hdc,
                                            0,
                                            iPALETTE_COLORS,
                                            palette))
                    for (int i = 0; i < iPALETTE_COLORS; i++) {
                        videoInfo->TrueColorInfo.bmiColors[i].rgbRed = palette[i].peRed;
                        videoInfo->TrueColorInfo.bmiColors[i].rgbBlue = palette[i].peBlue;
                        videoInfo->TrueColorInfo.bmiColors[i].rgbGreen = palette[i].peGreen;
                        videoInfo->TrueColorInfo.bmiColors[i].rgbReserved = 0;
                    }

                ReleaseDC(nullptr, hdc);
            }
        }

        break;

    case BI_BITFIELDS: {
            auto masks =
                    VideoFormatSpecsPrivate::byPixelFormat(format.format())->masks;

            if (masks)
                memcpy(videoInfo->TrueColorInfo.dwBitMasks, masks, 3);
        }

        break;

    default:
        break;
    }

    auto mediaType =
            reinterpret_cast<AM_MEDIA_TYPE *>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));
    memset(mediaType, 0, sizeof(AM_MEDIA_TYPE));

    // Initialize media type.
    mediaType->majortype = MEDIATYPE_Video;
    mediaType->subtype = subtype;
    mediaType->bFixedSizeSamples = TRUE;
    mediaType->bTemporalCompression = FALSE;
    mediaType->lSampleSize = ULONG(frameSize);
    mediaType->formattype = FORMAT_VideoInfo;
    mediaType->cbFormat = sizeof(VIDEOINFO);
    mediaType->pbFormat = reinterpret_cast<BYTE *>(videoInfo);

    return mediaType;
}

AkVCam::VideoFormat AkVCam::formatFromMediaType(const AM_MEDIA_TYPE *mediaType)
{
    if (!mediaType)
        return {};

    if (!IsEqualGUID(mediaType->majortype, MEDIATYPE_Video))
        return {};

    if (!isSubTypeSupported(mediaType->subtype))
        return {};

    if (!mediaType->pbFormat)
        return {};

    if (IsEqualGUID(mediaType->formattype, FORMAT_VideoInfo)) {
        auto format = reinterpret_cast<VIDEOINFOHEADER *>(mediaType->pbFormat);
        auto fps = Fraction {uint32_t(TIME_BASE),
                             uint32_t(format->AvgTimePerFrame)};

        return VideoFormat(formatFromGuid(mediaType->subtype),
                           format->bmiHeader.biWidth,
                           std::abs(format->bmiHeader.biHeight),
                           {fps});
    } else if (IsEqualGUID(mediaType->formattype, FORMAT_VideoInfo2)) {
        auto format = reinterpret_cast<VIDEOINFOHEADER2 *>(mediaType->pbFormat);
        auto fps = Fraction {uint32_t(TIME_BASE),
                             uint32_t(format->AvgTimePerFrame)};

        return VideoFormat(formatFromGuid(mediaType->subtype),
                           format->bmiHeader.biWidth,
                           std::abs(format->bmiHeader.biHeight),
                           {fps});
    }

    return {};
}

bool AkVCam::isEqualMediaType(const AM_MEDIA_TYPE *mediaType1,
                              const AM_MEDIA_TYPE *mediaType2,
                              bool exact)
{
    if (mediaType1 == mediaType2)
        return true;

    if (!mediaType1 || !mediaType2)
        return false;

    if (!IsEqualGUID(mediaType1->majortype, mediaType2->majortype)
        || !IsEqualGUID(mediaType1->subtype, mediaType2->subtype)
        || !IsEqualGUID(mediaType1->formattype, mediaType2->formattype))
        return false;

    if (mediaType1->pbFormat == mediaType2->pbFormat)
        return true;

    if (exact)
        return memcmp(mediaType1->pbFormat,
                      mediaType2->pbFormat,
                      mediaType1->cbFormat) == 0;

    if (IsEqualGUID(mediaType1->formattype, FORMAT_VideoInfo)) {
        auto format1 = reinterpret_cast<VIDEOINFOHEADER *>(mediaType1->pbFormat);
        auto format2 = reinterpret_cast<VIDEOINFOHEADER *>(mediaType2->pbFormat);

        if (format1->bmiHeader.biWidth == format2->bmiHeader.biWidth
            && format1->bmiHeader.biHeight == format2->bmiHeader.biHeight)
            return true;
    } else if (IsEqualGUID(mediaType1->formattype, FORMAT_VideoInfo2)) {
        auto format1 = reinterpret_cast<VIDEOINFOHEADER2 *>(mediaType1->pbFormat);
        auto format2 = reinterpret_cast<VIDEOINFOHEADER2 *>(mediaType2->pbFormat);

        if (format1->bmiHeader.biWidth == format2->bmiHeader.biWidth
            && format1->bmiHeader.biHeight == format2->bmiHeader.biHeight)
            return true;
    }

    return false;
}

bool AkVCam::copyMediaType(AM_MEDIA_TYPE *dstMediaType,
                           const AM_MEDIA_TYPE *srcMediaType)
{
    if (!dstMediaType)
        return false;

    if (!srcMediaType) {
        memset(dstMediaType, 0, sizeof(AM_MEDIA_TYPE));

        return false;
    }

    memcpy(dstMediaType, srcMediaType, sizeof(AM_MEDIA_TYPE));

    if (dstMediaType->cbFormat && dstMediaType->pbFormat) {
        dstMediaType->pbFormat =
                reinterpret_cast<BYTE *>(CoTaskMemAlloc(dstMediaType->cbFormat));
        memcpy(dstMediaType->pbFormat,
               srcMediaType->pbFormat,
               dstMediaType->cbFormat);
    }

    return true;
}

AM_MEDIA_TYPE *AkVCam::createMediaType(const AM_MEDIA_TYPE *mediaType)
{
    if (!mediaType)
        return nullptr;

    auto newMediaType =
            reinterpret_cast<AM_MEDIA_TYPE *>(CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE)));
    memcpy(newMediaType, mediaType, sizeof(AM_MEDIA_TYPE));

    if (newMediaType->cbFormat && newMediaType->pbFormat) {
        newMediaType->pbFormat =
                reinterpret_cast<BYTE *>(CoTaskMemAlloc(newMediaType->cbFormat));
        memcpy(newMediaType->pbFormat,
               mediaType->pbFormat,
               newMediaType->cbFormat);
    }

    return newMediaType;
}

void AkVCam::deleteMediaType(AM_MEDIA_TYPE **mediaType)
{
    if (!mediaType || !*mediaType)
        return;

    auto format = (*mediaType)->pbFormat;

    if (format && (*mediaType)->cbFormat)
        CoTaskMemFree(format);

    CoTaskMemFree(*mediaType);
    *mediaType = nullptr;
}

bool AkVCam::containsMediaType(const AM_MEDIA_TYPE *mediaType,
                               IEnumMediaTypes *mediaTypes)
{
    AM_MEDIA_TYPE *mt = nullptr;
    mediaTypes->Reset();
    auto isEqual = false;

    while (mediaTypes->Next(1, &mt, nullptr) == S_OK) {
        isEqual = isEqualMediaType(mt, mediaType);
        deleteMediaType(&mt);

        if (isEqual)
            break;
    }

    return isEqual;
}

std::string AkVCam::stringFromMajorType(const GUID &majorType)
{
    static const std::map<GUID, std::string> mtToStr {
        {GUID_NULL              , "GUID_NULL"              },
        {MEDIATYPE_AnalogAudio  , "MEDIATYPE_AnalogAudio"  },
        {MEDIATYPE_AnalogVideo  , "MEDIATYPE_AnalogVideo"  },
        {MEDIATYPE_Audio        , "MEDIATYPE_Audio"        },
        {MEDIATYPE_AUXLine21Data, "MEDIATYPE_AUXLine21Data"},
        {MEDIATYPE_File         , "MEDIATYPE_File"         },
        {MEDIATYPE_Interleaved  , "MEDIATYPE_Interleaved"  },
        {MEDIATYPE_LMRT         , "MEDIATYPE_LMRT"         },
        {MEDIATYPE_Midi         , "MEDIATYPE_Midi"         },
        {MEDIATYPE_MPEG2_PES    , "MEDIATYPE_MPEG2_PES"    },
        {MEDIATYPE_ScriptCommand, "MEDIATYPE_ScriptCommand"},
        {MEDIATYPE_Stream       , "MEDIATYPE_Stream"       },
        {MEDIATYPE_Text         , "MEDIATYPE_Text"         },
        {MEDIATYPE_Timecode     , "MEDIATYPE_Timecode"     },
        {MEDIATYPE_URL_STREAM   , "MEDIATYPE_URL_STREAM"   },
        {MEDIATYPE_VBI          , "MEDIATYPE_VBI"          },
        {MEDIATYPE_Video        , "MEDIATYPE_Video"        }
    };

    for (auto &mediaType: mtToStr)
        if (IsEqualGUID(mediaType.first, majorType))
            return mediaType.second;

    return stringFromIid(majorType);
}

std::string AkVCam::stringFromSubType(const GUID &subType)
{
    static const std::map<GUID, std::string> mstToStr {
        {GUID_NULL               , "GUID_NULL"               },
        {MEDIASUBTYPE_RGB1       , "MEDIASUBTYPE_RGB1"       },
        {MEDIASUBTYPE_RGB4       , "MEDIASUBTYPE_RGB4"       },
        {MEDIASUBTYPE_RGB8       , "MEDIASUBTYPE_RGB8"       },
        {MEDIASUBTYPE_RGB555     , "MEDIASUBTYPE_RGB555"     },
        {MEDIASUBTYPE_RGB565     , "MEDIASUBTYPE_RGB565"     },
        {MEDIASUBTYPE_RGB24      , "MEDIASUBTYPE_RGB24"      },
        {MEDIASUBTYPE_RGB32      , "MEDIASUBTYPE_RGB32"      },
        {MEDIASUBTYPE_ARGB1555   , "MEDIASUBTYPE_ARGB1555"   },
        {MEDIASUBTYPE_ARGB32     , "MEDIASUBTYPE_ARGB32"     },
        {MEDIASUBTYPE_ARGB4444   , "MEDIASUBTYPE_ARGB4444"   },
        {MEDIASUBTYPE_A2R10G10B10, "MEDIASUBTYPE_A2R10G10B10"},
        {MEDIASUBTYPE_A2B10G10R10, "MEDIASUBTYPE_A2B10G10R10"},
        {MEDIASUBTYPE_AYUV       , "MEDIASUBTYPE_AYUV"       },
        {MEDIASUBTYPE_YUY2       , "MEDIASUBTYPE_YUY2"       },
        {MEDIASUBTYPE_UYVY       , "MEDIASUBTYPE_UYVY"       },
        {MEDIASUBTYPE_IMC1       , "MEDIASUBTYPE_IMC1"       },
        {MEDIASUBTYPE_IMC3       , "MEDIASUBTYPE_IMC3"       },
        {MEDIASUBTYPE_IMC2       , "MEDIASUBTYPE_IMC2"       },
        {MEDIASUBTYPE_IMC4       , "MEDIASUBTYPE_IMC4"       },
        {MEDIASUBTYPE_YV12       , "MEDIASUBTYPE_YV12"       },
        {MEDIASUBTYPE_NV12       , "MEDIASUBTYPE_NV12"       },
        {MEDIASUBTYPE_IF09       , "MEDIASUBTYPE_IF09"       },
        {MEDIASUBTYPE_IYUV       , "MEDIASUBTYPE_IYUV"       },
        {MEDIASUBTYPE_Y211       , "MEDIASUBTYPE_Y211"       },
        {MEDIASUBTYPE_Y411       , "MEDIASUBTYPE_Y411"       },
        {MEDIASUBTYPE_Y41P       , "MEDIASUBTYPE_Y41P"       },
        {MEDIASUBTYPE_YVU9       , "MEDIASUBTYPE_YVU9"       },
        {MEDIASUBTYPE_YVYU       , "MEDIASUBTYPE_YVYU"       }
    };

    for (auto &mediaType: mstToStr)
        if (IsEqualGUID(mediaType.first, subType))
            return mediaType.second;

    return stringFromIid(subType);
}

std::string AkVCam::stringFromFormatType(const GUID &formatType)
{
    static const std::map<GUID, std::string> ftToStr {
        {GUID_NULL          , "GUID_NULL"          },
        {FORMAT_DvInfo      , "FORMAT_DvInfo"      },
        {FORMAT_MPEG2Video  , "FORMAT_MPEG2Video"  },
        {FORMAT_MPEGStreams , "FORMAT_MPEGStreams" },
        {FORMAT_MPEGVideo   , "FORMAT_MPEGVideo"   },
        {FORMAT_None        , "FORMAT_None"        },
        {FORMAT_VideoInfo   , "FORMAT_VideoInfo"   },
        {FORMAT_VideoInfo2  , "FORMAT_VideoInfo2"  },
        {FORMAT_WaveFormatEx, "FORMAT_WaveFormatEx"}
    };

    for (auto &mediaType: ftToStr)
        if (IsEqualGUID(mediaType.first, formatType))
            return mediaType.second;

    return stringFromIid(formatType);
}

std::string AkVCam::stringFromMediaType(const AM_MEDIA_TYPE *mediaType)
{
    if (!mediaType)
        return std::string("MediaType(NULL)");

    std::stringstream ss;
    ss << "MediaType("
       << stringFromMajorType(mediaType->majortype)
       << ", "
       << stringFromSubType(mediaType->subtype)
       << ", "
       << stringFromFormatType(mediaType->formattype);

    if (IsEqualGUID(mediaType->formattype, FORMAT_VideoInfo)) {
        auto format = reinterpret_cast<VIDEOINFOHEADER *>(mediaType->pbFormat);
        ss << ", "
           << format->bmiHeader.biWidth
           << ", "
           << format->bmiHeader.biHeight;
    } else if (IsEqualGUID(mediaType->formattype, FORMAT_VideoInfo2)) {
        auto format = reinterpret_cast<VIDEOINFOHEADER2 *>(mediaType->pbFormat);
        ss << ", "
           << format->bmiHeader.biWidth
           << ", "
           << format->bmiHeader.biHeight;
    }

    ss << ")";

    return ss.str();
}

std::string AkVCam::stringFromMediaSample(IMediaSample *mediaSample)
{
    if (!mediaSample)
        return std::string("MediaSample(NULL)");

    BYTE *buffer = nullptr;
    mediaSample->GetPointer(&buffer);
    auto bufferSize = mediaSample->GetSize();
    AM_MEDIA_TYPE *mediaType = nullptr;
    mediaSample->GetMediaType(&mediaType);
    REFERENCE_TIME timeStart = 0;
    REFERENCE_TIME timeEnd = 0;
    mediaSample->GetTime(&timeStart, &timeEnd);
    REFERENCE_TIME mediaTimeStart = 0;
    REFERENCE_TIME mediaTimeEnd = 0;
    mediaSample->GetMediaTime(&mediaTimeStart, &mediaTimeEnd);
    auto discontinuity = mediaSample->IsDiscontinuity() == S_OK;
    auto preroll = mediaSample->IsPreroll() == S_OK;
    auto syncPoint = mediaSample->IsSyncPoint() == S_OK;
    auto dataLength = mediaSample->GetActualDataLength();

    std::stringstream ss;
    ss << "MediaSample(" << std::endl
       << "    Buffer: " << size_t(buffer) << std::endl
       << "    Buffer Size: " << bufferSize << std::endl
       << "    Media Type: " << stringFromMediaType(mediaType) << std::endl
       << "    Time: (" << timeStart << ", " << timeEnd << ")" << std::endl
       << "    Media Time: (" << mediaTimeStart << ", " << mediaTimeEnd << ")" << std::endl
       << "    Discontinuity: " << discontinuity << std::endl
       << "    Preroll: " << preroll << std::endl
       << "    Sync Point: " << syncPoint << std::endl
       << "    Data Length: " << dataLength << std::endl
       << ")";

    deleteMediaType(&mediaType);

    return ss.str();
}

LSTATUS AkVCam::deleteTree(HKEY key, LPCSTR subkey, REGSAM samFlags)
{
    HKEY mainKey = key;
    LONG result = ERROR_SUCCESS;

    // Open subkey
    if (subkey) {
        result = RegOpenKeyExA(key,
                               subkey,
                               0,
                               KEY_ALL_ACCESS | samFlags,
                               &mainKey);

        if (result != ERROR_SUCCESS)
            return result;
    }

    // list subkeys and values.
    DWORD subKeys = 0;
    DWORD maxSubKeyLen = 0;
    DWORD values = 0;
    DWORD maxValueNameLen = 0;
    result = RegQueryInfoKey(mainKey,
                             nullptr,
                             nullptr,
                             nullptr,
                             &subKeys,
                             &maxSubKeyLen,
                             nullptr,
                             &values,
                             &maxValueNameLen,
                             nullptr,
                             nullptr,
                             nullptr);

    if (result != ERROR_SUCCESS) {
        RegCloseKey(mainKey);

        return result;
    }

    // Delete subkeys
    for (DWORD i = 0; i < subKeys; i++) {
        auto len = maxSubKeyLen + 1;
        CHAR *name = new CHAR[len];
        memset(name, 0, len * sizeof(CCHAR));
        DWORD nameLen = len;
        result = RegEnumKeyExA(mainKey,
                               i,
                               name,
                               &nameLen,
                               nullptr,
                               nullptr,
                               nullptr,
                               nullptr);

        if (result == ERROR_SUCCESS)
            deleteTree(mainKey, name, samFlags);

        delete [] name;
    }

    // Delete values
    for (DWORD i = 0; i < values; i++) {
        auto len = maxValueNameLen + 1;
        TCHAR *name = new TCHAR[len];
        memset(name, 0, len * sizeof(TCHAR));
        DWORD nameLen = len;
        result = RegEnumValue(mainKey,
                              i,
                              name,
                              &nameLen,
                              nullptr,
                              nullptr,
                              nullptr,
                              nullptr);

        if (result == ERROR_SUCCESS)
            RegDeleteValue(mainKey, name);

        delete [] name;
    }

    // Delete this key
    if (subkey) {
        result = RegDeleteKeyExA(key, subkey, samFlags, 0);
        RegCloseKey(mainKey);
    }

    return result;
}

LSTATUS AkVCam::copyTree(HKEY src, LPCSTR subkey, HKEY dst, REGSAM samFlags)
{
    HKEY hkeyFrom = src;
    LONG result = ERROR_SUCCESS;

    // Open source subkey
    if (subkey) {
        result = RegOpenKeyExA(src,
                               subkey,
                               0,
                               KEY_READ | samFlags,
                               &hkeyFrom);

        if (result != ERROR_SUCCESS)
            return result;
    }

    // list subkeys and values.
    DWORD subKeys = 0;
    DWORD maxSubKeyLen = 0;
    DWORD values = 0;
    DWORD maxValueNameLen = 0;
    DWORD maxValueLen = 0;
    result = RegQueryInfoKey(hkeyFrom,
                             nullptr,
                             nullptr,
                             nullptr,
                             &subKeys,
                             &maxSubKeyLen,
                             nullptr,
                             &values,
                             &maxValueNameLen,
                             &maxValueLen,
                             nullptr,
                             nullptr);

    if (result != ERROR_SUCCESS) {
        if (subkey)
            RegCloseKey(hkeyFrom);

        return result;
    }

    // Copy subkeys
    for (DWORD i = 0; i < subKeys; i++) {
        auto len = maxSubKeyLen + 1;
        CHAR *name = new CHAR[len];
        memset(name, 0, len * sizeof(CCHAR));
        DWORD nameLen = len;
        result = RegEnumKeyExA(hkeyFrom,
                               i,
                               name,
                               &nameLen,
                               nullptr,
                               nullptr,
                               nullptr,
                               nullptr);

        if (result == ERROR_SUCCESS) {
            HKEY subkeyTo = nullptr;
            result = RegCreateKeyExA(dst,
                                     name,
                                     0,
                                     nullptr,
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_WRITE | samFlags,
                                     nullptr,
                                     &subkeyTo,
                                     nullptr);

            if (result == ERROR_SUCCESS) {
                copyTree(hkeyFrom, name, subkeyTo, samFlags);
                RegCloseKey(subkeyTo);
            }
        }

        delete [] name;
    }

    // Copy values
    for (DWORD i = 0; i < values; i++) {
        auto len = maxValueNameLen + 1;
        TCHAR *name = new TCHAR[len];
        memset(name, 0, len * sizeof(TCHAR));
        DWORD nameLen = len;
        DWORD dataType = 0;
        BYTE *data = new BYTE[maxValueLen];
        DWORD dataSize = maxValueLen;
        result = RegEnumValue(hkeyFrom,
                              i,
                              name,
                              &nameLen,
                              nullptr,
                              &dataType,
                              data,
                              &dataSize);

        if (result == ERROR_SUCCESS)
            RegSetValueEx(dst,
                          name,
                          0,
                          dataType,
                          data,
                          dataSize);

        delete [] data;
        delete [] name;
    }

    if (subkey)
        RegCloseKey(hkeyFrom);

    return result;
}

AkVCam::VideoFrame AkVCam::loadPicture(const std::string &fileName)
{
    AkLogFunction();
    VideoFrame frame;

    AkLogInfo() << "Loading picture: " << fileName << std::endl;

    if (frame.load(fileName)) {
        AkLogDebug() << "Picture loaded as BMP" << std::endl;

        return frame;
    } else {
        frame = {};
    }

    IWICImagingFactory *imagingFactory = nullptr;
    auto hr = CoCreateInstance(CLSID_WICImagingFactory,
                               nullptr,
                               CLSCTX_INPROC_SERVER,
                               IID_PPV_ARGS(&imagingFactory));

    if (SUCCEEDED(hr)) {
        auto wfileName = wstrFromString(fileName);
        IWICBitmapDecoder *decoder = nullptr;
        hr = imagingFactory->CreateDecoderFromFilename(wfileName,
                                                       nullptr,
                                                       GENERIC_READ,
                                                       WICDecodeMetadataCacheOnLoad,
                                                       &decoder);
        CoTaskMemFree(wfileName);

        if (SUCCEEDED(hr)) {
            IWICBitmapFrameDecode *bmpFrame = nullptr;
            hr = decoder->GetFrame(0, &bmpFrame);

            if (SUCCEEDED(hr)) {
                IWICFormatConverter *formatConverter = nullptr;
                hr = imagingFactory->CreateFormatConverter(&formatConverter);

                if (SUCCEEDED(hr)) {
                    hr = formatConverter->Initialize(bmpFrame,
                                                     GUID_WICPixelFormat24bppRGB,
                                                     WICBitmapDitherTypeNone,
                                                     nullptr,
                                                     0.0,
                                                     WICBitmapPaletteTypeMedianCut);

                    if (SUCCEEDED(hr)) {
                        UINT width = 0;
                        UINT height = 0;
                        formatConverter->GetSize(&width, &height);
                        VideoFormat videoFormat(PixelFormat_rgb24,
                                                int(width),
                                                int(height));
                        frame = VideoFrame(videoFormat);
                        formatConverter->CopyPixels(nullptr,
                                                    3 * width,
                                                    UINT(frame.size()),
                                                    frame.data());
                    }

                    formatConverter->Release();
                }

                bmpFrame->Release();
            }

            decoder->Release();
        }

        imagingFactory->Release();
    }

    AkLogDebug() << "Picture loaded as: "
                 << VideoFormat::pixelFormatToString(frame.format().format())
                 << " "
                 << frame.format().width()
                 << "x"
                 << frame.format().height()
                 << std::endl;

    return frame;
}

std::string AkVCam::logPath(const std::string &logName)
{
    if (logName.empty())
        return {};

    auto defaultLogFile = tempPath() + "\\" + logName + ".log";

    return Preferences::readString("logfile", defaultLogFile);
}

void AkVCam::logSetup(const std::string &context)
{
    auto loglevel = Preferences::logLevel();
    Logger::setLogLevel(loglevel);
    auto contextName = context.empty()? basename(currentBinaryPath()): context;
    Logger::setContext(contextName);
    auto logFile = logPath(contextName);
    AkLogInfo() << "Sending debug output to " << logFile << std::endl;
    Logger::setLogFile(logFile);
}

bool AkVCam::isDeviceIdTaken(const std::string &deviceId)
{
    AkLogFunction();

    // List device IDs in use.
    std::vector<std::string> cameraIds;

    for (size_t i = 0; i < Preferences::camerasCount(); i++)
        cameraIds.push_back(Preferences::cameraId(i));

    // List device CLSIDs in use.
    auto cameraClsids = listAllCameras();

    auto clsid = createClsidFromStr(deviceId);
    auto pit = std::find(cameraIds.begin(), cameraIds.end(), deviceId);
    auto cit = std::find(cameraClsids.begin(), cameraClsids.end(), clsid);

    return pit != cameraIds.end() || cit != cameraClsids.end();
}

std::string AkVCam::createDeviceId()
{
    AkLogFunction();

    // List device IDs in use.
    std::vector<std::string> cameraIds;

    for (size_t i = 0; i < Preferences::camerasCount(); i++)
        cameraIds.push_back(Preferences::cameraId(i));

    // List device CLSIDs in use.
    auto cameraClsids = listAllCameras();
    const int maxId = 64;

    for (int i = 0; i < maxId; i++) {
        /* There are no rules for device IDs in Windows. Just append an
         * incremental index to a common prefix.
         */
        auto id = AKVCAM_DEVICE_PREFIX + std::to_string(i);
        auto clsid = createClsidFromStr(id);
        auto pit = std::find(cameraIds.begin(), cameraIds.end(), id);
        auto cit = std::find(cameraClsids.begin(), cameraClsids.end(), clsid);

        // Check if the ID is being used, if not return it.
        if (pit == cameraIds.end() && cit == cameraClsids.end())
            return id;
    }

    return {};
}

std::string AkVCam::cameraIdFromCLSID(const CLSID &clsid)
{
    auto cameraIndex = Preferences::cameraFromCLSID(clsid);

    if (cameraIndex < 0)
        return {};

    return Preferences::cameraId(cameraIndex);
}

std::vector<CLSID> AkVCam::listAllCameras()
{
    AkLogFunction();

    WCHAR *strIID = nullptr;
    StringFromIID(CLSID_VideoInputDeviceCategory, &strIID);

    std::wstringstream ss;
    ss << L"CLSID\\" << strIID << L"\\Instance";
    CoTaskMemFree(strIID);

    HKEY key = nullptr;
    auto result = RegOpenKeyExW(HKEY_CLASSES_ROOT,
                                ss.str().c_str(),
                                0,
                                MAXIMUM_ALLOWED,
                                &key);

    if (result != ERROR_SUCCESS)
        return {};

    DWORD subkeys = 0;

    result = RegQueryInfoKey(key,
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
        RegCloseKey(key);

        return {};
    }

    std::vector<CLSID> cameras;
    FILETIME lastWrite;

    for (DWORD i = 0; i < subkeys; i++) {
        WCHAR subKey[MAX_PATH];
        memset(subKey, 0, MAX_PATH * sizeof(WCHAR));
        DWORD subKeyLen = MAX_PATH;
        result = RegEnumKeyExW(key,
                               i,
                               subKey,
                               &subKeyLen,
                               nullptr,
                               nullptr,
                               nullptr,
                               &lastWrite);

        if (result == ERROR_SUCCESS) {
            CLSID clsid;
            memset(&clsid, 0, sizeof(CLSID));
            CLSIDFromString(subKey, &clsid);
            cameras.push_back(clsid);
        }
    }

    RegCloseKey(key);

    AkLogDebug() << "Found " << cameras.size() << " available cameras" << std::endl;

    return cameras;
}

std::vector<CLSID> AkVCam::listRegisteredCameras()
{
    AkLogFunction();
    auto pluginPath = locatePluginPath();
    AkLogDebug() << "Plugin path: " << pluginPath << std::endl;

    if (!fileExists(pluginPath)) {
        AkLogError() << "Plugin binary not found: " << pluginPath << std::endl;

        return {};
    }

    std::vector<CLSID> cameras;

    for (auto &clsid: listAllCameras()) {
        auto subKey = "CLSID\\" + stringFromIid(clsid) + "\\InprocServer32";
        CHAR path[MAX_PATH];
        memset(path, 0, MAX_PATH * sizeof(CHAR));
        DWORD pathSize = MAX_PATH;

        if (RegGetValueA(HKEY_CLASSES_ROOT,
                         subKey.c_str(),
                         nullptr,
                         RRF_RT_REG_SZ,
                         nullptr,
                         path,
                         &pathSize) == ERROR_SUCCESS) {
            if (path == pluginPath)
                cameras.push_back(clsid);
        }
    }

    AkLogDebug() << "Found " << cameras.size() << " registered virtual cameras" << std::endl;

    return cameras;
}

std::vector<uint64_t> AkVCam::systemProcesses()
{
    std::vector<uint64_t> pids;

    const DWORD nElements = 4096;
    DWORD process[nElements];
    memset(process, 0, nElements * sizeof(DWORD));
    DWORD needed = 0;

    if (!EnumProcesses(process, nElements * sizeof(DWORD), &needed))
        return {};

    size_t nProcess = needed / sizeof(DWORD);

    for (size_t i = 0; i < nProcess; i++)
        if (process[i] > 0)
            pids.push_back(process[i]);

    return pids;
}

uint64_t AkVCam::currentPid()
{
    return GetCurrentProcessId();
}

std::string AkVCam::exePath(uint64_t pid)
{
    std::string exe;
    auto processHnd = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                                  FALSE,
                                  DWORD(pid));
    if (processHnd) {
        CHAR exeName[MAX_PATH];
        memset(exeName, 0, MAX_PATH * sizeof(CHAR));
        auto size =
                GetModuleFileNameExA(processHnd, nullptr, exeName, MAX_PATH);

        if (size > 0)
            exe = std::string(exeName, size);

        CloseHandle(processHnd);
    }

    return exe;
}

std::string AkVCam::currentBinaryPath()
{
    AkLogFunction();
    char path[MAX_PATH];
    memset(path, 0, MAX_PATH);
    HMODULE hmodule = nullptr;

    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          LPCTSTR(&currentBinaryPath),
                          &hmodule)) {
        GetModuleFileNameA(hmodule, path, MAX_PATH);
    }

    if (strnlen(path, MAX_PATH) < 1)
        return {};

    return {path};
}

bool AkVCam::isServiceRunning()
{
    AkLogFunction();
    auto service = locateServicePath();
    AkLogDebug() << "Service path: " << service << std::endl;
    AkLogDebug() << "System processes:" << std::endl;

    for (auto &pid: systemProcesses()) {
        auto path = exePath(pid);

        if (path.empty())
            continue;

        AkLogDebug() << "    " << path << std::endl;

        if (path == service)
            return true;
    }

    return false;
}

bool AkVCam::isServicePortUp()
{
    AkLogFunction();

    return MessageClient::isUp(Preferences::servicePort());
}

int AkVCam::exec(const std::vector<std::string> &parameters,
                 const std::string &directory,
                 bool show)
{
    AkLogFunction();

    if (parameters.size() < 1)
        return E_FAIL;

    auto command = parameters[0];
    std::string params;
    size_t i = 0;

    for (auto &param: parameters) {
        if (i < 1) {
            i++;

            continue;
        }

        if (!params.empty())
            params += ' ';

        auto param_ = replace(param, "\"", "\"\"\"");

        if (param_.find(" ") == std::string::npos)
            params += param_;
        else
            params += "\"" + param_ + "\"";
    }

    AkLogDebug() << "Command: " << command << std::endl;
    AkLogDebug() << "Arguments: " << params << std::endl;

    SHELLEXECUTEINFOA execInfo;
    memset(&execInfo, 0, sizeof(SHELLEXECUTEINFO));
    execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    execInfo.hwnd = nullptr;
    execInfo.lpVerb = "";
    execInfo.lpFile = command.c_str();
    execInfo.lpParameters = params.c_str();
    execInfo.lpDirectory = directory.empty()? nullptr: directory.c_str();
    execInfo.nShow = show? SW_SHOWNORMAL: SW_HIDE;
    execInfo.hInstApp = nullptr;
    ShellExecuteExA(&execInfo);

    if (!execInfo.hProcess) {
        AkLogError() << "Failed executing command" << std::endl;

        return E_FAIL;
    }

    WaitForSingleObject(execInfo.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(execInfo.hProcess, &exitCode);
    CloseHandle(execInfo.hProcess);

    if (FAILED(exitCode))
        AkLogError() << "Command failed with code "
                     << exitCode
                     << " ("
                     << stringFromError(exitCode)
                     << ")"
                     << std::endl;

    AkLogError() << "Command exited with code " << exitCode << std::endl;

    return int(exitCode);
}

bool AkVCam::execDetached(const std::vector<std::string> &parameters,
                          const std::string &directory,
                          bool show)
{
    AkLogFunction();

    if (parameters.size() < 1)
        return false;

    auto command = parameters[0];
    std::string params;
    size_t i = 0;

    for (auto &param: parameters) {
        if (i < 1) {
            i++;

            continue;
        }

        if (!params.empty())
            params += ' ';

        auto param_ = replace(param, "\"", "\"\"\"");

        if (param_.find(" ") == std::string::npos)
            params += param_;
        else
            params += "\"" + param_ + "\"";
    }

    AkLogDebug() << "Command: " << command << std::endl;
    AkLogDebug() << "Arguments: " << params << std::endl;

    STARTUPINFOA startupInfo;
    memset(&startupInfo, 0, sizeof(STARTUPINFOA));
    PROCESS_INFORMATION processInformation;
    memset(&processInformation, 0, sizeof(PROCESS_INFORMATION));
    char cmdArgs[32768];
    snprintf(cmdArgs, 32768, "%s", params.c_str());

    if (!CreateProcessA(command.c_str(),
                        cmdArgs,
                        nullptr,
                        nullptr,
                        FALSE,
                        CREATE_DEFAULT_ERROR_MODE
                        | (show? 0: CREATE_NO_WINDOW)
                        | DETACHED_PROCESS,
                        nullptr,
                        directory.empty()? nullptr: directory.c_str(),
                        &startupInfo,
                        &processInformation)) {
        LPSTR errorMessage = nullptr;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                     | FORMAT_MESSAGE_FROM_SYSTEM
                                     | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     nullptr,
                                     GetLastError(),
                                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                     reinterpret_cast<LPSTR>(&errorMessage),
                                     0,
                                     nullptr);
        AkLogCritical() << "Failed to execute the command: " << errorMessage << std::endl;
        LocalFree(errorMessage);

        return false;
    }

    CloseHandle(processInformation.hProcess);
    CloseHandle(processInformation.hThread);
    AkLogDebug() << "Command executed" << std::endl;

    return true;
}

const AkVCam::VideoFormatSpecsPrivate *AkVCam::VideoFormatSpecsPrivate::formats()
{
    static const DWORD bits555[] = {0x007c00, 0x0003e0, 0x00001f};
    static const DWORD bits565[] = {0x00f800, 0x0007e0, 0x00001f};

    static const VideoFormatSpecsPrivate akvcamPixelFormatsTable[] = {
        {PixelFormat_bgrx   , "RGB32", BI_RGB                        , MEDIASUBTYPE_RGB32 , nullptr},
        {PixelFormat_rgb24  , "RGB24", BI_RGB                        , MEDIASUBTYPE_RGB24 , nullptr},
        {PixelFormat_rgb565 , "RGB16", BI_BITFIELDS                  , MEDIASUBTYPE_RGB565, bits565},
        {PixelFormat_rgb555 , "RGB15", BI_BITFIELDS                  , MEDIASUBTYPE_RGB555, bits555},
        {PixelFormat_uyvy422, "UYVY" , MAKEFOURCC('U', 'Y', 'V', 'Y'), MEDIASUBTYPE_UYVY  , nullptr},
        {PixelFormat_yuyv422, "YUY2" , MAKEFOURCC('Y', 'U', 'Y', '2'), MEDIASUBTYPE_YUY2  , nullptr},
        {PixelFormat_nv12   , "NV12" , MAKEFOURCC('N', 'V', '1', '2'), MEDIASUBTYPE_NV12  , nullptr},
        {PixelFormat_none   , ""     , 0                             , GUID_NULL          , nullptr}
    };

    return akvcamPixelFormatsTable;
}

const AkVCam::VideoFormatSpecsPrivate *AkVCam::VideoFormatSpecsPrivate::byGuid(const GUID &guid)
{
    auto it = VideoFormatSpecsPrivate::formats();

    for (; it->pixelFormat != PixelFormat_none; ++it)
        if (IsEqualGUID(it->guid, guid))
            return it;

    return it;
}

const AkVCam::VideoFormatSpecsPrivate *AkVCam::VideoFormatSpecsPrivate::byPixelFormat(PixelFormat pixelFormat)
{
    auto it = VideoFormatSpecsPrivate::formats();

    for (; it->pixelFormat != PixelFormat_none; ++it)
        if (it->pixelFormat == pixelFormat)
            return it;

    return it;
}

const AkVCam::VideoFormatSpecsPrivate *AkVCam::VideoFormatSpecsPrivate::byName(const char *name)
{
    auto it = VideoFormatSpecsPrivate::formats();

    for (; it->pixelFormat != PixelFormat_none; ++it)
        if (strcmp(it->name, name) == 0)
            return it;

    return it;
}

std::string AkVCam::currentArchitecture()
{
#ifdef _WIN64
#ifdef __ARM_ARCH
    return {"arm64"};
#else
    return {"x64"};
#endif
#else
#ifdef __ARM_ARCH
    return {"arm32"};
#else
    return {"x86"};
#endif
#endif
}

std::string AkVCam::altArchitecture()
{
    /* 32 bits binaries can be used in 64 bits architectures, but not in the
     * other direction.
     */
#ifdef _WIN64
#ifdef __ARM_ARCH
    return {"arm32"};
#else
    return {"x86"};
#endif
#else
    return {};
#endif
}

std::string AkVCam::pluginInstallPath()
{
    return realPath(dirname(currentBinaryPath()) + "\\..");
}

bool AkVCam::needsRoot(const std::string &task)
{
    UNUSED(task);

    static const char *akvcamRootRequiredTasks[] = {
        "add-device",
        "add-format",
        "load",
        "remove-device",
        "remove-devices",
        "remove-format",
        "remove-formats",
        "set-data-mode",
        "set-description",
        "set-direct-mode",
        "set-loglevel",
        "set-picture",
        "update",
        nullptr
    };

    for (auto str = akvcamRootRequiredTasks; *str; ++str)
        if (*str == task)
            return true;

    return false;
}

int AkVCam::sudo(const std::vector<std::string> &parameters)
{
    // Validate input
    if (parameters.empty())
        return -EINVAL; // Invalid argument

    // Build command line
    std::string commandLine = parameters[0];

    for (size_t i = 1; i < parameters.size(); ++i) {
        commandLine += " ";

        if (parameters[i].find(' ') != std::string::npos)
            commandLine += "\"" + parameters[i] + "\"";
        else
            commandLine += parameters[i];
    }

    // Set up ShellExecuteEx for elevated execution
    SHELLEXECUTEINFOA execInfo;
    memset(&execInfo, 0, sizeof(SHELLEXECUTEINFOA));
    execInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    execInfo.lpVerb = "runas"; // Request elevation
    execInfo.lpFile = parameters[0].c_str();
    execInfo.lpParameters =
            commandLine.substr(parameters[0].length() + 1).c_str();
    execInfo.nShow = SW_HIDE;

    // Execute command with elevation
    if (!ShellExecuteExA(&execInfo) || !execInfo.hProcess)
        return -ENOEXEC; // Executable format error or execution failure

    // Wait for process to complete
    WaitForSingleObject(execInfo.hProcess, INFINITE);

    // Get exit code
    DWORD exitCode;
    GetExitCodeProcess(execInfo.hProcess, &exitCode);
    CloseHandle(execInfo.hProcess);

    return static_cast<int>(exitCode);
}
