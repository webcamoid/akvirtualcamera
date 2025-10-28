/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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

#ifndef MEDIAEVENTGENERATOR_H
#define MEDIAEVENTGENERATOR_H

#include <cinttypes>
#include <memory>
#include <mfapi.h>

#include "PlatformUtils/src/utils.h"
#include "MFUtils/src/utils.h"

#define DECLARE_EVENT_GENERATOR \
    private: \
        std::shared_ptr<IMFMediaEventQueue> m_eventQueue; \
        \
        std::shared_ptr<IMFMediaEventQueue> eventQueue() \
        { \
            if (!this->m_eventQueue) {\
                IMFMediaEventQueue *eventQueue = nullptr; \
                MFCreateEventQueue(&eventQueue); \
                \
                this->m_eventQueue = \
                    std::shared_ptr<IMFMediaEventQueue>(eventQueue, \
                                                        [] (IMFMediaEventQueue *eventQueue) { \
                        eventQueue->Shutdown(); \
                        eventQueue->Release(); \
                    }); \
            } \
            \
            return this->m_eventQueue; \
        } \
    \
    public: \
        HRESULT STDMETHODCALLTYPE GetEvent(DWORD dwFlags, \
                                           IMFMediaEvent **ppEvent) override \
        { \
            AkLogFunction(); \
            \
            return eventQueue()->GetEvent(dwFlags, ppEvent); \
        } \
        \
        HRESULT STDMETHODCALLTYPE BeginGetEvent(IMFAsyncCallback *pCallback, \
                                                IUnknown *punkState) override \
        { \
            AkLogFunction(); \
            \
            return eventQueue()->BeginGetEvent(pCallback, punkState); \
        } \
        \
        HRESULT STDMETHODCALLTYPE EndGetEvent(IMFAsyncResult *pResult, \
                                              IMFMediaEvent **ppEvent) override \
        { \
            AkLogFunction(); \
            \
            return eventQueue()->EndGetEvent(pResult, ppEvent); \
        } \
        \
        HRESULT STDMETHODCALLTYPE QueueEvent(MediaEventType mediaEventType, \
                                             REFGUID guidExtendedType, \
                                             HRESULT hrStatus, \
                                             const PROPVARIANT *pvValue) override \
        { \
            AkLogFunction(); \
            \
            return eventQueue()->QueueEventParamVar(mediaEventType, \
                                                    guidExtendedType, \
                                                    hrStatus, \
                                                    pvValue); \
        }

#endif // MEDIAEVENTGENERATOR_H
