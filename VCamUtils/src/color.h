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

#ifndef AKVCAMUTILS_COLOR_H
#define AKVCAMUTILS_COLOR_H

#include <cstdint>

#include "utils.h"

namespace AkVCam
{
    namespace Color
    {
        inline uint32_t rgb(uint32_t r, uint32_t g, uint32_t b, uint32_t a=255)
        {
            return (a << 24) | (r << 16) | (g << 8) | b;
        }

        inline uint32_t red(uint32_t rgba)
        {
            return (rgba >> 16) & 0xff;
        }

        inline uint32_t green(uint32_t rgba)
        {
            return (rgba >> 8) & 0xff;
        }

        inline uint32_t blue(uint32_t rgba)
        {
            return rgba & 0xff;
        }

        inline uint32_t alpha(uint32_t rgba)
        {
            return rgba >> 24;
        }

        inline int grayval(int r, int g, int b)
        {
            return (11 * r + 16 * g + 5 * b) >> 5;
        }

        inline int gray(uint32_t rgba)
        {
            auto luma = grayval(int(red(rgba)),
                                int(green(rgba)),
                                int(blue(rgba)));

            return rgb(luma, luma, luma, alpha(rgba));
        }

        inline uint8_t rgb_y(int r, int g, int b)
        {
            return uint8_t(((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);
        }

        inline uint8_t rgb_u(int r, int g, int b)
        {
            return uint8_t(((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);
        }

        inline uint8_t rgb_v(int r, int g, int b)
        {
            return uint8_t(((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);
        }

        inline uint8_t yuv_r(int y, int u, int v)
        {
            UNUSED(u);
            int r = (298 * (y - 16) + 409 * (v - 128) + 128) >> 8;

            return uint8_t(bound(0, r, 255));
        }

        inline uint8_t yuv_g(int y, int u, int v)
        {
            int g = (298 * (y - 16) - 100 * (u - 128) - 208 * (v - 128) + 128) >> 8;

            return uint8_t(bound(0, g, 255));
        }

        inline uint8_t yuv_b(int y, int u, int v)
        {
            UNUSED(v);
            int b = (298 * (y - 16) + 516 * (u - 128) + 128) >> 8;

            return uint8_t(bound(0, b, 255));
        }
    }
}

#endif // AKVCAMUTILS_COLOR_H
