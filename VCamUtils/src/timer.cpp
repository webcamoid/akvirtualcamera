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

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "timer.h"

namespace AkVCam
{
    class TimerPrivate
    {
        public:
            Timer *self;
            std::thread m_thread;
            std::condition_variable m_cv;
            std::mutex m_mutex;
            int m_interval {0};
            bool m_running {false};
            bool m_singleShot {false};

            explicit TimerPrivate(Timer *self);
            void timerLoop();
    };
}

AkVCam::Timer::Timer()
{
    this->d = new TimerPrivate(this);
}

AkVCam::Timer::~Timer()
{
    AkLogFunction();

    this->stop();
    delete this->d;
}

int AkVCam::Timer::interval() const
{
    return this->d->m_interval;
}

void AkVCam::Timer::setInterval(int msec)
{
    this->d->m_interval = msec;
}

void AkVCam::Timer::start()
{
    AkLogFunction();

    this->stop();
    this->d->m_running = true;
    this->d->m_singleShot = false;
    this->d->m_thread = std::thread(&TimerPrivate::timerLoop, this->d);
}

void AkVCam::Timer::stop()
{
    AkLogFunction();

    {
        std::unique_lock<std::mutex> lock(this->d->m_mutex);

        if (!this->d->m_running) {
            AkLogDebug("Timer stopped");

            return;
        }

        this->d->m_running = false;
        this->d->m_cv.notify_all();
    }

    if (this->d->m_thread.joinable())
        this->d->m_thread.join();

    AkLogDebug("Timer stopped");
}

void AkVCam::Timer::singleShot()
{
    AkLogFunction();

    this->stop();
    this->d->m_running = true;
    this->d->m_singleShot = true;
    this->d->m_thread = std::thread(&TimerPrivate::timerLoop, this->d);
}

void AkVCam::Timer::singleShot(int msec)
{
    AkLogFunction();

    this->stop();
    this->d->m_running = true;
    this->d->m_singleShot = true;
    this->d->m_interval = msec;
    this->d->m_thread = std::thread(&TimerPrivate::timerLoop, this->d);
}

AkVCam::TimerPrivate::TimerPrivate(AkVCam::Timer *self):
    self(self),
    m_interval(0),
    m_running(false)
{

}

void AkVCam::TimerPrivate::timerLoop()
{
    while (true) {
        {
            std::unique_lock<std::mutex> lock(this->m_mutex);
            this->m_cv.wait_for(lock,
                                std::chrono::milliseconds(this->m_interval),
                                [this] { return !this->m_running; });

            if (!this->m_running)
                return;
        }

        AKVCAM_EMIT_NOARGS(this->self, Timeout)

        if (this->m_singleShot) {
            std::unique_lock<std::mutex> lock(this->m_mutex);
            this->m_running = false;

            return;
        }
    }
}
