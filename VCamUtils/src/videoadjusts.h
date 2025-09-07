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

#ifndef AKVCAMUTILS_VIDEOADJUSTS_H
#define AKVCAMUTILS_VIDEOADJUSTS_H

namespace AkVCam
{
    class VideoAdjustsPrivate;
    class VideoFrame;

    class VideoAdjusts
    {
        public:
            VideoAdjusts();
            ~VideoAdjusts();

            bool horizontalMirror() const;
            bool verticalMirror() const;
            bool swapRGB() const;
            int hue() const;
            int saturation() const;
            int luminance() const;
            int gamma() const;
            int contrast() const;
            bool grayScaled() const;

            void setHorizontalMirror(bool horizontalMirror);
            void setVerticalMirror(bool verticalMirror);
            void setSwapRGB(bool swapRGB);
            void setHue(int hue);
            void setSaturation(int saturation);
            void setLuminance(int luminance);
            void setGamma(int gamma);
            void setContrast(int contrast);
            void setGrayScaled(bool grayScaled);

            VideoFrame adjust(const VideoFrame &frame);

        private:
            VideoAdjustsPrivate *d;
    };
}

#endif // AKVCAMUTILS_VIDEOADJUSTS_H
