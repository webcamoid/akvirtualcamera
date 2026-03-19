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

#import <time.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <CoreAudio/AudioHardwareBase.h>

#import "extensiondevicesource.h"
#import "extensionstreamsource.h"

static const int    kDevWidth  = 1280;
static const int    kDevHeight = 720;
static const double kDevFPS    = 30.0;

@interface ExtensionDeviceSource () {
    dispatch_source_t _timer;
    uint64_t          _sequenceNumber;
    CMTime            _pts;
    CVPixelBufferRef  _currentBuffer;
    BOOL              _externalFrame;
    dispatch_queue_t  _frameQueue;
}
@end

@implementation ExtensionDeviceSource

/* init
 *
 * Creates the stream source, initializes timing state, and sets up the
 * serial dispatch queue used for frame delivery.
 */
- (instancetype) init {
    if (self = [super init]) {
        _streamSource = [[ExtensionStreamSource alloc]
                        initWithDeviceSource: self];
        _sequenceNumber = 0;
        _pts = kCMTimeInvalid;
        _currentBuffer = NULL;
        _externalFrame = NO;
        _frameQueue = dispatch_queue_create(BUNDLE_ID ".framequeue",
                                             DISPATCH_QUEUE_SERIAL);
    }

    return self;
}

- (void) dealloc {
    [self stopStreaming];

    if (_currentBuffer) {
        CVPixelBufferRelease(_currentBuffer);
        _currentBuffer = NULL;
    }
}

/* availableProperties
 *
 * Declares the device properties exposed to the CoreMediaIO framework.
 */
- (NSSet<CMIOExtensionProperty> *) availableProperties {
    return [NSSet setWithObjects:
                  CMIOExtensionPropertyDeviceTransportType,
                  CMIOExtensionPropertyDeviceModel,
                  nil];
}

/* devicePropertiesForProperties:error:
 *
 * Returns the current values for the requested device properties.
 */
- (nullable CMIOExtensionDeviceProperties *) devicePropertiesForProperties: (NSSet<CMIOExtensionProperty> *) properties
                                              error: (NSError **) outError {
    CMIOExtensionDeviceProperties *deviceProperties =
        [CMIOExtensionDeviceProperties devicePropertiesWithDictionary: @{}];

    if ([properties containsObject:CMIOExtensionPropertyDeviceTransportType])
        deviceProperties.transportType = @(kAudioDeviceTransportTypeVirtual);

    if ([properties containsObject:CMIOExtensionPropertyDeviceModel])
        deviceProperties.model = @COMMONS_APPNAME;

    return deviceProperties;
}

- (BOOL) setDeviceProperties: (CMIOExtensionDeviceProperties *) deviceProperties
         error: (NSError **) outError {
    return YES;
}

/* startStreaming
 *
 * Creates and starts a GCD timer that fires at the configured frame rate,
 * driving the stream loop. Resets the sequence counter and PTS on each call.
 */
- (void) startStreaming {
    if (_timer)
        return;

    _sequenceNumber = 0;
    _pts = kCMTimeInvalid;

    uint64_t intervalNs = (uint64_t) (NSEC_PER_SEC / kDevFPS);

    _timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,
                                    0,
                                    0,
                                    _frameQueue);
    dispatch_source_set_timer(_timer,
                              dispatch_time(DISPATCH_TIME_NOW, 0),
                              intervalNs,
                              intervalNs / 10);   // 10% leeway

    __weak typeof(self) weakSelf = self;
    dispatch_source_set_event_handler(_timer, ^{
        [weakSelf streamLoop];
    });

    dispatch_resume(_timer);
}

/* stopStreaming
 *
 * Cancels the GCD timer and releases it.
 */
- (void) stopStreaming {
    if (_timer) {
        dispatch_source_cancel(_timer);
        _timer = nil;
    }
}

/* frameReady:
 *
 * Receives an external pixel buffer from the IpcBridge and stores it for
 * the next stream loop iteration. The previous buffer is released before
 * the new one is retained.
 */
- (void) frameReady: (CVPixelBufferRef) pixelBuffer {
    dispatch_async(_frameQueue, ^{
        if (self->_currentBuffer)
            CVPixelBufferRelease(self->_currentBuffer);

        self->_currentBuffer = CVPixelBufferRetain(pixelBuffer);
        self->_externalFrame = YES;
    });
}

/* streamLoop
 *
 * Called on every timer tick. Selects the current pixel buffer — either
 * an external frame delivered via frameReady: or an internally generated
 * test frame — and forwards it to sendPixelBuffer:.
 */
- (void) streamLoop {
    CVPixelBufferRef pixelBuffer = NULL;

    if (_externalFrame && _currentBuffer)
        pixelBuffer = CVPixelBufferRetain(_currentBuffer);
    else
        pixelBuffer = [self generateTestFrame];


    if (!pixelBuffer)
        return;

    [self sendPixelBuffer: pixelBuffer];
    CVPixelBufferRelease(pixelBuffer);
}

/* sendPixelBuffer:
 *
 * Wraps a CVPixelBufferRef in a CMSampleBuffer with proper timing info and
 * sends it to the CoreMediaIO framework via the extension stream API.
 * Handles PTS initialization and resync detection on each call.
 */
- (void) sendPixelBuffer: (CVPixelBufferRef) pixelBuffer {
    uint64_t hostTimeNs = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);

    CMTime pts = CMTimeMake((int64_t)hostTimeNs, (int32_t)NSEC_PER_SEC);
    BOOL resync = NO;

    if (CMTIME_IS_INVALID(_pts)) {
        resync = YES;
    } else {
        double diff = CMTimeGetSeconds(CMTimeSubtract(_pts, pts));

        if (diff > 2.0 / kDevFPS)
            resync = YES;
    }

    if (resync)
        _pts = pts;

    CMVideoFormatDescriptionRef formatDesc = NULL;
    CMVideoFormatDescriptionCreateForImageBuffer(kCFAllocatorDefault,
                                                 pixelBuffer,
                                                 &formatDesc);

    if (!formatDesc)
        return;

    CMTime duration = CMTimeMake(1, (int32_t) kDevFPS);
    CMSampleTimingInfo timing = {duration, _pts, _pts};

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
    hostTimeInNanoseconds:hostTimeNs];

    CFRelease(sampleBuffer);

    _pts = CMTimeAdd(_pts, duration);
    _sequenceNumber++;
}

/* generateTestFrame
 *
 * Allocates a 32-bit BGRA pixel buffer and draws a white horizontal line
 * that bounces vertically, useful for verifying the stream without an
 * external video source.
 */
- (CVPixelBufferRef) generateTestFrame {
    CVPixelBufferRef buffer = NULL;
    CVPixelBufferCreate(kCFAllocatorDefault,
                        kDevWidth, kDevHeight,
                        kCVPixelFormatType_32BGRA,
                        NULL,
                        &buffer);

    if (!buffer)
        return NULL;

    CVPixelBufferLockBaseAddress(buffer, 0);
    uint8_t *base = (uint8_t *) CVPixelBufferGetBaseAddress(buffer);
    size_t bytesPerRow = CVPixelBufferGetBytesPerRow(buffer);

    memset(base, 0, bytesPerRow * kDevHeight);

    static int lineY = 0;
    static int direction = 1;
    lineY += direction * 4;

    if (lineY >= kDevHeight - 1 || lineY <= 0)
        direction = -direction;

    int row = MAX(0, MIN(kDevHeight - 1, lineY));
    uint8_t *rowPtr = base + row * bytesPerRow;

    for (int x = 0; x < kDevWidth; ++x) {
        rowPtr[x * 4 + 0] = 255;   // B
        rowPtr[x * 4 + 1] = 255;   // G
        rowPtr[x * 4 + 2] = 255;   // R
        rowPtr[x * 4 + 3] = 255;   // A
    }

    CVPixelBufferUnlockBaseAddress(buffer, 0);

    return buffer;
}

@end
