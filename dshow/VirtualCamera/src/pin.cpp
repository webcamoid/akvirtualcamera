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
#include <atomic>
#include <functional>
#include <limits>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>
#include <dshow.h>

#include "pin.h"
#include "basefilter.h"
#include "enummediatypes.h"
#include "memallocator.h"
#include "propertyset.h"
#include "pushsource.h"
#include "qualitycontrol.h"
#include "referenceclock.h"
#include "videoprocamp.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class PinPrivate
    {
        public:
            Pin *self;
            BaseFilter *m_baseFilter {nullptr};
            VideoProcAmp *m_videoProcAmp {nullptr};
            std::string m_pinName;
            std::string m_pinId;
            EnumMediaTypes *m_mediaTypes {nullptr};
            IPin *m_connectedTo {nullptr};
            IMemInputPin *m_memInputPin {nullptr};
            IMemAllocator *m_memAllocator {nullptr};
            REFERENCE_TIME m_pts {-1};
            REFERENCE_TIME m_ptsDrift {0};
            REFERENCE_TIME m_start {0};
            REFERENCE_TIME m_stop {MAXLONGLONG};
            double m_rate {1.0};
            FILTER_STATE m_prevState = State_Stopped;
            DWORD_PTR m_adviseCookie {0};
            HANDLE m_sendFrameEvent {nullptr};
            std::thread m_sendFrameThread;
            std::atomic<bool> m_running {false};
            std::mutex m_mutex;
            std::mutex m_controlsMutex;
            VideoFrame m_currentFrame;
            VideoFrame m_testFrame;
            bool m_horizontalFlip {false};   // Controlled by client
            bool m_verticalFlip {false};
            std::map<std::string, int> m_controls;
            LONG m_brightness {0};
            LONG m_contrast {0};
            LONG m_saturation {0};
            LONG m_gamma {0};
            LONG m_hue {0};
            LONG m_colorenable {0};

            void sendFrameOneShot();
            void sendFrameLoop();
            HRESULT sendFrame();
            VideoFrame applyAdjusts(const VideoFrame &frame);
            static void propertyChanged(void *userData,
                                        LONG Property,
                                        LONG lValue,
                                        LONG Flags);
            VideoFrame randomFrame();
    };
}

AkVCam::Pin::Pin(BaseFilter *baseFilter,
                 const std::vector<VideoFormat> &formats,
                 const std::string &pinName):
    StreamConfig(this)
{
    this->setParent(this, &IID_IPin);

    this->d = new PinPrivate;
    this->d->self = this;
    this->d->m_baseFilter = baseFilter;
    this->d->m_pinName = pinName;
    std::stringstream ss;
    ss << "pin(" << this << ")";
    this->d->m_pinId = ss.str();
    this->d->m_mediaTypes = new AkVCam::EnumMediaTypes(formats);
    this->d->m_mediaTypes->AddRef();

    auto cameraIndex = Preferences::cameraFromId(baseFilter->deviceId());
    this->d->m_controls["hflip"] =
            Preferences::cameraControlValue(cameraIndex, "hflip");
    this->d->m_controls["vflip"] =
            Preferences::cameraControlValue(cameraIndex, "vflip");
    this->d->m_controls["scaling"] =
            Preferences::cameraControlValue(cameraIndex, "scaling");
    this->d->m_controls["aspect_ratio"] =
            Preferences::cameraControlValue(cameraIndex, "aspect_ratio");
    this->d->m_controls["swap_rgb"] =
            Preferences::cameraControlValue(cameraIndex, "swap_rgb");

    auto picture = Preferences::picture();

    if (!picture.empty())
        this->d->m_testFrame = loadPicture(picture);

    baseFilter->QueryInterface(IID_IAMVideoProcAmp,
                               reinterpret_cast<void **>(&this->d->m_videoProcAmp));

    LONG flags = 0;
    this->d->m_videoProcAmp->Get(VideoProcAmp_Brightness,
                                 &this->d->m_brightness,
                                 &flags);
    this->d->m_videoProcAmp->Get(VideoProcAmp_Contrast,
                                 &this->d->m_contrast, &flags);
    this->d->m_videoProcAmp->Get(VideoProcAmp_Saturation,
                                 &this->d->m_saturation,
                                 &flags);
    this->d->m_videoProcAmp->Get(VideoProcAmp_Gamma,
                                 &this->d->m_gamma,
                                 &flags);
    this->d->m_videoProcAmp->Get(VideoProcAmp_Hue,
                                 &this->d->m_hue,
                                 &flags);
    this->d->m_videoProcAmp->Get(VideoProcAmp_ColorEnable,
                                 &this->d->m_colorenable,
                                 &flags);

    this->d->m_videoProcAmp->connectPropertyChanged(this->d,
                                                    &PinPrivate::propertyChanged);
}

AkVCam::Pin::~Pin()
{
    this->d->m_mediaTypes->Release();

    if (this->d->m_connectedTo)
        this->d->m_connectedTo->Release();

    if (this->d->m_memInputPin)
        this->d->m_memInputPin->Release();

    if (this->d->m_memAllocator)
        this->d->m_memAllocator->Release();

    if (this->d->m_videoProcAmp)
        this->d->m_videoProcAmp->Release();

    delete this->d;
}

AkVCam::BaseFilter *AkVCam::Pin::baseFilter() const
{
    AkLogFunction();

    return this->d->m_baseFilter;
}

void AkVCam::Pin::setBaseFilter(BaseFilter *baseFilter)
{
    AkLogFunction();
    this->d->m_baseFilter = baseFilter;
}

HRESULT AkVCam::Pin::stateChanged(void *userData, FILTER_STATE state)
{
    auto self = reinterpret_cast<Pin *>(userData);
    AkLogFunction();
    AkLogInfo() << "Old state: " << self->d->m_prevState << std::endl;
    AkLogInfo() << "New state: " << state << std::endl;

    if (state == self->d->m_prevState)
        return S_OK;

    if (self->d->m_prevState == State_Stopped) {
        if (FAILED(self->d->m_memAllocator->Commit()))
            return VFW_E_NOT_COMMITTED;

        self->d->m_pts = -1;
        self->d->m_ptsDrift = 0;

        self->d->m_sendFrameEvent =
                CreateSemaphore(nullptr, 1, 1, TEXT("SendFrame"));

        self->d->m_running = true;
        self->d->m_sendFrameThread =
                std::thread(&PinPrivate::sendFrameLoop, self->d);
        AkLogInfo() << "Launching thread "
                    << self->d->m_sendFrameThread.get_id()
                    << std::endl;

        auto clock = self->d->m_baseFilter->referenceClock();
        REFERENCE_TIME now = 0;
        clock->GetTime(&now);

        AM_MEDIA_TYPE *mediaType = nullptr;
        self->GetFormat(&mediaType);
        auto videoFormat = formatFromMediaType(mediaType);
        deleteMediaType(&mediaType);
        auto fps = videoFormat.minimumFrameRate();
        auto period = REFERENCE_TIME(TIME_BASE / fps.value());

        clock->AdvisePeriodic(now,
                              period,
                              HSEMAPHORE(self->d->m_sendFrameEvent),
                              &self->d->m_adviseCookie);
    } else if (state == State_Stopped) {
        self->d->m_running = false;
        self->d->m_sendFrameThread.join();
        auto clock = self->d->m_baseFilter->referenceClock();
        clock->Unadvise(self->d->m_adviseCookie);
        self->d->m_adviseCookie = 0;
        CloseHandle(self->d->m_sendFrameEvent);
        self->d->m_sendFrameEvent = nullptr;
        self->d->m_memAllocator->Decommit();
        self->d->m_mutex.lock();
        self->d->m_currentFrame.clear();
        self->d->m_mutex.unlock();
    }

    self->d->m_prevState = state;

    return S_OK;
}

void AkVCam::Pin::frameReady(const VideoFrame &frame, bool isActive)
{
    AkLogFunction();
    AkLogInfo() << "Running: " << this->d->m_running << std::endl;

    if (!this->d->m_running)
        return;

    this->d->m_mutex.lock();
    auto frameAdjusted =
            this->d->applyAdjusts(isActive? frame: this->d->m_testFrame);

    if (frameAdjusted.format().size() > 0)
        this->d->m_currentFrame = frameAdjusted;

    this->d->m_mutex.unlock();
}

void AkVCam::Pin::setPicture(const std::string &picture)
{
    AkLogFunction();
    AkLogDebug() << "Picture: " << picture << std::endl;
    this->d->m_mutex.lock();
    this->d->m_testFrame = loadPicture(picture);
    this->d->m_mutex.unlock();
}

void AkVCam::Pin::setControls(const std::map<std::string, int> &controls)
{
    AkLogFunction();
    this->d->m_controlsMutex.lock();

    if (this->d->m_controls == controls) {
        this->d->m_controlsMutex.unlock();

        return;
    }

    for (auto &control: controls)
        AkLogDebug() << control.first << ": " << control.second << std::endl;

    this->d->m_controls = controls;
    this->d->m_controlsMutex.unlock();
}

bool AkVCam::Pin::horizontalFlip() const
{
    return this->d->m_horizontalFlip;
}

void AkVCam::Pin::setHorizontalFlip(bool flip)
{
    this->d->m_horizontalFlip = flip;
}

bool AkVCam::Pin::verticalFlip() const
{
    return this->d->m_verticalFlip;
}

void AkVCam::Pin::setVerticalFlip(bool flip)
{
    this->d->m_verticalFlip = flip;
}

HRESULT AkVCam::Pin::QueryInterface(const IID &riid, void **ppvObject)
{
    AkLogFunction();
    AkLogInfo() << "IID: " << AkVCam::stringFromClsid(riid) << std::endl;

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IPin)) {
        AkLogInterface(IPin, this);
        this->AddRef();
        *ppvObject = this;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IAMStreamConfig)) {
        auto streamConfig = static_cast<IAMStreamConfig *>(this);
        AkLogInterface(IAMStreamConfig, streamConfig);
        streamConfig->AddRef();
        *ppvObject = streamConfig;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IAMPushSource)) {
        auto pushSource = new PushSource(this);
        AkLogInterface(IAMPushSource, pushSource);
        pushSource->AddRef();
        *ppvObject = pushSource;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IKsPropertySet)) {
        auto propertySet = new PropertySet();
        AkLogInterface(IKsPropertySet, propertySet);
        propertySet->AddRef();
        *ppvObject = propertySet;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IQualityControl)) {
        auto qualityControl = new QualityControl();
        AkLogInterface(IQualityControl, qualityControl);
        qualityControl->AddRef();
        *ppvObject = qualityControl;

        return S_OK;
    }

    return CUnknown::QueryInterface(riid, ppvObject);
}

HRESULT AkVCam::Pin::Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
{
    AkLogFunction();
    AkLogInfo() << "Receive pin: " << pReceivePin << std::endl;
    AkLogInfo() << "Media type: " << stringFromMediaType(pmt) << std::endl;

    if (!pReceivePin)
        return E_POINTER;

    if (this->d->m_connectedTo)
        return VFW_E_ALREADY_CONNECTED;

    if (this->d->m_baseFilter) {
        FILTER_STATE state;

        if (SUCCEEDED(this->d->m_baseFilter->GetState(0, &state))
            && state != State_Stopped)
            return VFW_E_NOT_STOPPED;
    }

    PIN_DIRECTION direction = PINDIR_OUTPUT;

    // Only connect to an input pin.
    if (FAILED(pReceivePin->QueryDirection(&direction))
        || direction != PINDIR_INPUT)
        return VFW_E_NO_TRANSPORT;

    /* When the Filter Graph Manager calls Connect, the output pin must request
     * a IMemInputPin and get a IMemAllocator interface to the input pin.
     */
    IMemInputPin *memInputPin = nullptr;

    if (FAILED(pReceivePin->QueryInterface(IID_IMemInputPin,
                                           reinterpret_cast<void **>(&memInputPin)))) {
        return VFW_E_NO_TRANSPORT;
    }

    AM_MEDIA_TYPE *mediaType = nullptr;

    if (pmt) {
        // Try setting requested media type.
        if (!containsMediaType(pmt, this->d->m_mediaTypes))
            return VFW_E_TYPE_NOT_ACCEPTED;

        mediaType = createMediaType(pmt);
    } else {
        // Test currently set media type.
        AM_MEDIA_TYPE *mt = nullptr;

        if (SUCCEEDED(this->GetFormat(&mt)) && mt) {
            if (pReceivePin->QueryAccept(mt) == S_OK)
                mediaType = mt;
            else
                deleteMediaType(&mt);
        }

        if (!mediaType) {
            // Test media types supported by the input pin.
            AM_MEDIA_TYPE *mt = nullptr;
            IEnumMediaTypes *mediaTypes = nullptr;

            if (SUCCEEDED(pReceivePin->EnumMediaTypes(&mediaTypes))) {
                mediaTypes->Reset();

                while (mediaTypes->Next(1, &mt, nullptr) == S_OK) {
                    AkLogInfo() << "Testing media type: "
                                << stringFromMediaType(mt)
                                << std::endl;

                    // If the mediatype match our suported mediatypes...
                    if (this->QueryAccept(mt) == S_OK) {
                        // set it.
                        mediaType = mt;

                        break;
                    }

                    deleteMediaType(&mt);
                }

                mediaTypes->Release();
            }
        }

        if (!mediaType) {
            /* If none of the input media types was suitable for us, ask to
             * input pin if it at least supports one of us.
             */
            AM_MEDIA_TYPE *mt = nullptr;
            this->d->m_mediaTypes->Reset();

            while (this->d->m_mediaTypes->Next(1, &mt, nullptr) == S_OK) {
                if (pReceivePin->QueryAccept(mt) == S_OK) {
                    mediaType = mt;

                    break;
                }

                deleteMediaType(&mt);
            }
        }
    }

    if (!mediaType)
        return VFW_E_NO_ACCEPTABLE_TYPES;

    AkLogInfo() << "Setting Media Type: "
                << stringFromMediaType(mediaType)
                << std::endl;
    auto result = pReceivePin->ReceiveConnection(this, mediaType);

    if (FAILED(result)) {
        deleteMediaType(&mediaType);

        return result;
    }

    AkLogInfo() << "Connection accepted by input pin" << std::endl;

    // Define memory allocator requirements.
    ALLOCATOR_PROPERTIES allocatorRequirements;
    memset(&allocatorRequirements, 0, sizeof(ALLOCATOR_PROPERTIES));
    memInputPin->GetAllocatorRequirements(&allocatorRequirements);
    auto videoFormat = formatFromMediaType(mediaType);

    if (allocatorRequirements.cBuffers < 1)
        allocatorRequirements.cBuffers = 1;

    allocatorRequirements.cbBuffer = LONG(videoFormat.size());

    if (allocatorRequirements.cbAlign < 1)
        allocatorRequirements.cbAlign = 1;

    // Get a memory allocator.
    IMemAllocator *memAllocator = nullptr;

    // if it fail use our own.
    if (FAILED(memInputPin->GetAllocator(&memAllocator))) {
        memAllocator = new MemAllocator;
        memAllocator->AddRef();
    }

    ALLOCATOR_PROPERTIES actualRequirements;
    memset(&actualRequirements, 0, sizeof(ALLOCATOR_PROPERTIES));

    if (FAILED(memAllocator->SetProperties(&allocatorRequirements,
                                           &actualRequirements))) {
        memAllocator->Release();
        memInputPin->Release();
        deleteMediaType(&mediaType);

        return VFW_E_NO_TRANSPORT;
    }

    if (FAILED(memInputPin->NotifyAllocator(memAllocator, S_OK))) {
        memAllocator->Release();
        memInputPin->Release();
        deleteMediaType(&mediaType);

        return VFW_E_NO_TRANSPORT;
    }

    if (this->d->m_memInputPin)
        this->d->m_memInputPin->Release();

    this->d->m_memInputPin = memInputPin;

    if (this->d->m_memAllocator)
        this->d->m_memAllocator->Release();

    this->d->m_memAllocator = memAllocator;
    this->SetFormat(mediaType);

    if (this->d->m_connectedTo)
        this->d->m_connectedTo->Release();

    this->d->m_connectedTo = pReceivePin;
    this->d->m_connectedTo->AddRef();
    this->d->m_baseFilter->connectStateChanged(this, &Pin::stateChanged);
    AkLogInfo() << "Connected to " << pReceivePin << std::endl;

    return S_OK;
}

HRESULT AkVCam::Pin::ReceiveConnection(IPin *pConnector,
                                       const AM_MEDIA_TYPE *pmt)
{
    UNUSED(pConnector);
    UNUSED(pmt);
    AkLogFunction();

    return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT AkVCam::Pin::Disconnect()
{
    AkLogFunction();
    this->d->m_baseFilter->disconnectStateChanged(this, &Pin::stateChanged);

    if (this->d->m_baseFilter) {
        FILTER_STATE state;

        if (SUCCEEDED(this->d->m_baseFilter->GetState(0, &state))
            && state != State_Stopped)
            return VFW_E_NOT_STOPPED;
    }

    if (this->d->m_connectedTo) {
        this->d->m_connectedTo->Release();
        this->d->m_connectedTo = nullptr;
    }

    if (this->d->m_memInputPin) {
        this->d->m_memInputPin->Release();
        this->d->m_memInputPin = nullptr;
    }

    if (this->d->m_memAllocator) {
        this->d->m_memAllocator->Release();
        this->d->m_memAllocator = nullptr;
    }

    return S_OK;
}

HRESULT AkVCam::Pin::ConnectedTo(IPin **pPin)
{
    AkLogFunction();

    if (!pPin)
        return E_POINTER;

    *pPin = nullptr;

    if (!this->d->m_connectedTo)
        return VFW_E_NOT_CONNECTED;

    *pPin = this->d->m_connectedTo;
    (*pPin)->AddRef();

    return S_OK;
}

HRESULT AkVCam::Pin::ConnectionMediaType(AM_MEDIA_TYPE *pmt)
{
    AkLogFunction();

    if (!pmt)
        return E_POINTER;

    memset(pmt, 0, sizeof(AM_MEDIA_TYPE));

    if (!this->d->m_connectedTo)
        return VFW_E_NOT_CONNECTED;

    AM_MEDIA_TYPE *mediaType = nullptr;
    this->GetFormat(&mediaType);
    copyMediaType(pmt, mediaType);
    AkLogInfo() << "Media Type: "
                << stringFromMediaType(mediaType)
                << std::endl;

    return S_OK;
}

HRESULT AkVCam::Pin::QueryPinInfo(PIN_INFO *pInfo)
{
    AkLogFunction();

    if (!pInfo)
        return E_POINTER;

    pInfo->pFilter = this->d->m_baseFilter;

    if (pInfo->pFilter)
        pInfo->pFilter->AddRef();

    pInfo->dir = PINDIR_OUTPUT;
    memset(pInfo->achName, 0, MAX_PIN_NAME * sizeof(WCHAR));

    if (!this->d->m_pinName.empty()) {
        auto pinName = stringToWSTR(this->d->m_pinName);
        memcpy(pInfo->achName,
               pinName,
               (std::min<size_t>)(wcsnlen(pinName, MAX_PIN_NAME)
                                  * sizeof(WCHAR),
                                  MAX_PIN_NAME));
        CoTaskMemFree(pinName);
    }

    return S_OK;
}

HRESULT AkVCam::Pin::QueryDirection(PIN_DIRECTION *pPinDir)
{
    AkLogFunction();

    if (!pPinDir)
        return E_POINTER;

    *pPinDir = PINDIR_OUTPUT;

    return S_OK;
}

HRESULT AkVCam::Pin::QueryId(LPWSTR *Id)
{
    AkLogFunction();

    if (!Id)
        return E_POINTER;

    *Id = stringToWSTR(this->d->m_pinId);

    if (!*Id)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT AkVCam::Pin::QueryAccept(const AM_MEDIA_TYPE *pmt)
{
    AkLogFunction();

    if (!pmt)
        return E_POINTER;

    AkLogInfo() << "Accept? " << stringFromMediaType(pmt) << std::endl;

    if (!containsMediaType(pmt, this->d->m_mediaTypes)) {
        AkLogInfo() << "NO" << std::endl;

        return S_FALSE;
    }

    AkLogInfo() << "YES" << std::endl;

    return S_OK;
}

HRESULT AkVCam::Pin::EnumMediaTypes(IEnumMediaTypes **ppEnum)
{
    AkLogFunction();

    if (!ppEnum)
        return E_POINTER;

    *ppEnum = new AkVCam::EnumMediaTypes(this->d->m_mediaTypes->formats());
    (*ppEnum)->AddRef();

    return S_OK;
}

HRESULT AkVCam::Pin::QueryInternalConnections(IPin **apPin, ULONG *nPin)
{
    AkLogFunction();
    UNUSED(apPin);
    UNUSED(nPin);

    return E_NOTIMPL;
}

HRESULT AkVCam::Pin::EndOfStream()
{
    AkLogFunction();

    return E_UNEXPECTED;
}

HRESULT AkVCam::Pin::BeginFlush()
{
    AkLogFunction();

    return E_UNEXPECTED;
}

HRESULT AkVCam::Pin::EndFlush()
{
    AkLogFunction();

    return E_UNEXPECTED;
}

HRESULT AkVCam::Pin::NewSegment(REFERENCE_TIME tStart,
                                REFERENCE_TIME tStop,
                                double dRate)
{
    AkLogFunction();
    this->d->m_start = tStart;
    this->d->m_stop = tStop;
    this->d->m_rate = dRate;

    return S_OK;
}

void AkVCam::PinPrivate::sendFrameOneShot()
{
    AkLogFunction();

    WaitForSingleObject(this->m_sendFrameEvent, INFINITE);
    this->sendFrame();
    AkLogInfo() << "Thread "
                << std::this_thread::get_id()
                << " finnished"
                << std::endl;
    this->m_running = false;
}

void AkVCam::PinPrivate::sendFrameLoop()
{
    AkLogFunction();

    while (this->m_running) {
        WaitForSingleObject(this->m_sendFrameEvent, INFINITE);
        auto result = this->sendFrame();

        if (FAILED(result)) {
            AkLogError() << "Error sending frame: "
                         << result
                         << ": "
                         << stringFromResult(result)
                         << std::endl;
            this->m_running = false;

            break;
        }
    }

    AkLogInfo() << "Thread "
                << std::this_thread::get_id()
                << " finnished"
                << std::endl;
}

HRESULT AkVCam::PinPrivate::sendFrame()
{
    AkLogFunction();
    IMediaSample *sample = nullptr;

    if (FAILED(this->m_memAllocator->GetBuffer(&sample,
                                               nullptr,
                                               nullptr,
                                               0))
        || !sample)
        return E_FAIL;

    BYTE *buffer = nullptr;
    LONG size = sample->GetSize();

    if (size < 1 || FAILED(sample->GetPointer(&buffer)) || !buffer) {
        sample->Release();

        return E_FAIL;
    }

    this->m_mutex.lock();

    if (this->m_currentFrame.format().size() > 0) {
        auto copyBytes = (std::min)(size_t(size),
                                    this->m_currentFrame.data().size());

        if (copyBytes > 0)
            memcpy(buffer, this->m_currentFrame.data().data(), copyBytes);
    } else {
        auto frame = this->randomFrame();
        auto copyBytes = (std::min)(size_t(size),
                                    frame.data().size());

        if (copyBytes > 0)
            memcpy(buffer, frame.data().data(), copyBytes);
    }

    this->m_mutex.unlock();

    REFERENCE_TIME clock = 0;
    this->m_baseFilter->referenceClock()->GetTime(&clock);

    AM_MEDIA_TYPE *mediaType = nullptr;
    self->GetFormat(&mediaType);
    auto format = formatFromMediaType(mediaType);
    deleteMediaType(&mediaType);
    auto fps = format.minimumFrameRate();
    auto duration = REFERENCE_TIME(TIME_BASE / fps.value());

    if (this->m_pts < 0) {
        this->m_pts = 0;
        this->m_ptsDrift = this->m_pts - clock;
    } else {
        auto diff = clock - this->m_pts + this->m_ptsDrift;

        if (diff <= 2 * duration) {
            this->m_pts = clock + this->m_ptsDrift;
        } else {
            this->m_pts += duration;
            this->m_ptsDrift = this->m_pts - clock;
        }
    }

    auto startTime = this->m_pts;
    auto endTime = startTime + duration;

    sample->SetTime(&startTime, &endTime);
    sample->SetMediaTime(&startTime, &endTime);
    sample->SetActualDataLength(size);
    sample->SetDiscontinuity(false);
    sample->SetSyncPoint(true);
    sample->SetPreroll(false);
    AkLogInfo() << "Sending " << stringFromMediaSample(sample) << std::endl;
    auto result = this->m_memInputPin->Receive(sample);
    AkLogInfo() << "Frame sent" << std::endl;
    sample->Release();

    return result;
}

AkVCam::VideoFrame AkVCam::PinPrivate::applyAdjusts(const VideoFrame &frame)
{
    AM_MEDIA_TYPE *mediaType = nullptr;

    if (FAILED(this->self->GetFormat(&mediaType)))
        return {};

    auto format = formatFromMediaType(mediaType);
    deleteMediaType(&mediaType);
    FourCC fourcc = format.fourcc();
    int width = format.width();
    int height = format.height();

    /* In Windows red and blue channels are swapped, so hack it with the
     * opposite format. Endianness problem maybe?
     */
    std::map<FourCC, FourCC> fixFormat {
        {PixelFormatRGB32, PixelFormatBGR32},
        {PixelFormatRGB24, PixelFormatBGR24},
        {PixelFormatRGB16, PixelFormatBGR16},
        {PixelFormatRGB15, PixelFormatBGR15},
    };

    bool horizontalMirror = false;
    bool verticalMirror = false;
    Scaling scaling = ScalingFast;
    AspectRatio aspectRatio = AspectRatioIgnore;
    bool swapRgb = false;
    this->m_controlsMutex.lock();

    if (this->m_controls.count("hflip") > 0)
        horizontalMirror = this->m_controls["hflip"];

    if (this->m_controls.count("vflip") > 0)
        verticalMirror = this->m_controls["vflip"];

    if (this->m_controls.count("scaling") > 0)
        scaling = Scaling(this->m_controls["scaling"]);

    if (this->m_controls.count("aspect_ratio") > 0)
        aspectRatio = AspectRatio(this->m_controls["aspect_ratio"]);

    if (this->m_controls.count("swap_rgb") > 0)
        swapRgb = this->m_controls["swap_rgb"];

    this->m_controlsMutex.unlock();
    bool vmirror;

    if (fixFormat.count(fourcc) > 0) {
        fourcc = fixFormat[fourcc];
        vmirror = verticalMirror == this->m_verticalFlip;
    } else {
        vmirror = verticalMirror != this->m_verticalFlip;
    }

    VideoFrame newFrame;

    if (width * height > frame.format().width() * frame.format().height()) {
        newFrame =
                frame
                .mirror(horizontalMirror != this->m_horizontalFlip,
                        vmirror)
                .swapRgb(swapRgb)
                .adjust(this->m_hue,
                        this->m_saturation,
                        this->m_brightness,
                        this->m_gamma,
                        this->m_contrast,
                        !this->m_colorenable)
                .scaled(width, height, scaling, aspectRatio)
                .convert(fourcc);
    } else {
        newFrame =
                frame
                .scaled(width, height, scaling, aspectRatio)
                .mirror(horizontalMirror != this->m_horizontalFlip,
                        vmirror)
                .swapRgb(swapRgb)
                .adjust(this->m_hue,
                        this->m_saturation,
                        this->m_brightness,
                        this->m_gamma,
                        this->m_contrast,
                        !this->m_colorenable)
                .convert(fourcc);

    }

    newFrame.format().fourcc() = format.fourcc();

    return newFrame;
}

void AkVCam::PinPrivate::propertyChanged(void *userData,
                                         LONG Property,
                                         LONG lValue,
                                         LONG Flags)
{
    AkLogFunction();
    UNUSED(Flags);
    auto self = reinterpret_cast<PinPrivate *>(userData);

    switch (Property) {
    case VideoProcAmp_Brightness:
        self->m_brightness = lValue;

        break;

    case VideoProcAmp_Contrast:
        self->m_contrast = lValue;

        break;

    case VideoProcAmp_Saturation:
        self->m_saturation = lValue;

        break;

    case VideoProcAmp_Gamma:
        self->m_gamma = lValue;

        break;

    case VideoProcAmp_Hue:
        self->m_hue = lValue;

        break;

    case VideoProcAmp_ColorEnable:
        self->m_colorenable = lValue;
        break;

    default:
        break;
    }
}

AkVCam::VideoFrame AkVCam::PinPrivate::randomFrame()
{
    AM_MEDIA_TYPE *mediaType = nullptr;

    if (FAILED(this->self->GetFormat(&mediaType)))
        return {};

    auto format = formatFromMediaType(mediaType);
    deleteMediaType(&mediaType);

    VideoFormat rgbFormat(PixelFormatRGB24, format.width(), format.height());
    VideoData data(rgbFormat.size());
    static std::uniform_int_distribution<int> distribution(0, 255);
    static std::default_random_engine engine;
    std::generate(data.begin(), data.end(), [] () {
        return uint8_t(distribution(engine));
    });

    VideoFrame rgbFrame;
    rgbFrame.format() = rgbFormat;
    rgbFrame.data() = data;

    return rgbFrame.adjust(this->m_hue,
                           this->m_saturation,
                           this->m_brightness,
                           this->m_gamma,
                           this->m_contrast,
                           !this->m_colorenable)
                   .convert(format.fourcc());
}
