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

#include "rect.h"

namespace AkVCam
{
    class RectPrivate
    {
        public:
            int m_x {0};
            int m_y {0};
            int m_width {0};
            int m_height {0};
    };
}

AkVCam::Rect::Rect()
{
    this->d = new RectPrivate;
}

AkVCam::Rect::Rect(int x, int y, int width, int height)
{
    this->d = new RectPrivate;
    this->d->m_x = x;
    this->d->m_y = y;
    this->d->m_width = width;
    this->d->m_height = height;
}

AkVCam::Rect::Rect(const Rect &other)
{
    this->d = new RectPrivate;
    this->d->m_x = other.d->m_x;
    this->d->m_y = other.d->m_y;
    this->d->m_width = other.d->m_width;
    this->d->m_height = other.d->m_height;
}

AkVCam::Rect::~Rect()
{
    delete this->d;
}

AkVCam::Rect &AkVCam::Rect::operator =(const Rect &other)
{
    if (this != &other) {
        this->d->m_x = other.d->m_x;
        this->d->m_y = other.d->m_y;
        this->d->m_width = other.d->m_width;
        this->d->m_height = other.d->m_height;
    }

    return *this;
}

bool AkVCam::Rect::operator ==(const Rect &other) const
{
    return this->d->m_x == other.d->m_x
        && this->d->m_y == other.d->m_y
        && this->d->m_width == other.d->m_width
        && this->d->m_height == other.d->m_height;
}

int AkVCam::Rect::x() const
{
    return this->d->m_x;
}

int AkVCam::Rect::y() const
{
    return this->d->m_y;
}

int AkVCam::Rect::width() const
{
    return this->d->m_width;
}

int AkVCam::Rect::height() const
{
    return this->d->m_height;
}

void AkVCam::Rect::setX(int x)
{
    this->d->m_x = x;
}

void AkVCam::Rect::setY(int y)
{
    this->d->m_y = y;
}

void AkVCam::Rect::setWidth(int width)
{
    this->d->m_width = width;
}

void AkVCam::Rect::setHeight(int height)
{
    this->d->m_height = height;
}

bool AkVCam::Rect::isEmpty() const
{
    return this->d->m_width < 1 || this->d->m_height < 1;
}

AkVCam::Rect AkVCam::Rect::intersected(const Rect &rectangle) const
{
    int x = std::max(this->d->m_x, rectangle.d->m_x);
    int y = std::max(this->d->m_y, rectangle.d->m_y);

    int xMax = std::min(this->d->m_x + this->d->m_width , rectangle.d->m_x + rectangle.d->m_width );
    int yMax = std::min(this->d->m_y + this->d->m_height, rectangle.d->m_y + rectangle.d->m_height);

    auto width = xMax - x;
    auto height = yMax - y;

    if (width <= 0 || height <= 0)
        return {};

    return {x, y, width, height};
}
