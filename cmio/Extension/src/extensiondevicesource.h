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

#pragma once

#import <CoreMediaIO/CMIOExtensionDevice.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class ExtensionStreamSource;

@interface ExtensionDeviceSource: NSObject <CMIOExtensionDeviceSource>

// The CoreMediaIO device that owns this source.
@property (nonatomic, strong, nullable) CMIOExtensionDevice *device;

// The child stream source associated with this device.
@property (nonatomic, strong, readonly) ExtensionStreamSource *streamSource;

- (instancetype) init;

/* startStreaming
 *
 * Starts the frame generation timer. Called by the stream source when
 * the client begins consuming the stream.
 */
- (void) startStreaming;

/* stopStreaming
 *
 * Stops the frame generation timer. Called by the stream source when
 * the client stops consuming the stream.
 */
- (void) stopStreaming;

/* frameReady:
 *
 * Delivers an incoming frame to the device source. Call this from the
 * IpcBridge whenever a new VideoFrame arrives for this device.
 */
- (void) frameReady: (CVPixelBufferRef) pixelBuffer;

@end

NS_ASSUME_NONNULL_END
