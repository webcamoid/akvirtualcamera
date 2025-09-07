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

#include <limits>
#include <ostream>

#include "colorconvert.h"
#include "videoformat.h"

namespace AkVCam
{
    class ColorConvertPrivate
    {
        public:
            ColorConvert *self {nullptr};
            ColorConvert::YuvColorSpace m_yuvColorSpace {ColorConvert::YuvColorSpace_ITUR_BT601};
            ColorConvert::YuvColorSpaceType m_yuvColorSpaceType {ColorConvert::YuvColorSpaceType_StudioSwing};

            explicit ColorConvertPrivate(ColorConvert *self);
            void rbConstants(ColorConvert::YuvColorSpace colorSpace,
                             int64_t &kr,
                             int64_t &kb,
                             int64_t &div) const;
            int64_t roundedDiv(int64_t num, int64_t den) const;
            int64_t nearestPowOf2(int64_t value) const;
            void limitsY(int bits,
                         ColorConvert::YuvColorSpaceType type,
                         int64_t &minY,
                         int64_t &maxY) const;
            void limitsUV(int bits,
                          ColorConvert::YuvColorSpaceType type,
                          int64_t &minUV,
                          int64_t &maxUV) const;
            void loadAbc2xyzMatrix(int abits,
                                   int bbits,
                                   int cbits,
                                   int xbits,
                                   int ybits,
                                   int zbits);
            void loadRgb2yuvMatrix(ColorConvert::YuvColorSpace YuvColorSpace,
                                   ColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                   int rbits,
                                   int gbits,
                                   int bbits,
                                   int ybits,
                                   int ubits,
                                   int vbits);
            void loadYuv2rgbMatrix(ColorConvert::YuvColorSpace YuvColorSpace,
                                   ColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                   int ybits,
                                   int ubits,
                                   int vbits,
                                   int rbits,
                                   int gbits,
                                   int bbits);
            void loadRgb2grayMatrix(ColorConvert::YuvColorSpace YuvColorSpace,
                                    int rbits,
                                    int gbits,
                                    int bbits,
                                    int graybits);
            void loadGray2rgbMatrix(int graybits,
                                    int rbits,
                                    int gbits,
                                    int bbits);
            void loadYuv2grayMatrix(ColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                    int ybits,
                                    int ubits,
                                    int vbits,
                                    int graybits);
            void loadGray2yuvMatrix(ColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                    int graybits,
                                    int ybits,
                                    int ubits,
                                    int vbits);
            void loadAlphaRgbMatrix(int alphaBits);
            void loadAlphaYuvMatrix(ColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                    int alphaBits,
                                    int ybits,
                                    int ubits,
                                    int vbits);
            void loadAlphaGrayMatrix(int alphaBits, int graybits);
    };
}

AkVCam::ColorConvert::ColorConvert()
{
    this->d = new ColorConvertPrivate(this);
}

AkVCam::ColorConvert::ColorConvert(YuvColorSpace yuvColorSpace,
                                   YuvColorSpaceType yuvColorSpaceType)
{
    this->d = new ColorConvertPrivate(this);
    this->d->m_yuvColorSpace = yuvColorSpace;
    this->d->m_yuvColorSpaceType = yuvColorSpaceType;
}

AkVCam::ColorConvert::ColorConvert(YuvColorSpaceType yuvColorSpaceType)
{
    this->d = new ColorConvertPrivate(this);
    this->d->m_yuvColorSpaceType = yuvColorSpaceType;
}

AkVCam::ColorConvert::ColorConvert(const ColorConvert &other)
{
    this->d = new ColorConvertPrivate(this);
    this->d->m_yuvColorSpace = other.d->m_yuvColorSpace;
    this->d->m_yuvColorSpaceType = other.d->m_yuvColorSpaceType;
    this->m00 = other.m00; this->m01 = other.m01; this->m02 = other.m02; this->m03 = other.m03;
    this->m10 = other.m10; this->m11 = other.m11; this->m12 = other.m12; this->m13 = other.m13;
    this->m20 = other.m20; this->m21 = other.m21; this->m22 = other.m22; this->m23 = other.m23;
    this->a00 = other.a00; this->a01 = other.a01; this->a02 = other.a02;
    this->a10 = other.a10; this->a11 = other.a11; this->a12 = other.a12;
    this->a20 = other.a20; this->a21 = other.a21; this->a22 = other.a22;
    this->xmin = other.xmin; this->xmax = other.xmax;
    this->ymin = other.ymin; this->ymax = other.ymax;
    this->zmin = other.zmin; this->zmax = other.zmax;
    this->colorShift = other.colorShift;
    this->alphaShift = other.alphaShift;
}

AkVCam::ColorConvert::~ColorConvert()
{
    delete this->d;
}

AkVCam::ColorConvert &AkVCam::ColorConvert::operator =(const ColorConvert &other)
{
    if (this != &other) {
        this->d->m_yuvColorSpace = other.d->m_yuvColorSpace;
        this->d->m_yuvColorSpaceType = other.d->m_yuvColorSpaceType;
        this->m00 = other.m00; this->m01 = other.m01; this->m02 = other.m02; this->m03 = other.m03;
        this->m10 = other.m10; this->m11 = other.m11; this->m12 = other.m12; this->m13 = other.m13;
        this->m20 = other.m20; this->m21 = other.m21; this->m22 = other.m22; this->m23 = other.m23;
        this->a00 = other.a00; this->a01 = other.a01; this->a02 = other.a02;
        this->a10 = other.a10; this->a11 = other.a11; this->a12 = other.a12;
        this->a20 = other.a20; this->a21 = other.a21; this->a22 = other.a22;
        this->xmin = other.xmin; this->xmax = other.xmax;
        this->ymin = other.ymin; this->ymax = other.ymax;
        this->zmin = other.zmin; this->zmax = other.zmax;
        this->colorShift = other.colorShift;
        this->alphaShift = other.alphaShift;
    }

    return *this;
}

AkVCam::ColorConvert::YuvColorSpace AkVCam::ColorConvert::yuvColorSpace() const
{
    return this->d->m_yuvColorSpace;
}

AkVCam::ColorConvert::YuvColorSpaceType AkVCam::ColorConvert::yuvColorSpaceType() const
{
    return this->d->m_yuvColorSpaceType;
}

void AkVCam::ColorConvert::setYuvColorSpace(YuvColorSpace yuvColorSpace)
{
    this->d->m_yuvColorSpace = yuvColorSpace;
}

void AkVCam::ColorConvert::setYuvColorSpaceType(YuvColorSpaceType yuvColorSpaceType)
{
    this->d->m_yuvColorSpaceType = yuvColorSpaceType;
}

void AkVCam::ColorConvert::loadColorMatrix(ColorMatrix colorMatrix,
                                           int ibitsa,
                                           int ibitsb,
                                           int ibitsc,
                                           int obitsx,
                                           int obitsy,
                                           int obitsz)
{
    switch (colorMatrix) {
    case ColorMatrix_ABC2XYZ:
        this->d->loadAbc2xyzMatrix(ibitsa,
                                   ibitsb,
                                   ibitsc,
                                   obitsx,
                                   obitsy,
                                   obitsz);

        break;

    case ColorMatrix_RGB2YUV:
        this->d->loadRgb2yuvMatrix(this->d->m_yuvColorSpace,
                                   this->d->m_yuvColorSpaceType,
                                   ibitsa,
                                   ibitsb,
                                   ibitsc,
                                   obitsx,
                                   obitsy,
                                   obitsz);

        break;

    case ColorMatrix_YUV2RGB:
        this->d->loadYuv2rgbMatrix(this->d->m_yuvColorSpace,
                                   this->d->m_yuvColorSpaceType,
                                   ibitsa,
                                   ibitsb,
                                   ibitsc,
                                   obitsx,
                                   obitsy,
                                   obitsz);

        break;

    case ColorMatrix_RGB2GRAY:
        this->d->loadRgb2grayMatrix(this->d->m_yuvColorSpace,
                                    ibitsa,
                                    ibitsb,
                                    ibitsc,
                                    obitsx);

        break;

    case ColorMatrix_GRAY2RGB:
        this->d->loadGray2rgbMatrix(ibitsa,
                                    obitsx,
                                    obitsy,
                                    obitsz);

        break;

    case ColorMatrix_YUV2GRAY:
        this->d->loadYuv2grayMatrix(this->d->m_yuvColorSpaceType,
                                    ibitsa,
                                    ibitsb,
                                    ibitsc,
                                    obitsx);

        break;

    case ColorMatrix_GRAY2YUV:
        this->d->loadGray2yuvMatrix(this->d->m_yuvColorSpaceType,
                                    ibitsa,
                                    obitsx,
                                    obitsy,
                                    obitsz);

    default:
        break;
    }
}

void AkVCam::ColorConvert::loadAlphaMatrix(VideoFormatSpec::VideoFormatType formatType,
                                           int ibitsAlpha,
                                           int obitsx,
                                           int obitsy,
                                           int obitsz)
{
    switch (formatType) {
    case VideoFormatSpec::VFT_RGB:
        this->d->loadAlphaRgbMatrix(ibitsAlpha);

        break;

    case VideoFormatSpec::VFT_YUV:
        this->d->loadAlphaYuvMatrix(this->d->m_yuvColorSpaceType,
                                    ibitsAlpha,
                                    obitsx,
                                    obitsy,
                                    obitsz);

        break;

    case VideoFormatSpec::VFT_Gray:
        this->d->loadAlphaGrayMatrix(ibitsAlpha, obitsx);

        break;

    default:
        break;
    }
}

void AkVCam::ColorConvert::loadMatrix(const VideoFormatSpec &from,
                                      const VideoFormatSpec &to)
{
    ColorMatrix colorMatrix = ColorMatrix_ABC2XYZ;
    int ibitsa = 0;
    int ibitsb = 0;
    int ibitsc = 0;
    int obitsx = 0;
    int obitsy = 0;
    int obitsz = 0;

    if (from.type() == VideoFormatSpec::VFT_RGB
        && to.type() == VideoFormatSpec::VFT_RGB) {
        colorMatrix = ColorMatrix_ABC2XYZ;
        ibitsa = from.component(ColorComponent::CT_R).depth();
        ibitsb = from.component(ColorComponent::CT_G).depth();
        ibitsc = from.component(ColorComponent::CT_B).depth();
        obitsx = to.component(ColorComponent::CT_R).depth();
        obitsy = to.component(ColorComponent::CT_G).depth();
        obitsz = to.component(ColorComponent::CT_B).depth();
    } else if (from.type() == VideoFormatSpec::VFT_RGB
               && to.type() == VideoFormatSpec::VFT_YUV) {
        colorMatrix = ColorMatrix_RGB2YUV;
        ibitsa = from.component(ColorComponent::CT_R).depth();
        ibitsb = from.component(ColorComponent::CT_G).depth();
        ibitsc = from.component(ColorComponent::CT_B).depth();
        obitsx = to.component(ColorComponent::CT_Y).depth();
        obitsy = to.component(ColorComponent::CT_U).depth();
        obitsz = to.component(ColorComponent::CT_V).depth();
    } else if (from.type() == VideoFormatSpec::VFT_RGB
               && to.type() == VideoFormatSpec::VFT_Gray) {
        colorMatrix = ColorMatrix_RGB2GRAY;
        ibitsa = from.component(ColorComponent::CT_R).depth();
        ibitsb = from.component(ColorComponent::CT_G).depth();
        ibitsc = from.component(ColorComponent::CT_B).depth();
        obitsx = to.component(ColorComponent::CT_Y).depth();
        obitsy = obitsx;
        obitsz = obitsx;
    } else if (from.type() == VideoFormatSpec::VFT_YUV
               && to.type() == VideoFormatSpec::VFT_RGB) {
        colorMatrix = ColorMatrix_YUV2RGB;
        ibitsa = from.component(ColorComponent::CT_Y).depth();
        ibitsb = from.component(ColorComponent::CT_U).depth();
        ibitsc = from.component(ColorComponent::CT_V).depth();
        obitsx = to.component(ColorComponent::CT_R).depth();
        obitsy = to.component(ColorComponent::CT_G).depth();
        obitsz = to.component(ColorComponent::CT_B).depth();
    } else if (from.type() == VideoFormatSpec::VFT_YUV
               && to.type() == VideoFormatSpec::VFT_YUV) {
        colorMatrix = ColorMatrix_ABC2XYZ;
        ibitsa = from.component(ColorComponent::CT_Y).depth();
        ibitsb = from.component(ColorComponent::CT_U).depth();
        ibitsc = from.component(ColorComponent::CT_V).depth();
        obitsx = to.component(ColorComponent::CT_Y).depth();
        obitsy = to.component(ColorComponent::CT_U).depth();
        obitsz = to.component(ColorComponent::CT_V).depth();
    } else if (from.type() == VideoFormatSpec::VFT_YUV
               && to.type() == VideoFormatSpec::VFT_Gray) {
        colorMatrix = ColorMatrix_YUV2GRAY;
        ibitsa = from.component(ColorComponent::CT_Y).depth();
        ibitsb = from.component(ColorComponent::CT_U).depth();
        ibitsc = from.component(ColorComponent::CT_V).depth();
        obitsx = to.component(ColorComponent::CT_Y).depth();
        obitsy = obitsx;
        obitsz = obitsx;
    } else if (from.type() == VideoFormatSpec::VFT_Gray
               && to.type() == VideoFormatSpec::VFT_RGB) {
        colorMatrix = ColorMatrix_GRAY2RGB;
        ibitsa = from.component(ColorComponent::CT_Y).depth();
        ibitsb = ibitsa;
        ibitsc = ibitsa;
        obitsx = to.component(ColorComponent::CT_R).depth();
        obitsy = to.component(ColorComponent::CT_G).depth();
        obitsz = to.component(ColorComponent::CT_B).depth();
    } else if (from.type() == VideoFormatSpec::VFT_Gray
               && to.type() == VideoFormatSpec::VFT_YUV) {
        colorMatrix = ColorMatrix_GRAY2YUV;
        ibitsa = from.component(ColorComponent::CT_Y).depth();
        ibitsb = ibitsa;
        ibitsc = ibitsa;
        obitsx = to.component(ColorComponent::CT_Y).depth();
        obitsy = to.component(ColorComponent::CT_U).depth();
        obitsz = to.component(ColorComponent::CT_V).depth();
    } else if (from.type() == VideoFormatSpec::VFT_Gray
               && to.type() == VideoFormatSpec::VFT_Gray) {
        colorMatrix = ColorMatrix_ABC2XYZ;
        ibitsa = from.component(ColorComponent::CT_Y).depth();
        ibitsb = ibitsa;
        ibitsc = ibitsa;
        obitsx = to.component(ColorComponent::CT_Y).depth();
        obitsy = obitsx;
        obitsz = obitsx;
    }

    this->loadColorMatrix(colorMatrix,
                          ibitsa,
                          ibitsb,
                          ibitsc,
                          obitsx,
                          obitsy,
                          obitsz);

    if (from.contains(ColorComponent::CT_A))
        this->loadAlphaMatrix(to.type(),
                              from.component(ColorComponent::CT_A).depth(),
                              obitsx,
                              obitsy,
                              obitsz);
}

void AkVCam::ColorConvert::loadMatrix(PixelFormat from,
                                      PixelFormat to)
{
    this->loadMatrix(VideoFormat::formatSpecs(from),
                     VideoFormat::formatSpecs(to));
}

#define DEFINE_CASE_CS(cs) \
    case AkVCam::ColorConvert::cs: \
        os << #cs; \
        \
        break;

std::ostream &operator <<(std::ostream &os, AkVCam::ColorConvert::YuvColorSpace yuvColorSpace)
{
    switch (yuvColorSpace) {
    DEFINE_CASE_CS(YuvColorSpace_AVG)
    DEFINE_CASE_CS(YuvColorSpace_ITUR_BT601)
    DEFINE_CASE_CS(YuvColorSpace_ITUR_BT709)
    DEFINE_CASE_CS(YuvColorSpace_ITUR_BT2020)
    DEFINE_CASE_CS(YuvColorSpace_SMPTE_240M)

    default:
        os << "YuvColorSpace_Unknown";

        break;
    }

    return os;
}

#define DEFINE_CASE_CST(cst) \
    case AkVCam::ColorConvert::cst: \
        os << #cst; \
        \
        break;

std::ostream &operator <<(std::ostream &os, AkVCam::ColorConvert::YuvColorSpaceType yuvColorSpaceType)
{
    switch (yuvColorSpaceType) {
    DEFINE_CASE_CST(YuvColorSpaceType_StudioSwing)
    DEFINE_CASE_CST(YuvColorSpaceType_FullSwing)

    default:
        os << "YuvColorSpaceType_Unknown";

        break;
    }

    return os;
}

#define DEFINE_CASE_CM(cm) \
    case AkVCam::ColorConvert::cm: \
        os << #cm; \
        \
        break;

std::ostream &operator <<(std::ostream &os, AkVCam::ColorConvert::ColorMatrix colorMatrix)
{
    switch (colorMatrix) {
    DEFINE_CASE_CM(ColorMatrix_ABC2XYZ)
    DEFINE_CASE_CM(ColorMatrix_RGB2YUV)
    DEFINE_CASE_CM(ColorMatrix_YUV2RGB)
    DEFINE_CASE_CM(ColorMatrix_RGB2GRAY)
    DEFINE_CASE_CM(ColorMatrix_GRAY2RGB)
    DEFINE_CASE_CM(ColorMatrix_YUV2GRAY)
    DEFINE_CASE_CM(ColorMatrix_GRAY2YUV)

    default:
        os << "ColorMatrix_Unknown";

        break;
    }

    return os;
}

AkVCam::ColorConvertPrivate::ColorConvertPrivate(ColorConvert *self):
    self(self)
{

}

void AkVCam::ColorConvertPrivate::rbConstants(AkVCam::ColorConvert::YuvColorSpace colorSpace,
                                              int64_t &kr,
                                              int64_t &kb,
                                              int64_t &div) const
{
    kr = 0;
    kb = 0;
    div = 10000;

    // Coefficients taken from https://en.wikipedia.org/wiki/YUV
    switch (colorSpace) {
    // Same weight for all components
    case ColorConvert::YuvColorSpace_AVG:
        kr = 3333;
        kb = 3333;

        break;

        // https://www.itu.int/rec/R-REC-BT.601/en
    case ColorConvert::YuvColorSpace_ITUR_BT601:
        kr = 2990;
        kb = 1140;

        break;

        // https://www.itu.int/rec/R-REC-BT.709/en
    case ColorConvert::YuvColorSpace_ITUR_BT709:
        kr = 2126;
        kb = 722;

        break;

        // https://www.itu.int/rec/R-REC-BT.2020/en
    case ColorConvert::YuvColorSpace_ITUR_BT2020:
        kr = 2627;
        kb = 593;

        break;

        // http://car.france3.mars.free.fr/HD/INA-%2026%20jan%2006/SMPTE%20normes%20et%20confs/s240m.pdf
    case ColorConvert::YuvColorSpace_SMPTE_240M:
        kr = 2120;
        kb = 870;

        break;

    default:
        break;
    }
}

int64_t AkVCam::ColorConvertPrivate::roundedDiv(int64_t num, int64_t den) const
{
    if (den == 0)
        return num < 0?
            std::numeric_limits<int64_t>::min():
            std::numeric_limits<int64_t>::max();

    if (((num < 0) && (den > 0)) || ((num > 0) && (den < 0)))
        return (2 * num - den) / (2 * den);

    return (2 * num + den) / (2 * den);
}

int64_t AkVCam::ColorConvertPrivate::nearestPowOf2(int64_t value) const
{
    int64_t val = value;
    int64_t res = 0;

    while (val >>= 1)
        res++;

    if (std::abs((1 << (res + 1)) - value) <= std::abs((1 << res) - value))
        return 1 << (res + 1);

    return 1 << res;
}

void AkVCam::ColorConvertPrivate::limitsY(int bits,
                                          ColorConvert::YuvColorSpaceType type,
                                          int64_t &minY,
                                          int64_t &maxY) const
{
    if (type == ColorConvert::YuvColorSpaceType_FullSwing) {
        minY = 0;
        maxY = (1 << bits) - 1;

        return;
    }

    /* g = 9% is the theoretical maximal overshoot (Gibbs phenomenon)
     *
     * https://en.wikipedia.org/wiki/YUV#Numerical_approximations
     * https://en.wikipedia.org/wiki/Gibbs_phenomenon
     * https://math.stackexchange.com/a/259089
     * https://www.youtube.com/watch?v=Ol0uTeXoKaU
     */
    static const int64_t g = 9;

    int64_t maxValue = (1 << bits) - 1;
    minY = this->nearestPowOf2(this->roundedDiv(maxValue * g, 2 * g + 100));
    maxY = maxValue * (g + 100) / (2 * g + 100);
}

void AkVCam::ColorConvertPrivate::limitsUV(int bits,
                                           ColorConvert::YuvColorSpaceType type,
                                           int64_t &minUV,
                                           int64_t &maxUV) const
{
    if (type == ColorConvert::YuvColorSpaceType_FullSwing) {
        minUV = 0;
        maxUV = (1 << bits) - 1;

        return;
    }

    static const int64_t g = 9;
    int64_t maxValue = (1 << bits) - 1;
    minUV = this->nearestPowOf2(this->roundedDiv(maxValue * g, 2 * g + 100));
    maxUV = (1L << bits) - minUV;
}

void AkVCam::ColorConvertPrivate::loadAbc2xyzMatrix(int abits,
                                                    int bbits,
                                                    int cbits,
                                                    int xbits,
                                                    int ybits,
                                                    int zbits)
{
    int shift = std::max(abits, std::max(bbits, cbits));
    int64_t shiftDiv = 1L << shift;
    int64_t rounding = 1L << std::abs(shift - 1);

    int64_t amax = (1L << abits) - 1;
    int64_t bmax = (1L << bbits) - 1;
    int64_t cmax = (1L << cbits) - 1;

    int64_t xmax = (1L << xbits) - 1;
    int64_t ymax = (1L << ybits) - 1;
    int64_t zmax = (1L << zbits) - 1;

    int64_t kx = this->roundedDiv(shiftDiv * xmax, amax);
    int64_t ky = this->roundedDiv(shiftDiv * ymax, bmax);
    int64_t kz = this->roundedDiv(shiftDiv * zmax, cmax);

    self->m00 = kx; self->m01 = 0 ; self->m02 = 0 ; self->m03 = rounding;
    self->m10 = 0 ; self->m11 = ky; self->m12 = 0 ; self->m13 = rounding;
    self->m20 = 0 ; self->m21 = 0 ; self->m22 = kz; self->m23 = rounding;

    self->xmin = 0; self->xmax = xmax;
    self->ymin = 0; self->ymax = ymax;
    self->zmin = 0; self->zmax = zmax;

    self->colorShift = shift;
}

void AkVCam::ColorConvertPrivate::loadRgb2yuvMatrix(ColorConvert::YuvColorSpace yuvColorSpace,
                                                    ColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                                    int rbits,
                                                    int gbits,
                                                    int bbits,
                                                    int ybits,
                                                    int ubits,
                                                    int vbits)
{
    int64_t kyr = 0;
    int64_t kyb = 0;
    int64_t div = 0;
    this->rbConstants(yuvColorSpace, kyr, kyb, div);
    int64_t kyg = div - kyr - kyb;

    int64_t kur = -kyr;
    int64_t kug = -kyg;
    int64_t kub = div - kyb;

    int64_t kvr = div - kyr;
    int64_t kvg = -kyg;
    int64_t kvb = -kyb;

    int shift = std::max(rbits, std::max(gbits, bbits));
    int64_t shiftDiv = 1L << shift;
    int64_t rounding = 1L << (shift - 1);

    int64_t rmax = (1L << rbits) - 1;
    int64_t gmax = (1L << gbits) - 1;
    int64_t bmax = (1L << bbits) - 1;

    int64_t minY = 0;
    int64_t maxY = 0;
    this->limitsY(ybits, yuvColorSpaceType, minY, maxY);
    auto diffY = maxY - minY;

    int64_t kiyr = this->roundedDiv(shiftDiv * diffY * kyr, div * rmax);
    int64_t kiyg = this->roundedDiv(shiftDiv * diffY * kyg, div * gmax);
    int64_t kiyb = this->roundedDiv(shiftDiv * diffY * kyb, div * bmax);

    int64_t minU = 0;
    int64_t maxU = 0;
    this->limitsUV(ubits, yuvColorSpaceType, minU, maxU);
    auto diffU = maxU - minU;

    int64_t kiur = this->roundedDiv(shiftDiv * diffU * kur, 2 * rmax * kub);
    int64_t kiug = this->roundedDiv(shiftDiv * diffU * kug, 2 * gmax * kub);
    int64_t kiub = this->roundedDiv(shiftDiv * diffU      , 2 * bmax);

    int64_t minV = 0;
    int64_t maxV = 0;
    this->limitsUV(vbits, yuvColorSpaceType, minV, maxV);
    auto diffV = maxV - minV;

    int64_t kivr = this->roundedDiv(shiftDiv * diffV      , 2 * rmax);
    int64_t kivg = this->roundedDiv(shiftDiv * diffV * kvg, 2 * gmax * kvr);
    int64_t kivb = this->roundedDiv(shiftDiv * diffV * kvb, 2 * bmax * kvr);

    int64_t ciy = rounding + shiftDiv * minY;
    int64_t ciu = rounding + shiftDiv * (minU + maxU) / 2;
    int64_t civ = rounding + shiftDiv * (minV + maxV) / 2;

    self->m00 = kiyr; self->m01 = kiyg; self->m02 = kiyb; self->m03 = ciy;
    self->m10 = kiur; self->m11 = kiug; self->m12 = kiub; self->m13 = ciu;
    self->m20 = kivr; self->m21 = kivg; self->m22 = kivb; self->m23 = civ;

    self->xmin = minY; self->xmax = maxY;
    self->ymin = minU; self->ymax = maxU;
    self->zmin = minV; self->zmax = maxV;

    self->colorShift = shift;
}

void AkVCam::ColorConvertPrivate::loadYuv2rgbMatrix(ColorConvert::YuvColorSpace yuvColorSpace,
                                                    ColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                                    int ybits,
                                                    int ubits,
                                                    int vbits,
                                                    int rbits,
                                                    int gbits,
                                                    int bbits)
{
    int64_t kyr = 0;
    int64_t kyb = 0;
    int64_t div = 0;
    this->rbConstants(yuvColorSpace, kyr, kyb, div);
    int64_t kyg = div - kyr - kyb;

    int64_t minY = 0;
    int64_t maxY = 0;
    this->limitsY(ybits, yuvColorSpaceType, minY, maxY);
    auto diffY = maxY - minY;

    int64_t minU = 0;
    int64_t maxU = 0;
    this->limitsUV(ubits, yuvColorSpaceType, minU, maxU);
    auto diffU = maxU - minU;

    int64_t minV = 0;
    int64_t maxV = 0;
    this->limitsUV(vbits, yuvColorSpaceType, minV, maxV);
    auto diffV = maxV - minV;

    int shift = std::max(ybits, std::max(ubits, vbits));
    int64_t shiftDiv = 1L << shift;
    int64_t rounding = 1L << (shift - 1);

    int64_t rmax = (1L << rbits) - 1;
    int64_t gmax = (1L << gbits) - 1;
    int64_t bmax = (1L << bbits) - 1;

    int64_t kry = this->roundedDiv(shiftDiv * rmax, diffY);
    int64_t krv = this->roundedDiv(2 * shiftDiv * rmax * (div - kyr), div * diffV);

    int64_t kgy = this->roundedDiv(shiftDiv * gmax, diffY);
    int64_t kgu = this->roundedDiv(2 * shiftDiv * gmax * kyb * (kyb - div), div * kyg * diffU);
    int64_t kgv = this->roundedDiv(2 * shiftDiv * gmax * kyr * (kyr - div), div * kyg * diffV);

    int64_t kby = this->roundedDiv(shiftDiv * bmax, diffY);
    int64_t kbu = this->roundedDiv(2 * shiftDiv * bmax * (div - kyb), div * diffU);

    int64_t cir = rounding - kry * minY - krv * (minV + maxV) / 2;
    int64_t cig = rounding - kgy * minY - (kgu * (minU + maxU) + kgv * (minV + maxV)) / 2;
    int64_t cib = rounding - kby * minY - kbu * (minU + maxU) / 2;

    self->m00 = kry; self->m01 = 0  ; self->m02 = krv; self->m03 = cir;
    self->m10 = kgy; self->m11 = kgu; self->m12 = kgv; self->m13 = cig;
    self->m20 = kby; self->m21 = kbu; self->m22 = 0  ; self->m23 = cib;

    self->xmin = 0; self->xmax = rmax;
    self->ymin = 0; self->ymax = gmax;
    self->zmin = 0; self->zmax = bmax;

    self->colorShift = shift;
}

void AkVCam::ColorConvertPrivate::loadRgb2grayMatrix(ColorConvert::YuvColorSpace yuvColorSpace,
                                                     int rbits,
                                                     int gbits,
                                                     int bbits,
                                                     int graybits)
{
    ColorConvert::YuvColorSpaceType type = ColorConvert::YuvColorSpaceType_FullSwing;

    int64_t kyr = 0;
    int64_t kyb = 0;
    int64_t div = 0;
    this->rbConstants(yuvColorSpace, kyr, kyb, div);
    int64_t kyg = div - kyr - kyb;

    int shift = std::max(rbits, std::max(gbits, bbits));
    int64_t shiftDiv = 1L << shift;
    int64_t rounding = 1L << (shift - 1);

    int64_t rmax = (1L << rbits) - 1;
    int64_t gmax = (1L << gbits) - 1;
    int64_t bmax = (1L << bbits) - 1;

    int64_t minY = 0;
    int64_t maxY = 0;
    this->limitsY(graybits, type, minY, maxY);
    auto diffY = maxY - minY;

    int64_t kiyr = this->roundedDiv(shiftDiv * diffY * kyr, div * rmax);
    int64_t kiyg = this->roundedDiv(shiftDiv * diffY * kyg, div * gmax);
    int64_t kiyb = this->roundedDiv(shiftDiv * diffY * kyb, div * bmax);

    int64_t minU = 0;
    int64_t maxU = 0;
    this->limitsUV(graybits, type, minU, maxU);

    int64_t minV = 0;
    int64_t maxV = 0;
    this->limitsUV(graybits, type, minV, maxV);

    int64_t ciy = rounding + shiftDiv * minY;
    int64_t ciu = rounding + shiftDiv * (minU + maxU) / 2;
    int64_t civ = rounding + shiftDiv * (minV + maxV) / 2;

    self->m00 = kiyr; self->m01 = kiyg; self->m02 = kiyb; self->m03 = ciy;
    self->m10 = 0   ; self->m11 = 0   ; self->m12 = 0   ; self->m13 = ciu;
    self->m20 = 0   ; self->m21 = 0   ; self->m22 = 0   ; self->m23 = civ;

    self->xmin = minY; self->xmax = maxY;
    self->ymin = minU; self->ymax = maxU;
    self->zmin = minV; self->zmax = maxV;

    self->colorShift = shift;
}

void AkVCam::ColorConvertPrivate::loadGray2rgbMatrix(int graybits,
                                                     int rbits,
                                                     int gbits,
                                                     int bbits)
{
    int shift = graybits;
    int64_t shiftDiv = 1L << shift;
    int64_t rounding = 1L << (shift - 1);

    int64_t graymax = (1L << graybits) - 1;

    int64_t rmax = (1L << rbits) - 1;
    int64_t gmax = (1L << gbits) - 1;
    int64_t bmax = (1L << bbits) - 1;

    int64_t kr = this->roundedDiv(shiftDiv * rmax, graymax);
    int64_t kg = this->roundedDiv(shiftDiv * gmax, graymax);
    int64_t kb = this->roundedDiv(shiftDiv * bmax, graymax);

    self->m00 = kr; self->m01 = 0; self->m02 = 0; self->m03 = rounding;
    self->m10 = kg; self->m11 = 0; self->m12 = 0; self->m13 = rounding;
    self->m20 = kb; self->m21 = 0; self->m22 = 0; self->m23 = rounding;

    self->xmin = 0; self->xmax = rmax;
    self->ymin = 0; self->ymax = gmax;
    self->zmin = 0; self->zmax = bmax;

    self->colorShift = shift;
}

void AkVCam::ColorConvertPrivate::loadYuv2grayMatrix(ColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                                     int ybits,
                                                     int ubits,
                                                     int vbits,
                                                     int graybits)
{
    ColorConvert::YuvColorSpaceType otype = ColorConvert::YuvColorSpaceType_FullSwing;

    int shift = ybits;
    int64_t shiftDiv = 1L << shift;
    int64_t rounding = 1L << (shift - 1);

    int64_t graymax = (1L << graybits) - 1;

    int64_t minY = 0;
    int64_t maxY = 0;
    this->limitsY(ybits, yuvColorSpaceType, minY, maxY);
    auto diffY = maxY - minY;

    int64_t ky = this->roundedDiv(shiftDiv * graymax, diffY);

    int64_t minU = 0;
    int64_t maxU = 0;
    this->limitsUV(graybits, otype, minU, maxU);

    int64_t minV = 0;
    int64_t maxV = 0;
    this->limitsUV(graybits, otype, minV, maxV);

    int64_t ciy = rounding - this->roundedDiv(shiftDiv * minY * graymax, diffY);
    int64_t ciu = rounding + shiftDiv * (minU + maxU) / 2;
    int64_t civ = rounding + shiftDiv * (minV + maxV) / 2;

    self->m00 = ky; self->m01 = 0; self->m02 = 0; self->m03 = ciy;
    self->m10 = 0 ; self->m11 = 0; self->m12 = 0; self->m13 = ciu;
    self->m20 = 0 ; self->m21 = 0; self->m22 = 0; self->m23 = civ;

    self->xmin = 0; self->xmax = graymax;
    self->ymin = 0; self->ymax = graymax;
    self->zmin = 0; self->zmax = graymax;

    self->colorShift = shift;
}

void AkVCam::ColorConvertPrivate::loadGray2yuvMatrix(ColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                                     int graybits,
                                                     int ybits,
                                                     int ubits,
                                                     int vbits)
{
    int shift = graybits;
    int64_t shiftDiv = 1L << shift;
    int64_t rounding = 1L << (shift - 1);

    int64_t graymax = (1L << graybits) - 1;

    int64_t minY = 0;
    int64_t maxY = 0;
    this->limitsY(ybits, yuvColorSpaceType, minY, maxY);
    auto diffY = maxY - minY;

    int64_t ky = this->roundedDiv(shiftDiv * diffY, graymax);

    int64_t minU = 0;
    int64_t maxU = 0;
    this->limitsUV(ubits, yuvColorSpaceType, minU, maxU);

    int64_t minV = 0;
    int64_t maxV = 0;
    this->limitsUV(vbits, yuvColorSpaceType, minV, maxV);

    int64_t ciy = rounding + shiftDiv * minY;
    int64_t ciu = rounding + shiftDiv * (minU + maxU) / 2;
    int64_t civ = rounding + shiftDiv * (minV + maxV) / 2;

    self->m00 = ky; self->m01 = 0; self->m02 = 0; self->m03 = ciy;
    self->m10 = 0 ; self->m11 = 0; self->m12 = 0; self->m13 = ciu;
    self->m20 = 0 ; self->m21 = 0; self->m22 = 0; self->m23 = civ;

    self->xmin = minY; self->xmax = maxY;
    self->ymin = minU; self->ymax = maxU;
    self->zmin = minV; self->zmax = maxV;

    self->colorShift = shift;
}

void AkVCam::ColorConvertPrivate::loadAlphaRgbMatrix(int alphaBits)
{
    int64_t amax = (1L << alphaBits) - 1;
    self->alphaShift = alphaBits;
    int64_t shiftDiv = 1L << self->alphaShift;
    int64_t rounding = 1L << (self->alphaShift - 1);

    auto k = this->roundedDiv(shiftDiv, amax);

    self->a00 = k; self->a01 = 0; self->a02 = rounding;
    self->a10 = k; self->a11 = 0; self->a12 = rounding;
    self->a20 = k; self->a21 = 0; self->a22 = rounding;

}

void AkVCam::ColorConvertPrivate::loadAlphaYuvMatrix(ColorConvert::YuvColorSpaceType yuvColorSpaceType,
                                                     int alphaBits,
                                                     int ybits,
                                                     int ubits,
                                                     int vbits)
{
    int64_t amax = (1L << alphaBits) - 1;
    self->alphaShift = alphaBits;
    int64_t shiftDiv = 1L << self->alphaShift;
    int64_t rounding = 1L << (self->alphaShift - 1);

    uint64_t k = shiftDiv / amax;

    int64_t minY = 0;
    int64_t maxY = 0;
    this->limitsY(ybits, yuvColorSpaceType, minY, maxY);
    int64_t ky = -this->roundedDiv(shiftDiv * minY, amax);

    int64_t minU = 0;
    int64_t maxU = 0;
    this->limitsUV(ubits, yuvColorSpaceType, minU, maxU);
    int64_t ku = -this->roundedDiv(shiftDiv * (minU + maxU), 2 * amax);

    int64_t minV = 0;
    int64_t maxV = 0;
    this->limitsUV(vbits, yuvColorSpaceType, minV, maxV);
    int64_t kv = -this->roundedDiv(shiftDiv * (minV + maxV), 2 * amax);

    int64_t ciy = rounding + shiftDiv * minY;
    int64_t ciu = rounding + shiftDiv * (minU + maxU) / 2;
    int64_t civ = rounding + shiftDiv * (minV + maxV) / 2;

    self->a00 = k; self->a01 = ky; self->a02 = ciy;
    self->a10 = k; self->a11 = ku; self->a12 = ciu;
    self->a20 = k; self->a21 = kv; self->a22 = civ;
}

void AkVCam::ColorConvertPrivate::loadAlphaGrayMatrix(int alphaBits,
                                                      int graybits)
{
    ColorConvert::YuvColorSpaceType otype = ColorConvert::YuvColorSpaceType_FullSwing;

    int64_t amax = (1L << alphaBits) - 1;
    self->alphaShift = alphaBits;
    int64_t shiftDiv = 1L << self->alphaShift;
    int64_t rounding = 1L << (self->alphaShift - 1);

    auto k = this->roundedDiv(self->alphaShift, amax);

    int64_t minU = 0;
    int64_t maxU = 0;
    this->limitsUV(graybits, otype, minU, maxU);

    int64_t minV = 0;
    int64_t maxV = 0;
    this->limitsUV(graybits, otype, minV, maxV);

    int64_t ciu = rounding + shiftDiv * (minU + maxU) / 2;
    int64_t civ = rounding + shiftDiv * (minV + maxV) / 2;

    self->a00 = k; self->a01 = 0; self->a02 = rounding;
    self->a10 = 0; self->a11 = 0; self->a12 = ciu;
    self->a20 = 0; self->a21 = 0; self->a22 = civ;
}
