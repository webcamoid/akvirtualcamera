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

#include <vector>
#include <dshow.h>

#include "mediafilter.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    typedef std::pair<void *, StateChangedCallbackT> StateChangedCallback;

    class MediaFilterPrivate
    {
        public:
            IBaseFilter *m_baseFilter {nullptr};
            IReferenceClock *m_clock {nullptr};
            std::vector<StateChangedCallback> m_stateChanged;
            FILTER_STATE m_state {State_Stopped};
            REFERENCE_TIME m_start {0};
    };
}

AkVCam::MediaFilter::MediaFilter(const IID &classCLSID,
                                 IBaseFilter *baseFilter):
    PersistPropertyBag(classCLSID)
{
    this->setParent(this, &IID_IMediaFilter);
    this->d = new MediaFilterPrivate;
    this->d->m_baseFilter = baseFilter;
}

AkVCam::MediaFilter::~MediaFilter()
{
    if (this->d->m_clock)
        this->d->m_clock->Release();

    delete this->d;
}

void AkVCam::MediaFilter::connectStateChanged(void *userData,
                                              StateChangedCallbackT callback)
{
    AkLogFunction();
    this->d->m_stateChanged.push_back({userData, callback});
}

void AkVCam::MediaFilter::disconnectStateChanged(void *userData,
                                                 StateChangedCallbackT callback)
{
    AkLogFunction();

    for (auto it = this->d->m_stateChanged.begin();
         it != this->d->m_stateChanged.end();
         it++) {
        if (it->first == userData
            && it->second == callback) {
            this->d->m_stateChanged.erase(it);

            break;
        }
    }
}

HRESULT AkVCam::MediaFilter::Stop()
{
    AkLogFunction();
    this->d->m_state = State_Stopped;
    HRESULT result = S_OK;

    for (auto &callback: this->d->m_stateChanged) {
        auto r = callback.second(callback.first, this->d->m_state);

        if (r != S_OK)
            result = r;
    }

    this->stateChanged(this->d->m_state);

    return result;
}

HRESULT AkVCam::MediaFilter::Pause()
{
    AkLogFunction();
    this->d->m_state = State_Paused;
    HRESULT result = S_OK;

    for (auto &callback: this->d->m_stateChanged) {
        auto r = callback.second(callback.first, this->d->m_state);

        if (r != S_OK)
            result = r;
    }

    this->stateChanged(this->d->m_state);

    return result;
}

HRESULT AkVCam::MediaFilter::Run(REFERENCE_TIME tStart)
{
    AkLogFunction();
    this->d->m_start = tStart;
    this->d->m_state = State_Running;
    HRESULT result = S_OK;

    for (auto &callback: this->d->m_stateChanged) {
        auto r = callback.second(callback.first, this->d->m_state);

        if (r != S_OK)
            result = r;
    }

    this->stateChanged(this->d->m_state);

    return result;
}

HRESULT AkVCam::MediaFilter::GetState(DWORD dwMilliSecsTimeout,
                                      FILTER_STATE *State)
{
    UNUSED(dwMilliSecsTimeout);
    AkLogFunction();

    if (!State)
        return E_POINTER;

    *State = this->d->m_state;
    AkLogDebug("State: %d", *State);

    return S_OK;
}

HRESULT AkVCam::MediaFilter::SetSyncSource(IReferenceClock *pClock)
{
    AkLogFunction();

    if (this->d->m_clock)
        this->d->m_clock->Release();

    this->d->m_clock = pClock;

    if (this->d->m_clock)
        this->d->m_clock->AddRef();

    return S_OK;
}

HRESULT AkVCam::MediaFilter::GetSyncSource(IReferenceClock **pClock)
{
    AkLogFunction();

    if (!pClock)
        return E_POINTER;

    *pClock = this->d->m_clock;

    if (*pClock)
        (*pClock)->AddRef();

    return S_OK;
}

void AkVCam::MediaFilter::stateChanged(FILTER_STATE state)
{
    UNUSED(state);
}
