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

#ifndef AKVCAMUTILS_VIDEOCONVERTER_H
#define AKVCAMUTILS_VIDEOCONVERTER_H

#include "colorconvert.h"

namespace AkVCam
{
    class VideoConverterPrivate;
    class Rect;
    class VideoFormat;
    class VideoFrame;

    class VideoConverter
    {
        public:
            enum ScalingMode {
                ScalingMode_Fast,
                ScalingMode_Linear
            };

            enum AspectRatioMode {
                AspectRatioMode_Ignore,
                AspectRatioMode_Keep,
                AspectRatioMode_Expanding,
                AspectRatioMode_Fit,
            };

            VideoConverter();
            VideoConverter(const VideoFormat &outputFormat);
            VideoConverter(const VideoConverter &other);
            ~VideoConverter();
            VideoConverter &operator =(const VideoConverter &other);

            VideoFormat outputFormat() const;
            ColorConvert::YuvColorSpace yuvColorSpace() const;
            ColorConvert::YuvColorSpaceType yuvColorSpaceType() const;
            VideoConverter::ScalingMode scalingMode() const;
            VideoConverter::AspectRatioMode aspectRatioMode() const;
            Rect inputRect() const;

            bool begin();
            void end();
            VideoFrame convert(const VideoFrame &frame);

            void setCacheIndex(int index);
            void setOutputFormat(const VideoFormat &outputFormat);
            void setYuvColorSpace(ColorConvert::YuvColorSpace yuvColorSpace);
            void setYuvColorSpaceType(ColorConvert::YuvColorSpaceType yuvColorSpaceType);
            void setScalingMode(VideoConverter::ScalingMode scalingMode);
            void setAspectRatioMode(VideoConverter::AspectRatioMode aspectRatioMode);
            void setInputRect(const Rect &inputRect);
            void reset();

        private:
            VideoConverterPrivate *d;
    };
}

std::ostream &operator <<(std::ostream &os, AkVCam::VideoConverter::ScalingMode mode);
std::ostream &operator <<(std::ostream &os, AkVCam::VideoConverter::AspectRatioMode mode);

#endif // AKVCAMUTILS_VIDEOCONVERTER_H
