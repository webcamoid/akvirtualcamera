/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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
#include <random>
#include <ctime>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <CoreAudio/AudioHardwareBase.h>

#import "extensiondevicesource.h"
#import "extensionstreamsource.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/fraction.h"
#include "VCamUtils/src/logger.h"
#include "VCamUtils/src/videoadjusts.h"
#include "VCamUtils/src/videoconverter.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"

@interface ExtensionDeviceSource () {
    // Device identity
    std::string m_deviceId;
    std::vector<AkVCam::VideoFormat> m_formats;
    NSUInteger m_activeFormatIndex;

    // IpcBridge is used to signal deviceStart/deviceStop to the daemon.
    AkVCam::IpcBridgePtr m_bridge;

    // When YES, frames whose format matches the active format are copied
    // directly without going through VideoAdjusts/VideoConverter.
    BOOL m_directMode;

    // Frame processing
    AkVCam::VideoFrame m_currentFrame;
    AkVCam::VideoFrame m_testFrame;
    AkVCam::VideoAdjusts m_videoAdjusts;
    AkVCam::VideoConverter m_videoConverter;
    BOOL m_frameReady;

    // Stream timing
    dispatch_source_t m_timer;
    dispatch_queue_t m_frameQueue;
    uint64_t m_sequenceNumber;
    CMTime m_pts;
}
@end

@implementation ExtensionDeviceSource

/* Stores device identity and supported formats, reads the initial values for
 * all video adjustments and converter settings from Preferences, creates the
 * stream source, and sets up the serial dispatch queue used for frame delivery.
 */
- (instancetype) initWithDeviceId: (const std::string &) deviceId
                      description: (const std::string &) description
                          formats: (const std::vector<AkVCam::VideoFormat> &) formats
{
    AkLogFunction();

    if (self = [super init]) {
        m_deviceId           = deviceId;
        m_formats            = formats;
        m_activeFormatIndex  = 0;
        m_directMode         = AkVCam::Preferences::cameraDirectMode(deviceId)? YES: NO;
        m_frameReady         = NO;
        m_sequenceNumber     = 0;
        m_pts                = kCMTimeInvalid;

        if (!m_formats.empty())
            m_videoConverter.setOutputFormat(m_formats[m_activeFormatIndex]);

        // Initialize adjusts and converter from stored preferences.
        auto cameraIndex = AkVCam::Preferences::cameraFromId(deviceId);

        m_videoAdjusts.setHorizontalMirror(
            AkVCam::Preferences::cameraControlValue(cameraIndex, "hflip") > 0);
        m_videoAdjusts.setVerticalMirror(
            AkVCam::Preferences::cameraControlValue(cameraIndex, "vflip") > 0);
        m_videoAdjusts.setSwapRGB(
            AkVCam::Preferences::cameraControlValue(cameraIndex, "swap_rgb") > 0);
        m_videoConverter.setScalingMode(
            AkVCam::VideoConverter::ScalingMode(
                AkVCam::Preferences::cameraControlValue(cameraIndex, "scaling")));
        m_videoConverter.setAspectRatioMode(
            AkVCam::VideoConverter::AspectRatioMode(
                AkVCam::Preferences::cameraControlValue(cameraIndex, "aspect_ratio")));

        _streamSource =
            [[ExtensionStreamSource alloc] initWithDeviceSource: self];

        m_frameQueue = dispatch_queue_create(
            (std::string(BUNDLE_ID) + "." + deviceId + ".framequeue").c_str(),
            DISPATCH_QUEUE_SERIAL);
    }

    return self;
}

- (void) dealloc
{
    AkLogFunction();
    [self stopStreaming];
}

- (std::vector<AkVCam::VideoFormat>) formats
{
    return m_formats;
}

- (NSUInteger) activeFormatIndex
{
    return m_activeFormatIndex;
}

/* Switches the active format to the one at the given index. If the stream
 * is running, it is restarted at the new frame rate. No-op if the index is
 * out of range or already selected.
 */
- (void) setActiveFormatIndex: (NSUInteger) index
{
    AkLogFunction();

    if (index >= m_formats.size() || index == m_activeFormatIndex)
        return;

    m_activeFormatIndex = index;
    m_videoConverter.setOutputFormat(m_formats[m_activeFormatIndex]);

    // If the timer is running, restart it at the new frame rate.
    if (m_timer) {
        [self stopStreaming];
        [self startStreaming];
    }
}

// Declares the device properties exposed to the CoreMediaIO framework.
- (NSSet<CMIOExtensionProperty> *) availableProperties
{
    AkLogFunction();

    return [NSSet setWithObjects:
            CMIOExtensionPropertyDeviceTransportType,
            CMIOExtensionPropertyDeviceModel,
            nil];
}

// Returns the current values for the requested device properties.
- (nullable CMIOExtensionDeviceProperties *)
    devicePropertiesForProperties: (NSSet<CMIOExtensionProperty> *) properties
                            error: (NSError **) outError
{
    AkLogFunction();

    CMIOExtensionDeviceProperties *deviceProperties =
        [CMIOExtensionDeviceProperties devicePropertiesWithDictionary: @{}];

    if ([properties containsObject: CMIOExtensionPropertyDeviceTransportType])
        deviceProperties.transportType = @(kAudioDeviceTransportTypeVirtual);

    if ([properties containsObject: CMIOExtensionPropertyDeviceModel])
        deviceProperties.model = @COMMONS_APPNAME;

    return deviceProperties;
}

- (BOOL) setDeviceProperties: (CMIOExtensionDeviceProperties *) deviceProperties
                       error: (NSError **) outError
{
    AkLogFunction();

    return YES;
}

// Stores the IpcBridge shared pointer.
- (void) setBridge: (AkVCam::IpcBridgePtr) bridge
{
    AkLogFunction();
    m_bridge = bridge;
}

/* Creates and starts a GCD timer that fires at the first format's frame
 * rate. Resets the sequence counter and PTS on each call. If no formats
 * are available, falls back to 30 fps.
 */
- (void) startStreaming
{
    AkLogFunction();

    if (m_timer)
        return;

    m_sequenceNumber = 0;
    m_pts            = kCMTimeInvalid;
    m_frameReady     = NO;
    m_currentFrame   = {};

    double fps = m_formats.empty()? 30.0: m_formats[m_activeFormatIndex].fps().value();

    if (fps <= 0.0)
        fps = 30.0;

    uint64_t intervalNs = (uint64_t) (NSEC_PER_SEC / fps);

    m_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,
                                    0,
                                    0,
                                    m_frameQueue);
    dispatch_source_set_timer(m_timer,
                              dispatch_time(DISPATCH_TIME_NOW, 0),
                              intervalNs,
                              intervalNs / 10);   // 10 % leeway

    __weak typeof(self) weakSelf = self;
    dispatch_source_set_event_handler(m_timer, ^{
        [weakSelf streamLoop];
    });

    dispatch_resume(m_timer);

    if (m_bridge)
        m_bridge->deviceStart(AkVCam::IpcBridge::StreamType_Input, m_deviceId);
}

// Cancels the GCD timer and releases it.
- (void) stopStreaming
{
    AkLogFunction();

    if (!m_timer)
        return;

    if (m_bridge)
        m_bridge->deviceStop(m_deviceId);

    dispatch_source_cancel(m_timer);
    m_timer = nil;
}

/* Handles a new video frame received from the IPC bridge.
 *
 * Direct mode: when the incoming format matches the active format, the
 * pixel data is copied directly into m_currentFrame without going through
 * VideoAdjusts or VideoConverter. When the source is inactive, the test
 * frame is used with adjustments applied.
 *
 * Normal mode: VideoAdjusts and VideoConverter are always applied,
 * choosing between the live frame and the test frame based on isActive.
 *
 * All state mutations are dispatched onto the serial frame queue so they
 * are naturally serialized with streamLoop without an explicit mutex.
 */
- (void) frameReady: (const AkVCam::VideoFrame &) frame
           isActive: (BOOL) isActive
{
    AkLogFunction();

    if (!m_timer)
        return;

    // Capture by value, the reference is only valid for this call stack.
    AkVCam::VideoFrame frameCopy = frame;

    dispatch_async(m_frameQueue, ^{
        if (self->m_directMode) {
            // Direct mode: copy raw pixels only when format matches.
            if (isActive) {
                const AkVCam::VideoFormat &activeFormat =
                    self->m_formats.empty()?
                        frameCopy.format():
                        self->m_formats[self->m_activeFormatIndex];

                if (activeFormat.isSameFormat(frameCopy.format())) {
                    // Resize m_currentFrame if needed, then copy pixels.
                    self->m_currentFrame = AkVCam::VideoFrame(activeFormat);
                    memcpy(self->m_currentFrame.data(),
                           frameCopy.constData(),
                           frameCopy.size());
                    self->m_frameReady = YES;
                } else {
                    self->m_frameReady = NO;
                }
            } else if (self->m_testFrame) {
                // Inactive source in direct mode: show test frame with adjustments.
                AkVCam::VideoFrame adjusted =
                    [self applyAdjusts: self->m_testFrame];

                if (adjusted) {
                    self->m_currentFrame = adjusted;
                    self->m_frameReady   = YES;
                } else {
                    self->m_frameReady = NO;
                }
            } else {
                self->m_frameReady = NO;
            }
        } else {
            // Normal mode: always apply adjustments and conversion.
            const AkVCam::VideoFrame &source =
                isActive? frameCopy: self->m_testFrame;
            AkVCam::VideoFrame adjusted = [self applyAdjusts: source];

            if (adjusted) {
                self->m_currentFrame = adjusted;
                self->m_frameReady   = YES;
            } else {
                self->m_frameReady = NO;
            }
        }
    });
}

/* Loads the image at the given path and stores it as the static test frame
 * displayed when no active VideoFrame is being received.
 */
- (void) setPicture: (const std::string &) picture
{
    AkLogFunction();

    AkVCam::VideoFrame loaded = AkVCam::loadPicture(picture);

    dispatch_async(m_frameQueue, ^{
        self->m_testFrame = loaded;
    });
}

/* Applies control values to the video adjuster and converter.
 * Recognized keys: hflip, vflip, swap_rgb, scaling, aspect_ratio.
 */
- (void) setControls: (const std::map<std::string, int> &) controls
{
    AkLogFunction();

    dispatch_async(m_frameQueue, ^{
        for (auto &control: controls) {
            if (control.first == "hflip")
                self->m_videoAdjusts.setHorizontalMirror(control.second > 0);
            else if (control.first == "vflip")
                self->m_videoAdjusts.setVerticalMirror(control.second > 0);
            else if (control.first == "swap_rgb")
                self->m_videoAdjusts.setSwapRGB(control.second > 0);
            else if (control.first == "scaling")
                self->m_videoConverter.setScalingMode(
                    AkVCam::VideoConverter::ScalingMode(control.second));
            else if (control.first == "aspect_ratio")
                self->m_videoConverter.setAspectRatioMode(
                    AkVCam::VideoConverter::AspectRatioMode(control.second));
        }
    });
}

/* Called on every timer tick. Sends the current frame if one is ready,
 * otherwise falls back to the test frame or a generated placeholder.
 */
- (void) streamLoop
{
    AkLogFunction();

    CVPixelBufferRef pixelBuffer = NULL;

    if (m_frameReady && m_currentFrame) {
        pixelBuffer = [self pixelBufferFromVideoFrame: m_currentFrame];
    } else if (m_testFrame) {
        AkVCam::VideoFrame adjusted = [self applyAdjusts: m_testFrame];

        if (adjusted)
            pixelBuffer = [self pixelBufferFromVideoFrame: adjusted];
    }

    if (!pixelBuffer)
        pixelBuffer = [self randomFrame];

    if (!pixelBuffer)
        return;

    [self sendPixelBuffer: pixelBuffer];
    CVPixelBufferRelease(pixelBuffer);
}

/* Apply format conversion, mirroring, and coloring to the frames.
 *
 * Direct mode: only the format conversion is applied; VideoAdjusts
 * is intentionally skipped, matching the behaviour of the DAL where
 * directMode bypasses all image processing.
 *
 * Normal mode: chooses the order of adjust vs. convert based on
 * whether the output resolution is larger than the input (upscale:
 * adjust first, then convert; downscale: convert first, then adjust).
 */
- (AkVCam::VideoFrame) applyAdjusts: (const AkVCam::VideoFrame &) frame
{
    AkLogFunction();

    if (!m_formats.empty())
        m_videoConverter.setOutputFormat(m_formats[m_activeFormatIndex]);

    AkVCam::VideoFrame result;
    m_videoConverter.begin();

    if (m_directMode) {
        result = m_videoConverter.convert(frame);
    } else {
        const AkVCam::VideoFormat &fmt =
            m_formats.empty()? frame.format(): m_formats[m_activeFormatIndex];

        int outPixels = fmt.width() * fmt.height();
        int inPixels  = frame.format().width() * frame.format().height();

        if (outPixels > inPixels) {
            result = m_videoAdjusts.adjust(frame);
            result = m_videoConverter.convert(result);
        } else {
            result = m_videoConverter.convert(frame);
            result = m_videoAdjusts.adjust(result);
        }
    }

    m_videoConverter.end();

    return result;
}

/* Wraps the raw pixel data of a VideoFrame into a CVPixelBufferRef without
 * copying, using a no-op release callback. The VideoFrame must outlive the
 * returned buffer, callers consume the buffer within the same scope.
 */
- (CVPixelBufferRef) pixelBufferFromVideoFrame: (const AkVCam::VideoFrame &) frame
{
    if (!frame)
        return NULL;

    auto fmt = frame.format();
    OSType fourcc = AkVCam::formatToCM(fmt.format());

    CVPixelBufferRef buffer = NULL;
    CVReturn status =
        CVPixelBufferCreateWithBytes(kCFAllocatorDefault,
                                     size_t(fmt.width()),
                                     size_t(fmt.height()),
                                     fourcc,
                                     static_cast<void *>(frame.data()),
                                     frame.lineSize(0),
                                     // No-op release: VideoFrame owns the data.
                                     [] (void *, const void *) {},
                                     nullptr,
                                     nullptr,
                                     &buffer);

    return (status == kCVReturnSuccess)? buffer: NULL;
}

/* Wraps a CVPixelBufferRef in a CMSampleBuffer with correct timing info
 * and delivers it to the CoreMediaIO framework via the extension stream API.
 * Handles PTS initialization and resync detection.
 */
- (void) sendPixelBuffer: (CVPixelBufferRef) pixelBuffer
{
    AkLogFunction();

    double fps = m_formats.empty()? 30.0: m_formats[m_activeFormatIndex].fps().value();

    if (fps <= 0.0)
        fps = 30.0;

    uint64_t hostTimeNs = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
    CMTime pts = CMTimeMake((int64_t) hostTimeNs, (int32_t) NSEC_PER_SEC);
    BOOL resync = NO;

    if (CMTIME_IS_INVALID(m_pts)) {
        resync = YES;
    } else {
        double diff = CMTimeGetSeconds(CMTimeSubtract(m_pts, pts));

        if (diff > 2.0 / fps)
            resync = YES;
    }

    if (resync)
        m_pts = pts;

    CMVideoFormatDescriptionRef formatDesc = NULL;
    CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault,
                                                 pixelBuffer,
                                                 &formatDesc);

    if (!formatDesc)
        return;

    CMTime duration = CMTimeMake(1, (int32_t) fps);
    CMSampleTimingInfo timing = {duration, m_pts, m_pts};

    CMSampleBufferRef sampleBuffer = NULL;
    OSStatus status =
        CMSampleBufferCreateReadyWithImageBuffer(kCFAllocatorDefault,
                                                 pixelBuffer,
                                                 formatDesc,
                                                 &timing,
                                                 &sampleBuffer);
    CFRelease(formatDesc);

    if (status != noErr || !sampleBuffer)
        return;

    [self.streamSource.stream
     sendSampleBuffer: sampleBuffer
     discontinuity: (resync?
                         CMIOExtensionStreamDiscontinuityFlagUnknown:
                         CMIOExtensionStreamDiscontinuityFlagNone)
     hostTimeInNanoseconds: hostTimeNs];

    CFRelease(sampleBuffer);

    m_pts = CMTimeAdd(m_pts, duration);
    m_sequenceNumber++;
}

/* Allocates a VideoFrame at the active format's dimensions, fills it with
 * random pixel data, applies the current VideoAdjusts, and wraps the result
 * in a CVPixelBufferRef for delivery. Used as a last-resort fallback when
 * neither an external frame nor a loaded test picture is available.
 */
- (CVPixelBufferRef) randomFrame
{
    AkLogFunction();

    if (m_formats.empty())
        return NULL;

    AkVCam::VideoFormat format = m_formats[m_activeFormatIndex];
    AkVCam::VideoFrame frame(format);

    static std::uniform_int_distribution<int> distribution(0, 255);
    static std::default_random_engine engine;

    std::generate(frame.data(),
                  frame.data() + frame.size(),
                  [] () {
        return uint8_t(distribution(engine));
    });

    AkVCam::VideoFrame adjusted = m_videoAdjusts.adjust(frame);

    return adjusted? [self pixelBufferFromVideoFrame: adjusted]: NULL;
}

@end
