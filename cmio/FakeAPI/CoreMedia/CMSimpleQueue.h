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

#ifndef COREMEDIA_CMSIMPLEQUEUE_H
#define COREMEDIA_CMSIMPLEQUEUE_H

#include <queue>

#include "CoreFoundation/Allocators.h"
#include "CoreFoundation/CFType.h"

using CMSimpleQueue = CFType;
using CMSimpleQueueRef = CFTypeRef;

struct _CMSimpleQueueData
{
    std::queue<const void *> items;
    int32_t capacity;

    _CMSimpleQueueData(int32_t capacity):
        capacity(capacity)
    {
    }
};

using _CMSimpleQueueDataRef = _CMSimpleQueueData *;

inline CFTypeID CMSimpleQueueGetTypeID()
{
    return 0x5;
}

inline OSStatus CMSimpleQueueCreate(CFAllocatorRef allocator,
                                    int32_t capacity,
                                    CMSimpleQueueRef *queueOut)
{
    auto self = new CMSimpleQueue;
    self->type = CMSimpleQueueGetTypeID();
    self->data = new _CMSimpleQueueData(capacity);
    self->size = sizeof(_CMSimpleQueueData);
    self->deleter = [] (void *data) {
        delete reinterpret_cast<_CMSimpleQueueDataRef>(data);
    };
    self->ref = 1;
    *queueOut = self;

    return noErr;
}

inline OSStatus CMSimpleQueueEnqueue(CMSimpleQueueRef queue,
                                     const void *element)
{
    auto self = reinterpret_cast<_CMSimpleQueueDataRef>(queue->data);

    if (self->items.size() == self->capacity)
        return -1;

    self->items.push(element);

    return noErr;
}

inline const void *CMSimpleQueueDequeue(CMSimpleQueueRef queue)
{
    auto self = reinterpret_cast<_CMSimpleQueueDataRef>(queue->data);

    if (self->items.empty())
        return nullptr;

    auto element = self->items.front();
    self->items.pop();

    return element;
}

inline OSStatus CMSimpleQueueReset(CMSimpleQueueRef queue)
{
    auto self = reinterpret_cast<_CMSimpleQueueDataRef>(queue->data);

    while (!self->items.empty())
        self->items.pop();

    return noErr;
}

inline int32_t CMSimpleQueueGetCapacity(CMSimpleQueueRef queue)
{
    return reinterpret_cast<_CMSimpleQueueDataRef>(queue->data)->capacity;
}

inline int32_t CMSimpleQueueGetCount(CMSimpleQueueRef queue)
{
    return int32_t(reinterpret_cast<_CMSimpleQueueDataRef>(queue->data)->items.size());
}

inline Float32 CMSimpleQueueGetFullness(CMSimpleQueueRef queue)
{
    auto self = reinterpret_cast<_CMSimpleQueueDataRef>(queue->data);

    return Float32(self->items.size()) / self->capacity;
}

#endif // COREMEDIA_CMSIMPLEQUEUE_H
