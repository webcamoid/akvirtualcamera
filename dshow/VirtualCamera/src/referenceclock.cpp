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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>

#include "referenceclock.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class AdviseCookiePrivate
    {
        public:
            IReferenceClock *m_clock;
            std::thread m_thread;
            std::mutex m_mutex;
            std::condition_variable_any m_timeout;
            std::atomic<bool> m_run {false};

            explicit AdviseCookiePrivate(IReferenceClock *clock);
            void adviseTime(REFERENCE_TIME baseTime,
                            REFERENCE_TIME streamTime,
                            HEVENT hEvent);
            void adviseTimeTh(REFERENCE_TIME baseTime,
                              REFERENCE_TIME streamTime,
                              HEVENT hEvent);
            void advisePeriodic(REFERENCE_TIME startTime,
                                REFERENCE_TIME periodTime,
                                HSEMAPHORE hSemaphore);
            void advisePeriodicTh(REFERENCE_TIME startTime,
                                  REFERENCE_TIME periodTime,
                                  HSEMAPHORE hSemaphore);
            void unadvise();
    };

    class ReferenceClockPrivate
    {
        public:
            ReferenceClock *self;
            std::vector<DWORD_PTR> m_cookies;
            REFERENCE_TIME m_lastTime {0};

            explicit ReferenceClockPrivate(ReferenceClock *self);
            void cleanup();
    };
}

AkVCam::ReferenceClock::ReferenceClock():
    CUnknown(this, IID_IReferenceClock)
{
    this->d = new ReferenceClockPrivate(this);
}

AkVCam::ReferenceClock::~ReferenceClock()
{
    for (auto &cookie: this->d->m_cookies) {
        auto adviseCookie = reinterpret_cast<AdviseCookiePrivate *>(cookie);
        adviseCookie->unadvise();
        delete adviseCookie;
    }

    delete this->d;
}

HRESULT AkVCam::ReferenceClock::GetTime(REFERENCE_TIME *pTime)
{
    AkLogFunction();

    if (!pTime)
        return E_POINTER;

    *pTime = REFERENCE_TIME(TIME_BASE * timeGetTime() / 1e3);

    if (*pTime <= this->d->m_lastTime)
        return S_FALSE;

    this->d->m_lastTime = *pTime;

    return S_OK;
}

HRESULT AkVCam::ReferenceClock::AdviseTime(REFERENCE_TIME baseTime,
                                           REFERENCE_TIME streamTime,
                                           HEVENT hEvent,
                                           DWORD_PTR *pdwAdviseCookie)
{
    AkLogFunction();
    this->d->cleanup();

    if (!pdwAdviseCookie)
        return E_POINTER;

    *pdwAdviseCookie = 0;

    const REFERENCE_TIME time = baseTime + streamTime;

    if (time <= 0 || time == (std::numeric_limits<LONGLONG>::max)())
        return E_INVALIDARG;

    auto adviseCookie = new AdviseCookiePrivate(this);
    *pdwAdviseCookie = DWORD_PTR(adviseCookie);
    this->d->m_cookies.push_back(*pdwAdviseCookie);
    adviseCookie->adviseTime(baseTime, streamTime, hEvent);

    return S_OK;
}

HRESULT AkVCam::ReferenceClock::AdvisePeriodic(REFERENCE_TIME startTime,
                                               REFERENCE_TIME periodTime,
                                               HSEMAPHORE hSemaphore,
                                               DWORD_PTR *pdwAdviseCookie)
{
    AkLogFunction();
    this->d->cleanup();

    if (!pdwAdviseCookie)
        return E_POINTER;

    *pdwAdviseCookie = 0;

    if (startTime <= 0
        || periodTime <= 0
        || startTime == (std::numeric_limits<LONGLONG>::max)())
        return E_INVALIDARG;

    auto adviseCookie = new AdviseCookiePrivate(this);
    adviseCookie->advisePeriodic(startTime, periodTime, hSemaphore);
    *pdwAdviseCookie = DWORD_PTR(adviseCookie);
    this->d->m_cookies.push_back(*pdwAdviseCookie);

    return S_OK;
}

HRESULT AkVCam::ReferenceClock::Unadvise(DWORD_PTR dwAdviseCookie)
{
    AkLogFunction();

    auto it = std::find(this->d->m_cookies.begin(),
                        this->d->m_cookies.end(),
                        dwAdviseCookie);

    if (it == this->d->m_cookies.end())
        return S_FALSE;

    auto adviseCookie = reinterpret_cast<AdviseCookiePrivate *>(*it);
    adviseCookie->unadvise();
    delete adviseCookie;
    this->d->m_cookies.erase(it);
    this->d->cleanup();

    return S_OK;
}

AkVCam::AdviseCookiePrivate::AdviseCookiePrivate(IReferenceClock *clock):
    m_clock(clock)
{
}

void AkVCam::AdviseCookiePrivate::adviseTime(REFERENCE_TIME baseTime,
                                             REFERENCE_TIME streamTime,
                                             HEVENT hEvent)
{
    AkLogFunction();

    this->m_run = true;
    this->m_thread = std::thread(&AdviseCookiePrivate::adviseTimeTh,
                                 this,
                                 baseTime,
                                 streamTime,
                                 hEvent);
    AkLogInfo() << "Launching thread " << this->m_thread.get_id() << std::endl;
}

void AkVCam::AdviseCookiePrivate::adviseTimeTh(REFERENCE_TIME baseTime,
                                               REFERENCE_TIME streamTime,
                                               HEVENT hEvent)
{
    AkLogFunction();

    REFERENCE_TIME clockTime;
    this->m_clock->GetTime(&clockTime);

    auto startSleep =
            REFERENCE_TIME(1e3
                           * (baseTime + streamTime - clockTime)
                           / TIME_BASE);

    if (startSleep > 0) {
        std::chrono::milliseconds start(startSleep);
        this->m_mutex.lock();
        this->m_timeout.wait_for(this->m_mutex, start);
        this->m_mutex.unlock();
    }

    if (this->m_run)
        SetEvent(HANDLE(hEvent));

    this->m_run = false;
    AkLogInfo() << "Thread "
                << std::this_thread::get_id()
                << " finnished"
                << std::endl;
}

void AkVCam::AdviseCookiePrivate::advisePeriodic(REFERENCE_TIME startTime,
                                                 REFERENCE_TIME periodTime,
                                                 HSEMAPHORE hSemaphore)
{
    AkLogFunction();

    this->m_run = true;
    this->m_thread = std::thread(&AdviseCookiePrivate::advisePeriodicTh,
                                 this,
                                 startTime,
                                 periodTime,
                                 hSemaphore);
    AkLogInfo() << "Launching thread " << this->m_thread.get_id() << std::endl;
}

void AkVCam::AdviseCookiePrivate::advisePeriodicTh(REFERENCE_TIME startTime,
                                                   REFERENCE_TIME periodTime,
                                                   HSEMAPHORE hSemaphore)
{
    AkLogFunction();

    REFERENCE_TIME clockTime;
    this->m_clock->GetTime(&clockTime);

    auto startSleep =
            REFERENCE_TIME(1e3
                           * (startTime - clockTime)
                           / TIME_BASE);

    if (startSleep > 0) {
        std::chrono::milliseconds start(startSleep);
        this->m_mutex.lock();
        this->m_timeout.wait_for(this->m_mutex, start);
        this->m_mutex.unlock();
    }

    auto periodSleep = REFERENCE_TIME(1e3 * periodTime / TIME_BASE);
    std::chrono::milliseconds period(periodSleep);

    while (this->m_run) {
        ReleaseSemaphore(HANDLE(hSemaphore), 1, nullptr);
        this->m_mutex.lock();
        this->m_timeout.wait_for(this->m_mutex, period);
        this->m_mutex.unlock();
    }

    AkLogInfo() << "Thread "
                << std::this_thread::get_id()
                << " finnished"
                << std::endl;
}

void AkVCam::AdviseCookiePrivate::unadvise()
{
    AkLogFunction();

    this->m_run = false;
    this->m_mutex.lock();
    this->m_timeout.notify_one();
    this->m_mutex.unlock();
    this->m_thread.join();
}

AkVCam::ReferenceClockPrivate::ReferenceClockPrivate(ReferenceClock *self):
    self(self)
{
}

void AkVCam::ReferenceClockPrivate::cleanup()
{
    std::vector<DWORD_PTR> cookies;

    for (auto &cookie: this->m_cookies) {
        auto adviseCookie = reinterpret_cast<AdviseCookiePrivate *>(cookie);

        if (!adviseCookie->m_run)
            cookies.push_back(cookie);
    }

    for (auto &cookie: cookies)
        this->self->Unadvise(cookie);
}
