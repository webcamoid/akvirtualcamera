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

#ifndef PIN_H
#define PIN_H

#include <string>
#include <vector>
#include <strmif.h>

#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframetypes.h"
#include "VCamUtils/src/ipcbridge.h"
#include "PlatformUtils/src/cunknown.h"

namespace AkVCam
{
    class PinPrivate;
    class BaseFilter;
    class VideoFrame;

    class Pin:
            public virtual IPin,
            public virtual IAMStreamConfig,
            public virtual IAMPushSource
    {
        public:
            Pin(BaseFilter *baseFilter=nullptr,
                const std::vector<AkVCam::VideoFormat> &formats={},
                const std::string &pinName={});
            virtual ~Pin();

            BaseFilter *baseFilter() const;
            HRESULT stop();
            HRESULT pause();
            HRESULT run(REFERENCE_TIME tStart);
            void frameReady(const VideoFrame &frame, bool isActive);
            void setPicture(const std::string &picture);
            void setControls(const std::map<std::string, int> &controls);
            bool horizontalFlip() const;
            void setHorizontalFlip(bool flip);
            bool verticalFlip() const;
            void setVerticalFlip(bool flip);

            BEGIN_COM_MAP_NO(Pin)
                COM_INTERFACE_ENTRY(IPin)
                COM_INTERFACE_ENTRY(IAMStreamConfig)
                COM_INTERFACE_ENTRY(IAMLatency)
                COM_INTERFACE_ENTRY(IAMPushSource)
                COM_INTERFACE_ENTRY2(IUnknown, IPin)
            END_COM_MAP_NU(Pin)

            // IAMLatency

            HRESULT WINAPI GetLatency(REFERENCE_TIME *prtLatency) override;

            // IAMPushSource

            HRESULT WINAPI GetPushSourceFlags(ULONG *pFlags) override;
            HRESULT WINAPI SetPushSourceFlags(ULONG Flags) override;
            HRESULT WINAPI SetStreamOffset(REFERENCE_TIME rtOffset) override;
            HRESULT WINAPI GetStreamOffset(REFERENCE_TIME *prtOffset) override;
            HRESULT WINAPI GetMaxStreamOffset(REFERENCE_TIME *prtMaxOffset) override;
            HRESULT WINAPI SetMaxStreamOffset(REFERENCE_TIME rtMaxOffset) override;

            // IAMStreamConfig

            HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt) override;
            HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **pmt) override;
            HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount,
                                                              int *piSize) override;
            HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex,
                                                    AM_MEDIA_TYPE **pmt,
                                                    BYTE *pSCC) override;

            // IPin

            HRESULT STDMETHODCALLTYPE Connect(IPin *pReceivePin,
                                              const AM_MEDIA_TYPE *pmt) override;
            HRESULT STDMETHODCALLTYPE ReceiveConnection(IPin *pConnector,
                                                        const AM_MEDIA_TYPE *pmt) override;
            HRESULT STDMETHODCALLTYPE Disconnect() override;
            HRESULT STDMETHODCALLTYPE ConnectedTo(IPin **pPin) override;
            HRESULT STDMETHODCALLTYPE ConnectionMediaType(AM_MEDIA_TYPE *pmt) override;
            HRESULT STDMETHODCALLTYPE QueryPinInfo(PIN_INFO *pInfo) override;
            HRESULT STDMETHODCALLTYPE QueryDirection(PIN_DIRECTION *pPinDir) override;
            HRESULT STDMETHODCALLTYPE QueryId(LPWSTR *Id) override;
            HRESULT STDMETHODCALLTYPE QueryAccept(const AM_MEDIA_TYPE *pmt) override;
            HRESULT STDMETHODCALLTYPE EnumMediaTypes(IEnumMediaTypes **ppEnum) override;
            HRESULT STDMETHODCALLTYPE QueryInternalConnections(IPin **apPin,
                                                               ULONG *nPin) override;
            HRESULT STDMETHODCALLTYPE EndOfStream() override;
            HRESULT STDMETHODCALLTYPE BeginFlush() override;
            HRESULT STDMETHODCALLTYPE EndFlush() override;
            HRESULT STDMETHODCALLTYPE NewSegment(REFERENCE_TIME tStart,
                                                 REFERENCE_TIME tStop,
                                                 double dRate) override;

        private:
            PinPrivate *d;

        friend class PinPrivate;
    };
}

#endif // PIN_H
