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

#include <map>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <dlfcn.h>
#include <CoreGraphics/CGImage.h>
#include <CoreGraphics/CGDataProvider.h>
#include <libproc.h>

#include "utils.h"
#include "preferences.h"
#include "VCamUtils/src/logger.h"
#include "VCamUtils/src/messageclient.h"
#include "VCamUtils/src/utils.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"

namespace AkVCam {
    namespace Utils {
        struct RGB24
        {
            uint8_t b;
            uint8_t g;
            uint8_t r;
        };

        struct RGB32
        {
            uint8_t b;
            uint8_t g;
            uint8_t r;
            uint8_t x;
        };
    }

    class VideoFormatCmio
    {
        public:
            PixelFormat format;
            const char *name;
            FourCharCode fourcc;

            static const VideoFormatCmio *table()
            {
                static const VideoFormatCmio cmioFormatsTable[] = {
                    {PixelFormat_argb   , "RGB32", kCMPixelFormat_32ARGB         },
                    {PixelFormat_rgb24  , "RGB24", kCMPixelFormat_24RGB          },
                    {PixelFormat_rgb565 , "RGB16", kCMPixelFormat_16LE565        },
                    {PixelFormat_rgb555 , "RGB15", kCMPixelFormat_16LE555        },
                    {PixelFormat_uyvy422, "UYVY" , kCMPixelFormat_422YpCbCr8     },
                    {PixelFormat_yuyv422, "YUY2" , kCMPixelFormat_422YpCbCr8_yuvs},
                    {PixelFormat_none   , ""     , 0                             }
                };

                return cmioFormatsTable;
            }

            static inline const VideoFormatCmio *byPixelFormat(PixelFormat format)
            {
                auto it = table();

                for (; it->format != PixelFormat_none; ++it)
                    if (it->format == format)
                        return it;

                return it;
            }

            static inline const VideoFormatCmio *byFourCharCode(FourCharCode fourcc)
            {
                auto it = table();

                for (; it->format != PixelFormat_none; ++it)
                    if (it->fourcc == fourcc)
                        return it;

                return it;
            }

            static inline const VideoFormatCmio *byName(const char *name)
            {
                auto it = table();

                for (; it->format != PixelFormat_none; ++it)
                    if (strcmp(it->name, name) == 0)
                        return it;

                return it;
            }
    };

    std::string pluginInstallPath();
}

std::string AkVCam::locateManagerPath()
{
    auto file = pluginInstallPath() + "/" + DATAROOTDIR "/" AKVCAM_MANAGER_NAME;

    return fileExists(file)? file: std::string();
}

std::string AkVCam::locateServicePath()
{
    auto file = pluginInstallPath() + "/" + DATAROOTDIR "/" AKVCAM_SERVICE_NAME;

    return fileExists(file)? file: std::string();
}

std::string AkVCam::locatePluginPath()
{
    auto file = pluginInstallPath() + "/" + BINDIR "/" AKVCAM_PLUGIN_NAME;

    return fileExists(file)? file: std::string();
}

std::string AkVCam::tempPath()
{
    return {"/tmp"};
}

bool AkVCam::uuidEqual(const REFIID &uuid1, const CFUUIDRef uuid2)
{
    auto iid2 = CFUUIDGetUUIDBytes(uuid2);
    auto puuid1 = reinterpret_cast<const UInt8 *>(&uuid1);
    auto puuid2 = reinterpret_cast<const UInt8 *>(&iid2);

    for (int i = 0; i < 16; i++)
        if (puuid1[i] != puuid2[i])
            return false;

    return true;
}

std::string AkVCam::enumToString(uint32_t value)
{
    auto valueChr = reinterpret_cast<char *>(&value);
    std::stringstream ss;

    for (int i = 3; i >= 0; i--)
        if (valueChr[i] < 0)
            ss << std::hex << valueChr[i];
        else if (valueChr[i] < 32)
            ss << int(valueChr[i]);
        else
            ss << valueChr[i];

    return "'" + ss.str() + "'";
}

FourCharCode AkVCam::formatToCM(PixelFormat format)
{
    return VideoFormatCmio::byPixelFormat(format)->fourcc;
}

AkVCam::PixelFormat AkVCam::formatFromCM(FourCharCode format)
{
    return VideoFormatCmio::byFourCharCode(format)->format;
}

AkVCam::PixelFormat AkVCam::pixelFormatFromCommonString(const std::string &format)
{
    auto pixelFormat = VideoFormatCmio::byName(format.c_str())->format;

    if (pixelFormat != PixelFormat_none)
        return pixelFormat;

    return VideoFormat::pixelFormatFromString(format);
}

std::string AkVCam::pixelFormatToCommonString(PixelFormat format)
{
    auto name = std::string(VideoFormatCmio::byPixelFormat(format)->name);

    if (!name.empty())
        return name;

    return VideoFormat::pixelFormatToString(format);
}

std::string AkVCam::dirname(const std::string &path)
{
    return path.substr(0, path.rfind("/"));
}

std::string AkVCam::realPath(const std::string &path)
{
    char resolvedPath[4096];
    memset(resolvedPath, 0, 4096);
    ::realpath(path.c_str(), resolvedPath);

    char realPath[4096];
    memset(realPath, 0, 4096);
    readlink(resolvedPath, realPath, 4096);

    if (strnlen(realPath, 4096) < 1)
        return {resolvedPath};

    return {realPath};
}

AkVCam::VideoFrame AkVCam::loadPicture(const std::string &fileName)
{
    AkLogFunction();
    VideoFrame frame;

    if (frame.load(fileName)) {
        AkLogInfo() << "Picture loaded as BMP" << std::endl;

        return frame;
    }

#ifndef FAKE_APPLE
    auto fileDataProvider = CGDataProviderCreateWithFilename(fileName.c_str());

    if (!fileDataProvider) {
        AkLogError() << "Can't create a data provider for '"
                     << fileName
                     << "'"
                     << std::endl;

        return {};
    }

    // Check if the file is a PNG and open it.
    auto cgImage = CGImageCreateWithPNGDataProvider(fileDataProvider,
                                                    nullptr,
                                                    true,
                                                    kCGRenderingIntentDefault);

    // If the file is not a PNG, try opening as JPEG.
    if (!cgImage) {
        AkLogWarning() << "Can't read '"
                       << fileName
                       << "' as a PNG image."
                       << std::endl;
        cgImage = CGImageCreateWithJPEGDataProvider(fileDataProvider,
                                                    nullptr,
                                                    true,
                                                    kCGRenderingIntentDefault);
    }

    CGDataProviderRelease(fileDataProvider);

    // The file format is not supported, fail.
    if (!cgImage) {
        AkLogError() << "Can't read '"
                     << fileName
                     << "' as a JPEG image."
                     << std::endl;

        return {};
    }

    PixelFormat format = PixelFormat_none;

    if (CGImageGetBitsPerComponent(cgImage) == 8) {
        if (CGImageGetBitsPerPixel(cgImage) == 24)
            format = PixelFormat_rgb24;
        else if (CGImageGetBitsPerPixel(cgImage) == 32) {
            format = PixelFormat_xrgb;
        }
    }

    auto width = CGImageGetWidth(cgImage);
    auto height = CGImageGetHeight(cgImage);

    if (format == 0 || width < 1 || height < 1) {
        AkLogError() << "Invalid picture format: "
                     << "BPC="
                     << CGImageGetBitsPerComponent(cgImage)
                     << "BPP="
                     << CGImageGetBitsPerPixel(cgImage)
                     << " "
                     << width
                     << "x"
                     << height
                     << std::endl;
        CGImageRelease(cgImage);

        return {};
    }

    VideoFormat videoFormat(PixelFormat_rgb24, width, height);
    frame = VideoFrame(videoFormat);

    auto imageDataProvider = CGImageGetDataProvider(cgImage);

    if (!imageDataProvider) {
        AkLogError() << "Can't get data provider for picture." << std::endl;
        CGImageRelease(cgImage);

        return {};
    }

    auto data = CGDataProviderCopyData(imageDataProvider);

    if (!data) {
        AkLogError() << "Can't copy data from image provider." << std::endl;
        CGImageRelease(cgImage);

        return {};
    }

    auto lineSize = CGImageGetBytesPerRow(cgImage);

    if (CGImageGetBitsPerPixel(cgImage) == 24) {
        for (int y = 0; y < videoFormat.height(); y++) {
            auto srcLine = reinterpret_cast<const Utils::RGB24 *>(CFDataGetBytePtr(data) + y * lineSize);
            auto dstLine = reinterpret_cast<Utils::RGB24 *>(frame.line(0, y));

            for (int x = 0; x < videoFormat.height(); x++) {
                dstLine[x].r = srcLine[x].r;
                dstLine[x].g = srcLine[x].g;
                dstLine[x].b = srcLine[x].b;
            }
        }
    } else if (CGImageGetBitsPerPixel(cgImage) == 32) {
        if (CGImageGetAlphaInfo(cgImage) == kCGImageAlphaNone) {
            for (int y = 0; y < videoFormat.height(); y++) {
                auto srcLine = reinterpret_cast<const Utils::RGB32 *>(CFDataGetBytePtr(data) + y * lineSize);
                auto dstLine = reinterpret_cast<Utils::RGB24 *>(frame.line(0, y));

                for (int x = 0; x < videoFormat.height(); x++) {
                    dstLine[x].r = srcLine[x].r;
                    dstLine[x].g = srcLine[x].g;
                    dstLine[x].b = srcLine[x].b;
                }
            }
        } else {
            for (int y = 0; y < videoFormat.height(); y++) {
                auto srcLine = reinterpret_cast<const Utils::RGB32 *>(CFDataGetBytePtr(data) + y * lineSize);
                auto dstLine = reinterpret_cast<Utils::RGB24 *>(frame.line(0, y));

                for (int x = 0; x < videoFormat.width(); x++) {
                    dstLine[x].r = srcLine[x].r * srcLine[x].x / 255;
                    dstLine[x].g = srcLine[x].g * srcLine[x].x / 255;
                    dstLine[x].b = srcLine[x].b * srcLine[x].x / 255;
                }
            }
        }
    }

    CFRelease(data);
    CGImageRelease(cgImage);

    AkLogDebug() << "Picture loaded as: "
                 << VideoFormat::pixelFormatToString(frame.format().format())
                 << " "
                 << frame.format().width()
                 << "x"
                 << frame.format().height()
                 << std::endl;

    return frame;
#else
    return {};
#endif
}

bool AkVCam::fileExists(const std::string &path)
{
    struct stat stats;
    memset(&stats, 0, sizeof(struct stat));

    return stat(path.c_str(), &stats) == 0;
}

bool AkVCam::readEntitlements(const std::string &app,
                              const std::string &output)
{
    bool writen = false;
    std::string cmd = "codesign -d --entitlements - \"" + app + "\"";
    auto proc = popen(cmd.c_str(), "r");

    if (proc) {
        auto entitlements = fopen(output.c_str(), "w");

        if (entitlements) {
            for (size_t i = 0; !feof(proc); i++) {
                char data[1024];
                auto len = fread(data, 1, 1024, proc);

                if (len < 1)
                    break;

                size_t offset = 0;

                if (i == 0)
                    offset = std::string(data, len).find("<?xml");

                fwrite(data + offset, 1, len - offset, entitlements);
                writen = true;
            }

            fclose(entitlements);
        }

        pclose(proc);
    }

    return writen;
}

bool AkVCam::isDeviceIdTaken(const std::string &deviceId)
{
    AkLogFunction();

    // List device IDs in use.
    std::vector<std::string> cameraIds;

    for (size_t i = 0; i < Preferences::camerasCount(); i++)
        cameraIds.push_back(Preferences::cameraId(i));

    return std::find(cameraIds.begin(),
                     cameraIds.end(),
                     deviceId) != cameraIds.end();
}

std::string AkVCam::createDeviceId()
{
    AkLogFunction();

    // List device IDs in use.
    std::vector<std::string> cameraIds;

    for (size_t i = 0; i < Preferences::camerasCount(); i++)
        cameraIds.push_back(Preferences::cameraId(i));

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

std::vector<uint64_t> AkVCam::systemProcesses()
{
    auto npids = proc_listallpids(nullptr, 0);
    pid_t pidsvec[npids];
    memset(pidsvec, 0, npids * sizeof(pid_t));
    npids = std::min(proc_listallpids(pidsvec, npids * sizeof(pid_t)), npids);
    std::vector<uint64_t> pids;

    for (int i = 0; i < npids; i++) {
        auto it = std::find(pids.begin(), pids.end(), pidsvec[i]);

        if (pidsvec[i] > 0 && it == pids.end())
            pids.push_back(pidsvec[i]);
    }

    return pids;
}

uint64_t AkVCam::currentPid()
{
    return getpid();
}

std::string AkVCam::exePath(uint64_t pid)
{
    char path[4096];
    memset(path, 0, 4096);
    proc_pidpath(pid, path, 4096);

    return {path};
}

std::string AkVCam::currentBinaryPath()
{
    Dl_info info;
    memset(&info, 0, sizeof(Dl_info));

    if (dladdr(reinterpret_cast<void *>(currentBinaryPath), &info) == 0)
        return exePath(currentPid());

    return {info.dli_fname};
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
    return MessageClient::isUp(Preferences::servicePort());
}

std::string AkVCam::pluginInstallPath()
{
    return realPath(dirname(currentBinaryPath()) + "/../../..");
}

bool AkVCam::needsRoot(const std::string &task)
{
    UNUSED(task);

    return false;
}

int AkVCam::sudo(const std::vector<std::string> &parameters)
{
    UNUSED(parameters);

    return 0;
}
