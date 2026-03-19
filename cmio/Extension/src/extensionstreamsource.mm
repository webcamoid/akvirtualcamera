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

// Pixel format and dimensions exposed by this stream.
static const int    kStreamWidth  = 1280;
static const int    kStreamHeight = 720;
static const double kStreamFPS    = 30.0;

@implementation ExtensionStreamSource

/* initWithDeviceSource:
 *
 * Initializes the stream source and stores a weak back-reference to the
 * parent device source.
 */
- (instancetype) initWithDeviceSource: (ExtensionDeviceSource *) deviceSource {
    if (self = [super init])
        _deviceSource = deviceSource;

    return self;
}

/* availableProperties
 *
 * Declares the stream properties exposed to the CoreMediaIO framework.
 */
- (NSSet<CMIOExtensionProperty> *) availableProperties {
    return [NSSet setWithObjects:
            CMIOExtensionPropertyStreamActiveFormatIndex,
            CMIOExtensionPropertyStreamFrameDuration,
            nil];
}

/* streamPropertiesForProperties:error:
 *
 * Returns the current values for the requested stream properties.
 */
- (nullable CMIOExtensionStreamProperties *) streamPropertiesForProperties: (NSSet<CMIOExtensionProperty> *)properties
                                             error: (NSError **) outError {
    CMIOExtensionStreamProperties *streamProperties =
        [CMIOExtensionStreamProperties streamPropertiesWithDictionary:@{}];

    if ([properties containsObject:CMIOExtensionPropertyStreamActiveFormatIndex])
        streamProperties.activeFormatIndex = @(0);

    if ([properties containsObject:CMIOExtensionPropertyStreamFrameDuration]) {
        CMTime duration = CMTimeMake(1, (int32_t) kStreamFPS);
        NSDictionary *dict = CFBridgingRelease(CMTimeCopyAsDictionary(duration,
                                                                      kCFAllocatorDefault));
        streamProperties.frameDuration = dict;
    }

    return streamProperties;
}

/* setStreamProperties:error:
 *
 * Handles writable stream property updates. Read activeFormatIndex here
 * to switch formats if multiple video formats are supported.
 */
- (BOOL) setStreamProperties: (CMIOExtensionStreamProperties *) streamProperties
         error: (NSError **) outError {
    return YES;
}

/* formats
 *
 * Returns the list of video formats advertised by this stream.
 * Currently exposes a single 32-bit BGRA format at the configured
 * resolution and frame rate.
 */
- (NSArray<CMIOExtensionStreamFormat *> *)formats {
    CMVideoFormatDescriptionRef formatDescription = NULL;
    CMVideoFormatDescriptionCreate(kCFAllocatorDefault,
                                   kCVPixelFormatType_32BGRA,
                                   kStreamWidth,
                                   kStreamHeight,
                                   NULL,
                                   &formatDescription);

    CMIOExtensionStreamFormat *format =
        [CMIOExtensionStreamFormat
         streamFormatWithFormatDescription: (CMFormatDescriptionRef) formatDescription
         maxFrameDuration: CMTimeMake(1, (int32_t) kStreamFPS)
         minFrameDuration: CMTimeMake(1, (int32_t) kStreamFPS)
         validFrameDurations: nil];

    CFRelease(formatDescription);

    return @[format];
}

/* authorizedToStartStreamForClient:
 *
 * Always returns YES for a virtual camera. For physical devices, delegate
 * to AVCaptureDevice authorization instead.
 */
- (BOOL) authorizedToStartStreamForClient: (CMIOExtensionClient *) client {
    return YES;
}

/* startStreamAndReturnError:
 *
 * Starts the device source's frame generation timer.
 */
- (BOOL) startStreamAndReturnError: (NSError **) outError {
    [self.deviceSource startStreaming];

    return YES;
}

/* stopStreamAndReturnError:
 *
 * Stops the device source's frame generation timer.
 */
- (BOOL) stopStreamAndReturnError: (NSError **) outError {
    [self.deviceSource stopStreaming];

    return YES;
}

@end
