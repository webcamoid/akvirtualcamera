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
#include <ks.h>
#include <ksproxy.h>

#include "attributes.h"
#include "mfvcam.h"
#include "mediaeventgenerator.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class MediaSourcePrivate;

    class MediaSource:
            public virtual IMFMediaSrcEx,
            public virtual IMFAttributes,
            public virtual IMFGetService,
            public virtual IAMVideoProcAmp,
            public virtual IKsControl
    {
        AKVCAM_SIGNAL(PropertyChanged, LONG Property, LONG lValue, LONG Flags)

        public:
            MediaSource(const GUID &clsid);
            ~MediaSource();

            std::string deviceId() const;
            bool directMode() const;

            DECLARE_ATTRIBUTES
            DECLARE_EVENT_GENERATOR

            // IUnknown

            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                                     void **ppv) override;
            ULONG STDMETHODCALLTYPE AddRef() override;
            ULONG STDMETHODCALLTYPE Release() override;

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

            // IKsControl

            HRESULT STDMETHODCALLTYPE KsProperty(PKSPROPERTY Property,
                                                 ULONG PropertyLength,
                                                 LPVOID PropertyData,
                                                 ULONG DataLength,
                                                 ULONG *BytesReturned) override;
            HRESULT STDMETHODCALLTYPE KsMethod(PKSMETHOD Method,
                                               ULONG MethodLength,
                                               LPVOID MethodData,
                                               ULONG DataLength,
                                               ULONG *BytesReturned) override;
            HRESULT STDMETHODCALLTYPE KsEvent(PKSEVENT Event,
                                              ULONG EventLength,
                                              LPVOID EventData,
                                              ULONG DataLength,
                                              ULONG *BytesReturned) override;

            // IMFMediaSource

            HRESULT STDMETHODCALLTYPE GetCharacteristics(DWORD *pdwCharacteristics) override;
            HRESULT STDMETHODCALLTYPE CreatePresentationDescriptor(IMFPresentationDescriptor **presentationDescriptor) override;
            HRESULT STDMETHODCALLTYPE Start(IMFPresentationDescriptor *pPresentationDescriptor,
                                            const GUID *pguidTimeFormat,
                                            const PROPVARIANT *pvarStartPosition) override;
            HRESULT STDMETHODCALLTYPE Stop() override;
            HRESULT STDMETHODCALLTYPE Pause() override;
            HRESULT STDMETHODCALLTYPE Shutdown() override;

            // IMFMediaSourceEx

            HRESULT STDMETHODCALLTYPE GetSourceAttributes(IMFAttributes **ppAttributes) override;
            HRESULT STDMETHODCALLTYPE GetStreamAttributes(DWORD dwStreamIdentifier,
                                                          IMFAttributes **ppAttributes) override;
            HRESULT STDMETHODCALLTYPE SetD3DManager(IUnknown *pManager) override;

        private:
            MediaSourcePrivate *d;
   };
}

#endif // MEDIASOURCE_H
