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

#import <CoreMediaIO/CMIOExtensionStream.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class ExtensionDeviceSource;

@interface ExtensionStreamSource: NSObject <CMIOExtensionStreamSource>

// The CoreMediaIO stream that owns this source, assigned by the framework.
@property (nonatomic, strong, nullable) CMIOExtensionStream *stream;

// Back-reference to the parent device source.
@property (nonatomic, weak) ExtensionDeviceSource *deviceSource;

- (instancetype) initWithDeviceSource: (ExtensionDeviceSource *) deviceSource;

@end

NS_ASSUME_NONNULL_END
