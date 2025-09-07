/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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

#ifndef AKVCAMUTILS_VIDEOFORMATSPEC_H
#define AKVCAMUTILS_VIDEOFORMATSPEC_H

#include "colorplane.h"

namespace AkVCam
{
    class VideoFormatSpec;
    class VideoFormatSpecPrivate;

    class VideoFormatSpec
    {
        public:
            enum VideoFormatType
            {
                VFT_Unknown,
                VFT_RGB,
                VFT_YUV,
                VFT_Gray
            };

            VideoFormatSpec();
            VideoFormatSpec(VideoFormatType type,
                            int endianness,
                            const ColorPlanes &planes);
            VideoFormatSpec(const VideoFormatSpec &other);
            ~VideoFormatSpec();
            VideoFormatSpec &operator =(const VideoFormatSpec &other);
            bool operator ==(const VideoFormatSpec &other) const;
            bool operator !=(const VideoFormatSpec &other) const;

            VideoFormatSpec::VideoFormatType type() const;
            int endianness() const;
            size_t planes() const;
            const ColorPlane &plane(size_t plane) const;
            int bpp() const;
            ColorComponent component(ColorComponent::ComponentType componentType) const;
            int componentPlane(ColorComponent::ComponentType component) const;
            bool contains(ColorComponent::ComponentType component) const;
            size_t byteDepth() const;
            size_t depth() const;
            size_t numberOfComponents() const;
            size_t mainComponents() const;
            bool isFast() const;

        private:
            VideoFormatSpecPrivate *d;
    };
}

std::ostream &operator <<(std::ostream &os, const AkVCam::VideoFormatSpec &spec);
std::ostream &operator <<(std::ostream &os, AkVCam::VideoFormatSpec::VideoFormatType type);

#endif // AKVCAMUTILS_VIDEOFORMATSPEC_H
