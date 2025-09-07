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

#include "colorcomponent.h"

namespace AkVCam
{
    class ColorComponentPrivate
    {
        public:
            ColorComponent::ComponentType m_type {ColorComponent::CT_Unknown};
            size_t m_step {0};       // Bytes to increment for reading th next pixel.
            size_t m_offset {0};     // Bytes to skip before reading the component.
            size_t m_shift {0};      // Shift the value n-bits to the left before reading the component.
            size_t m_byteDepth {0};  // Read n-bytes for the value.
            size_t m_depth {0};      // Size of the component in bits.
            size_t m_widthDiv {0};   // Plane width should be divided by 2^widthDiv
            size_t m_heightDiv {0};  // Plane height should be divided by 2^heightDiv
    };
}

AkVCam::ColorComponent::ColorComponent()
{
    this->d = new ColorComponentPrivate();
}

AkVCam::ColorComponent::ColorComponent(ComponentType type,
                                       size_t step,
                                       size_t offset,
                                       size_t shift,
                                       size_t byteDepth,
                                       size_t depth,
                                       size_t widthDiv,
                                       size_t heightDiv)
{
    this->d = new ColorComponentPrivate();
    this->d->m_type = type;
    this->d->m_step = step;
    this->d->m_offset = offset;
    this->d->m_shift = shift;
    this->d->m_byteDepth = byteDepth;
    this->d->m_depth = depth;
    this->d->m_widthDiv = widthDiv;
    this->d->m_heightDiv = heightDiv;
}

AkVCam::ColorComponent::ColorComponent(const ColorComponent &other)
{
    this->d = new ColorComponentPrivate();
    this->d->m_type = other.d->m_type;
    this->d->m_step = other.d->m_step;
    this->d->m_offset = other.d->m_offset;
    this->d->m_shift = other.d->m_shift;
    this->d->m_byteDepth = other.d->m_byteDepth;
    this->d->m_depth = other.d->m_depth;
    this->d->m_widthDiv = other.d->m_widthDiv;
    this->d->m_heightDiv = other.d->m_heightDiv;
}

AkVCam::ColorComponent::~ColorComponent()
{
    delete this->d;
}

AkVCam::ColorComponent &AkVCam::ColorComponent::operator =(const ColorComponent &other)
{
    if (this != &other) {
        this->d->m_type = other.d->m_type;
        this->d->m_step = other.d->m_step;
        this->d->m_offset = other.d->m_offset;
        this->d->m_shift = other.d->m_shift;
        this->d->m_byteDepth = other.d->m_byteDepth;
        this->d->m_depth = other.d->m_depth;
        this->d->m_widthDiv = other.d->m_widthDiv;
        this->d->m_heightDiv = other.d->m_heightDiv;
    }

    return *this;
}

bool AkVCam::ColorComponent::operator ==(const AkVCam::ColorComponent &other) const
{
    return this->d->m_type == other.d->m_type
           && this->d->m_step == other.d->m_step
           && this->d->m_offset == other.d->m_offset
           && this->d->m_shift == other.d->m_shift
           && this->d->m_byteDepth == other.d->m_byteDepth
           && this->d->m_depth == other.d->m_depth
           && this->d->m_widthDiv == other.d->m_widthDiv
           && this->d->m_heightDiv == other.d->m_heightDiv;
}

bool AkVCam::ColorComponent::operator !=(const ColorComponent &other) const
{
    return !(*this == other);
}

AkVCam::ColorComponent::ComponentType AkVCam::ColorComponent::type() const
{
    return this->d->m_type;
}

size_t AkVCam::ColorComponent::step() const
{
    return this->d->m_step;
}

size_t AkVCam::ColorComponent::offset() const
{
    return this->d->m_offset;
}

size_t AkVCam::ColorComponent::shift() const
{
    return this->d->m_shift;
}

size_t AkVCam::ColorComponent::byteDepth() const
{
    return this->d->m_byteDepth;
}

size_t AkVCam::ColorComponent::depth() const
{
    return this->d->m_depth;
}

size_t AkVCam::ColorComponent::widthDiv() const
{
    return this->d->m_widthDiv;
}

size_t AkVCam::ColorComponent::heightDiv() const
{
    return this->d->m_heightDiv;
}

std::ostream &operator <<(std::ostream &os, const AkVCam::ColorComponent &colorComponent)
{
    os << "ColorComponent("
       << "type="
       << colorComponent.type()
       << ", step="
       << colorComponent.step()
       << ", offset="
       << colorComponent.offset()
       << ", shift="
       << colorComponent.shift()
       << ", byteDepth="
       << colorComponent.byteDepth()
       << ", depth="
       << colorComponent.depth()
       << ", widthDiv="
       << colorComponent.widthDiv()
       << ", heightDiv="
       << colorComponent.heightDiv()
       << ")";

    return os;
}

#define DEFINE_CASE(ct) \
    case AkVCam::ColorComponent::ct: \
        os << #ct; \
        \
        break;

std::ostream &operator <<(std::ostream &os, AkVCam::ColorComponent::ComponentType type)
{
    switch (type) {
    DEFINE_CASE(CT_R)
    DEFINE_CASE(CT_G)
    DEFINE_CASE(CT_B)
    DEFINE_CASE(CT_Y)
    DEFINE_CASE(CT_U)
    DEFINE_CASE(CT_V)

    default:
        os << "CT_Unknown";

        break;
    }

    return os;
}
