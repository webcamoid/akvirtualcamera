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

#ifndef AKVCAMUTILS_TIMER_H
#define AKVCAMUTILS_TIMER_H

#include "utils.h"

namespace AkVCam
{
    class TimerPrivate;

    class Timer
    {
        AKVCAM_SIGNAL_NOARGS(Timeout)

        public:
            Timer();
            Timer(const Timer &other) = delete;
            ~Timer();

            int interval() const;
            void setInterval(int msec);
            void start();
            void stop();
            void singleShot();
            void singleShot(int msec);

        private:
            TimerPrivate *d;
            friend class TimerPrivate;
    };
}

#endif // AKVCAMUTILS_TIMER_H
