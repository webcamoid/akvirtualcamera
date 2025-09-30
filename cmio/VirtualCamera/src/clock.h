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

#ifndef CLOCK_H
#define CLOCK_H

#include <string>
#include <memory>
#include <CoreMedia/CMTime.h>

namespace AkVCam
{
    class Clock;
    class ClockPrivate;
    typedef std::shared_ptr<Clock> ClockPtr;

    class Clock
    {
        public:
            Clock(const std::string& name,
                  const CMTime getTimeCallMinimumInterval,
                  UInt32 numberOfEventsForRateSmoothing,
                  UInt32 numberOfAveragesForRateSmoothing,
                  void *parent=nullptr);
            ~Clock();

            CFTypeRef ref() const;
            OSStatus postTimingEvent(CMTime eventTime,
                                     UInt64 hostTime,
                                     Boolean resynchronize);

        private:
            ClockPrivate *d;
    };
}

#endif // CLOCK_H
