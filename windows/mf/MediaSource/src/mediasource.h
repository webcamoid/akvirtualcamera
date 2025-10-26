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

#ifndef MEDIASOURCE_H
#define MEDIASOURCE_H

#include <string>
#include <mfidl.h>

#include "attributes.h"
#include "mediaeventgenerator.h"
#include "PlatformUtils/src/cunknown.h"

namespace AkVCam
{
    class MediaSourcePrivate;

    class MediaSource:
            public virtual IMFMediaSource,
            public virtual IMFAttributes,
            public virtual IMFGetService,
            public virtual IAMVideoProcAmp
    {
        AKVCAM_SIGNAL(PropertyChanged, LONG Property, LONG lValue, LONG Flags)

        public:
            MediaSource(const GUID &clsid);
            ~MediaSource();

            std::string deviceId() const;
            bool directMode() const;

            BEGIN_COM_MAP_NOD(MediaSource)
                COM_INTERFACE_ENTRY(IMFMediaEventGenerator)
                COM_INTERFACE_ENTRY(IMFMediaSource)
                COM_INTERFACE_ENTRY(IMFAttributes)
                COM_INTERFACE_ENTRY(IMFGetService)
                COM_INTERFACE_ENTRY(IAMVideoProcAmp)
                COM_INTERFACE_ENTRY2(IUnknown, IMFMediaSource)
            END_COM_MAP_NU(MediaSource)

            DECLARE_ATTRIBUTES
            DECLARE_EVENT_GENERATOR

            // IMFGetService

            HRESULT STDMETHODCALLTYPE GetService(REFGUID service,
                                                 REFIID riid,
                                                 LPVOID *ppvObject) override;

            // IAMVideoProcAmp

            HRESULT STDMETHODCALLTYPE GetRange(LONG property,
                                               LONG *pMin,
                                               LONG *pMax,
                                               LONG *pSteppingDelta,
                                               LONG *pDefault,
                                               LONG *pCapsFlags) override;
            HRESULT STDMETHODCALLTYPE Set(LONG property,
                                          LONG lValue,
                                          LONG flags) override;
            HRESULT STDMETHODCALLTYPE Get(LONG property,
                                          LONG *lValue,
                                          LONG *flags) override;

            // IMFMediaSource

            HRESULT STDMETHODCALLTYPE GetCharacteristics(DWORD *pdwCharacteristics) override;
            HRESULT STDMETHODCALLTYPE CreatePresentationDescriptor(IMFPresentationDescriptor **presentationDescriptor) override;
            HRESULT STDMETHODCALLTYPE Start(IMFPresentationDescriptor *pPresentationDescriptor,
                                            const GUID *pguidTimeFormat,
                                            const PROPVARIANT *pvarStartPosition) override;
            HRESULT STDMETHODCALLTYPE Stop() override;
            HRESULT STDMETHODCALLTYPE Pause() override;
            HRESULT STDMETHODCALLTYPE Shutdown() override;

        private:
            MediaSourcePrivate *d;

        protected:
            bool isInterfaceDisabled(REFIID riid) const;
    };
}

#endif // MEDIASOURCE_H
