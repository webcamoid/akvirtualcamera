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
            public MediaEventGenerator
    {
        public:
            MediaSource(const GUID &clsid);
            ~MediaSource();

            std::string deviceId();

            DECLARE_IMFMEDIAEVENTGENERATOR_NQ

            // IUnknown
            HRESULT QueryInterface(REFIID riid, void **ppvObject) override;

            // IMFMediaSource
            HRESULT GetCharacteristics(DWORD *pdwCharacteristics) override;
            HRESULT CreatePresentationDescriptor(IMFPresentationDescriptor **presentationDescriptor) override;
            HRESULT Start(IMFPresentationDescriptor *pPresentationDescriptor,
                          const GUID *pguidTimeFormat,
                          const PROPVARIANT *pvarStartPosition) override;
            HRESULT Stop() override;
            HRESULT Pause() override;
            HRESULT Shutdown() override;

        private:
            MediaSourcePrivate *d;
    };
}

#endif // MEDIASOURCE_H
