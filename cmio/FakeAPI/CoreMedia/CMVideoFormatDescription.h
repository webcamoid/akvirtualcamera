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

#ifndef COREMEDIA_CMVIDEOFORMATDESCRIPTION_H
#define COREMEDIA_CMVIDEOFORMATDESCRIPTION_H

#include "CMFormatDescription.h"
#include "CMVideoDimensions.h"

using CMVideoFormatDescription = CFType;
using CMVideoFormatDescriptionRef = CFTypeRef;

enum CMIOVideoFormats
{
    kCMPixelFormat_32ARGB,
    kCMPixelFormat_24RGB,
    kCMPixelFormat_16LE565,
    kCMPixelFormat_16LE555,
    kCMPixelFormat_422YpCbCr8,
    kCMPixelFormat_422YpCbCr8_yuvs
};

using CMVideoCodecType = FourCharCode;

struct _CMVideoFormatDescriptionData
{
    CMVideoCodecType codecType;
    int32_t width;
    int32_t height;

    _CMVideoFormatDescriptionData(CMVideoCodecType codecType,
                                  int32_t width,
                                  int32_t height):
        codecType(codecType),
        width(width),
        height(height)
    {
    }
};

using _CMVideoFormatDescriptionDataRef = _CMVideoFormatDescriptionData *;

inline CFTypeID CMVideoFormatDescriptionGetTypeID()
{
    return 0x6;
}

inline OSStatus CMVideoFormatDescriptionCreate(CFAllocatorRef allocator,
                                               CMVideoCodecType codecType,
                                               int32_t width,
                                               int32_t height,
                                               CFDictionaryRef extensions,
                                               CMVideoFormatDescriptionRef *formatDescriptionOut)
{
    auto self = new CMVideoFormatDescription;
    self->type = CMVideoFormatDescriptionGetTypeID();
    self->data = new _CMVideoFormatDescriptionData(codecType, width, height);
    self->size = sizeof(_CMVideoFormatDescriptionData);
    self->deleter = [] (void *data) {
        delete reinterpret_cast<_CMVideoFormatDescriptionDataRef>(data);
    };
    self->ref = 1;
    *formatDescriptionOut = self;

    return noErr;
}

inline FourCharCode CMFormatDescriptionGetMediaSubType(CMVideoFormatDescriptionRef videoDesc)
{
    auto self = reinterpret_cast<_CMVideoFormatDescriptionDataRef>(videoDesc->data);

    return self->codecType;
}

inline CMVideoDimensions CMVideoFormatDescriptionGetDimensions(CMVideoFormatDescriptionRef videoDesc)
{
    auto self = reinterpret_cast<_CMVideoFormatDescriptionDataRef>(videoDesc->data);

    return {self->width, self->height};
}

#endif // COREMEDIA_CMVIDEOFORMATDESCRIPTION_H
