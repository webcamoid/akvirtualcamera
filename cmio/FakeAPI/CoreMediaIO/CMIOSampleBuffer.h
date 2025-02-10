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

#ifndef COREMEDIAIO_CMIOSAMPLEBUFFER_H
#define COREMEDIAIO_CMIOSAMPLEBUFFER_H

#include "../CoreMedia/CMTime.h"
#include "../CoreMedia/CMSampleBuffer.h"
#include "../CoreMedia/CMSampleTimingInfo.h"

enum kCMIOSampleBufferFlags
{
    kCMIOSampleBufferNoDiscontinuities                      = 0,
    kCMIOSampleBufferDiscontinuityFlag_UnknownDiscontinuity = 0x1,
};

inline OSStatus CMIOSampleBufferCreateForImageBuffer(CFAllocatorRef allocator,
                                                     CVImageBufferRef imageBuffer,
                                                     CMVideoFormatDescriptionRef formatDescription,
                                                     const CMSampleTimingInfo *sampleTiming,
                                                     UInt64 sequenceNumber,
                                                     UInt32 discontinuityFlags,
                                                     CMSampleBufferRef *sBufOut)
{
    auto self = new CMSampleBuffer;
    self->type = CMSampleBufferGetTypeID();
    self->data = new _CMSampleBufferData(imageBuffer,
                                         formatDescription,
                                         sampleTiming,
                                         sequenceNumber,
                                         discontinuityFlags);
    self->size = sizeof(_CMSampleBufferData);
    self->deleter = [] (void *data) {
        auto self = reinterpret_cast<_CMSampleBufferDataRef>(data);
        CFRelease(self->formatDescription);
        CFRelease(self->imageBuffer);

        delete self;
    };
    self->ref = 1;
    *sBufOut = self;

    return noErr;
}

#endif // COREMEDIAIO_CMIOSAMPLEBUFFER_H
