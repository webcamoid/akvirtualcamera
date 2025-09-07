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

#ifndef AKVCAMUTILS_COMMONS_H
#define AKVCAMUTILS_COMMONS_H

#define ENDIANNESS_LE 1234
#define ENDIANNESS_BE 4321
#define ENDIANNESS_BO ENDIANNESS_LE

#if ENDIANNESS_BO == ENDIANNESS_LE
    #define AKVCAM_MAKE_FOURCC_LE(a, b, c, d) \
          (((uint32_t(a) & 0xff)) \
         | ((uint32_t(b) & 0xff) <<  8) \
         | ((uint32_t(c) & 0xff) << 16) \
         |  (uint32_t(d) & 0xff) << 24)

    #define AKVCAM_MAKE_FOURCC_BE(a, b, c, d) \
        (((uint32_t(a) & 0xff) << 24) \
         | ((uint32_t(b) & 0xff) << 16) \
         | ((uint32_t(c) & 0xff) <<  8) \
         |  (uint32_t(d) & 0xff))
    #define AKVCAM_MAKE_FOURCC(a, b, c, d) AKVCAM_MAKE_FOURCC_LE(a, b, c, d)
#else
    #define AKVCAM_MAKE_FOURCC_LE(a, b, c, d) \
          (((uint32_t(a) & 0xff) << 24) \
         | ((uint32_t(b) & 0xff) << 16) \
         | ((uint32_t(c) & 0xff) <<  8) \
         |  (uint32_t(d) & 0xff))
    #define AKVCAM_MAKE_FOURCC_BE(a, b, c, d) \
          (((uint32_t(a) & 0xff)) \
         | ((uint32_t(b) & 0xff) <<  8) \
         | ((uint32_t(c) & 0xff) << 16) \
         |  (uint32_t(d) & 0xff) << 24)
    #define AKVCAM_MAKE_FOURCC(a, b, c, d) AKVCAM_MAKE_FOURCC_BE(a, b, c, d)
#endif

#endif // AKVCAMUTILS_COMMONS_H
