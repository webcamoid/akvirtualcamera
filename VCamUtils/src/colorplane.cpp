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

#include "colorplane.h"

namespace AkVCam
{
    class ColorPlanePrivate
    {
        public:
            ColorComponentList m_components;
            size_t m_bitsSize {0};
            size_t m_pixelSize {0};
            size_t m_widthDiv {0};
            size_t m_heightDiv {0};
    };
}

AkVCam::ColorPlane::ColorPlane()
{
    this->d = new ColorPlanePrivate();
}

AkVCam::ColorPlane::ColorPlane(const ColorComponentList &components,
                               size_t bitsSize)
{
    this->d = new ColorPlanePrivate();
    this->d->m_components = components;
    this->d->m_bitsSize = bitsSize;

    for (auto &component: components) {
        this->d->m_pixelSize = std::max(this->d->m_pixelSize, component.step());
        this->d->m_widthDiv = this->d->m_widthDiv < 1?
                                  component.widthDiv():
                                  std::min(this->d->m_widthDiv, component.widthDiv());
        this->d->m_heightDiv = std::max(this->d->m_heightDiv, component.heightDiv());
    }
}

AkVCam::ColorPlane::ColorPlane(const ColorPlane &other)
{
    this->d = new ColorPlanePrivate();
    this->d->m_components = other.d->m_components;
    this->d->m_bitsSize = other.d->m_bitsSize;
    this->d->m_pixelSize = other.d->m_pixelSize;
    this->d->m_widthDiv = other.d->m_widthDiv;
    this->d->m_heightDiv = other.d->m_heightDiv;
}

AkVCam::ColorPlane::~ColorPlane()
{
    delete this->d;
}

AkVCam::ColorPlane &AkVCam::ColorPlane::operator =(const ColorPlane &other)
{
    if (this != &other) {
        this->d->m_components = other.d->m_components;
        this->d->m_bitsSize = other.d->m_bitsSize;
        this->d->m_pixelSize = other.d->m_pixelSize;
        this->d->m_widthDiv = other.d->m_widthDiv;
        this->d->m_heightDiv = other.d->m_heightDiv;
    }

    return *this;
}

bool AkVCam::ColorPlane::operator ==(const ColorPlane &other) const
{
    return this->d->m_components == other.d->m_components
           && this->d->m_bitsSize == other.d->m_bitsSize;
}

bool AkVCam::ColorPlane::operator !=(const ColorPlane &other) const
{
    return !(*this == other);
}

size_t AkVCam::ColorPlane::components() const
{
    return this->d->m_components.size();
}

const AkVCam::ColorComponent &AkVCam::ColorPlane::component(size_t component) const
{
    return this->d->m_components[component];
}

size_t AkVCam::ColorPlane::bitsSize() const
{
    return this->d->m_bitsSize;
}

size_t AkVCam::ColorPlane::pixelSize() const
{
    return this->d->m_pixelSize;
}

size_t AkVCam::ColorPlane::widthDiv() const
{
    return this->d->m_widthDiv;
}

size_t AkVCam::ColorPlane::heightDiv() const
{
    return this->d->m_heightDiv;
}

std::ostream &operator <<(std::ostream &os, const AkVCam::ColorPlane &colorPlane)
{
    os << "AkColorPlane("
       << "components="
       << colorPlane.components()
       << ", bitsSize="
       << colorPlane.bitsSize()
       << ", pixelSize="
       << colorPlane.pixelSize()
       << ", heightDiv="
       << colorPlane.heightDiv()
       << ")";

    return os;
}
