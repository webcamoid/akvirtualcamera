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

#import "extensionprovidersource.h"
#import "extensiondevicesource.h"
#import "extensionstreamsource.h"

// Fixed UUID identifying this virtual camera device. Generate your own with uuidgen.
static NSString * const kDeviceUUID = @"A1B2C3D4-E5F6-7890-ABCD-EF1234567890";

// Display name shown in FaceTime, QuickTime, and other camera clients.
static NSString * const kDeviceName = @COMMONS_APPNAME;

@interface ExtensionProviderSource () {
    ExtensionDeviceSource *_deviceSource;
}
@end

@implementation ExtensionProviderSource

/* init
 *
 * Initializes the provider source and creates the virtual device.
 */
- (instancetype) init {
    if (self = [super init]) {
        [self setupDevice];
    }

    return self;
}

/* setupDevice
 *
 * Creates the CMIOExtensionDevice and its associated stream, wires them
 * together, and stores references for later registration with the provider.
 * The device is not added to the provider here; that happens in main.mm
 * after the provider instance exists.
 */
- (void) setupDevice {
    _deviceSource = [[ExtensionDeviceSource alloc] init];

    CMIOExtensionStream *stream =
        [[CMIOExtensionStream alloc]
          initWithLocalizedName: @"Video"
          streamID: [[NSUUID alloc]
                     initWithUUIDString: @"B2C3D4E5-F6A7-8901-BCDE-F12345678901"]
          direction: CMIOExtensionStreamDirectionSource
          clockType: CMIOExtensionStreamClockTypeHostTime
          source: _deviceSource.streamSource];

    _deviceSource.streamSource.stream = stream;

    /* legacyDeviceID lets AVCaptureDevice keep the same uniqueIdentifier
     * that was used by the previous DAL plug-in, preserving app preferences
     * that reference the device by ID.
     */
    CMIOExtensionDevice *device =
        [[CMIOExtensionDevice alloc]
          initWithLocalizedName: kDeviceName
          deviceID: [[NSUUID alloc] initWithUUIDString: kDeviceUUID]
          legacyDeviceID: kDeviceUUID
          source: _deviceSource];

    _deviceSource.device = device;

    NSError *error = nil;
    [device addStream:stream error: &error];

    if (error)
        NSLog(@"Error adding stream: %@", error);
}

/* deviceSource
 *
 * Returns the device source so that main.mm can register it with the provider.
 */
- (ExtensionDeviceSource *) deviceSource {
    return _deviceSource;
}

/* availableProperties
 *
 * Declares the provider-level properties exposed to the CoreMediaIO framework.
 */
- (NSSet<CMIOExtensionProperty> *) availableProperties {
    return [NSSet setWithObject: CMIOExtensionPropertyProviderManufacturer];
}

/* providerPropertiesForProperties:error:
 *
 * Returns the current values for the requested provider properties.
 */
- (nullable CMIOExtensionProviderProperties *) providerPropertiesForProperties: (NSSet<CMIOExtensionProperty> *) properties
                                               error: (NSError **) outError {
    CMIOExtensionProviderProperties *providerProperties =
        [CMIOExtensionProviderProperties providerPropertiesWithDictionary: @{}];

    if ([properties containsObject:CMIOExtensionPropertyProviderManufacturer])
        providerProperties.manufacturer = @COMMONS_APPNAME;

    return providerProperties;
}

- (BOOL) setProviderProperties: (CMIOExtensionProviderProperties *) providerProperties
         error: (NSError **) outError {
    return YES;
}

/* connectClient:error:
 *
 * Called when a client application connects to the provider.
 * Returns YES to accept all connections.
 */
- (BOOL) connectClient: (CMIOExtensionClient *) client
         error: (NSError * _Nullable *) outError {
    return YES;
}

/* disconnectClient:
 *
 * Called when a client application disconnects from the provider.
 */
- (void) disconnectClient: (CMIOExtensionClient *) client {
    NSLog(@"Client disconnected: %@", client);
}

@end
