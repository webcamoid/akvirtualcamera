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

#ifndef AKVCAMUTILS_VIDEOFORMATTYPES_H
#define AKVCAMUTILS_VIDEOFORMATTYPES_H

#include <cstdint>

#include "commons.h"

namespace AkVCam
{
    enum PixelFormat
    {
        PixelFormat_none    = AKVCAM_MAKE_FOURCC(0, 0, 0, 0),
        PixelFormat_unknown = PixelFormat_none,

        // RGB formats
        PixelFormat_xrgb     = AKVCAM_MAKE_FOURCC('X', 'R', 'G', 'B'),
        PixelFormat_rgb24    = AKVCAM_MAKE_FOURCC('R', 'G', 'B', 24),
        PixelFormat_rgb555be = AKVCAM_MAKE_FOURCC_BE('R', 'G', 5, 55),
        PixelFormat_rgb555le = AKVCAM_MAKE_FOURCC_LE('R', 'G', 5, 55),
        PixelFormat_rgb565be = AKVCAM_MAKE_FOURCC_BE('R', 'G', 5, 65),
        PixelFormat_rgb565le = AKVCAM_MAKE_FOURCC_LE('R', 'G', 5, 65),
        PixelFormat_argb     = AKVCAM_MAKE_FOURCC('A', 'R', 'G', 'B'),

        // BGR formats
        PixelFormat_bgrx     = AKVCAM_MAKE_FOURCC('B', 'G', 'R', 'X'),
        PixelFormat_bgr24    = AKVCAM_MAKE_FOURCC('B', 'G', 'R', 24),
        PixelFormat_bgr555be = AKVCAM_MAKE_FOURCC_BE('B', 'G', 5, 55),
        PixelFormat_bgr555le = AKVCAM_MAKE_FOURCC_LE('B', 'G', 5, 55),
        PixelFormat_bgr565be = AKVCAM_MAKE_FOURCC_BE('B', 'G', 5, 65),
        PixelFormat_bgr565le = AKVCAM_MAKE_FOURCC_LE('B', 'G', 5, 65),
        PixelFormat_bgra     = AKVCAM_MAKE_FOURCC('B', 'G', 'R', 'A'),

        // Luminance+Chrominance formats
        PixelFormat_uyvy422 = AKVCAM_MAKE_FOURCC('U', 'Y', 'V', 'Y'),
        PixelFormat_yuyv422 = AKVCAM_MAKE_FOURCC('Y', 'U', 'Y', 2),

        // two planes -- one Y, one Cr + Cb interleaved
        PixelFormat_nv12 = AKVCAM_MAKE_FOURCC('N', 'V', 12, 0),
        PixelFormat_nv21 = AKVCAM_MAKE_FOURCC('N', 'V', 21, 0),

#if ENDIANNESS_BO == ENDIANNESS_LE
        PixelFormat_xrgbpackbe    = PixelFormat_xrgb,
        PixelFormat_xrgbpackle    = PixelFormat_bgrx,
        PixelFormat_xrgbpack      = PixelFormat_xrgbpackle,
        PixelFormat_bgrxpackbe    = PixelFormat_bgrx,
        PixelFormat_bgrxpackle    = PixelFormat_xrgb,
        PixelFormat_bgrxpack      = PixelFormat_bgrxpackle,
        PixelFormat_rgb555        = PixelFormat_rgb555le,
        PixelFormat_rgb565        = PixelFormat_rgb565le,
        PixelFormat_bgr555        = PixelFormat_bgr555le,
        PixelFormat_bgr565        = PixelFormat_bgr565le,
        PixelFormat_argbpackbe    = PixelFormat_argb,
        PixelFormat_argbpackle    = PixelFormat_bgra,
        PixelFormat_argbpack      = PixelFormat_argbpackle,
#else
        PixelFormat_xrgbpackbe    = PixelFormat_bgrx,
        PixelFormat_xrgbpackle    = PixelFormat_xrgb,
        PixelFormat_xrgbpack      = PixelFormat_xrgbpackbe,
        PixelFormat_bgrxpackbe    = PixelFormat_xrgb,
        PixelFormat_bgrxpackle    = PixelFormat_bgrx,
        PixelFormat_bgrxpack      = PixelFormat_bgrxpackbe,
        PixelFormat_rgb555        = PixelFormat_rgb555be,
        PixelFormat_rgb565        = PixelFormat_rgb565be,
        PixelFormat_bgr555        = PixelFormat_bgr555be,
        PixelFormat_bgr565        = PixelFormat_bgr565be,
        PixelFormat_argbpackbe    = PixelFormat_bgra,
        PixelFormat_argbpackle    = PixelFormat_argb,
        PixelFormat_argbpack      = PixelFormat_argbpackbe,
#endif
    };
}

#endif // AKVCAMUTILS_VIDEOFORMATTYPES_H
