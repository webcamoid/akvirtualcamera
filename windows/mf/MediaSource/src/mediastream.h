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

#ifndef MEDIASTREAM_H
#define MEDIASTREAM_H

#include <mfidl.h>

#include "mediaeventgenerator.h"
#include "VCamUtils/src/ipcbridge.h"

namespace AkVCam
{
    class MediaStreamPrivate;
    class MediaSource;

    class MediaStream:
            public IMFMediaStream,
            public MediaEventGenerator
    {
        public:
            MediaStream(MediaSource *mediaSource,
                        IMFStreamDescriptor *streamDescriptor);
            ~MediaStream();

            void setBridge(IpcBridgePtr bridge);
            void frameReady(const VideoFrame &frame, bool isActive);
            void setPicture(const std::string &picture);
            void setControls(const std::map<std::string, int> &controls);
            HRESULT start(IMFMediaType *mediaType);
            HRESULT stop();
            HRESULT pause();

            DECLARE_IMFMEDIAEVENTGENERATOR_NQ

            // IUnknown
            HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid,
                                                     void **ppvObject) override;

            // IMFMediaStream
            HRESULT STDMETHODCALLTYPE GetMediaSource(IMFMediaSource **ppMediaSource) override;
            HRESULT STDMETHODCALLTYPE GetStreamDescriptor(IMFStreamDescriptor **ppStreamDescriptor) override;
            HRESULT STDMETHODCALLTYPE RequestSample(IUnknown *pToken) override;

        private:
            MediaStreamPrivate *d;
    };
}

#endif // MEDIASTREAM_H
