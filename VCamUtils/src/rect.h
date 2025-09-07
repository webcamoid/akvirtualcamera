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

#ifndef AKVCAMUTILS_RECT_H
#define AKVCAMUTILS_RECT_H

#include <ostream>

namespace AkVCam
{
    class RectPrivate;

    class Rect
    {
        public:
            Rect();
            Rect(int x, int y, int width, int height);
            Rect(const Rect &other);
            virtual ~Rect();
            Rect &operator =(const Rect &other);
            bool operator ==(const Rect &other) const;

            int x() const;
            int y() const;
            int width() const;
            int height() const;

            void setX(int x);
            void setY(int y);
            void setWidth(int width);
            void setHeight(int height);

            bool isEmpty() const;
            Rect intersected(const Rect &rectangle) const;

        private:
            RectPrivate *d;
    };
}

std::ostream &operator <<(std::ostream &os, AkVCam::Rect rect);

#endif // AKVCAMUTILS_RECT_H
