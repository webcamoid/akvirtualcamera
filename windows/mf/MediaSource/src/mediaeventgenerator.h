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

#include <mfidl.h>

#include "PlatformUtils/src/cunknown.h"

namespace AkVCam
{
    class MediaEventGeneratorPrivate;

    class MediaEventGenerator:
        public IMFMediaEventGenerator,
        public CUnknown
    {
        public:
            MediaEventGenerator();
            ~MediaEventGenerator();

            IMFMediaEventQueue *eventQueue() const;

            DECLARE_IUNKNOWN(IID_IMFMediaEventGenerator)

            // IMFMediaEventGenerator
            HRESULT STDMETHODCALLTYPE GetEvent(DWORD dwFlags,
                                               IMFMediaEvent **ppEvent) override;
            HRESULT STDMETHODCALLTYPE BeginGetEvent(IMFAsyncCallback *pCallback,
                                                    IUnknown *punkState) override;
            HRESULT STDMETHODCALLTYPE EndGetEvent(IMFAsyncResult *pResult,
                                                  IMFMediaEvent **ppEvent) override;
            HRESULT STDMETHODCALLTYPE QueueEvent(MediaEventType mediaEventType,
                                                 REFGUID guidExtendedType,
                                                 HRESULT hrStatus,
                                                 const PROPVARIANT *pvValue) override;

        private:
            MediaEventGeneratorPrivate *d;
    };
}

#define DECLARE_IMFMEDIAEVENTGENERATOR_NQ \
    DECLARE_IUNKNOWN_NQ \
    \
    HRESULT STDMETHODCALLTYPE GetEvent(DWORD dwFlags, IMFMediaEvent **ppEvent) \
    { \
        return MediaEventGenerator::GetEvent(dwFlags, ppEvent); \
    } \
    \
    HRESULT STDMETHODCALLTYPE BeginGetEvent(IMFAsyncCallback *pCallback, \
                                            IUnknown *punkState) \
    { \
        return MediaEventGenerator::BeginGetEvent(pCallback, punkState); \
    } \
    \
    HRESULT STDMETHODCALLTYPE EndGetEvent(IMFAsyncResult *pResult, \
                                          IMFMediaEvent **ppEvent) \
    { \
        return MediaEventGenerator::EndGetEvent(pResult, ppEvent); \
    } \
    \
    HRESULT STDMETHODCALLTYPE QueueEvent(MediaEventType mediaEventType, \
                                         REFGUID guidExtendedType, \
                                         HRESULT hrStatus, \
                                         const PROPVARIANT *pvValue) \
    { \
        return MediaEventGenerator::QueueEvent(mediaEventType, \
                                               guidExtendedType, \
                                               hrStatus, \
                                               pvValue); \
    }

#define DECLARE_IMFMEDIAEVENTGENERATOR(interfaceIid) \
    DECLARE_IUNKNOWN_Q(interfaceIid) \
    DECLARE_IMFMEDIAEVENTGENERATOR_NQ

#endif // MEDIAEVENTGENERATOR_H
