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

#ifndef AKVCAMUTILS_VIDEOFORMAT_H
#define AKVCAMUTILS_VIDEOFORMAT_H

#include <string>
#include <vector>

#include "videoformattypes.h"

namespace AkVCam
{
    class VideoFormat;
    class VideoFormatPrivate;
    class Fraction;
    class VideoFormatSpec;
    using VideoFormats = std::vector<VideoFormat>;

    class VideoFormat
    {
        public:
            VideoFormat();
            VideoFormat(PixelFormat format,
                        int width,
                        int height);
            VideoFormat(PixelFormat format,
                        int width,
                        int height,
                        const Fraction &fps);
            VideoFormat(const VideoFormat &other);
            ~VideoFormat();
            VideoFormat &operator =(const VideoFormat &other);
            bool operator ==(const VideoFormat &other) const;
            bool operator !=(const VideoFormat &other) const;
            operator bool() const;

            PixelFormat format() const;
            int width() const;
            int height() const;
            Fraction fps() const;
            size_t bpp() const;

            void setFormat(PixelFormat format);
            void setWidth(int width);
            void setHeight(int height);
            void setFps(const Fraction &fps);

            VideoFormat nearest(const VideoFormats &caps) const;
            bool isSameFormat(const VideoFormat &other) const;
            size_t dataSize() const;
            bool isValid() const;

            static int bitsPerPixel(PixelFormat pixelFormat);
            static std::string pixelFormatToString(PixelFormat pixelFormat);
            static PixelFormat pixelFormatFromString(const std::string &pixelFormat);
            static VideoFormatSpec formatSpecs(PixelFormat pixelFormat);
            static std::vector<PixelFormat> supportedPixelFormats();

        private:
            VideoFormatPrivate *d;

        friend bool operator <(const VideoFormat &format1, const VideoFormat &format2);
    };
}

std::ostream &operator <<(std::ostream &os, const AkVCam::VideoFormat &format);
std::ostream &operator <<(std::ostream &os, const AkVCam::VideoFormats &formats);
bool operator <(const AkVCam::VideoFormat &format1, const AkVCam::VideoFormat &format2);

#endif // AKVCAMUTILS_VIDEOFORMAT_H
