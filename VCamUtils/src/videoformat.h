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
#include "fraction.h"

namespace AkVCam
{
    class VideoFormat;
    class VideoFormatPrivate;
    using VideoFormats = std::vector<VideoFormat>;

    class VideoFormat
    {
        public:
            VideoFormat();
            VideoFormat(FourCC fourcc,
                        int width,
                        int height,
                        const std::vector<Fraction> &frameRates={});
            VideoFormat(const VideoFormat &other);
            ~VideoFormat();
            VideoFormat &operator =(const VideoFormat &other);
            bool operator ==(const VideoFormat &other) const;
            bool operator !=(const VideoFormat &other) const;
            operator bool() const;

            FourCC fourcc() const;
            FourCC &fourcc();
            int width() const;
            int &width();
            int height() const;
            int &height();
            std::vector<Fraction> frameRates() const;
            std::vector<Fraction> &frameRates();
            std::vector<FractionRange> frameRateRanges() const;
            Fraction minimumFrameRate() const;
            size_t bpp() const;
            size_t bypl(size_t plane) const;
            size_t size() const;
            size_t planes() const;
            size_t offset(size_t plane) const;
            size_t planeSize(size_t plane) const;
            bool isValid() const;
            void clear();
            VideoFormat nearest(const std::vector<VideoFormat> &formats) const;

            static void roundNearest(int width, int height,
                                     int *owidth, int *oheight,
                                     int align=32);
            static FourCC fourccFromString(const std::string &fourccStr);
            static std::string stringFromFourcc(FourCC fourcc);

        private:
            VideoFormatPrivate *d;
    };
}

std::ostream &operator <<(std::ostream &os, const AkVCam::VideoFormat &format);
std::ostream &operator <<(std::ostream &os, const AkVCam::VideoFormats &formats);

#endif // AKVCAMUTILS_VIDEOFORMAT_H
