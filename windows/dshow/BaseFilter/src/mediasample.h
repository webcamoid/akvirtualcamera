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

#ifndef MEDIASAMPLE_H
#define MEDIASAMPLE_H

#include <strmif.h>

#include "PlatformUtils/src/cunknown.h"

namespace AkVCam
{
    class MediaSamplePrivate;

    class MediaSample: public CUnknown<IMediaSample2>
    {
        public:
            MediaSample(IMemAllocator *memAllocator,
                         LONG bufferSize,
                         LONG align,
                         LONG prefix);
            virtual ~MediaSample();

            void setMemAllocator(IMemAllocator *memAllocator);

            BEGIN_COM_MAP_NR(MediaSample)
                COM_INTERFACE_ENTRY(IMediaSample)
                COM_INTERFACE_ENTRY(IMediaSample2)
            END_COM_MAP(MediaSample)

            // IUnknown

            ULONG STDMETHODCALLTYPE Release() override;

            // IMediaSample

            HRESULT STDMETHODCALLTYPE GetPointer(BYTE **ppBuffer) override;
            LONG STDMETHODCALLTYPE GetSize() override;
            HRESULT STDMETHODCALLTYPE GetTime(REFERENCE_TIME *pTimeStart,
                                              REFERENCE_TIME *pTimeEnd) override;
            HRESULT STDMETHODCALLTYPE SetTime(REFERENCE_TIME *pTimeStart,
                                              REFERENCE_TIME *pTimeEnd) override;
            HRESULT STDMETHODCALLTYPE IsSyncPoint() override;
            HRESULT STDMETHODCALLTYPE SetSyncPoint(BOOL bIsSyncPoint) override;
            HRESULT STDMETHODCALLTYPE IsPreroll() override;
            HRESULT STDMETHODCALLTYPE SetPreroll(BOOL bIsPreroll) override;
            LONG STDMETHODCALLTYPE GetActualDataLength() override;
            HRESULT STDMETHODCALLTYPE SetActualDataLength(LONG lLen) override;
            HRESULT STDMETHODCALLTYPE GetMediaType(AM_MEDIA_TYPE **ppMediaType) override;
            HRESULT STDMETHODCALLTYPE SetMediaType(AM_MEDIA_TYPE *pMediaType) override;
            HRESULT STDMETHODCALLTYPE IsDiscontinuity() override;
            HRESULT STDMETHODCALLTYPE SetDiscontinuity(BOOL bDiscontinuity) override;
            HRESULT STDMETHODCALLTYPE GetMediaTime(LONGLONG *pTimeStart,
                                                   LONGLONG *pTimeEnd) override;
            HRESULT STDMETHODCALLTYPE SetMediaTime(LONGLONG *pTimeStart,
                                                   LONGLONG *pTimeEnd) override;

            // IMediaSample2

            HRESULT STDMETHODCALLTYPE GetProperties(DWORD cbProperties,
                                                    BYTE *pbProperties) override;
            HRESULT STDMETHODCALLTYPE SetProperties(DWORD cbProperties,
                                                    const BYTE *pbProperties) override;

        private:
            MediaSamplePrivate *d;
    };
}

#endif // MEDIASAMPLE_H
