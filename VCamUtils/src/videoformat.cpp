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

#include <algorithm>
#include <cstring>
#include <limits>
#include <map>
#include <ostream>

#include "videoformat.h"
#include "algorithm.h"
#include "fraction.h"
#include "videoformatspec.h"
#include "utils.h"

#define VFT_Unknown VideoFormatSpec::VFT_Unknown
#define VFT_RGB     VideoFormatSpec::VFT_RGB
#define VFT_YUV     VideoFormatSpec::VFT_YUV
#define VFT_Gray    VideoFormatSpec::VFT_Gray

#define CT_END ColorComponent::CT_Unknown
#define CT_R   ColorComponent::CT_R
#define CT_G   ColorComponent::CT_G
#define CT_B   ColorComponent::CT_B
#define CT_Y   ColorComponent::CT_Y
#define CT_U   ColorComponent::CT_U
#define CT_V   ColorComponent::CT_V
#define CT_A   ColorComponent::CT_A

#define MAX_PLANES 4
#define MAX_COMPONENTS 4

namespace AkVCam
{
    struct Component
    {
        ColorComponent::ComponentType type;
        size_t step;
        size_t offset;
        size_t shift;
        size_t byteDepth;
        size_t depth;
        size_t widthDiv;
        size_t heightDiv;
    };

    struct Plane
    {
        size_t ncomponents;
        Component components[MAX_COMPONENTS];
        size_t bitsSize;
    };

    struct VideoFmt
    {
        PixelFormat format;
        const char *name;
        VideoFormatSpec::VideoFormatType type;
        int endianness;
        size_t nplanes;
        Plane planes[MAX_PLANES];

        static inline const VideoFmt *formats()
        {
            static const VideoFmt akvcamVideoFormatSpecTable[] {
                {PixelFormat_xrgb,
                 "XRGB",
                 VFT_RGB,
                 ENDIANNESS_BO,
                 1,
                 {{3, {{CT_R, 4, 1, 0, 1, 8, 0, 0},
                       {CT_G, 4, 2, 0, 1, 8, 0, 0},
                       {CT_B, 4, 3, 0, 1, 8, 0, 0}}, 32}
                 }},
                {PixelFormat_rgb24,
                 "RGB24",
                 VFT_RGB,
                 ENDIANNESS_BO,
                 1,
                 {{3, {{CT_R, 3, 0, 0, 1, 8, 0, 0},
                 {CT_G, 3, 1, 0, 1, 8, 0, 0},
                 {CT_B, 3, 2, 0, 1, 8, 0, 0}}, 24}
                 }},
                {PixelFormat_rgb565be,
                 "RGB565BE",
                 VFT_RGB,
                 ENDIANNESS_BE,
                 1,
                 {{3, {{CT_R, 2, 0, 11, 2, 5, 0, 0},
                 {CT_G, 2, 0,  5, 2, 6, 0, 0},
                 {CT_B, 2, 0,  0, 2, 5, 0, 0}}, 16}
                 }},
                {PixelFormat_rgb565le,
                 "RGB565LE",
                 VFT_RGB,
                 ENDIANNESS_LE,
                 1,
                 {{3, {{CT_R, 2, 0, 11, 2, 5, 0, 0},
                 {CT_G, 2, 0,  5, 2, 6, 0, 0},
                 {CT_B, 2, 0,  0, 2, 5, 0, 0}}, 16}
                 }},
                {PixelFormat_rgb555be,
                 "RGB555BE",
                 VFT_RGB,
                 ENDIANNESS_BE,
                 1,
                 {{3, {{CT_R, 2, 0, 10, 2, 5, 0, 0},
                 {CT_G, 2, 0,  5, 2, 5, 0, 0},
                 {CT_B, 2, 0,  0, 2, 5, 0, 0}}, 16}
                 }},
                {PixelFormat_rgb555le,
                 "RGB555LE",
                 VFT_RGB,
                 ENDIANNESS_LE,
                 1,
                 {{3, {{CT_R, 2, 0, 10, 2, 5, 0, 0},
                 {CT_G, 2, 0,  5, 2, 5, 0, 0},
                 {CT_B, 2, 0,  0, 2, 5, 0, 0}}, 16}
                 }},
                {PixelFormat_argb,
                 "ARGB",
                 VFT_RGB,
                 ENDIANNESS_BO,
                 1,
                 {{4, {{CT_A, 4, 0, 0, 1, 8, 0, 0},
                       {CT_R, 4, 1, 0, 1, 8, 0, 0},
                       {CT_G, 4, 2, 0, 1, 8, 0, 0},
                       {CT_B, 4, 3, 0, 1, 8, 0, 0}}, 32}
                 }},
                {PixelFormat_bgrx,
                 "BGRX",
                 VFT_RGB,
                 ENDIANNESS_BO,
                 1,
                 {{3, {{CT_B, 4, 0, 0, 1, 8, 0, 0},
                 {CT_G, 4, 1, 0, 1, 8, 0, 0},
                 {CT_R, 4, 2, 0, 1, 8, 0, 0}}, 32}
                 }},
                {PixelFormat_bgr24,
                 "BGR24",
                 VFT_RGB,
                 ENDIANNESS_BO,
                 1,
                 {{3, {{CT_B, 3, 0, 0, 1, 8, 0, 0},
                 {CT_G, 3, 1, 0, 1, 8, 0, 0},
                 {CT_R, 3, 2, 0, 1, 8, 0, 0}}, 24}
                 }},
                {PixelFormat_bgr565be,
                 "BGR565BE",
                 VFT_RGB,
                 ENDIANNESS_BE,
                 1,
                 {{3, {{CT_B, 2, 0, 11, 2, 5, 0, 0},
                 {CT_G, 2, 0,  5, 2, 6, 0, 0},
                 {CT_R, 2, 0,  0, 2, 5, 0, 0}}, 16}
                 }},
                {PixelFormat_bgr565le,
                 "BGR565LE",
                 VFT_RGB,
                 ENDIANNESS_LE,
                 1,
                 {{3, {{CT_B, 2, 0, 11, 2, 5, 0, 0},
                 {CT_G, 2, 0,  5, 2, 6, 0, 0},
                 {CT_R, 2, 0,  0, 2, 5, 0, 0}}, 16}
                 }},
                {PixelFormat_bgr555be,
                 "BGR555BE",
                 VFT_RGB,
                 ENDIANNESS_BE,
                 1,
                 {{3, {{CT_B, 2, 0, 10, 2, 5, 0, 0},
                 {CT_G, 2, 0,  5, 2, 5, 0, 0},
                 {CT_R, 2, 0,  0, 2, 5, 0, 0}}, 16}
                 }},
                {PixelFormat_bgr555le,
                 "BGR555LE",
                 VFT_RGB,
                 ENDIANNESS_LE,
                 1,
                 {{3, {{CT_B, 2, 0, 10, 2, 5, 0, 0},
                 {CT_G, 2, 0,  5, 2, 5, 0, 0},
                 {CT_R, 2, 0,  0, 2, 5, 0, 0}}, 16}
                 }},
                {PixelFormat_bgra,
                 "BGRA",
                 VFT_RGB,
                 ENDIANNESS_BO,
                 1,
                 {{4, {{CT_B, 4, 0, 0, 1, 8, 0, 0},
                       {CT_G, 4, 1, 0, 1, 8, 0, 0},
                       {CT_R, 4, 2, 0, 1, 8, 0, 0},
                       {CT_A, 4, 3, 0, 1, 8, 0, 0}}, 32}
                 }},
                {PixelFormat_uyvy422,
                 "UYVY",
                 VFT_YUV,
                 ENDIANNESS_BO,
                 1,
                 {{3, {{CT_U, 4, 0, 0, 1, 8, 1, 0},
                 {CT_Y, 2, 1, 0, 1, 8, 0, 0},
                 {CT_V, 4, 2, 0, 1, 8, 1, 0}}, 16}
                 }},
                {PixelFormat_yuyv422,
                 "YUY2",
                 VFT_YUV,
                 ENDIANNESS_BO,
                 1,
                 {{3, {{CT_Y, 2, 0, 0, 1, 8, 0, 0},
                 {CT_U, 4, 1, 0, 1, 8, 1, 0},
                 {CT_V, 4, 3, 0, 1, 8, 1, 0}}, 16}
                 }},
                {PixelFormat_nv12,
                 "NV12",
                 VFT_YUV,
                 ENDIANNESS_BO,
                 2,
                 {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
                 {2, {{CT_U, 2, 0, 0, 1, 8, 1, 1},
                 {CT_V, 2, 1, 0, 1, 8, 1, 1}}, 8}
                 }},
                {PixelFormat_nv21,
                 "NV21",
                 VFT_YUV,
                 ENDIANNESS_BO,
                 2,
                 {{1, {{CT_Y, 1, 0, 0, 1, 8, 0, 0}}, 8},
                 {2, {{CT_V, 2, 0, 0, 1, 8, 1, 1},
                 {CT_U, 2, 1, 0, 1, 8, 1, 1}}, 8}
                 }},
                {PixelFormat_none,
                 "none",
                 VFT_Unknown,
                 ENDIANNESS_BO,
                 0,
                 {}},
            };

            return akvcamVideoFormatSpecTable;
        }

        static inline const VideoFmt *byPixelFormat(PixelFormat pixelFormat)
        {
            auto it = formats();

            for (; it->format != PixelFormat_none; ++it)
                if (it->format == pixelFormat)
                    return it;

            return it;
        }

        static inline const VideoFmt *byName(const std::string &name)
        {
            auto it = formats();

            for (; it->format != PixelFormat_none; ++it)
                if (strcmp(it->name, name.data()) == 0)
                    return it;

            return it;
        }

        static inline std::vector<PixelFormat> allPixelFormats()
        {
            std::vector<PixelFormat> allFormats;

            for (auto it = formats(); it->format != PixelFormat_none; ++it)
                allFormats.push_back(it->format);

            return allFormats;
        }

        static VideoFormatSpec formatSpecs(PixelFormat format)
        {
            auto fmt = formats();

            for (; fmt->format != PixelFormat_none; fmt++)
                if (fmt->format == format) {
                    ColorPlanes planes;

                    for (size_t i = 0; i < fmt->nplanes; ++i) {
                        auto &plane = fmt->planes[i];
                        ColorComponentList components;

                        for (size_t i = 0; i < plane.ncomponents; ++i) {
                            auto &component = plane.components[i];
                            components.push_back(ColorComponent(component.type,
                                                                component.step,
                                                                component.offset,
                                                                component.shift,
                                                                component.byteDepth,
                                                                component.depth,
                                                                component.widthDiv,
                                                                component.heightDiv));
                        }

                        planes.push_back(ColorPlane(components,
                                                    plane.bitsSize));
                    }

                    return VideoFormatSpec(fmt->type,
                                           fmt->endianness,
                                           planes);
                }

            return {};
        }
    };

    class VideoFormatPrivate
    {
        public:
            PixelFormat m_format {PixelFormat_none};
            int m_width {0};
            int m_height {0};
            Fraction m_fps;
    };
}

AkVCam::VideoFormat::VideoFormat()
{
    this->d = new VideoFormatPrivate;
}

AkVCam::VideoFormat::VideoFormat(PixelFormat format,
                                 int width,
                                 int height)
{
    this->d = new VideoFormatPrivate;
    this->d->m_format = format;
    this->d->m_width = width;
    this->d->m_height = height;
}

AkVCam::VideoFormat::VideoFormat(PixelFormat format,
                                 int width,
                                 int height,
                                 const Fraction &fps)
{
    this->d = new VideoFormatPrivate;
    this->d->m_format = format;
    this->d->m_width = width;
    this->d->m_height = height;
    this->d->m_fps = fps;
}

AkVCam::VideoFormat::VideoFormat(const VideoFormat &other)
{
    this->d = new VideoFormatPrivate;
    this->d->m_format = other.d->m_format;
    this->d->m_width = other.d->m_width;
    this->d->m_height = other.d->m_height;
    this->d->m_fps = other.d->m_fps;
}

AkVCam::VideoFormat::~VideoFormat()
{
    delete this->d;
}

AkVCam::VideoFormat &AkVCam::VideoFormat::operator =(const VideoFormat &other)
{
    if (this != &other) {
        this->d->m_format = other.d->m_format;
        this->d->m_width = other.d->m_width;
        this->d->m_height = other.d->m_height;
        this->d->m_fps = other.d->m_fps;
    }

    return *this;
}

bool AkVCam::VideoFormat::operator ==(const AkVCam::VideoFormat &other) const
{
    return this->d->m_format == other.d->m_format
           && this->d->m_width == other.d->m_width
           && this->d->m_height == other.d->m_height
           && this->d->m_fps == other.d->m_fps;
}

bool AkVCam::VideoFormat::operator !=(const AkVCam::VideoFormat &other) const
{
    return this->d->m_format != other.d->m_format
           || this->d->m_width != other.d->m_width
           || this->d->m_height != other.d->m_height
           || this->d->m_fps != other.d->m_fps;
}

AkVCam::VideoFormat::operator bool() const
{
    return this->d->m_format != PixelFormat_none
           && this->d->m_width > 0
           && this->d->m_height > 0;
}

AkVCam::PixelFormat AkVCam::VideoFormat::format() const
{
    return this->d->m_format;
}

int AkVCam::VideoFormat::width() const
{
    return this->d->m_width;
}

int AkVCam::VideoFormat::height() const
{
    return this->d->m_height;
}

AkVCam::Fraction AkVCam::VideoFormat::fps() const
{
    return this->d->m_fps;
}

size_t AkVCam::VideoFormat::bpp() const
{
    return VideoFmt::formatSpecs(this->d->m_format).bpp();
}

void AkVCam::VideoFormat::setFormat(PixelFormat format)
{
    this->d->m_format = format;
}

void AkVCam::VideoFormat::setWidth(int width)
{
    this->d->m_width = width;
}

void AkVCam::VideoFormat::setHeight(int height)
{
    this->d->m_height = height;
}

void AkVCam::VideoFormat::setFps(const Fraction &fps)
{
    this->d->m_fps = fps;
}

AkVCam::VideoFormat AkVCam::VideoFormat::nearest(const VideoFormats &caps) const
{
    VideoFormat nearestCap;
    auto q = std::numeric_limits<uint64_t>::max();
    auto sspecs = VideoFmt::formatSpecs(this->d->m_format);

    for (auto &cap: caps) {
        auto specs = VideoFmt::formatSpecs(cap.d->m_format);
        uint64_t diffFourcc = cap.d->m_format == this->d->m_format? 0: 1;
        auto diffWidth = cap.d->m_width - this->d->m_width;
        auto diffHeight = cap.d->m_height - this->d->m_height;
        auto diffBpp = specs.bpp() - sspecs.bpp();
        auto diffPlanes = specs.planes() - sspecs.planes();
        int diffPlanesBits = 0;

        if (specs.planes() != sspecs.planes()) {
            for (size_t j = 0; j < specs.planes(); ++j) {
                auto &plane = specs.plane(j);

                for (size_t i = 0; i < plane.components(); ++i)
                    diffPlanesBits += plane.component(i).depth();
            }

            for (size_t j = 0; j < sspecs.planes(); ++j) {
                auto &plane = sspecs.plane(j);

                for (size_t i = 0; i < plane.components(); ++i)
                    diffPlanesBits -= plane.component(i).depth();
            }
        }

        uint64_t k = diffFourcc
                   + uint64_t(diffWidth * diffWidth)
                   + uint64_t(diffHeight * diffHeight)
                   + diffBpp * diffBpp
                   + diffPlanes * diffPlanes
                   + diffPlanesBits * diffPlanesBits;

        if (k < q) {
            nearestCap = cap;
            q = k;
        }
    }

    return nearestCap;
}

bool AkVCam::VideoFormat::isSameFormat(const VideoFormat &other) const
{
    return this->d->m_format == other.d->m_format
            && this->d->m_width == other.d->m_width
            && this->d->m_height == other.d->m_height;
}

size_t AkVCam::VideoFormat::dataSize() const
{
    size_t dataSize = 0;
    static const size_t align = 32;
    auto specs = VideoFormat::formatSpecs(this->d->m_format);

    // Calculate parameters for each plane
    for (size_t i = 0; i < specs.planes(); ++i) {
        auto &plane = specs.plane(i);

        // Calculate bytes used per line (bits per pixel * width / 8)
        size_t bytesUsed = plane.bitsSize() * this->d->m_width / 8;

        // Align line size for SIMD compatibility
        size_t lineSize =
                Algorithm::alignUp(bytesUsed, align);

        // Calculate plane size, considering sub-sampling
        size_t planeSize = (lineSize * this->d->m_height) >> plane.heightDiv();

        // Align plane size to ensure next plane starts aligned and update
        // total data size
        dataSize += Algorithm::alignUp(planeSize, align);
    }

    // Align total data size for buffer allocation

    return Algorithm::alignUp(dataSize, align);
}

bool AkVCam::VideoFormat::isValid() const
{
    if (this->d->m_format < PixelFormat_none)
        return false;

    if (this->d->m_width < 1 || this->d->m_height < 1)
        return false;

    if (this->d->m_fps.num() < 1 || this->d->m_fps.den() < 1)
        return false;

    return true;
}

std::string AkVCam::VideoFormat::toString() const
{
    return std::string("VideoFormat(")
            + pixelFormatToString(this->d->m_format)
            + ' '
            + std::to_string(this->d->m_width)
            + 'x'
            + std::to_string(this->d->m_height)
            + ' '
            + this->d->m_fps.toString()
            + ')';
}

int AkVCam::VideoFormat::bitsPerPixel(PixelFormat pixelFormat)
{
    return VideoFmt::formatSpecs(pixelFormat).bpp();
}

std::string AkVCam::VideoFormat::pixelFormatToString(PixelFormat pixelFormat)
{
    return VideoFmt::byPixelFormat(pixelFormat)->name;
}

AkVCam::PixelFormat AkVCam::VideoFormat::pixelFormatFromString(const std::string &pixelFormat)
{
    return VideoFmt::byName(pixelFormat)->format;
}

AkVCam::VideoFormatSpec AkVCam::VideoFormat::formatSpecs(PixelFormat pixelFormat)
{
    return VideoFmt::formatSpecs(pixelFormat);
}

std::vector<AkVCam::PixelFormat> AkVCam::VideoFormat::supportedPixelFormats()
{
    return VideoFmt::allPixelFormats();
}

std::ostream &operator <<(std::ostream &os, const AkVCam::VideoFormat &format)
{
    auto formatStr = AkVCam::VideoFormat::pixelFormatToString(format.format());

    os << "VideoFormat("
       << formatStr
       << ' '
       << format.width()
       << 'x'
       << format.height()
       << ' '
       << format.fps()
       << ')';

    return os;
}

std::ostream &operator <<(std::ostream &os, const AkVCam::VideoFormats &formats)
{
    bool writeComma = false;
    os << "VideoFormats(";

    for (auto &format: formats) {
        if (writeComma)
            os << ", ";

        os << format;
        writeComma = true;
    }

    os << ')';

    return os;
}
