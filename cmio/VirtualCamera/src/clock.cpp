/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

#include <CoreMediaIO/CMIOHardwareStream.h>

#include "clock.h"

namespace AkVCam
{
    class ClockPrivate
    {
        public:
            void *m_parent {nullptr};
            CFTypeRef m_clock {nullptr};
    };
}

AkVCam::Clock::Clock(const std::string& name,
                     const CMTime getTimeCallMinimumInterval,
                     UInt32 numberOfEventsForRateSmoothing,
                     UInt32 numberOfAveragesForRateSmoothing,
                     void *parent)
{
    this->d = new ClockPrivate;
    this->d->m_parent = parent;
    auto nameRef =
            CFStringCreateWithCString(kCFAllocatorDefault,
                                      name.c_str(),
                                      kCFStringEncodingUTF8);

    auto status =
            CMIOStreamClockCreate(kCFAllocatorDefault,
                                  nameRef,
                                  this->d->m_parent,
                                  getTimeCallMinimumInterval,
                                  numberOfEventsForRateSmoothing,
                                  numberOfAveragesForRateSmoothing,
                                  &this->d->m_clock);

    if (status != noErr)
        this->d->m_clock = nullptr;

    CFRelease(nameRef);
}

AkVCam::Clock::~Clock()
{
    if (this->d->m_clock) {
        CMIOStreamClockInvalidate(this->d->m_clock);
        CFRelease(this->d->m_clock);
    }

    delete this->d;
}

CFTypeRef AkVCam::Clock::ref() const
{
    return this->d->m_clock;
}

OSStatus AkVCam::Clock::postTimingEvent(CMTime eventTime,
                                        UInt64 hostTime,
                                        Boolean resynchronize)
{
    return CMIOStreamClockPostTimingEvent(eventTime,
                                          hostTime,
                                          resynchronize,
                                          this->d->m_clock);
}
