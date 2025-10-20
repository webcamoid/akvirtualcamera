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

#ifndef MEDIAFILTER_H
#define MEDIAFILTER_H

#include <strmif.h>

#include "persistpropertybag.h"

namespace AkVCam
{
    class MediaFilterPrivate;
    typedef HRESULT (*StateChangedCallbackT)(void *userData,
                                             FILTER_STATE state);

    class MediaFilter:
            public IMediaFilter,
            public PersistPropertyBag
    {
        public:
            MediaFilter(REFIID classCLSID, IBaseFilter *baseFilter);
            virtual ~MediaFilter();

            void connectStateChanged(void *userData,
                                     StateChangedCallbackT callback);
            void disconnectStateChanged(void *userData,
                                        StateChangedCallbackT callback);

            DECLARE_IPERSISTPROPERTYBAG(IID_IMediaFilter)

            // IMediaFilter
            HRESULT STDMETHODCALLTYPE Stop() override;
            HRESULT STDMETHODCALLTYPE Pause() override;
            HRESULT STDMETHODCALLTYPE Run(REFERENCE_TIME tStart) override;
            HRESULT STDMETHODCALLTYPE GetState(DWORD dwMilliSecsTimeout,
                                               FILTER_STATE *State) override;
            HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock *pClock) override;
            HRESULT STDMETHODCALLTYPE GetSyncSource(IReferenceClock **pClock) override;

        private:
            MediaFilterPrivate *d;

        protected:
            virtual void stateChanged(FILTER_STATE state);
    };
}

#define DECLARE_IMEDIAFILTER_NQ \
    DECLARE_IPERSISTPROPERTYBAG_NQ \
    \
    void connectStateChanged(void *userData, \
                             StateChangedCallbackT callback) override \
    { \
        MediaFilter::connectStateChanged(userData, callback); \
    } \
    \
    void disconnectStateChanged(void *userData, \
                                StateChangedCallbackT callback) override \
    { \
        MediaFilter::disconnectStateChanged(userData, callback); \
    } \
    \
    HRESULT STDMETHODCALLTYPE Stop() override \
    { \
        return MediaFilter::Stop(); \
    } \
    \
    HRESULT STDMETHODCALLTYPE Pause() override \
    { \
        return MediaFilter::Pause(); \
    } \
    \
    HRESULT STDMETHODCALLTYPE Run(REFERENCE_TIME tStart) override \
    { \
        return MediaFilter::Run(tStart); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetState(DWORD dwMilliSecsTimeout, \
                                       FILTER_STATE *State) override \
    { \
        return MediaFilter::GetState(dwMilliSecsTimeout, State); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock *pClock) override \
    { \
        return MediaFilter::SetSyncSource(pClock); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetSyncSource(IReferenceClock **pClock) override \
    { \
        return MediaFilter::GetSyncSource(pClock); \
    }

#define DECLARE_IMEDIAFILTER(interfaceIid) \
    DECLARE_IPERSISTPROPERTYBAG_Q(interfaceIid) \
    DECLARE_IMEDIAFILTER_NQ

#endif // MEDIAFILTER_H
