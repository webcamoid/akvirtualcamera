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

#ifndef MEDIASTREAM_H
#define MEDIASTREAM_H

#include <mfidl.h>

#include "attributes.h"
#include "mediaeventgenerator.h"
#include "VCamUtils/src/ipcbridge.h"
#include "PlatformUtils/src/cunknown.h"

namespace AkVCam
{
    class MediaStreamPrivate;
    class MediaSource;

    class MediaStream:
            public virtual IMFMediaStream,
            public virtual IMFAttributes
    {
        public:
            MediaStream(MediaSource *mediaSource,
                        IMFStreamDescriptor *streamDescriptor);
            ~MediaStream();

            void setBridge(IpcBridgePtr bridge);
            void frameReady(const VideoFrame &frame, bool isActive);
            void setPicture(const std::string &picture);
            void setControls(const std::map<std::string, int> &controls);
            bool horizontalFlip() const;
            void setHorizontalFlip(bool flip);
            bool verticalFlip() const;
            void setVerticalFlip(bool flip);
            HRESULT start(IMFMediaType *mediaType);
            HRESULT stop();
            HRESULT pause();

            BEGIN_COM_MAP_NO(MediaStream)
                COM_INTERFACE_ENTRY(IMFMediaEventGenerator)
                COM_INTERFACE_ENTRY(IMFMediaStream)
                COM_INTERFACE_ENTRY(IMFAttributes)
                COM_INTERFACE_ENTRY2(IUnknown, IMFMediaStream)
            END_COM_MAP_NU(MediaStream)

            DECLARE_ATTRIBUTES
            DECLARE_EVENT_GENERATOR

            // IMFMediaStream

            HRESULT STDMETHODCALLTYPE GetMediaSource(IMFMediaSource **ppMediaSource) override;
            HRESULT STDMETHODCALLTYPE GetStreamDescriptor(IMFStreamDescriptor **ppStreamDescriptor) override;
            HRESULT STDMETHODCALLTYPE RequestSample(IUnknown *pToken) override;

        private:
            MediaStreamPrivate *d;

        friend class MediaStreamPrivate;
    };
}

#endif // MEDIASTREAM_H
