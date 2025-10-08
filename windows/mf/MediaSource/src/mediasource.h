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

#include "mediaeventgenerator.h"

namespace AkVCam
{
    class MediaSourcePrivate;

    class MediaSource:
            public IMFMediaSource,
            public IMFGetService,
            public MediaEventGenerator
    {
        public:
            MediaSource(const GUID &clsid);
            ~MediaSource();

            std::string deviceId() const;
            bool directMode() const;

            DECLARE_IMFMEDIAEVENTGENERATOR_NQ

            // IUnknown
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                                     void **ppvObject) override;

            // IMFMediaSource
            HRESULT STDMETHODCALLTYPE GetCharacteristics(DWORD *pdwCharacteristics) override;
            HRESULT STDMETHODCALLTYPE CreatePresentationDescriptor(IMFPresentationDescriptor **presentationDescriptor) override;
            HRESULT STDMETHODCALLTYPE Start(IMFPresentationDescriptor *pPresentationDescriptor,
                                            const GUID *pguidTimeFormat,
                                            const PROPVARIANT *pvarStartPosition) override;
            HRESULT STDMETHODCALLTYPE Stop() override;
            HRESULT STDMETHODCALLTYPE Pause() override;
            HRESULT STDMETHODCALLTYPE Shutdown() override;

            // IMFGetService
            HRESULT STDMETHODCALLTYPE GetService(REFGUID service,
                                                 REFIID riid,
                                                 LPVOID *ppvObject) override;

        private:
            MediaSourcePrivate *d;
    };
}

#endif // MEDIASOURCE_H
