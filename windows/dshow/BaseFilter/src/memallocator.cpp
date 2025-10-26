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

#include <algorithm>
#include <cinttypes>
#include <condition_variable>
#include <vector>
#include <mutex>
#include <dshow.h>

#include "memallocator.h"
#include "mediasample.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/utils.h"

#define MINIMUM_BUFFERS 4

namespace AkVCam
{
    class MemAllocatorPrivate
    {
        public:
            std::vector<MediaSample *> m_samples;
            ALLOCATOR_PROPERTIES m_properties;
            std::mutex m_mutex;
            std::condition_variable_any m_bufferReleased;
            size_t m_position {0};
            std::atomic<size_t> m_buffersTaken {0};
            bool m_commited {false};
            bool m_decommiting {false};
    };
}

AkVCam::MemAllocator::MemAllocator():
    CUnknown()
{
    this->d = new MemAllocatorPrivate;
    memset(&this->d->m_properties, 0, sizeof(ALLOCATOR_PROPERTIES));
}

AkVCam::MemAllocator::~MemAllocator()
{
    this->Decommit();

    delete this->d;
}

HRESULT AkVCam::MemAllocator::SetProperties(ALLOCATOR_PROPERTIES *pRequest,
                                            ALLOCATOR_PROPERTIES *pActual)
{
    AkLogFunction();

    if (!pRequest || !pActual)
        return E_POINTER;

    std::lock_guard<std::mutex> lock(this->d->m_mutex);

    this->d->m_properties = *pRequest;

    if (this->d->m_properties.cBuffers < MINIMUM_BUFFERS)
        this->d->m_properties.cBuffers = MINIMUM_BUFFERS;

    if (this->d->m_properties.cbBuffer < 1)
        this->d->m_properties.cbBuffer = 1;

    if (this->d->m_properties.cbAlign == 0)
        this->d->m_properties.cbAlign = 1;

    *pActual = this->d->m_properties;

    return S_OK;
}

HRESULT AkVCam::MemAllocator::GetProperties(ALLOCATOR_PROPERTIES *pProps)
{
    AkLogFunction();

    if (!pProps)
        return E_POINTER;

    std::lock_guard<std::mutex> lock(this->d->m_mutex);
    memcpy(pProps, &this->d->m_properties, sizeof(ALLOCATOR_PROPERTIES));

    return S_OK;
}

HRESULT AkVCam::MemAllocator::Commit()
{
    AkLogFunction();

    std::lock_guard<std::mutex> lock(this->d->m_mutex);

    if (this->d->m_commited)
        return S_OK;

    if (this->d->m_properties.cBuffers < 1
        || this->d->m_properties.cbBuffer < 1) {
        AkLogError("Wrong memory allocator size");

        return VFW_E_SIZENOTSET;
    }

    AkLogDebug("Created buffers: %d", this->d->m_properties.cBuffers);
    AkLogDebug("Buffer size: %d", this->d->m_properties.cbBuffer);
    AkLogDebug("Buffer align: %d", this->d->m_properties.cbAlign);
    AkLogDebug("Buffers prefix: %d", this->d->m_properties.cbPrefix);

    for (auto &sample: this->d->m_samples) {
        sample->setMemAllocator(this);
        sample->Release();
    }

    this->d->m_samples.clear();
    this->d->m_position = 0;
    this->d->m_buffersTaken = 0;

    for (LONG i = 0; i < this->d->m_properties.cBuffers; ++i) {
        auto sample =
                new MediaSample(this,
                                 this->d->m_properties.cbBuffer,
                                 this->d->m_properties.cbAlign,
                                 this->d->m_properties.cbPrefix);
        this->d->m_samples.push_back(sample);
    }

    this->d->m_commited = true;

    return S_OK;
}

HRESULT AkVCam::MemAllocator::Decommit()
{
    AkLogFunction();

    std::lock_guard<std::mutex> lock(this->d->m_mutex);

    if (!this->d->m_commited)
        return S_OK;

    this->d->m_decommiting = true;
    this->d->m_bufferReleased.notify_all();

    for (auto sample: this->d->m_samples) {
        sample->setMemAllocator(nullptr);
        sample->Release();
    }

    this->d->m_samples.clear();
    this->d->m_commited = false;
    this->d->m_decommiting = false;
    this->d->m_position = 0;
    this->d->m_buffersTaken = 0;

    return S_OK;
}

HRESULT AkVCam::MemAllocator::GetBuffer(IMediaSample **ppBuffer,
                                        REFERENCE_TIME *pStartTime,
                                        REFERENCE_TIME *pEndTime,
                                        DWORD dwFlags)
{
    AkLogFunction();

    if (!ppBuffer)
        return E_POINTER;

    *ppBuffer = nullptr;

    if (pStartTime)
        *pStartTime = 0;

    if (pEndTime)
        *pEndTime = 0;

    std::unique_lock<std::mutex> lock(this->d->m_mutex);

    if (!this->d->m_commited
        || this->d->m_decommiting
        || this->d->m_samples.empty()) {
        AkLogError("Allocator not committed.");

        return VFW_E_NOT_COMMITTED;
    }

    while (!this->d->m_decommiting
           && this->d->m_buffersTaken >= this->d->m_samples.size()) {
        if (dwFlags & AM_GBF_NOWAIT)
            return VFW_E_TIMEOUT;

        this->d->m_bufferReleased.wait(lock);
    }

    if (this->d->m_decommiting)
        return VFW_E_NOT_COMMITTED;

    for (size_t i = 0; i < this->d->m_samples.size(); ++i) {
        auto sample = this->d->m_samples[this->d->m_position];
        this->d->m_position =
                (this->d->m_position + 1)  % this->d->m_samples.size();

        if (sample->ref() == 1) {
            *ppBuffer = sample;
            sample->AddRef();
            sample->GetTime(pStartTime, pEndTime);
            this->d->m_buffersTaken++;
            AkLogDebug("Buffer passed");

            return S_OK;
        }
    }

    return VFW_E_BUFFER_UNDERFLOW;
}

HRESULT AkVCam::MemAllocator::ReleaseBuffer(IMediaSample *pBuffer)
{
    AkLogFunction();

    if (!pBuffer)
        return E_POINTER;

    auto it = std::find_if(this->d->m_samples.cbegin(),
                           this->d->m_samples.cend(),
                           [pBuffer] (IMediaSample *sample) {
        return sample == pBuffer;
    });

    if (it == this->d->m_samples.cend())
        return E_INVALIDARG;

    this->d->m_buffersTaken--;
    this->d->m_bufferReleased.notify_one();

    return S_OK;
}
