/* Webcamoid, camera capture application.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
 *
 * Webcamoid is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Webcamoid is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Webcamoid. If not, see <http://www.gnu.org/licenses/>.
 *
 * Web-Site: http://webcamoid.github.io/
 */

#include <mfapi.h>

#include "mediaeventgenerator.h"
#include "PlatformUtils/src/utils.h"

namespace AkVCam
{
    class MediaEventGeneratorPrivate
    {
        public:
            IMFMediaEventQueue *m_pEventQueue {nullptr};
    };
}

AkVCam::MediaEventGenerator::MediaEventGenerator():
    CUnknown(this, IID_IMFMediaEventGenerator)
{
    this->d = new MediaEventGeneratorPrivate;

    MFCreateEventQueue(&this->d->m_pEventQueue);
}

AkVCam::MediaEventGenerator::~MediaEventGenerator()
{
    if (this->d->m_pEventQueue) {
        this->d->m_pEventQueue->Shutdown();
        this->d->m_pEventQueue->Release();
    }

    delete this->d;
}

IMFMediaEventQueue *AkVCam::MediaEventGenerator::eventQueue() const
{
    return this->d->m_pEventQueue;
}

HRESULT AkVCam::MediaEventGenerator::GetEvent(DWORD dwFlags,
                                              IMFMediaEvent **ppEvent)
{
    AkLogFunction();

    return this->d->m_pEventQueue->GetEvent(dwFlags, ppEvent);
}

HRESULT AkVCam::MediaEventGenerator::BeginGetEvent(IMFAsyncCallback *pCallback,
                                                   IUnknown *pUnkState)
{
    AkLogFunction();

    return this->d->m_pEventQueue->BeginGetEvent(pCallback, pUnkState);
}

HRESULT AkVCam::MediaEventGenerator::EndGetEvent(IMFAsyncResult *pResult,
                                                 IMFMediaEvent **ppEvent)
{
    AkLogFunction();

    return this->d->m_pEventQueue->EndGetEvent(pResult, ppEvent);
}

HRESULT AkVCam::MediaEventGenerator::QueueEvent(MediaEventType mediaEventType,
                                                REFGUID guidExtendedType,
                                                HRESULT hrStatus,
                                                const PROPVARIANT *pvValue)
{
    AkLogFunction();

    return this->d->m_pEventQueue->QueueEventParamVar(mediaEventType,
                                                      guidExtendedType,
                                                      hrStatus,
                                                      pvValue);
}
