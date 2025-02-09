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

#ifndef COREMEDIAIO_CMIOSTREAMCLOCK_H
#define COREMEDIAIO_CMIOSTREAMCLOCK_H

#include <cmath>

#include "../CoreFoundation/Allocators.h"
#include "../CoreFoundation/CFString.h"
#include "../CoreFoundation/CFType.h"
#include "../CoreMedia/CMTime.h"
#include "VCamUtils/src/timer.h"

struct _CMTimerData
{
    CFStringRef clockName {nullptr};
    void *sourceIdentifier {nullptr};
    CMTime getTimeCallMinimumInterval;

    // I have not idea how these parameters are being used.
    UInt32 numberOfEventsForRateSmoothing {0};
    UInt32 numberOfAveragesForRateSmoothing {0};

    CMTime deviceTime;
    UInt64 hostTime {0};

    /* The timeout signal should be connected somewhere in the client side but
     * isn't clear where yet, since a testing client has not been written.
     */
    AkVCam::Timer timer;

    _CMTimerData(CFStringRef clockName,
                 const void *sourceIdentifier,
                 CMTime getTimeCallMinimumInterval,
                 UInt32 numberOfEventsForRateSmoothing,
                 UInt32 numberOfAveragesForRateSmoothing):
        clockName(CFRetain(clockName)),
        sourceIdentifier(const_cast<void *>(sourceIdentifier)),
        getTimeCallMinimumInterval(getTimeCallMinimumInterval),
        numberOfEventsForRateSmoothing(numberOfEventsForRateSmoothing),
        numberOfAveragesForRateSmoothing(numberOfAveragesForRateSmoothing)
    {
    }
};

using _CMTimerDataRef = _CMTimerData *;

inline CFTypeID CMIOStreamClockGetTypeID()
{
    return 0x4;
}

inline OSStatus CMIOStreamClockCreate(CFAllocatorRef allocator,
                                      CFStringRef clockName,
                                      const void *sourceIdentifier,
                                      CMTime getTimeCallMinimumInterval,
                                      UInt32 numberOfEventsForRateSmoothing,
                                      UInt32 numberOfAveragesForRateSmoothing,
                                      CFTypeRef *clock)
{
    auto cmioClockType = new CFType;
    cmioClockType->type = CMIOStreamClockGetTypeID();
    cmioClockType->data = new _CMTimerData(clockName,
                                           sourceIdentifier,
                                           getTimeCallMinimumInterval,
                                           numberOfEventsForRateSmoothing,
                                           numberOfAveragesForRateSmoothing);
    cmioClockType->size = sizeof(_CMTimerData);
    cmioClockType->deleter = [] (void *data) {
        auto timerData = reinterpret_cast<_CMTimerDataRef>(data);
        CFRelease(timerData->clockName);

        delete timerData;
    };
    cmioClockType->ref = 1;
    *clock = cmioClockType;

    return noErr;
}

inline OSStatus CMIOStreamClockInvalidate(CFTypeRef clock)
{
    auto timerData = reinterpret_cast<_CMTimerDataRef>(clock->data);
    timerData->timer.stop();

    return noErr;
}

inline CMTime CMIOStreamClockConvertHostTimeToDeviceTime(UInt64 hostTime,
                                                         CFTypeRef clock)
{
    auto timerData = reinterpret_cast<_CMTimerDataRef>(clock->data);

    return CMTimeMake(std::round(hostTime
                                 * timerData->getTimeCallMinimumInterval.timescale
                                 / 1e9),
                      timerData->getTimeCallMinimumInterval.timescale);
}

inline OSStatus CMIOStreamClockPostTimingEvent(CMTime eventTime,
                                               UInt64 hostTime,
                                               Boolean resynchronize,
                                               CFTypeRef clock)
{
    auto timerData = reinterpret_cast<_CMTimerDataRef>(clock->data);

    if (CMTIME_IS_VALID(timerData->deviceTime) && !resynchronize) {
        auto hostDiff = (hostTime - timerData->hostTime) / 1e9;
        auto deviceDiff =
                CMTimeGetSeconds(CMTimeSubtract(eventTime, timerData->deviceTime));
        auto delay = deviceDiff - Float64(hostDiff);

        // Discard the event
        if (delay < 0.0)
            return -1;

        // Scheduled the event
        delay = std::max(delay, CMTimeGetSeconds(timerData->getTimeCallMinimumInterval));

        timerData->timer.singleShot(std::round(1000.0 * delay));
    } else {
        timerData->timer.singleShot(0);
    }

    timerData->hostTime = hostTime;
    timerData->deviceTime = eventTime;

    return noErr;
}

#endif // COREMEDIAIO_CMIOSTREAMCLOCK_H
