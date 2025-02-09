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

#ifndef COREFOUNDATION_CFTYPE_H
#define COREFOUNDATION_CFTYPE_H

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <functional>

using CFTypeID = unsigned long;
using CFTypeDeleter = void (*)(void *data);

using SInt32 = int32_t;
using UInt8 = uint8_t;
using UInt32 = uint32_t;
using UInt64 = uint64_t;
using Float32 = float;
using Float64 = double;
using Boolean = bool;
using OSStatus = int32_t;

#define CFTYPE_FOURCC(a, b, c, d) ((int32_t(a) << 24) | (int32_t(b) << 16) | (int32_t(c) << 8) | int32_t(d))

enum OSStatus_status
{
    noErr = 0
};

struct CFType
{
    CFTypeID type;
    void *data;
    size_t size;
    CFTypeDeleter deleter;
    uint64_t ref;

    CFType(CFTypeID type=0,
           const void *data=nullptr,
           size_t size=0,
           CFTypeDeleter deleter=nullptr):
        type(type),
        data(const_cast<void *>(data)),
        size(size),
        deleter(deleter),
        ref(1)
    {

    }
};

using CFTypeRef = CFType *;

inline void CFRelease(CFTypeRef cf)
{
    --cf->ref;

    if (cf->ref < 1 && cf->deleter)
        cf->deleter(cf->data);

    delete cf;
}

inline CFTypeRef CFRetain(CFTypeRef cf)
{
    ++cf->ref;

    return cf;
}

inline Boolean CFEqual(CFTypeRef cf1, CFTypeRef cf2)
{
    return (cf1->type == cf2->type)
            && (cf1->size == cf2->size)
            && memcmp(cf1->data, cf2->data, cf1->size) == 0;
}

#endif // COREFOUNDATION_CFTYPE_H
