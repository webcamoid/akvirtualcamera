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

#ifndef COREVIDEO_CVPIXELBUFFER_H
#define COREVIDEO_CVPIXELBUFFER_H

#include "CoreFoundation/CFDictionary.h"
#include "CoreMedia/CMVideoFormatDescription.h"

using CVPixelBuffer = CFType;
using CVPixelBufferRef = CFTypeRef;
using CVImageBufferRef = CVPixelBufferRef;

using CVReturn = int32_t;
using OSType = FourCharCode;
using CVOptionFlags = uint64_t;

struct _CVPixelBufferData
{
    OSType pixelFormatType;
    size_t width;
    size_t height;
    std::vector<char> data;

    _CVPixelBufferData(OSType pixelFormatType,
                       size_t width,
                       size_t height):
        pixelFormatType(pixelFormatType),
        width(width),
        height(height)
    {
        this->data.resize(sizeof(uint32_t) * width * height);
    }
};

using _CVPixelBufferDataRef = _CVPixelBufferData *;

inline CFTypeID CVPixelBufferGetTypeID()
{
    return 0x8;
}

inline CVReturn CVPixelBufferCreate(CFAllocatorRef allocator,
                                    size_t width,
                                    size_t height,
                                    OSType pixelFormatType,
                                    CFDictionaryRef pixelBufferAttributes,
                                    CVPixelBufferRef *pixelBufferOut)
{
    auto self = new CVPixelBuffer;
    self->type = CVPixelBufferGetTypeID();
    self->data = new _CVPixelBufferData(pixelFormatType, width, height);
    self->size = sizeof(_CVPixelBufferData);
    self->deleter = [] (void *data) {
        delete reinterpret_cast<_CVPixelBufferDataRef>(data);
    };
    self->ref = 1;
    *pixelBufferOut = self;

    return noErr;
}

inline CVReturn CVPixelBufferLockBaseAddress(CVPixelBufferRef pixelBuffer,
                                             CVOptionFlags lockFlags)
{
    return noErr;
}

inline CVReturn CVPixelBufferUnlockBaseAddress(CVPixelBufferRef pixelBuffer,
                                               CVOptionFlags unlockFlags)
{
    return noErr;
}

inline void *CVPixelBufferGetBaseAddress(CVPixelBufferRef pixelBuffer)
{
    return reinterpret_cast<_CVPixelBufferDataRef>(pixelBuffer->data)->data.data();
}

inline OSStatus CMVideoFormatDescriptionCreateForImageBuffer(CFAllocatorRef allocator,
                                                             CVImageBufferRef imageBuffer,
                                                             CMVideoFormatDescriptionRef *formatDescriptionOut)
{
    auto self = reinterpret_cast<_CVPixelBufferDataRef>(imageBuffer->data);

    return CMVideoFormatDescriptionCreate(allocator,
                                          self->pixelFormatType,
                                          self->width,
                                          self->height,
                                          nullptr,
                                          formatDescriptionOut);
}

#endif // COREVIDEO_CVPIXELBUFFER_H
