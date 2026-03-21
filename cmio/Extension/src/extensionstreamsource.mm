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

#import "extensionstreamsource.h"
#import "extensiondevicesource.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/logger.h"
#include "VCamUtils/src/videoformat.h"

@implementation ExtensionStreamSource

/* Initializes the stream source and stores a weak back-reference to the
 * parent device source.
 */
- (instancetype) initWithDeviceSource: (ExtensionDeviceSource *) deviceSource
{
    AkLogFunction();

    if (self = [super init])
        _deviceSource = deviceSource;

    return self;
}

// Declares the stream properties exposed to the CoreMediaIO framework.
- (NSSet<CMIOExtensionProperty> *) availableProperties
{
    AkLogFunction();

    return [NSSet setWithObjects:
            CMIOExtensionPropertyStreamActiveFormatIndex,
            CMIOExtensionPropertyStreamFrameDuration,
            nil];
}

/* Returns the current values for the requested stream properties.
 * Frame duration is derived from the first format reported by the device
 * source, falling back to 30 fps if none are available.
 */
- (nullable CMIOExtensionStreamProperties *)
    streamPropertiesForProperties: (NSSet<CMIOExtensionProperty> *) properties
                            error: (NSError **) outError
{
    AkLogFunction();

    CMIOExtensionStreamProperties *streamProperties =
        [CMIOExtensionStreamProperties streamPropertiesWithDictionary: @{}];

    if ([properties containsObject: CMIOExtensionPropertyStreamActiveFormatIndex])
        streamProperties.activeFormatIndex = @(self.deviceSource.activeFormatIndex);

    if ([properties containsObject: CMIOExtensionPropertyStreamFrameDuration]) {
        CMTime duration = [self primaryFrameDuration];
        NSDictionary *dict =
            CFBridgingRelease(CMTimeCopyAsDictionary(duration,
                                                     kCFAllocatorDefault));
        streamProperties.frameDuration = dict;
    }

    return streamProperties;
}

/* Handles writable stream property updates from the framework. When the
 * client requests a different activeFormatIndex, the device source is
 * notified so it can restart the timer at the new frame rate and update
 * the video converter's output format.
 */
- (BOOL) setStreamProperties: (CMIOExtensionStreamProperties *) streamProperties
                       error: (NSError **) outError
{
    AkLogFunction();

    if (streamProperties.activeFormatIndex) {
        NSUInteger index = streamProperties.activeFormatIndex.unsignedIntegerValue;
        [self.deviceSource setActiveFormatIndex: index];
    }

    return YES;
}

/* Converts each AkVCam::VideoFormat reported by the device source into a
 * CMIOExtensionStreamFormat and returns the full list.
 */
- (NSArray<CMIOExtensionStreamFormat *> *) formats
{
    AkLogFunction();

    NSMutableArray<CMIOExtensionStreamFormat *> *result =
        [NSMutableArray array];

    auto akFormats = self.deviceSource.formats;

    for (auto &akFmt: akFormats) {
        double fps = akFmt.fps().value();

        if (fps <= 0.0)
            fps = 30.0;

        OSType fourcc = formatToCM(AkVCam::PixelFormat(akFmt.format()));

        CMVideoFormatDescriptionRef formatDesc = NULL;
        OSStatus status =
            CMVideoFormatDescriptionCreate(kCFAllocatorDefault,
                                           fourcc,
                                           akFmt.width(),
                                           akFmt.height(),
                                           NULL,
                                           &formatDesc);

        if (status != noErr || !formatDesc)
            continue;

        CMTime frameDuration = CMTimeMake(1, (int32_t) fps);

        CMIOExtensionStreamFormat *fmt =
            [CMIOExtensionStreamFormat
             streamFormatWithFormatDescription: (CMFormatDescriptionRef) formatDesc
             maxFrameDuration: frameDuration
             minFrameDuration: frameDuration
             validFrameDurations: nil];

        CFRelease(formatDesc);
        [result addObject: fmt];
    }

    return result;
}

/* Always returns YES for a virtual camera. Physical devices should delegate
 * to AVCaptureDevice authorization instead.
 */
- (BOOL) authorizedToStartStreamForClient: (CMIOExtensionClient *) client
{
    AkLogFunction();

    return YES;
}

// Starts the device source's frame generation timer.
- (BOOL) startStreamAndReturnError: (NSError **) outError
{
    AkLogFunction();
    [self.deviceSource startStreaming];

    return YES;
}

// Stops the device source's frame generation timer.
- (BOOL) stopStreamAndReturnError: (NSError **) outError
{
    AkLogFunction();
    [self.deviceSource stopStreaming];

    return YES;
}

/* Returns the CMTime frame duration for the first format in the device
 * source's format list. Falls back to 1/30 s if the list is empty or the
 * fps value is non-positive.
 */
- (CMTime) primaryFrameDuration
{
    auto akFormats = self.deviceSource.formats;
    NSUInteger index = self.deviceSource.activeFormatIndex;

    if (index < akFormats.size()) {
        double fps = akFormats[index].fps().value();

        if (fps > 0.0)
            return CMTimeMake(1, (int32_t) fps);
    }

    return CMTimeMake(1, 30);
}

@end
