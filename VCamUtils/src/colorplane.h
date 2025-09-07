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

#ifndef AKVCAMUTILS_COLORPLANE_H
#define AKVCAMUTILS_COLORPLANE_H

#include "colorcomponent.h"

namespace AkVCam
{
    class ColorPlane;
    class ColorPlanePrivate;

    using ColorPlanes = std::vector<ColorPlane>;

    class ColorPlane
    {
        public:
            ColorPlane();
            ColorPlane(const ColorComponentList &components,
                       size_t bitsSize);
            ColorPlane(const ColorPlane &other);
            ~ColorPlane();
            ColorPlane &operator =(const ColorPlane &other);
            bool operator ==(const ColorPlane &other) const;
            bool operator !=(const ColorPlane &other) const;

            size_t components() const;
            const ColorComponent &component(size_t component) const;
            size_t bitsSize() const;
            size_t pixelSize() const;
            size_t widthDiv() const;
            size_t heightDiv() const;

        private:
            ColorPlanePrivate *d;
    };
}

std::ostream &operator <<(std::ostream &os, const AkVCam::ColorPlane &plane);

#endif // AKVCAMUTILS_COLORPLANE_H
