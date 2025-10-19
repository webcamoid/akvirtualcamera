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
#include <codecvt>
#include <locale>
#include <mutex>
#include <random>
#include <thread>
#include <CoreMediaIO/CMIOSampleBuffer.h>

#include "stream.h"
#include "clock.h"
#include "device.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/logger.h"
#include "VCamUtils/src/timer.h"
#include "VCamUtils/src/fraction.h"
#include "VCamUtils/src/videoadjusts.h"
#include "VCamUtils/src/videoconverter.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"

namespace AkVCam
{
    class StreamPrivate
    {
        public:
            Stream *self;
            IpcBridgePtr m_bridge;
            Device *m_device {nullptr};
            ClockPtr m_clock;
            UInt64 m_sequence;
            CMTime m_pts;
            SampleBufferQueuePtr m_queue;
            CMIODeviceStreamQueueAlteredProc m_queueAltered {nullptr};
            VideoFormat m_format;
            VideoFrame m_currentFrame;
            VideoFrame m_testFrame;
            VideoAdjusts m_videoAdjusts;
            VideoConverter m_videoConverter;
            void *m_queueAlteredRefCon {nullptr};
            Timer m_timer;
            std::mutex m_mutex;
            Scaling m_scaling {ScalingFast};
            AspectRatio m_aspectRatio {AspectRatioIgnore};
            bool m_running {false};
            bool m_horizontalMirror {false};
            bool m_verticalMirror {false};
            bool m_swapRgb {false};
            bool m_frameReady {false};

            explicit StreamPrivate(Stream *self);
            bool startTimer();
            void stopTimer();
            static void streamLoop(void *userData);
            void sendFrame(const VideoFrame &frame);
            VideoFrame applyAdjusts(const VideoFrame &frame);
            VideoFrame randomFrame();
    };
}

AkVCam::Stream::Stream(bool registerObject,
                       Object *parent):
    Object(parent)
{
    this->d = new StreamPrivate(this);
    this->m_className = "Stream";
    this->m_classID = kCMIOStreamClassID;
    auto picture = Preferences::picture();

    if (!picture.empty())
        this->d->m_testFrame = loadPicture(picture);

    this->d->m_clock =
            std::make_shared<Clock>("CMIO::VirtualCamera::Stream",
                                    CMTimeMake(1, 10),
                                    100,
                                    10);
    this->d->m_queue = std::make_shared<SampleBufferQueue>(30);

    if (registerObject) {
        this->createObject();
        this->registerObject();
    }

    this->m_properties.setProperty(kCMIOStreamPropertyClock, this->d->m_clock);
    this->d->m_timer.connectTimeout(this->d, &StreamPrivate::streamLoop);
}

AkVCam::Stream::~Stream()
{
    this->registerObject(false);
    delete this->d;
}

OSStatus AkVCam::Stream::createObject()
{
    AkLogFunction();

    if (!this->m_pluginInterface
        || !*this->m_pluginInterface
        || !this->m_parent)
        return kCMIOHardwareUnspecifiedError;

    CMIOObjectID streamID = 0;

    auto status =
            CMIOObjectCreate(this->m_pluginInterface,
                             this->m_parent->objectID(),
                             this->m_classID,
                             &streamID);

    if (status == kCMIOHardwareNoError) {
        this->m_isCreated = true;
        this->m_objectID = streamID;
        AkLogInfo("Created stream: %d", this->m_objectID);
    }

    return status;
}

OSStatus AkVCam::Stream::registerObject(bool regist)
{
    AkLogFunction();
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!this->m_isCreated
        || !this->m_pluginInterface
        || !*this->m_pluginInterface
        || !this->m_parent)
        return status;

    if (regist) {
        status = CMIOObjectsPublishedAndDied(this->m_pluginInterface,
                                             this->m_parent->objectID(),
                                             1,
                                             &this->m_objectID,
                                             0,
                                             nullptr);
    } else {
        status = CMIOObjectsPublishedAndDied(this->m_pluginInterface,
                                             this->m_parent->objectID(),
                                             0,
                                             nullptr,
                                             1,
                                             &this->m_objectID);
    }

    return status;
}

AkVCam::Device *AkVCam::Stream::device() const
{
    return this->d->m_device;
}

void AkVCam::Stream::setDevice(Device *device)
{
    this->d->m_device = device;

    auto cameraIndex = Preferences::cameraFromId(device->deviceId());

    auto horizontalMirror = Preferences::cameraControlValue(cameraIndex, "hflip") > 0;
    auto verticalMirror = Preferences::cameraControlValue(cameraIndex, "vflip") > 0;
    auto scaling = VideoConverter::ScalingMode(Preferences::cameraControlValue(cameraIndex, "scaling"));
    auto aspectRatio = VideoConverter::AspectRatioMode(Preferences::cameraControlValue(cameraIndex, "aspect_ratio"));
    auto swapRgb = Preferences::cameraControlValue(cameraIndex, "swap_rgb") > 0;

    this->d->m_videoAdjusts.setHorizontalMirror(horizontalMirror);
    this->d->m_videoAdjusts.setVerticalMirror(verticalMirror);
    this->d->m_videoAdjusts.setSwapRGB(swapRgb);
    this->d->m_videoConverter.setAspectRatioMode(VideoConverter::AspectRatioMode(aspectRatio));
    this->d->m_videoConverter.setScalingMode(VideoConverter::ScalingMode(scaling));
}

void AkVCam::Stream::setPicture(const std::string &picture)
{
    AkLogFunction();
    AkLogDebug("Picture: %s", picture.c_str());

    this->d->m_testFrame = loadPicture(picture);
    this->d->m_mutex.unlock();
}

void AkVCam::Stream::setControls(const std::map<std::string, int> &controls)
{
    AkLogFunction();

    if (this->d->m_device->directMode())
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

void AkVCam::Stream::setBridge(IpcBridgePtr bridge)
{
    this->d->m_bridge = bridge;
}

void AkVCam::Stream::setFormats(const std::vector<VideoFormat> &formats)
{
    AkLogFunction();

    if (formats.empty())
        return;

    for (auto &format: formats)
        AkLogInfo("Format: %s %dx%d",
                  enumToString(format.format()).c_str(),
                  format.width(),
                  format.height());

    this->m_properties.setProperty(kCMIOStreamPropertyFormatDescriptions,
                                   formats);

    std::vector<Fraction> frameRates;
    auto minimumFrameRate = std::numeric_limits<double>::max();

    for (auto &format: formats) {
        auto fps = format.fps();
        auto value = fps.value();

        if (std::find(frameRates.begin(), frameRates.end(), fps) == frameRates.end())
            frameRates.push_back(fps);

        if (value < minimumFrameRate)
            minimumFrameRate = value;
    }

    std::sort(frameRates.begin(), frameRates.end());
    std::vector<FractionRange> frameRateRanges;

    if (!frameRates.empty()) {
        auto min = *std::min_element(frameRates.begin(),
                                     frameRates.end());
        auto max = *std::max_element(frameRates.begin(),
                                     frameRates.end());
        frameRateRanges.emplace_back(FractionRange {min, max});
    }

    if (!formats.empty()) {
        this->m_properties.setProperty(kCMIOStreamPropertyFrameRates,
                                       frameRates);
        this->m_properties.setProperty(kCMIOStreamPropertyFrameRateRanges,
                                       frameRateRanges);
        this->m_properties.setProperty(kCMIOStreamPropertyMinimumFrameRate,
                                       minimumFrameRate);
        this->setFormat(formats.front());
    }
}

void AkVCam::Stream::setFormat(const VideoFormat &format)
{
    AkLogFunction();
    this->m_properties.setProperty(kCMIOStreamPropertyFormatDescription,
                                   format);
    auto fps = format.fps();

    if (fps.isNull())
        this->m_properties.setProperty(kCMIOStreamPropertyFrameRate,
                                       30.0);
    else
        this->m_properties.setProperty(kCMIOStreamPropertyFrameRate,
                                       fps.value());

    this->d->m_format = format;
}

bool AkVCam::Stream::start()
{
    AkLogFunction();

    if (this->d->m_running)
        return false;

    this->d->m_sequence = 0;
    memset(&this->d->m_pts, 0, sizeof(CMTime));
    this->d->m_running = this->d->startTimer();
    this->d->m_frameReady = false;
    this->d->m_currentFrame = {this->d->m_format};
    AkLogInfo("Running: %d", this->d->m_running);
    this->d->m_videoConverter.setOutputFormat(this->d->m_format);

    if (this->d->m_running && this->d->m_bridge)
        this->d->m_bridge->deviceStart(IpcBridge::StreamType_Input,
                                       this->d->m_device->deviceId());

    return this->d->m_running;
}

void AkVCam::Stream::stop()
{
    AkLogFunction();

    if (!this->d->m_running)
        return;

    if (this->d->m_running && this->d->m_bridge)
        this->d->m_bridge->deviceStop(this->d->m_device->deviceId());

    this->d->m_running = false;
    this->d->stopTimer();
    this->d->m_currentFrame = {};
}

bool AkVCam::Stream::running()
{
    return this->d->m_running;
}

void AkVCam::Stream::frameReady(const VideoFrame &frame, bool isActive)
{
    AkLogFunction();
    AkLogInfo("Running: %d", this->d->m_running);

    if (!this->d->m_running)
        return;

    AkLogInfo("Active: %d", isActive);

    this->d->m_mutex.lock();

    if (this->d->m_device->directMode()) {
        if (isActive && frame & this->d->m_format.isSameFormat(frame.format())) {
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

OSStatus AkVCam::Stream::copyBufferQueue(CMIODeviceStreamQueueAlteredProc queueAlteredProc,
                                         void *queueAlteredRefCon,
                                         CMSimpleQueueRef *queue)
{
    AkLogFunction();

    this->d->m_queueAltered = queueAlteredProc;
    this->d->m_queueAlteredRefCon = queueAlteredRefCon;
    *queue = queueAlteredProc? this->d->m_queue->ref(): nullptr;

    if (*queue)
        CFRetain(*queue);

    return kCMIOHardwareNoError;
}

OSStatus AkVCam::Stream::deckPlay()
{
    AkLogFunction();
    AkLogDebug("STUB");

    return kCMIOHardwareUnspecifiedError;
}

OSStatus AkVCam::Stream::deckStop()
{
    AkLogFunction();
    AkLogDebug("STUB");

    return kCMIOHardwareUnspecifiedError;
}

OSStatus AkVCam::Stream::deckJog(SInt32 speed)
{
    UNUSED(speed);
    AkLogFunction();
    AkLogDebug("STUB");

    return kCMIOHardwareUnspecifiedError;
}

OSStatus AkVCam::Stream::deckCueTo(Float64 frameNumber, Boolean playOnCue)
{
    UNUSED(frameNumber);
    UNUSED(playOnCue);
    AkLogFunction();
    AkLogDebug("STUB");

    return kCMIOHardwareUnspecifiedError;
}

AkVCam::StreamPrivate::StreamPrivate(AkVCam::Stream *self):
    self(self)
{
}

bool AkVCam::StreamPrivate::startTimer()
{
    AkLogFunction();

    Float64 fps = 0;
    this->self->m_properties.getProperty(kCMIOStreamPropertyFrameRate, &fps);
    this->m_timer.setInterval(std::round(1000.0 / fps));
    this->m_timer.start();

    return true;
}

void AkVCam::StreamPrivate::stopTimer()
{
    AkLogFunction();
    this->m_timer.stop();
}

void AkVCam::StreamPrivate::streamLoop(void *userData)
{
    AkLogFunction();

    auto self = reinterpret_cast<StreamPrivate *>(userData);
    AkLogDebug("Running: %d", self->m_running);

    if (!self->m_running)
        return;

    self->m_mutex.lock();

    if (self->m_currentFrame.size() < 1)
        self->sendFrame(self->randomFrame());
    else
        self->sendFrame(self->m_currentFrame);

    self->m_mutex.unlock();
}

void AkVCam::StreamPrivate::sendFrame(const VideoFrame &frame)
{
    AkLogFunction();

    if (this->m_queue->fullness() >= 1.0f)
        return;

    PixelFormat fourcc = frame.format().format();
    int width = frame.format().width();
    int height = frame.format().height();

    AkLogInfo("Sending Frame: %s %dx%d",
              enumToString(fourcc).c_str(),
              width,
              height);

    bool resync = false;
    UInt64 hostTime =
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    auto pts = CMTimeMake(int64_t(hostTime), 1e9);
    auto ptsDiff = CMTimeGetSeconds(CMTimeSubtract(this->m_pts, pts));

    if (CMTimeCompare(pts, this->m_pts) == 0)
        return;

    Float64 fps = 0;
    this->self->m_properties.getProperty(kCMIOStreamPropertyFrameRate, &fps);

    if (CMTIME_IS_INVALID(this->m_pts)
        || (ptsDiff < 0)
        || (ptsDiff > 2. / fps)) {
        this->m_pts = pts;
        resync = true;
    }

    CMIOStreamClockPostTimingEvent(this->m_pts,
                                   UInt64(hostTime),
                                   resync,
                                   this->m_clock->ref());

    CVImageBufferRef imageBuffer = nullptr;
    CVPixelBufferCreate(kCFAllocatorDefault,
                        size_t(width),
                        size_t(height),
                        formatToCM(PixelFormat(fourcc)),
                        nullptr,
                        &imageBuffer);

    if (!imageBuffer)
        return;

    CVPixelBufferLockBaseAddress(imageBuffer, 0);
    auto data = CVPixelBufferGetBaseAddress(imageBuffer);
    memcpy(data, frame.constData(), frame.size());
    CVPixelBufferUnlockBaseAddress(imageBuffer, 0);

    CMVideoFormatDescriptionRef format = nullptr;
    CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault,
                                                 imageBuffer,
                                                 &format);

    auto duration = CMTimeMake(1e3, int32_t(1e3 * fps));
    CMSampleTimingInfo timingInfo {
        duration,
        this->m_pts,
        this->m_pts
    };

    CMSampleBufferRef buffer = nullptr;
    CMIOSampleBufferCreateForImageBuffer(kCFAllocatorDefault,
                                         imageBuffer,
                                         format,
                                         &timingInfo,
                                         this->m_sequence,
                                         resync?
                                             kCMIOSampleBufferDiscontinuityFlag_UnknownDiscontinuity:
                                             kCMIOSampleBufferNoDiscontinuities,
                                         &buffer);
    CFRelease(format);
    CFRelease(imageBuffer);

    this->m_queue->enqueue(buffer);
    this->m_pts = CMTimeAdd(this->m_pts, duration);
    this->m_sequence++;

    if (this->m_queueAltered)
        this->m_queueAltered(this->self->m_objectID,
                             buffer,
                             this->m_queueAlteredRefCon);
}

AkVCam::VideoFrame AkVCam::StreamPrivate::applyAdjusts(const VideoFrame &frame)
{
    AkLogFunction();

    VideoFrame newFrame;

    this->m_videoConverter.begin();

    if (this->m_device->directMode()) {
        newFrame = this->m_videoConverter.convert(frame);
    } else {
        VideoFormat format;
        this->self->m_properties.getProperty(kCMIOStreamPropertyFormatDescription,
                                             &format);
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

AkVCam::VideoFrame AkVCam::StreamPrivate::randomFrame()
{
    VideoFormat format;
    this->self->m_properties.getProperty(kCMIOStreamPropertyFormatDescription,
                                         &format);
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
