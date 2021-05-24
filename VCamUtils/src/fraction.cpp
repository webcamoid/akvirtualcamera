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

#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>

#include "fraction.h"
#include "utils.h"

namespace AkVCam
{
    class FractionPrivate
    {
        public:
            int64_t m_num;
            int64_t m_den;
    };
}

AkVCam::Fraction::Fraction()
{
    this->d = new FractionPrivate;
    this->d->m_num = 0;
    this->d->m_den = 0;
}

AkVCam::Fraction::Fraction(int64_t num, int64_t den)
{
    this->d = new FractionPrivate;
    this->d->m_num = num;
    this->d->m_den = den;
}

AkVCam::Fraction::Fraction(const std::string &str)
{
    this->d = new FractionPrivate;
    this->d->m_num = 0;
    this->d->m_den = 1;
    auto pos = str.find('/');

    if (pos == std::string::npos) {
        auto strCpy = trimmed(str);
        this->d->m_num = uint32_t(strtol(strCpy.c_str(), nullptr, 10));
    } else {
        auto numStr = trimmed(str.substr(0, pos));
        auto denStr = trimmed(str.substr(pos + 1));

        this->d->m_num = uint32_t(strtol(numStr.c_str(), nullptr, 10));
        this->d->m_den = uint32_t(strtol(denStr.c_str(), nullptr, 10));

        if (this->d->m_den < 1) {
            this->d->m_num = 0;
            this->d->m_den = 1;
        }
    }
}

AkVCam::Fraction::Fraction(const Fraction &other)
{
    this->d = new FractionPrivate;
    this->d->m_num = other.d->m_num;
    this->d->m_den = other.d->m_den;
}

AkVCam::Fraction::~Fraction()
{
    delete this->d;
}

AkVCam::Fraction &AkVCam::Fraction::operator =(const Fraction &other)
{
    if (this != &other) {
        this->d->m_num = other.d->m_num;
        this->d->m_den = other.d->m_den;
    }

    return *this;
}

bool AkVCam::Fraction::operator ==(const Fraction &other) const
{
    if (this->d->m_den == 0 && other.d->m_den != 0)
        return false;

    if (this->d->m_den != 0 && other.d->m_den == 0)
        return false;

    return this->d->m_num * other.d->m_den == this->d->m_den * other.d->m_num;
}

bool AkVCam::Fraction::operator <(const Fraction &other) const
{
    return this->d->m_num * other.d->m_den < this->d->m_den * other.d->m_num;
}

int64_t AkVCam::Fraction::num() const
{
    return this->d->m_num;
}

int64_t &AkVCam::Fraction::num()
{
    return this->d->m_num;
}

int64_t AkVCam::Fraction::den() const
{
    return this->d->m_den;
}

int64_t &AkVCam::Fraction::den()
{
    return this->d->m_den;
}

double AkVCam::Fraction::value() const
{
    return double(this->d->m_num) / double(this->d->m_den);
}

std::string AkVCam::Fraction::toString() const
{
    std::stringstream ss;
    ss << this->d->m_num << '/' << this->d->m_den;

    return ss.str();
}

bool AkVCam::Fraction::isInfinity() const
{
    return this->d->m_num != 0 && this->d->m_den == 0;
}

int AkVCam::Fraction::sign() const
{
    return std::signbit(this->d->m_num) == std::signbit(this->d->m_den)? 1: -1;
}

bool AkVCam::Fraction::isFraction(const std::string &str)
{
    auto pos = str.find('/');

    if (pos == std::string::npos) {
        auto strCpy = trimmed(str);
        char *p = nullptr;
        strtol(strCpy.c_str(), &p, 10);

        if (*p)
            return false;
    } else {
        auto numStr = trimmed(str.substr(0, pos));
        auto denStr = trimmed(str.substr(pos + 1));
        char *p = nullptr;
        char *q = nullptr;
        strtol(numStr.c_str(), &p, 10);
        strtol(denStr.c_str(), &q, 10);

        if (*p || *q)
            return false;
    }

    return true;
}

std::ostream &operator <<(std::ostream &os, const AkVCam::Fraction &fraction)
{
    os << fraction.toString();

    return os;
}
