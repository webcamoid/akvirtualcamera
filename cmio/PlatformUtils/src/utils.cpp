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
#import <CoreGraphics/CGImage.h>
#import <CoreGraphics/CGDataProvider.h>

#include "utils.h"
#include "VCamUtils/src/logger.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"

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

        inline const std::map<AkVCam::PixelFormat, FourCharCode> *formatsTable()
        {
            static const std::map<AkVCam::PixelFormat, FourCharCode> formatsTable {
                {AkVCam::PixelFormatRGB32, kCMPixelFormat_32ARGB         },
                {AkVCam::PixelFormatRGB24, kCMPixelFormat_24RGB          },
                {AkVCam::PixelFormatRGB16, kCMPixelFormat_16LE565        },
                {AkVCam::PixelFormatRGB15, kCMPixelFormat_16LE555        },
                {AkVCam::PixelFormatUYVY , kCMPixelFormat_422YpCbCr8     },
                {AkVCam::PixelFormatYUY2 , kCMPixelFormat_422YpCbCr8_yuvs}
            };

            return &formatsTable;
        }
    }
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

std::string AkVCam::enumToString(UInt32 value)
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
    for (auto &fmt: *Utils::formatsTable())
        if (fmt.first == format)
            return fmt.second;

    return FourCharCode(0);
}

AkVCam::PixelFormat AkVCam::formatFromCM(FourCharCode format)
{
    for (auto &fmt: *Utils::formatsTable())
        if (fmt.second == format)
            return fmt.first;

    return PixelFormat(0);
}

std::shared_ptr<CFTypeRef> AkVCam::cfTypeFromStd(const std::string &str)
{
    auto ref =
            new CFTypeRef(CFStringCreateWithCString(kCFAllocatorDefault,
                                                    str.c_str(),
                                                    kCFStringEncodingUTF8));

    return std::shared_ptr<CFTypeRef>(ref, [] (CFTypeRef *ptr) {
        CFRelease(*ptr);
        delete ptr;
    });
}

std::shared_ptr<CFTypeRef> AkVCam::cfTypeFromStd(int num)
{
    auto ref =
            new CFTypeRef(CFNumberCreate(kCFAllocatorDefault,
                                         kCFNumberIntType,
                                         &num));

    return std::shared_ptr<CFTypeRef>(ref, [] (CFTypeRef *ptr) {
        CFRelease(*ptr);
        delete ptr;
    });
}

std::shared_ptr<CFTypeRef> AkVCam::cfTypeFromStd(double num)
{
    auto ref =
            new CFTypeRef(CFNumberCreate(kCFAllocatorDefault,
                                         kCFNumberDoubleType,
                                         &num));

    return std::shared_ptr<CFTypeRef>(ref, [] (CFTypeRef *ptr) {
        CFRelease(*ptr);
        delete ptr;
    });
}

std::string AkVCam::stringFromCFType(CFTypeRef cfType)
{
    auto len = size_t(CFStringGetLength(CFStringRef(cfType)));
    auto data = CFStringGetCStringPtr(CFStringRef(cfType),
                                      kCFStringEncodingUTF8);

    if (data)
        return std::string(data, len);

    auto maxLen =
            CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8)  + 1;
    auto cstr = new char[maxLen];
    memset(cstr, 0, maxLen);

    if (!CFStringGetCString(CFStringRef(cfType),
                            cstr,
                            maxLen,
                            kCFStringEncodingUTF8)) {
        delete [] cstr;

        return {};
    }

    std::string str(cstr, len);
    delete [] cstr;

    return str;
}

std::string AkVCam::realPath(const std::string &path)
{
    char resolvedPath[PATH_MAX];
    memset(resolvedPath, 0, PATH_MAX);
    ::realpath(path.c_str(), resolvedPath);

    char realPath[PATH_MAX];
    memset(realPath, 0, PATH_MAX);
    readlink(resolvedPath, realPath, PATH_MAX);

    if (strlen(realPath) < 1)
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

    FourCC format = 0;

    if (CGImageGetBitsPerComponent(cgImage) == 8) {
        if (CGImageGetBitsPerPixel(cgImage) == 24)
            format = PixelFormatRGB24;
        else if (CGImageGetBitsPerPixel(cgImage) == 32) {
            format = PixelFormatRGB32;
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

    VideoFormat videoFormat(PixelFormatRGB24, width, height);
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
                 << VideoFormat::stringFromFourcc(frame.format().fourcc())
                 << " "
                 << frame.format().width()
                 << "x"
                 << frame.format().height()
                 << std::endl;

    return frame;
}
