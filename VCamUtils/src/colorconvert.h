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

#ifndef AKVCAMUTILS_COLORCONVERT_H
#define AKVCAMUTILS_COLORCONVERT_H

#include "videoformattypes.h"
#include "videoformatspec.h"
#include "utils.h"

namespace AkVCam
{
    class ColorConvertPrivate;

    class ColorConvert
    {
        public:
            enum YuvColorSpace
            {
                YuvColorSpace_AVG,
                YuvColorSpace_ITUR_BT601,
                YuvColorSpace_ITUR_BT709,
                YuvColorSpace_ITUR_BT2020,
                YuvColorSpace_SMPTE_240M
            };

            enum YuvColorSpaceType
            {
                YuvColorSpaceType_StudioSwing,
                YuvColorSpaceType_FullSwing
            };

            enum ColorMatrix
            {
                ColorMatrix_ABC2XYZ,
                ColorMatrix_RGB2YUV,
                ColorMatrix_YUV2RGB,
                ColorMatrix_RGB2GRAY,
                ColorMatrix_GRAY2RGB,
                ColorMatrix_YUV2GRAY,
                ColorMatrix_GRAY2YUV,
            };

            ColorConvert();
            ColorConvert(YuvColorSpace yuvColorSpace,
                         YuvColorSpaceType yuvColorSpaceType=YuvColorSpaceType_StudioSwing);
            ColorConvert(YuvColorSpaceType yuvColorSpaceType);
            ColorConvert(const ColorConvert &other);
            ~ColorConvert();
            ColorConvert &operator =(const ColorConvert &other);

            YuvColorSpace yuvColorSpace() const;
            YuvColorSpaceType yuvColorSpaceType() const;
            void setYuvColorSpace(YuvColorSpace yuvColorSpace);
            void setYuvColorSpaceType(YuvColorSpaceType yuvColorSpaceType);
            void loadColorMatrix(ColorMatrix colorMatrix,
                                 int ibitsa,
                                 int ibitsb,
                                 int ibitsc,
                                 int obitsx,
                                 int obitsy,
                                 int obitsz);
            void loadAlphaMatrix(VideoFormatSpec::VideoFormatType formatType,
                                 int ibitsAlpha,
                                 int obitsx,
                                 int obitsy,
                                 int obitsz);
            void loadMatrix(const VideoFormatSpec &from,
                            const VideoFormatSpec &to);
            void loadMatrix(PixelFormat from,
                            PixelFormat to);

            inline void applyMatrix(int64_t a, int64_t b, int64_t c,
                                    int64_t *x, int64_t *y, int64_t *z) const
            {
                *x = bound(this->xmin, (a * this->m00 + b * this->m01 + c * this->m02 + this->m03) >> this->colorShift, this->xmax);
                *y = bound(this->ymin, (a * this->m10 + b * this->m11 + c * this->m12 + this->m13) >> this->colorShift, this->ymax);
                *z = bound(this->zmin, (a * this->m20 + b * this->m21 + c * this->m22 + this->m23) >> this->colorShift, this->zmax);
            }

            inline void applyVector(int64_t a, int64_t b, int64_t c,
                                    int64_t *x, int64_t *y, int64_t *z) const
            {
                *x = (a * this->m00 + this->m03) >> this->colorShift;
                *y = (b * this->m11 + this->m13) >> this->colorShift;
                *z = (c * this->m22 + this->m23) >> this->colorShift;
            }

            inline void applyPoint(int64_t p,
                                   int64_t *x, int64_t *y, int64_t *z) const
            {
                *x = (p * this->m00 + this->m03) >> this->colorShift;
                *y = (p * this->m10 + this->m13) >> this->colorShift;
                *z = (p * this->m20 + this->m23) >> this->colorShift;
            }

            inline void applyPoint(int64_t a, int64_t b, int64_t c,
                                   int64_t *p) const
            {
                *p = bound(this->xmin, (a * this->m00 + b * this->m01 + c * this->m02 + this->m03) >> this->colorShift, this->xmax);
            }

            inline void applyPoint(int64_t p, int64_t *q) const
            {
                *q = (p * this->m00 + this->m03) >> this->colorShift;
            }

            inline void applyAlpha(int64_t x, int64_t y, int64_t z, int64_t a,
                                   int64_t *xa, int64_t *ya, int64_t *za) const
            {
                *xa = bound<int64_t>(this->xmin, (a * (x * this->a00 + this->a01) + this->a02) >> this->alphaShift, this->xmax);
                *ya = bound<int64_t>(this->ymin, (a * (y * this->a10 + this->a11) + this->a12) >> this->alphaShift, this->ymax);
                *za = bound<int64_t>(this->zmin, (a * (z * this->a20 + this->a21) + this->a22) >> this->alphaShift, this->zmax);
            }

            inline void applyAlpha(int64_t a,
                                   int64_t *x, int64_t *y, int64_t *z) const
            {
                this->applyAlpha(*x, *y, *z, a, x, y, z);
            }

            inline void applyAlpha(int64_t p, int64_t a, int64_t *pa) const
            {
                *pa = bound<int64_t>(this->ymin, (a * (p * this->a00 + this->a01) + this->a02) >> this->alphaShift, this->ymax);
            }

            inline void applyAlpha(int64_t a, int64_t *p) const
            {
                this->applyAlpha(*p, a, p);
            }

            template <typename T>
            inline void readMatrix(T *colorMatrix=nullptr,
                                   T *alphaMatrix=nullptr,
                                   T *minValues=nullptr,
                                   T *maxValues=nullptr,
                                   T *colorShift=nullptr,
                                   T *alphaShift=nullptr)
            {
                // Copy the color matrix (3x4, including offsets)

                if (colorMatrix) {
                    colorMatrix[0]  = static_cast<T>(this->m00);
                    colorMatrix[1]  = static_cast<T>(this->m01);
                    colorMatrix[2]  = static_cast<T>(this->m02);
                    colorMatrix[3]  = static_cast<T>(this->m03);
                    colorMatrix[4]  = static_cast<T>(this->m10);
                    colorMatrix[5]  = static_cast<T>(this->m11);
                    colorMatrix[6]  = static_cast<T>(this->m12);
                    colorMatrix[7]  = static_cast<T>(this->m13);
                    colorMatrix[8]  = static_cast<T>(this->m20);
                    colorMatrix[9]  = static_cast<T>(this->m21);
                    colorMatrix[10] = static_cast<T>(this->m22);
                    colorMatrix[11] = static_cast<T>(this->m23);
                }

                // Copy the alpha matrix (3x3)

                if (alphaMatrix) {
                    alphaMatrix[0] = static_cast<T>(this->a00);
                    alphaMatrix[1] = static_cast<T>(this->a01);
                    alphaMatrix[2] = static_cast<T>(this->a02);
                    alphaMatrix[3] = static_cast<T>(this->a10);
                    alphaMatrix[4] = static_cast<T>(this->a11);
                    alphaMatrix[5] = static_cast<T>(this->a12);
                    alphaMatrix[6] = static_cast<T>(this->a20);
                    alphaMatrix[7] = static_cast<T>(this->a21);
                    alphaMatrix[8] = static_cast<T>(this->a22);
                }

                // Copy limits

                if (minValues) {
                    minValues[0] = static_cast<T>(this->xmin);
                    minValues[1] = static_cast<T>(this->ymin);
                    minValues[2] = static_cast<T>(this->zmin);
                }

                if (maxValues) {
                    maxValues[0] = static_cast<T>(this->xmax);
                    maxValues[1] = static_cast<T>(this->ymax);
                    maxValues[2] = static_cast<T>(this->zmax);
                }

                // Copy shifts

                if (colorShift)
                    *colorShift = static_cast<T>(this->colorShift);

                if (alphaShift)
                    *alphaShift = static_cast<T>(this->alphaShift);
            }

        private:
            ColorConvertPrivate *d;

        protected:
            // Color matrix
            int64_t m00 {0}, m01 {0}, m02 {0}, m03 {0};
            int64_t m10 {0}, m11 {0}, m12 {0}, m13 {0};
            int64_t m20 {0}, m21 {0}, m22 {0}, m23 {0};

            // Alpha matrix
            int64_t a00 {0}, a01 {0}, a02 {0};
            int64_t a10 {0}, a11 {0}, a12 {0};
            int64_t a20 {0}, a21 {0}, a22 {0};

            int64_t xmin {0}, xmax {0};
            int64_t ymin {0}, ymax {0};
            int64_t zmin {0}, zmax {0};

            int64_t colorShift {0};
            int64_t alphaShift {0};

        friend class ColorConvertPrivate;
    };
}

std::ostream &operator <<(std::ostream &os, AkVCam::ColorConvert::YuvColorSpace yuvColorSpace);
std::ostream &operator <<(std::ostream &os, AkVCam::ColorConvert::YuvColorSpaceType yuvColorSpaceType);
std::ostream &operator <<(std::ostream &os, AkVCam::ColorConvert::ColorMatrix colorMatrix);

#endif // AKVCAMUTILS_COLORCONVERT_H
