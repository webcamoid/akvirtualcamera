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

#ifndef CFARRAY_H
#define CFARRAY_H

#include <vector>

#include "Allocators.h"
#include "CFIndex.h"
#include "CFString.h"
#include "CFType.h"

using CFArray = CFType;
using CFArrayRef = CFTypeRef;

using CFArrayRetainCallBack = const void *(*)(CFAllocatorRef allocator, const void *value);
using CFArrayReleaseCallBack = void (*)(CFAllocatorRef allocator, const void *value);
using CFArrayCopyDescriptionCallBack = CFStringRef(*)(const void *value);
using CFArrayEqualCallBack = bool (*)(const void *value1, const void *value2);

struct CFArrayCallBacks
{
    CFArrayRetainCallBack          retain;
    CFArrayReleaseCallBack         release;
    CFArrayCopyDescriptionCallBack copyDescription;
    CFArrayEqualCallBack           equal;
};

struct _CFArrayData
{
    std::vector<void *> items;
    CFArrayCallBacks callBacks;

    _CFArrayData(const void **values,
                  CFIndex numValues,
                  const CFArrayCallBacks *callBacks)
    {
        this->items.resize(numValues);
        memcpy(this->items.data(), values, numValues * sizeof(void *));
        memcpy(&this->callBacks, callBacks, sizeof(CFArrayCallBacks));
    }
};

using _CFArrayDataRef = _CFArrayData *;

inline CFTypeID CFArrayGetTypeID()
{
    return 0x3;
}

inline CFArrayRef CFArrayCreate(CFAllocatorRef allocator,
                                const void **values,
                                CFIndex numValues,
                                const CFArrayCallBacks *callBacks)
{
    auto arrayType = new CFArray;
    arrayType->type = CFArrayGetTypeID();
    arrayType->data = new _CFArrayData(values, numValues, callBacks);
    arrayType->size = sizeof(_CFArrayData);
    arrayType->deleter = [] (void *data) {
        auto self = reinterpret_cast<_CFArrayDataRef>(data);

        for (auto &item: self->items)
            self->callBacks.release(kCFAllocatorDefault, item);

        delete self;
    };
    arrayType->ref = 1;

    return arrayType;
}

inline CFIndex CFArrayGetCount(CFArrayRef array)
{
    auto self = reinterpret_cast<_CFArrayDataRef>(array->data);

    return self->items.size();
}

inline void *CFArrayGetValueAtIndex(CFArrayRef array, CFIndex index)
{
    auto self = reinterpret_cast<_CFArrayDataRef>(array->data);

    return self->items.at(index);
}

#endif // CFARRAY_H
