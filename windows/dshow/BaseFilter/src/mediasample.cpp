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

#include <atomic>
#include <dshow.h>

#include "mediasample.h"
#include "PlatformUtils/src/cunknown.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class MediaSamplePrivate
    {
        public:
            std::atomic<uint64_t> m_refCount {1};
            IMemAllocator *m_memAllocator {nullptr};
            BYTE *m_buffer {nullptr};
            LONG m_bufferSize {0};
            LONG m_dataLength {0};
            LONG m_prefix {0};
            AM_MEDIA_TYPE *m_mediaType {nullptr};
            REFERENCE_TIME m_sampleTimeStart {-1};
            REFERENCE_TIME m_sampleTimeEnd {-1};
            REFERENCE_TIME m_mediaTimeStart {-1};
            REFERENCE_TIME m_mediaTimeEnd {-1};
            BOOL m_syncPoint {S_FALSE};
            BOOL m_preroll {S_FALSE};
            BOOL m_discontinuity {S_FALSE};
            BOOL m_mediaTypeChanged {S_FALSE};
            DWORD m_data {0};
            DWORD m_typeSpecificFlags {0};
            DWORD m_sampleFlags {0};
            DWORD m_streamId {0};
    };
}

AkVCam::MediaSample::MediaSample(IMemAllocator *memAllocator,
                                 LONG bufferSize,
                                 LONG align,
                                 LONG prefix)
{
    this->d = new MediaSamplePrivate;
    this->d->m_memAllocator = memAllocator;
    this->d->m_bufferSize = bufferSize;
    this->d->m_dataLength = bufferSize;
    this->d->m_prefix = prefix;
    auto realSize = size_t(bufferSize + prefix + align - 1) & ~size_t(align - 1);
    this->d->m_buffer = new BYTE[realSize];
    memset(this->d->m_buffer, 0, realSize * sizeof(BYTE));
}

AkVCam::MediaSample::~MediaSample()
{
    delete [] this->d->m_buffer;
    deleteMediaType(&this->d->m_mediaType);

    delete this->d;
}

uint64_t AkVCam::MediaSample::ref() const
{
    return static_cast<uint64_t>(this->d->m_refCount);
}

void AkVCam::MediaSample::setMemAllocator(IMemAllocator *memAllocator)
{
    this->d->m_memAllocator = memAllocator;
}

HRESULT AkVCam::MediaSample::QueryInterface(REFIID riid, void **ppv)
{
    AkLogFunction();
    AkLogDebug("IID: %s", stringFromClsid(riid).c_str());

    if (!ppv)
        return E_POINTER;

    static const struct
    {
        const IID *iid;
        void *ptr;
    } comInterfaceEntryMediaSample[] = {
        COM_INTERFACE(IMediaSample)
        COM_INTERFACE(IMediaSample2)
        COM_INTERFACE(IUnknown)
        COM_INTERFACE_NULL
    };
\
    for (auto map = comInterfaceEntryMediaSample; map->ptr; ++map)
        if (*map->iid == riid) {
            *ppv = map->ptr;
            this->AddRef();

            return S_OK;
        }

    *ppv = nullptr;
    AkLogDebug("Interface not found");

    return E_NOINTERFACE;
}

ULONG AkVCam::MediaSample::AddRef()
{
    AkLogFunction();

    return ++this->d->m_refCount;
}

ULONG AkVCam::MediaSample::Release()
{
    AkLogFunction();
    ULONG ref = --this->d->m_refCount;

    if (this->d->m_memAllocator && ref == 1)
        this->d->m_memAllocator->ReleaseBuffer(this);

    if (ref == 0)
        delete this;

    return ref;
}

HRESULT AkVCam::MediaSample::GetPointer(BYTE **ppBuffer)
{
    AkLogFunction();

    if (!ppBuffer)
        return E_POINTER;

    *ppBuffer = this->d->m_buffer + this->d->m_prefix;

    return S_OK;
}

LONG AkVCam::MediaSample::GetSize()
{
    AkLogFunction();

    return this->d->m_bufferSize;
}

HRESULT AkVCam::MediaSample::GetTime(REFERENCE_TIME *pTimeStart,
                                     REFERENCE_TIME *pTimeEnd)
{
    AkLogFunction();

    if (!pTimeStart || !pTimeEnd)
        return E_POINTER;

    *pTimeStart = this->d->m_sampleTimeStart;
    *pTimeEnd = this->d->m_sampleTimeEnd;

    if (*pTimeStart < 0)
        return VFW_E_SAMPLE_TIME_NOT_SET;

    if (*pTimeEnd < 0) {
        *pTimeEnd = *pTimeStart + 1;

        return VFW_S_NO_STOP_TIME;
    }

    return S_OK;
}

HRESULT AkVCam::MediaSample::SetTime(REFERENCE_TIME *pTimeStart,
                                     REFERENCE_TIME *pTimeEnd)
{
    AkLogFunction();

    this->d->m_sampleTimeStart = pTimeStart? *pTimeStart: -1;
    this->d->m_sampleTimeEnd = pTimeEnd? *pTimeEnd: -1;

    return S_OK;
}

HRESULT AkVCam::MediaSample::IsSyncPoint()
{
    AkLogFunction();

    return this->d->m_syncPoint? S_OK: S_FALSE;
}

HRESULT AkVCam::MediaSample::SetSyncPoint(BOOL bIsSyncPoint)
{
    AkLogFunction();
    this->d->m_syncPoint = bIsSyncPoint;

    return S_OK;
}

HRESULT AkVCam::MediaSample::IsPreroll()
{
    AkLogFunction();

    return this->d->m_preroll? S_OK: S_FALSE;
}

HRESULT AkVCam::MediaSample::SetPreroll(BOOL bIsPreroll)
{
    AkLogFunction();
    this->d->m_preroll = bIsPreroll;

    if (bIsPreroll)
        this->d->m_sampleFlags |= AM_SAMPLE_PREROLL;
    else
        this->d->m_sampleFlags &= ~AM_SAMPLE_PREROLL;

    return S_OK;
}

LONG AkVCam::MediaSample::GetActualDataLength()
{
    AkLogFunction();

    return this->d->m_dataLength;
}

HRESULT AkVCam::MediaSample::SetActualDataLength(LONG lLen)
{
    AkLogFunction();

    if (lLen < 0 || lLen > this->d->m_bufferSize)
        return VFW_E_BUFFER_OVERFLOW;

    this->d->m_dataLength = lLen;

    return S_OK;
}

HRESULT AkVCam::MediaSample::GetMediaType(AM_MEDIA_TYPE **ppMediaType)
{
    AkLogFunction();

    if (!ppMediaType)
        return E_POINTER;

    *ppMediaType = nullptr;

    if (this->d->m_mediaTypeChanged == S_FALSE)
        return S_FALSE;

    *ppMediaType = createMediaType(this->d->m_mediaType);

    if (!*ppMediaType)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT AkVCam::MediaSample::SetMediaType(AM_MEDIA_TYPE *pMediaType)
{
    AkLogFunction();

    if (!pMediaType)
        return E_POINTER;

    if (isEqualMediaType(pMediaType, this->d->m_mediaType, true)) {
        this->d->m_mediaTypeChanged = S_FALSE;

        return S_OK;
    }

    deleteMediaType(&this->d->m_mediaType);
    this->d->m_mediaType = createMediaType(pMediaType);
    this->d->m_mediaTypeChanged = S_OK;

    return S_OK;
}

HRESULT AkVCam::MediaSample::IsDiscontinuity()
{
    AkLogFunction();

    return this->d->m_discontinuity? S_OK: S_FALSE;
}

HRESULT AkVCam::MediaSample::SetDiscontinuity(BOOL bDiscontinuity)
{
    AkLogFunction();
    this->d->m_discontinuity = bDiscontinuity;

    if (bDiscontinuity)
        this->d->m_sampleFlags |= AM_SAMPLE_DATADISCONTINUITY;
    else
        this->d->m_sampleFlags &= ~AM_SAMPLE_DATADISCONTINUITY;

    return S_OK;
}

HRESULT AkVCam::MediaSample::GetMediaTime(LONGLONG *pTimeStart,
                                          LONGLONG *pTimeEnd)
{
    AkLogFunction();

    if (!pTimeStart || !pTimeEnd)
        return E_POINTER;

    *pTimeStart = this->d->m_mediaTimeStart;
    *pTimeEnd = this->d->m_mediaTimeEnd;

    if (*pTimeStart < 0 || *pTimeEnd < 0)
        return VFW_E_MEDIA_TIME_NOT_SET;

    return S_OK;
}

HRESULT AkVCam::MediaSample::SetMediaTime(LONGLONG *pTimeStart,
                                          LONGLONG *pTimeEnd)
{
    AkLogFunction();

    this->d->m_mediaTimeStart = pTimeStart? *pTimeStart: -1;
    this->d->m_mediaTimeEnd = pTimeEnd? *pTimeEnd: -1;

    return S_OK;
}

HRESULT AkVCam::MediaSample::SetProperties(DWORD cbProperties,
                                           const BYTE *pbProperties)
{
    AkLogFunction();

    if (cbProperties < sizeof(AM_SAMPLE2_PROPERTIES))
        return E_INVALIDARG;

    if (!pbProperties)
        return E_POINTER;

    auto properties =
            reinterpret_cast<const AM_SAMPLE2_PROPERTIES *>(pbProperties);

    if (properties->pbBuffer || properties->cbBuffer)
        return E_INVALIDARG;

    this->d->m_data = properties->cbData;
    this->d->m_typeSpecificFlags = properties->dwTypeSpecificFlags;
    this->d->m_sampleFlags = properties->dwSampleFlags;
    this->SetDiscontinuity(this->d->m_sampleFlags & AM_SAMPLE_DATADISCONTINUITY);
    this->SetSyncPoint(this->d->m_sampleFlags & AM_SAMPLE_SPLICEPOINT);
    this->SetPreroll(this->d->m_sampleFlags & AM_SAMPLE_PREROLL);
    this->SetActualDataLength(properties->lActual);
    auto start = properties->tStart;
    auto stop = properties->tStop;
    this->SetTime(&start, &stop);
    this->SetMediaTime(&start, &stop);
    this->d->m_streamId = properties->dwStreamId;
    deleteMediaType(&this->d->m_mediaType);
    this->d->m_mediaType = createMediaType(properties->pMediaType);
    this->SetMediaType(properties->pMediaType);
    this->d->m_mediaTypeChanged = S_OK;

    return S_OK;
}

HRESULT AkVCam::MediaSample::GetProperties(DWORD cbProperties,
                                           BYTE *pbProperties)
{
    AkLogFunction();

    if (cbProperties < sizeof(AM_SAMPLE2_PROPERTIES))
        return E_INVALIDARG;

    if (!pbProperties)
        return E_POINTER;

    auto properties =
            reinterpret_cast<AM_SAMPLE2_PROPERTIES *>(pbProperties);

    properties->cbData = this->d->m_data;
    properties->dwTypeSpecificFlags = this->d->m_typeSpecificFlags;
    properties->dwSampleFlags = 0;

    if (this->d->m_discontinuity)
        properties->dwSampleFlags |= AM_SAMPLE_DATADISCONTINUITY;

    if (this->d->m_syncPoint)
        properties->dwSampleFlags |= AM_SAMPLE_SPLICEPOINT;

    if (this->d->m_preroll)
        properties->dwSampleFlags |= AM_SAMPLE_PREROLL;

    properties->lActual = this->GetActualDataLength();
    this->GetTime(&properties->tStart, &properties->tStop);
    properties->dwStreamId = this->d->m_streamId;
    properties->pMediaType = createMediaType(this->d->m_mediaType);
    this->GetPointer(&properties->pbBuffer);
    properties->cbBuffer = this->GetSize();

    return S_OK;
}
