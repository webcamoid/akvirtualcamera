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

#include <map>
#include <string>
#include <vector>

#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"

NS_ASSUME_NONNULL_BEGIN

@class ExtensionStreamSource;

@interface ExtensionDeviceSource: NSObject <CMIOExtensionDeviceSource>

// The CoreMediaIO device that owns this source.
@property (nonatomic, strong, nullable) CMIOExtensionDevice *device;

// The child stream source associated with this device.
@property (nonatomic, strong, readonly) ExtensionStreamSource *streamSource;

// The list of video formats this device was initialized with.
@property (nonatomic, readonly) std::vector<AkVCam::VideoFormat> formats;

// Index into formats[] identifying the currently active format.
@property (nonatomic, readonly) NSUInteger activeFormatIndex;

/* Switches the active format. If streaming is in progress, the timer is
 * restarted at the new frame rate. Out-of-range indices are ignored.
 */
- (void) setActiveFormatIndex: (NSUInteger) index;

/* Designated initializer. Stores the device identity and the list of
 * supported video formats used to configure the stream source and the
 * frame converter.
 *
 * @param deviceId    Unique identifier of the virtual device
 * @param description User-visible name/description of the device
 * @param formats     List of supported video formats
 */
- (instancetype) initWithDeviceId: (const std::string &) deviceId
                      description: (const std::string &) description
                          formats: (const std::vector<AkVCam::VideoFormat> &) formats;

/* Stores the IpcBridge shared pointer used to notify the daemon when the
 * stream starts or stops. Must be called before startStreaming.
 */
- (void) setBridge: (AkVCam::IpcBridgePtr) bridge;

/* Starts the frame generation timer. Called by the stream source when
 * the client begins consuming the stream.
 */
- (void) startStreaming;

/* Stops the frame generation timer. Called by the stream source when
 * the client stops consuming the stream.
 */
- (void) stopStreaming;

/* Handles a new video frame received from the IPC bridge.
 * Converts the frame, applies adjustments, and stores it for the next delivery.
 *
 * @param frame    The incoming video frame
 * @param isActive YES if this frame should be used (instead of fallback image)
 */
- (void) frameReady: (const AkVCam::VideoFrame &) frame
           isActive: (BOOL) isActive;

/* Loads the image at the given path and uses it as the static test frame
 * shown when no active VideoFrame is being received.
 */
- (void) setPicture: (const std::string &) picture;

/* Applies the given control values (hflip, vflip, swap_rgb, scaling,
 * aspect_ratio) to the video adjuster and converter.
 */
- (void) setControls: (const std::map<std::string, int> &) controls;

@end

NS_ASSUME_NONNULL_END
