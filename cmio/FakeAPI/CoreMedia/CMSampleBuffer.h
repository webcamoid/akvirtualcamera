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

#ifndef COREMEDIA_CMSAMPLEBUFFER_H
#define COREMEDIA_CMSAMPLEBUFFER_H

#include "CoreFoundation/Allocators.h"
#include "CoreFoundation/CFType.h"
#include "CoreVideo/CVPixelBuffer.h"
#include "CMSampleTimingInfo.h"

using CMSampleBuffer = CFType;
using CMSampleBufferRef = CFTypeRef;

struct _CMSampleBufferData
{
    CVImageBufferRef imageBuffer {nullptr};
    CMVideoFormatDescriptionRef formatDescription {nullptr};
    CMSampleTimingInfo sampleTiming;
    UInt64 sequenceNumber {0};
    UInt32 discontinuityFlags {0};

    _CMSampleBufferData(CVImageBufferRef imageBuffer,
                        CMVideoFormatDescriptionRef formatDescription,
                        const CMSampleTimingInfo *sampleTiming,
                        UInt64 sequenceNumber,
                        UInt32 discontinuityFlags):
        imageBuffer(CFRetain(imageBuffer)),
        formatDescription(CFRetain(formatDescription)),
        sequenceNumber(sequenceNumber),
        discontinuityFlags(discontinuityFlags)
    {
        memcpy(&this->sampleTiming, sampleTiming, sizeof(CMSampleTimingInfo));
    }
};

using _CMSampleBufferDataRef = _CMSampleBufferData *;

inline CFTypeID CMSampleBufferGetTypeID()
{
    return 0x9;
}

#endif // COREMEDIA_CMSAMPLEBUFFER_H
