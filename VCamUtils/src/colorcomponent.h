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

#ifndef AKVCAMUTILS_COLORCOMPONENT_H
#define AKVCAMUTILS_COLORCOMPONENT_H

#include <sstream>
#include <vector>

namespace AkVCam
{
    class ColorComponent;
    class ColorComponentPrivate;

    using ColorComponentList = std::vector<ColorComponent>;

    class ColorComponent
    {
        public:
            enum ComponentType
            {
                CT_Unknown,
                CT_R,
                CT_G,
                CT_B,
                CT_Y,
                CT_U,
                CT_V,
                CT_A
            };

            ColorComponent();
            ColorComponent(ComponentType type,
                           size_t step,
                           size_t offset,
                           size_t shift,
                           size_t byteDepth,
                           size_t depth,
                           size_t widthDiv,
                           size_t heightDiv);
            ColorComponent(const ColorComponent &other);
            ~ColorComponent();
            ColorComponent &operator =(const ColorComponent &other);
            bool operator ==(const ColorComponent &other) const;
            bool operator !=(const ColorComponent &other) const;

            ColorComponent::ComponentType type() const;
            size_t step() const;
            size_t offset() const;
            size_t shift() const;
            size_t byteDepth() const;
            size_t depth() const;
            size_t widthDiv() const;
            size_t heightDiv() const;

            template <typename T>
            inline T max() const
            {
                return (T(1) << this->depth()) - 1;
            }

        private:
            ColorComponentPrivate *d;
    };
}

std::ostream &operator <<(std::ostream &os, const AkVCam::ColorComponent &component);
std::ostream &operator <<(std::ostream &os, AkVCam::ColorComponent::ComponentType type);

#endif // AKVCAMUTILS_COLORCOMPONENT_H
