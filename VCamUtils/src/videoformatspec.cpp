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

#include <sstream>

#include "videoformatspec.h"
#include "commons.h"

namespace AkVCam
{
    class VideoFormatSpecPrivate
    {
        public:
            VideoFormatSpec::VideoFormatType m_type {VideoFormatSpec::VFT_Unknown};
            int m_endianness {ENDIANNESS_BO};
            ColorPlanes m_planes;
    };
}

AkVCam::VideoFormatSpec::VideoFormatSpec()
{
    this->d = new VideoFormatSpecPrivate();
}

AkVCam::VideoFormatSpec::VideoFormatSpec(VideoFormatType type,
                                         int endianness,
                                         const ColorPlanes &planes)
{
    this->d = new VideoFormatSpecPrivate();
    this->d->m_type = type;
    this->d->m_endianness = endianness;
    this->d->m_planes = planes;
}

AkVCam::VideoFormatSpec::VideoFormatSpec(const VideoFormatSpec &other)
{
    this->d = new VideoFormatSpecPrivate();
    this->d->m_type = other.d->m_type;
    this->d->m_endianness = other.d->m_endianness;
    this->d->m_planes = other.d->m_planes;
}

AkVCam::VideoFormatSpec::~VideoFormatSpec()
{
    delete this->d;
}

AkVCam::VideoFormatSpec &AkVCam::VideoFormatSpec::operator =(const VideoFormatSpec &other)
{
    if (this != &other) {
        this->d->m_type = other.d->m_type;
        this->d->m_endianness = other.d->m_endianness;
        this->d->m_planes = other.d->m_planes;
    }

    return *this;
}

bool AkVCam::VideoFormatSpec::operator ==(const VideoFormatSpec &other) const
{
    return this->d->m_type == other.d->m_type
            && this->d->m_endianness == other.d->m_endianness
            && this->d->m_planes == other.d->m_planes;
}

bool AkVCam::VideoFormatSpec::operator !=(const VideoFormatSpec &other) const
{
    return !(*this == other);
}

int AkVCam::VideoFormatSpec::endianness() const
{
    return this->d->m_endianness;
}

AkVCam::VideoFormatSpec::VideoFormatType AkVCam::VideoFormatSpec::type() const
{
    return this->d->m_type;
}

size_t AkVCam::VideoFormatSpec::planes() const
{
    return this->d->m_planes.size();
}

const AkVCam::ColorPlane &AkVCam::VideoFormatSpec::plane(size_t plane) const
{
    return this->d->m_planes[plane];
}

int AkVCam::VideoFormatSpec::bpp() const
{
    int bpp = 0;

    for (auto &plane: this->d->m_planes)
        bpp += plane.bitsSize();

    return bpp;
}

AkVCam::ColorComponent AkVCam::VideoFormatSpec::component(AkVCam::ColorComponent::ComponentType componentType) const
{
    for (auto &plane: this->d->m_planes)
        for (size_t i = 0; i < plane.components(); ++i) {
            auto &component = plane.component(i);

            if (component.type() == componentType)
                return component;
        }

    return {};
}

int AkVCam::VideoFormatSpec::componentPlane(AkVCam::ColorComponent::ComponentType component) const
{
    int planeIndex = 0;

    for (auto &plane: this->d->m_planes) {
        for (size_t i = 0; i < plane.components(); ++i) {
            auto &component_ = plane.component(i);

            if (component_.type() == component)
                return planeIndex;
        }

        planeIndex++;
    }

    return -1;
}

bool AkVCam::VideoFormatSpec::contains(AkVCam::ColorComponent::ComponentType component) const
{
    for (auto &plane: this->d->m_planes)
        for (size_t i = 0; i < plane.components(); ++i) {
            auto &component_ = plane.component(i);

            if (component_.type() == component)
                return true;
        }

    return false;
}

size_t AkVCam::VideoFormatSpec::byteDepth() const
{
    if (this->d->m_type == VFT_RGB)
        return this->component(ColorComponent::CT_R).byteDepth();

    return this->component(ColorComponent::CT_Y).byteDepth();
}

size_t AkVCam::VideoFormatSpec::depth() const
{
    if (this->d->m_type == VFT_RGB)
        return this->component(ColorComponent::CT_R).depth();

    return this->component(ColorComponent::CT_Y).depth();
}

size_t AkVCam::VideoFormatSpec::numberOfComponents() const
{
    auto n = this->mainComponents();

    if (this->contains(ColorComponent::CT_A))
        n++;

    return n;
}

size_t AkVCam::VideoFormatSpec::mainComponents() const
{
    size_t n = 0;

    switch (this->d->m_type) {
    case VFT_RGB:
    case VFT_YUV:
        n = 3;

        break;

    case VFT_Gray:
        n = 1;

        break;

    default:
        break;
    }

    return n;
}

bool AkVCam::VideoFormatSpec::isFast() const
{
    if (this->d->m_endianness != ENDIANNESS_BO)
        return false;

    size_t curDepth = 0;

    for (const auto &plane: this->d->m_planes)
        for (size_t c = 0; c < plane.components(); ++c) {
            auto component = plane.component(c);

            if (component.shift() > 0)
                return false;

            auto depth = component.depth();

            if (depth < 1 || (depth & (depth - 1)))
                return false;

            if (curDepth < 1)
                curDepth = depth;
            else if (depth != curDepth)
                return false;
        }

    return true;
}

std::ostream &operator <<(std::ostream &os, const AkVCam::VideoFormatSpec &spec)
{
    os << "VideoFormatSpec("
                    << "type="
                    << spec.type()
                    << ", endianness="
                    << spec.endianness()
                    << ", planes="
                    << spec.planes()
                    << ", bpp="
                    << spec.bpp()
                    << ")";

    return os;
}

#define DEFINE_CASE(vft) \
    case AkVCam::VideoFormatSpec::vft: \
        os << #vft; \
        \
        break;

std::ostream &operator <<(std::ostream &os, AkVCam::VideoFormatSpec::VideoFormatType type)
{
    switch (type) {
    DEFINE_CASE(VFT_RGB)
    DEFINE_CASE(VFT_YUV)
    DEFINE_CASE(VFT_Gray)

    default:
        os << "VFT_Unknown";

        break;
    }

    return os;
}
