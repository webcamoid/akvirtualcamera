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

#ifndef COREMEDIA_CMTIME_H
#define COREMEDIA_CMTIME_H

#include "../CoreFoundation/CFType.h"
#include "VCamUtils/src/utils.h"

using CMTimeValue = int64_t;
using CMTimeScale = int32_t;

enum CMTimeFlags
{
    kCMTimeFlags_Valid = 0x1,
    kCMTimeFlags_HasBeenRounded = 0x2,
    kCMTimeFlags_PositiveInfinity = 0x4,
    kCMTimeFlags_NegativeInfinity = 0x8,
    kCMTimeFlags_Indefinite = 0x10,
    kCMTimeFlags_ImpliedValueFlagsMask = kCMTimeFlags_PositiveInfinity
                                       | kCMTimeFlags_NegativeInfinity
                                       | kCMTimeFlags_Indefinite
};

using CMTimeEpoch = int64_t;

struct CMTime
{
    CMTimeValue value = 0;
    CMTimeScale timescale = 0;
    CMTimeFlags flags = CMTimeFlags(0);
    CMTimeEpoch epoch = 0;

    CMTime(CMTimeValue value=0,
           CMTimeScale timescale=0,
           CMTimeFlags flags=CMTimeFlags(0),
           CMTimeEpoch epoch=0):
        value(value),
        timescale(timescale),
        flags(flags),
        epoch(flags)
    {

    }
};

#define CMTIME_IS_VALID(time) ((time.flags & kCMTimeFlags_Valid) != 0)
#define CMTIME_IS_INVALID(time) ((time.flags & kCMTimeFlags_Valid) == 0)

inline CMTime CMTimeMake(int64_t value, int32_t timescale)
{
    return {value, timescale, kCMTimeFlags_Valid, 0};
}

inline Float64 CMTimeGetSeconds(CMTime time)
{
    return Float64(time.value) / time.timescale;
}

inline int32_t CMTimeCompare(CMTime time1, CMTime time2)
{
    return int32_t(AkVCam::bound<CMTimeValue>(-1, time1.value * time2.timescale - time2.value * time1.timescale, 1));
}

inline CMTime CMTimeAdd(CMTime time1, CMTime time2)
{
    return {(time1.value * time2.timescale + time2.value * time1.timescale)
            / AkVCam::gcd(time1.timescale, time2.timescale),
            AkVCam::lcm(time1.timescale, time2.timescale),
            kCMTimeFlags_Valid,
            0};
}

inline CMTime CMTimeSubtract(CMTime time1, CMTime time2)
{
    return {(time1.value * time2.timescale - time2.value * time1.timescale)
            / AkVCam::gcd(time1.timescale, time2.timescale),
            AkVCam::lcm(time1.timescale, time2.timescale),
            kCMTimeFlags_Valid,
            0};
}

#endif // COREMEDIA_CMTIME_H
