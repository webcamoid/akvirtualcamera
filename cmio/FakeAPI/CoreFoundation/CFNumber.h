/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#ifndef CFNUMBER_H
#define CFNUMBER_H

#include "Allocators.h"
#include "CFType.h"

using CFNumber = CFType;
using CFNumberRef = CFTypeRef;

enum CFNumberType
{
    kCFNumberIntType = 9,
    kCFNumberLongLongType = 11,
    kCFNumberDoubleType = 13
};

struct _CFNumberData
{
    CFNumberType type;
    union
    {
        int i;
        double d;
    } data;

    _CFNumberData(CFNumberType type, const void *valuePtr):
        type(type)
    {
        switch (type) {
        case kCFNumberIntType:
            this->data.i = *reinterpret_cast<const int *>(valuePtr);

            break;
        case kCFNumberDoubleType:
            this->data.d = *reinterpret_cast<const double *>(valuePtr);

            break;
        }
    }
};

using _CFNumberDataRef = _CFNumberData *;

inline CFTypeID CFNumberGetTypeID()
{
    return 0x2;
}

inline CFNumberRef CFNumberCreate(CFAllocatorRef allocator,
                                  CFNumberType type,
                                  const void *valuePtr)
{
    auto numberType = new CFNumber;
    numberType->type = CFNumberGetTypeID();
    numberType->data = new _CFNumberData(type, valuePtr);
    numberType->size = sizeof(_CFNumberData);
    numberType->deleter = [] (void *data) {
        delete reinterpret_cast<_CFNumberDataRef>(data);
    };
    numberType->ref = 1;

    return numberType;
}

inline bool CFNumberGetValue(CFNumberRef number,
                             CFNumberType type,
                             void *valuePtr)
{
    auto self = reinterpret_cast<_CFNumberDataRef>(number->data);
    bool ok = false;

    switch (type) {
    case kCFNumberIntType:
        switch (self->type) {
        case kCFNumberIntType:
            *reinterpret_cast<int *>(valuePtr) = self->data.i;
            ok = true;

            break;

        case kCFNumberDoubleType:
            *reinterpret_cast<int *>(valuePtr) = int(self->data.d);
            ok = true;

            break;

        default:
            break;
        }

        break;
    case kCFNumberDoubleType:
        switch (self->type) {
        case kCFNumberIntType:
            *reinterpret_cast<double *>(valuePtr) = double(self->data.i);
            ok = true;

            break;

        case kCFNumberDoubleType:
            *reinterpret_cast<double *>(valuePtr) = self->data.d;
            ok = true;

            break;

        default:
            break;
        }

        break;
    }

    return ok;
}

#endif // CFNUMBER_H
