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

#include <algorithm>
#include <atomic>
#include <vector>
#include <memory>
#include <mutex>
#include <random>
#include <windows.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfobjects.h>

#include "mediastream.h"
#include "mediasource.h"
#include "PlatformUtils/src/utils.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/videoprocamp.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"
#include "MFUtils/src/utils.h"

#define TIME_BASE 1.0e7

enum MediaStreamState
{
    MediaStreamState_Stopped,
    MediaStreamState_Started,
    MediaStreamState_Paused
};

namespace AkVCam
{
    class MediaStreamPrivate
    {
        public:
            MediaStream *self;
            IpcBridgePtr m_bridge;
            MediaSource *m_mediaSource {nullptr};
            VideoProcAmp *m_videoProcAmp {nullptr};
            IMFStreamDescriptor *m_streamDescriptor {nullptr};
            MediaStreamState m_state {MediaStreamState_Stopped};
            LONGLONG m_sampleCount {0};
            std::vector<std::shared_ptr<IUnknown>> m_sampleTokens;
            VideoFormat m_format;
            std::atomic<bool> m_running {false};
            std::mutex m_mutex;
            std::mutex m_controlsMutex;
            VideoFrame m_currentFrame;
            VideoFrame m_testFrame;
            LONGLONG m_pts {-1};
            LONGLONG m_ptsDrift {0};
            bool m_horizontalFlip {false};   // Controlled by client
            bool m_verticalFlip {false};
            std::map<std::string, int> m_controls;
            LONG m_brightness {0};
            LONG m_contrast {0};
            LONG m_saturation {0};
            LONG m_gamma {0};
            LONG m_hue {0};
            LONG m_colorenable {0};

            explicit MediaStreamPrivate(MediaStream *self);
            HRESULT queueSample();
            VideoFrame applyAdjusts(const VideoFrame &frame);
            static void propertyChanged(void *userData,
                                        LONG Property,
                                        LONG lValue,
                                        LONG Flags);
            VideoFrame randomFrame();
    };
}

AkVCam::MediaStream::MediaStream(MediaSource *mediaSource,
                                 IMFStreamDescriptor *streamDescriptor):
    MediaEventGenerator()
{
    this->d = new MediaStreamPrivate(this);

    if (mediaSource) {
        mediaSource->AddRef();
        this->d->m_mediaSource = mediaSource;
    }

    if (streamDescriptor) {
        streamDescriptor->AddRef();
        this->d->m_streamDescriptor = streamDescriptor;
    }

    auto cameraIndex = Preferences::cameraFromId(mediaSource->deviceId());

    this->d->m_controlsMutex.lock();
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
    this->d->m_controlsMutex.unlock();

    auto picture = Preferences::picture();

    if (!picture.empty()) {
        this->d->m_mutex.lock();
        this->d->m_testFrame = loadPicture(picture);
        this->d->m_mutex.unlock();
    }

    if (mediaSource)
        mediaSource->QueryInterface(IID_IAMVideoProcAmp,
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
                                                    &MediaStreamPrivate::propertyChanged);
}

AkVCam::MediaStream::~MediaStream()
{
    if (this->d->m_streamDescriptor)
        this->d->m_streamDescriptor->Release();

    if (this->d->m_mediaSource)
        this->d->m_mediaSource->Release();

    if (this->d->m_videoProcAmp)
        this->d->m_videoProcAmp->Release();

    delete this->d;
}

void AkVCam::MediaStream::setBridge(IpcBridgePtr bridge)
{
    this->d->m_bridge = bridge;
}

void AkVCam::MediaStream::frameReady(const VideoFrame &frame, bool isActive)
{
    AkLogFunction();
    AkLogInfo() << "Running: " << this->d->m_running << std::endl;

    if (!this->d->m_running)
        return;

    AkLogInfo() << "Active: " << isActive << std::endl;

    this->d->m_mutex.lock();
    auto frameAdjusted =
            this->d->applyAdjusts(isActive? frame: this->d->m_testFrame);

    if (frameAdjusted.format().size() > 0)
        this->d->m_currentFrame = frameAdjusted;
    else
        this->d->m_currentFrame.clear();

    this->d->m_mutex.unlock();
}

void AkVCam::MediaStream::setPicture(const std::string &picture)
{
    AkLogFunction();
    AkLogDebug() << "Picture: " << picture << std::endl;
    this->d->m_mutex.lock();
    this->d->m_testFrame = loadPicture(picture);
    this->d->m_mutex.unlock();
}

void AkVCam::MediaStream::setControls(const std::map<std::string, int> &controls)
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

HRESULT AkVCam::MediaStream::start(IMFMediaType *mediaType)
{
    AkLogFunction();
    std::lock_guard<std::mutex> lock(this->d->m_mutex);

    if (this->d->m_state != MediaStreamState_Stopped)
        return MF_E_INVALID_STATE_TRANSITION;

    this->d->m_pts = -1;
    this->d->m_ptsDrift = 0;
    this->d->m_format = formatFromMFMediaType(mediaType);
    this->d->m_state = MediaStreamState_Started;
    this->d->m_running = true;

    if (this->d->m_bridge)
        this->d->m_bridge->deviceStart(IpcBridge::StreamType_Input,
                                       this->d->m_mediaSource->deviceId());

    return S_OK;
}

HRESULT AkVCam::MediaStream::stop()
{
    AkLogFunction();

    {
        std::lock_guard<std::mutex> lock(this->d->m_mutex);

        if (this->d->m_state != MediaStreamState_Started
            && this->d->m_state != MediaStreamState_Paused) {
            return MF_E_INVALID_STATE_TRANSITION;
        }

        this->d->m_state = MediaStreamState_Stopped;
        this->d->m_running = false;

        if (this->d->m_bridge)
            this->d->m_bridge->deviceStop(this->d->m_mediaSource->deviceId());
    }

    return this->eventQueue()->QueueEventParamVar(MEStreamStopped,
                                                  GUID_NULL,
                                                  S_OK,
                                                  nullptr);
}

HRESULT AkVCam::MediaStream::pause()
{
    AkLogFunction();
    std::lock_guard<std::mutex> lock(this->d->m_mutex);

    if (this->d->m_state != MediaStreamState_Started)
        return MF_E_INVALID_STATE_TRANSITION;

    this->d->m_state = MediaStreamState_Paused;
    this->d->m_running = true;

    return this->eventQueue()->QueueEventParamVar(MEStreamPaused,
                                                  GUID_NULL,
                                                  S_OK,
                                                  nullptr);
}

HRESULT AkVCam::MediaStream::QueryInterface(const IID &riid, void **ppvObject)
{
    AkLogFunction();
    AkLogInfo() << "IID: " << AkVCam::stringFromClsid(riid) << std::endl;

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IMFMediaEventGenerator)) {
        AkLogInterface(IMFMediaEventGenerator, this);
        this->AddRef();
        *ppvObject = this;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IMFMediaStream)) {
        AkLogInterface(IMFMediaStream, this);
        this->AddRef();
        *ppvObject = this;

        return S_OK;
    }

    return MediaEventGenerator::QueryInterface(riid, ppvObject);
}

HRESULT AkVCam::MediaStream::GetMediaSource(IMFMediaSource **ppMediaSource)
{
    AkLogFunction();

    if (!ppMediaSource)
        return E_POINTER;

    std::lock_guard<std::mutex> lock(this->d->m_mutex);

    if (!this->d->m_mediaSource)
        return E_UNEXPECTED;

    *ppMediaSource = this->d->m_mediaSource;
    (*ppMediaSource)->AddRef();

    return S_OK;
}

HRESULT AkVCam::MediaStream::GetStreamDescriptor(IMFStreamDescriptor **ppStreamDescriptor)
{
    AkLogFunction();

    if (!ppStreamDescriptor)
        return E_POINTER;

    std::lock_guard<std::mutex> lock(this->d->m_mutex);

    if (!this->d->m_streamDescriptor)
        return E_UNEXPECTED;

    *ppStreamDescriptor = this->d->m_streamDescriptor;
    (*ppStreamDescriptor)->AddRef();

    return S_OK;
}

HRESULT AkVCam::MediaStream::RequestSample(IUnknown *pToken)
{
    AkLogFunction();
    auto hr = this->d->queueSample();

    if (FAILED(hr)) {
        AkLogError() << "Failed to queue sample: " << hr << std::endl;

        return hr;
    }

    AkLogDebug() << "Saving token" << hr << std::endl;

    // Save the token if available
    if (pToken) {
        std::lock_guard<std::mutex> lock(this->d->m_mutex);

        std::shared_ptr<IUnknown> token(pToken, [] (IUnknown *pToken) {
            pToken->Release();
        });
        pToken->AddRef();

        this->d->m_sampleTokens.push_back(token);
    }

    AkLogDebug() << "Sending MEStreamSinkRequestSample event" << std::endl;

    return this->eventQueue()->QueueEventParamVar(MEStreamSinkRequestSample,
                                                  GUID_NULL,
                                                  S_OK,
                                                  nullptr);
}

AkVCam::MediaStreamPrivate::MediaStreamPrivate(MediaStream *self):
    self(self)
{

}

HRESULT AkVCam::MediaStreamPrivate::queueSample()
{
    AkLogFunction();

    IMFSample *pSample = nullptr;
    auto hr = MFCreateSample(&pSample);

    if (FAILED(hr)) {
        AkLogError() << "Failed creating sample: " << hr << std::endl;

        return hr;
    }

    auto bufferSize = DWORD(this->m_format.size());
    IMFMediaBuffer *pBuffer = nullptr;
    hr = MFCreateMemoryBuffer(bufferSize, &pBuffer);

    if (FAILED(hr)) {
        AkLogError() << "Failed create the buffer: " << hr << std::endl;
        pSample->Release();

        return hr;
    }

    hr = pSample->AddBuffer(pBuffer);

    if (FAILED(hr)) {
        AkLogError() << "Failed adding the buffer to the sample: " << hr << std::endl;
        pBuffer->Release();
        pSample->Release();

        return hr;
    }

    BYTE *pData = nullptr;
    DWORD maxLen = 0;
    DWORD curLen = 0;
    hr = pBuffer->Lock(&pData, &maxLen, &curLen);

    if (FAILED(hr)) {
        AkLogError() << "Failed to lock the buffer: " << hr << std::endl;
        pBuffer->Release();
        pSample->Release();

        return hr;
    }

    this->m_mutex.lock();

    if (this->m_currentFrame.format().size() > 0) {
        auto copyBytes = (std::min)(size_t(maxLen),
                                    this->m_currentFrame.data().size());

        if (copyBytes > 0)
            memcpy(pData, this->m_currentFrame.data().data(), copyBytes);
    } else {
        auto frame = this->randomFrame();
        auto copyBytes = (std::min)(size_t(maxLen),
                                    frame.data().size());

        if (copyBytes > 0)
            memcpy(pData, frame.data().data(), copyBytes);
    }

    this->m_mutex.unlock();

    pBuffer->Unlock();
    hr = pBuffer->SetCurrentLength(bufferSize);

    if (FAILED(hr)) {
        AkLogError() << "Failed setting the current buffer length: " << hr << std::endl;
        pBuffer->Release();
        pSample->Release();

        return hr;
    }

    auto clock = LONGLONG(TIME_BASE * timeGetTime() / 1e3);
    auto fps = this->m_format.minimumFrameRate();
    auto duration = LONGLONG(TIME_BASE / fps.value());

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

    hr = pSample->SetSampleTime(this->m_pts);

    if (FAILED(hr)) {
        AkLogError() << "Failed setting the sample time: " << hr << std::endl;
        pSample->Release();

        return hr;
    }

    hr = pSample->SetSampleDuration(duration);

    if (FAILED(hr)) {
        AkLogError() << "Failed setting the sample duration: " << hr << std::endl;
        pSample->Release();

        return hr;
    }

    // if there are any token available take the first
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);

        if (!this->m_sampleTokens.empty()) {
            auto token = this->m_sampleTokens.front();
            this->m_sampleTokens.erase(this->m_sampleTokens.begin());

            hr = pSample->SetUnknown(MFSampleExtension_Token, token.get());

            if (FAILED(hr)) {
                AkLogError() << "Failed setting the sample token: " << hr << std::endl;
                pSample->Release();

                return hr;
            }
        }
    }

    // Enqueue the sample event
    hr = self->eventQueue()->QueueEventParamUnk(MEMediaSample,
                                                GUID_NULL,
                                                S_OK,
                                                pSample);

    if (SUCCEEDED(hr))
        AkLogDebug() << "Sample queued" << std::endl;
    else
        AkLogError() << "Sample event queue failed: " << hr << std::endl;

    pSample->Release();
    pBuffer->Release();

    return hr;
}

void AkVCam::MediaStreamPrivate::propertyChanged(void *userData,
                                                 LONG Property,
                                                 LONG lValue,
                                                 LONG Flags)
{
    AkLogFunction();
    UNUSED(Flags);
    auto self = reinterpret_cast<MediaStreamPrivate *>(userData);

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

AkVCam::VideoFrame AkVCam::MediaStreamPrivate::applyAdjusts(const VideoFrame &frame)
{
    if (!this->m_format)
        return {};

    FourCC fourcc = this->m_format.fourcc();
    int width = this->m_format.width();
    int height = this->m_format.height();

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

    newFrame.format().fourcc() = this->m_format.fourcc();

    return newFrame;
}

AkVCam::VideoFrame AkVCam::MediaStreamPrivate::randomFrame()
{
    if (!this->m_format)
        return {};

    VideoFormat rgbFormat(PixelFormatRGB24,
                          this->m_format.width(),
                          this->m_format.height());
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
                   .convert(this->m_format.fourcc());
}
