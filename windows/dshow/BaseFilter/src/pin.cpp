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
#include "VCamUtils/src/fraction.h"
#include "VCamUtils/src/videoadjusts.h"
#include "VCamUtils/src/videoconverter.h"
#include "VCamUtils/src/videoframe.h"
#include "VCamUtils/src/videoformatspec.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class PinPrivate
    {
        public:
            Pin *self;
            IpcBridgePtr m_bridge;
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
            VideoFrame m_currentFrame;
            VideoFrame m_testFrame;
            VideoAdjusts m_videoAdjusts;
            VideoConverter m_videoConverter;
            bool m_horizontalFlip {false};   // Controlled by client
            bool m_verticalFlip {false};
            LONG m_brightness {0};
            LONG m_contrast {0};
            LONG m_saturation {0};
            LONG m_gamma {0};
            LONG m_hue {0};
            LONG m_colorEnable {1};
            bool m_isRgb {false};
            bool m_frameReady {false};

            void sendFrameOneShot();
            void sendFrameLoop();
            HRESULT sendFrame();
            VideoFrame applyAdjusts(const VideoFrame &frame);
            static void propertyChanged(void *userData,
                                        LONG property,
                                        LONG value,
                                        LONG flags);
            VideoFrame randomFrame();
    };
}

AkVCam::Pin::Pin(BaseFilter *baseFilter,
                 const std::vector<VideoFormat> &formats,
                 const std::string &pinName):
    StreamConfig(this)
{
    AkLogFunction();
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

    auto horizontalMirror = Preferences::cameraControlValue(cameraIndex, "hflip") > 0;
    auto verticalMirror = Preferences::cameraControlValue(cameraIndex, "vflip") > 0;
    auto scaling = VideoConverter::ScalingMode(Preferences::cameraControlValue(cameraIndex, "scaling"));
    auto aspectRatio = VideoConverter::AspectRatioMode(Preferences::cameraControlValue(cameraIndex, "aspect_ratio"));
    auto swapRgb = Preferences::cameraControlValue(cameraIndex, "swap_rgb") > 0;

    this->d->m_videoAdjusts.setHue(this->d->m_hue);
    this->d->m_videoAdjusts.setSaturation(this->d->m_saturation);
    this->d->m_videoAdjusts.setLuminance(this->d->m_brightness);
    this->d->m_videoAdjusts.setGamma(this->d->m_gamma);
    this->d->m_videoAdjusts.setContrast(this->d->m_contrast);
    this->d->m_videoAdjusts.setGrayScaled(!this->d->m_colorEnable);
    this->d->m_videoAdjusts.setHorizontalMirror(horizontalMirror);
    this->d->m_videoAdjusts.setVerticalMirror(verticalMirror);
    this->d->m_videoAdjusts.setSwapRGB(swapRgb);
    this->d->m_videoConverter.setAspectRatioMode(VideoConverter::AspectRatioMode(aspectRatio));
    this->d->m_videoConverter.setScalingMode(VideoConverter::ScalingMode(scaling));

    auto picture = Preferences::picture();

    if (!picture.empty()) {
        this->d->m_mutex.lock();
        this->d->m_testFrame = loadPicture(picture);
        this->d->m_mutex.unlock();
    }

    baseFilter->QueryInterface(IID_IAMVideoProcAmp,
                               reinterpret_cast<void **>(&this->d->m_videoProcAmp));

    if (this->d->m_videoProcAmp) {
        LONG flags = 0;
        this->d->m_videoProcAmp->Get(VideoProcAmp_Brightness,
                                     &this->d->m_brightness,
                                     &flags);
        this->d->m_videoProcAmp->Get(VideoProcAmp_Contrast,
                                     &this->d->m_contrast,
                                     &flags);
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
                                     &this->d->m_colorEnable,
                                     &flags);

        this->d->m_videoProcAmp->connectPropertyChanged(this->d,
                                                        &PinPrivate::propertyChanged);
    }
}

AkVCam::Pin::~Pin()
{
    AkLogFunction();
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

void AkVCam::Pin::setBridge(IpcBridgePtr bridge)
{
    this->d->m_bridge = bridge;
}

HRESULT AkVCam::Pin::stateChanged(void *userData, FILTER_STATE state)
{
    auto self = reinterpret_cast<Pin *>(userData);
    AkLogFunction();
    AkLogDebug("Old state: %d", self->d->m_prevState);
    AkLogDebug("New state: %d", state);

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
        AkLogInfo("Launching thread 0x%x", self->d->m_sendFrameThread.get_id());

        auto clock = self->d->m_baseFilter->referenceClock();
        REFERENCE_TIME now = 0;
        clock->GetTime(&now);

        AM_MEDIA_TYPE *mediaType = nullptr;
        self->GetFormat(&mediaType);
        auto videoFormat = formatFromMediaType(mediaType);
        deleteMediaType(&mediaType);
        auto fps = videoFormat.fps();
        auto period = REFERENCE_TIME(TIME_BASE / fps.value());

        clock->AdvisePeriodic(now,
                              period,
                              HSEMAPHORE(self->d->m_sendFrameEvent),
                              &self->d->m_adviseCookie);
        self->d->m_frameReady = false;
        self->d->m_currentFrame = {videoFormat};
        self->d->m_videoConverter.setOutputFormat(videoFormat);

        auto specs = VideoFormat::formatSpecs(videoFormat.format());
        self->d->m_isRgb = specs.type() == VideoFormatSpec::VFT_RGB;

        if (self->d->m_bridge)
            self->d->m_bridge->deviceStart(IpcBridge::StreamType_Input,
                                           self->d->m_baseFilter->deviceId());
    } else if (state == State_Stopped) {
        if (self->d->m_bridge)
            self->d->m_bridge->deviceStop(self->d->m_baseFilter->deviceId());

        self->d->m_running = false;
        self->d->m_sendFrameThread.join();
        auto clock = self->d->m_baseFilter->referenceClock();
        clock->Unadvise(self->d->m_adviseCookie);
        self->d->m_adviseCookie = 0;
        CloseHandle(self->d->m_sendFrameEvent);
        self->d->m_sendFrameEvent = nullptr;
        self->d->m_memAllocator->Decommit();
        self->d->m_mutex.lock();
        self->d->m_currentFrame = {};
        self->d->m_mutex.unlock();
        self->d->m_currentFrame = {};
    }

    self->d->m_prevState = state;

    return S_OK;
}

void AkVCam::Pin::frameReady(const VideoFrame &frame, bool isActive)
{
    AkLogFunction();
    AkLogDebug("Running: %d", static_cast<bool>(this->d->m_running));

    if (!this->d->m_running)
        return;

    AM_MEDIA_TYPE *mediaType = nullptr;
    this->GetFormat(&mediaType);
    auto format = formatFromMediaType(mediaType);
    deleteMediaType(&mediaType);

    AkLogDebug("Active: %d", isActive);

    this->d->m_mutex.lock();

    if (this->d->m_baseFilter->directMode()) {
        if (isActive && frame && format.isSameFormat(frame.format())) {
            memcpy(this->d->m_currentFrame.data(),
                   frame.constData(),
                   frame.size());
            this->d->m_frameReady = true;
        } else if (!isActive && this->d->m_testFrame) {
            this->d->m_currentFrame =
                    this->d->applyAdjusts(this->d->m_testFrame);
            this->d->m_frameReady = true;
        } else {
            this->d->m_frameReady = false;
        }
    } else {
        auto frameAdjusted =
                this->d->applyAdjusts(isActive? frame: this->d->m_testFrame);

        if (frameAdjusted) {
            this->d->m_currentFrame = frameAdjusted;
            this->d->m_frameReady = true;
        } else {
            this->d->m_frameReady = false;
        }
    }

    this->d->m_mutex.unlock();
}

void AkVCam::Pin::setPicture(const std::string &picture)
{
    AkLogFunction();
    AkLogDebug("Picture: %s", picture.c_str());
    this->d->m_mutex.lock();
    this->d->m_testFrame = loadPicture(picture);
    this->d->m_mutex.unlock();
}

void AkVCam::Pin::setControls(const std::map<std::string, int> &controls)
{
    AkLogFunction();

    if (this->d->m_baseFilter->directMode())
        return;

    for (auto &control: controls) {
        AkLogDebug("%s: %d", control.first.c_str(), control.second);

        if (control.first == "hflip")
            this->d->m_videoAdjusts.setHorizontalMirror(control.second > 0);
        else if (control.first == "vflip")
            this->d->m_videoAdjusts.setVerticalMirror(control.second > 0);
        else if (control.first == "swap_rgb")
            this->d->m_videoAdjusts.setSwapRGB(control.second > 0);
        else if (control.first == "aspect_ratio")
            this->d->m_videoConverter.setAspectRatioMode(VideoConverter::AspectRatioMode(control.second));
        else if (control.first == "scaling")
            this->d->m_videoConverter.setScalingMode(VideoConverter::ScalingMode(control.second));
    }
}

bool AkVCam::Pin::horizontalFlip() const
{
    return this->d->m_horizontalFlip;
}

void AkVCam::Pin::setHorizontalFlip(bool flip)
{
    this->d->m_horizontalFlip = flip;
    this->d->m_videoAdjusts.setHorizontalMirror(flip);
}

bool AkVCam::Pin::verticalFlip() const
{
    return this->d->m_verticalFlip;
}

void AkVCam::Pin::setVerticalFlip(bool flip)
{
    this->d->m_verticalFlip = flip;
    this->d->m_videoAdjusts.setVerticalMirror(flip);
}

HRESULT AkVCam::Pin::QueryInterface(const IID &riid, void **ppvObject)
{
    AkLogFunction();
    AkLogDebug("IID: %s", stringFromClsid(riid).c_str());

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
    AkLogDebug("Receive pin: %p", pReceivePin);
    AkLogDebug("Media type: %s", stringFromMediaType(pmt).c_str());

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
                    AkLogInfo("Testing media type: %s",
                              stringFromMediaType(mt).c_str());

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

    AkLogInfo("Setting Media Type: %s", stringFromMediaType(mediaType).c_str());
    auto result = pReceivePin->ReceiveConnection(this, mediaType);

    if (FAILED(result)) {
        deleteMediaType(&mediaType);

        return result;
    }

    AkLogInfo("Connection accepted by input pin");

    // Define memory allocator requirements.
    ALLOCATOR_PROPERTIES allocatorRequirements;
    memset(&allocatorRequirements, 0, sizeof(ALLOCATOR_PROPERTIES));
    memInputPin->GetAllocatorRequirements(&allocatorRequirements);
    auto videoFormat = formatFromMediaType(mediaType);

    if (allocatorRequirements.cBuffers < 1)
        allocatorRequirements.cBuffers = 1;

    allocatorRequirements.cbBuffer = LONG(videoFormat.dataSize());

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
    AkLogInfo("Connected to %p", pReceivePin);

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
    AkLogInfo("Media Type: %s", stringFromMediaType(mediaType).c_str());

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
        auto pinName = wstrFromString(this->d->m_pinName);
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

    *Id = wstrFromString(this->d->m_pinId);

    if (!*Id)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT AkVCam::Pin::QueryAccept(const AM_MEDIA_TYPE *pmt)
{
    AkLogFunction();

    if (!pmt)
        return E_POINTER;

    AkLogDebug("Accept? %s", stringFromMediaType(pmt).c_str());

    if (!containsMediaType(pmt, this->d->m_mediaTypes)) {
        AkLogInfo("NO");

        return S_FALSE;
    }

    AkLogInfo("YES");

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
    AkLogDebug("Thread 0x%x finnished", std::this_thread::get_id());
    this->m_running = false;
}

void AkVCam::PinPrivate::sendFrameLoop()
{
    AkLogFunction();

    while (this->m_running) {
        WaitForSingleObject(this->m_sendFrameEvent, INFINITE);
        auto result = this->sendFrame();

        if (FAILED(result)) {
            AkLogError("Error sending frame: 0x%x: %s",
                       result,
                       stringFromResult(result).c_str());
            this->m_running = false;

            break;
        }
    }

    AkLogDebug("Thread %ull finnished", std::this_thread::get_id());
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

    BYTE *pData = nullptr;
    LONG size = sample->GetSize();

    if (size < 1 || FAILED(sample->GetPointer(&pData)) || !pData) {
        sample->Release();

        return E_FAIL;
    }

    this->m_mutex.lock();

    if (this->m_frameReady && this->m_currentFrame.size() > 0) {
        if (this->m_isRgb) {
            auto line = pData;
            auto lineSize = this->m_currentFrame.lineSize(0);
            auto height = this->m_currentFrame.format().height();

            for (int y = 0; y < height; ++y) {
                memcpy(line, this->m_currentFrame.constLine(0, height - y - 1), lineSize);
                line += lineSize;
            }
        } else {
            auto copyBytes = std::min(size_t(size), this->m_currentFrame.size());

            if (copyBytes > 0)
                memcpy(pData, this->m_currentFrame.constData(), copyBytes);
        }
    } else {
        auto frame = this->randomFrame();
        auto copyBytes = std::min(size_t(size), frame.size());

        if (copyBytes > 0)
            memcpy(pData, frame.constData(), copyBytes);
    }

    this->m_mutex.unlock();

    REFERENCE_TIME clock = 0;
    this->m_baseFilter->referenceClock()->GetTime(&clock);

    AM_MEDIA_TYPE *mediaType = nullptr;
    self->GetFormat(&mediaType);
    auto format = formatFromMediaType(mediaType);
    deleteMediaType(&mediaType);
    auto fps = format.fps();
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
    AkLogDebug("Sending %s", stringFromMediaSample(sample).c_str());
    auto result = this->m_memInputPin->Receive(sample);
    AkLogDebug("Frame sent");
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
    VideoFrame newFrame;

    this->m_videoConverter.begin();

    if (this->m_baseFilter->directMode()) {
        newFrame = this->m_videoConverter.convert(frame);
    } else {
        int width = format.width();
        int height = format.height();

        if (width * height > frame.format().width() * frame.format().height()) {
            newFrame = this->m_videoAdjusts.adjust(frame);
            newFrame = this->m_videoConverter.convert(newFrame);
        } else {
            newFrame = this->m_videoConverter.convert(frame);
            newFrame = this->m_videoAdjusts.adjust(newFrame);
        }
    }

    this->m_videoConverter.end();

    return newFrame;
}

void AkVCam::PinPrivate::propertyChanged(void *userData,
                                         LONG property,
                                         LONG value,
                                         LONG flags)
{
    AkLogFunction();
    UNUSED(flags);
    auto self = reinterpret_cast<PinPrivate *>(userData);

    switch (property) {
    case VideoProcAmp_Brightness:
        self->m_brightness = value;
        self->m_videoAdjusts.setLuminance(self->m_brightness);

        break;

    case VideoProcAmp_Contrast:
        self->m_contrast = value;
        self->m_videoAdjusts.setContrast(self->m_contrast);

        break;

    case VideoProcAmp_Saturation:
        self->m_saturation = value;
        self->m_videoAdjusts.setSaturation(self->m_saturation);

        break;

    case VideoProcAmp_Gamma:
        self->m_gamma = value;
        self->m_videoAdjusts.setGamma(self->m_gamma);

        break;

    case VideoProcAmp_Hue:
        self->m_hue = value;
        self->m_videoAdjusts.setHue(self->m_hue);

        break;

    case VideoProcAmp_ColorEnable:
        self->m_colorEnable = value;
        self->m_videoAdjusts.setGrayScaled(!self->m_colorEnable);

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

    VideoFrame frame(format);
    static std::uniform_int_distribution<int> distribution(0, 255);
    static std::default_random_engine engine;
    std::generate(frame.data(),
                  frame.data() + frame.size(),
                  [] () {
        return uint8_t(distribution(engine));
    });

    return this->m_videoAdjusts.adjust(frame);
}
