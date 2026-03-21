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
#include "PlatformUtils/src/preferences.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/logger.h"
#include "VCamUtils/src/videoformat.h"

@interface ExtensionProviderSource () {
    AkVCam::IpcBridgePtr m_ipcBridge;

    // Maps deviceId to ExtensionDeviceSource for all currently registered devices.
    NSMutableDictionary<NSString *, ExtensionDeviceSource *> *m_deviceSources;
}
@end

@implementation ExtensionProviderSource

/* Creates the IpcBridge, subscribes to its four callbacks, starts
 * notifications, and registers one ExtensionDeviceSource for each device
 * the daemon already knows about.
 */
- (instancetype) init
{
    AkLogFunction();

    if (self = [super init]) {
        m_deviceSources = [NSMutableDictionary dictionary];
        m_ipcBridge = std::make_shared<AkVCam::IpcBridge>();

        m_ipcBridge->connectDevicesChanged((__bridge void *) self,
                                          ExtensionProviderSource::devicesChangedCallback);
        m_ipcBridge->connectFrameReady((__bridge void *) self,
                                      ExtensionProviderSource::frameReadyCallback);
        m_ipcBridge->connectPictureChanged((__bridge void *) self,
                                          ExtensionProviderSource::pictureChangedCallback);
        m_ipcBridge->connectControlsChanged((__bridge void *) self,
                                           ExtensionProviderSource::controlsChangedCallback);

        m_ipcBridge->startNotifications();

        for (auto &deviceId: m_ipcBridge->devices()) {
            auto description = m_ipcBridge->description(deviceId);
            auto formats     = m_ipcBridge->formats(deviceId);
            [self createDeviceWithId: deviceId
                         description: description
                             formats: formats];
        }
    }

    return self;
}

- (void) dealloc
{
    AkLogFunction();
    m_ipcBridge->stopNotifications();
}

/* Allocates an ExtensionDeviceSource, wires it to a new CMIOExtensionStream
 * and CMIOExtensionDevice, then registers the device with the provider.
 * Does nothing if a device with that ID already exists.
 */
- (void) createDeviceWithId: (const std::string &) deviceId
                description: (const std::string &) description
                    formats: (const std::vector<AkVCam::VideoFormat> &) formats
{
    AkLogFunction();

    NSString *deviceIdStr = @(deviceId.c_str());

    if (m_deviceSources[deviceIdStr]) {
        AkLogWarning("Device already registered: %s", deviceId.c_str());
        return;
    }

    ExtensionDeviceSource *deviceSource =
        [[ExtensionDeviceSource alloc] initWithDeviceId: deviceId
                                            description: description
                                                formats: formats];
    [deviceSource setBridge: m_ipcBridge];

    /* Derive a deterministic stream UUID from the device UUID by rotating
     * the first byte. This keeps the stream ID stable across restarts while
     * remaining unique per device.
     */
    NSUUID *deviceUUID = [[NSUUID alloc] initWithUUIDString: deviceIdStr]
                         ?: [[NSUUID alloc] init];
    uuid_t uuidBytes;
    [deviceUUID getUUIDBytes: uuidBytes];
    uuidBytes[0] ^= 0x01;
    NSUUID *streamUUID = [[NSUUID alloc] initWithUUIDBytes: uuidBytes];

    CMIOExtensionStream *stream =
        [[CMIOExtensionStream alloc]
         initWithLocalizedName: @"Video"
         streamID: streamUUID
         direction: CMIOExtensionStreamDirectionSource
         clockType: CMIOExtensionStreamClockTypeHostTime
         source: deviceSource.streamSource];

    deviceSource.streamSource.stream = stream;

    CMIOExtensionDevice *device =
        [[CMIOExtensionDevice alloc]
         initWithLocalizedName: @(description.c_str())
         deviceID: deviceUUID
         legacyDeviceID: deviceIdStr
         source: deviceSource];

    deviceSource.device = device;

    NSError *error = nil;
    [device addStream: stream error: &error];

    if (error) {
        AkPrintErr("Error adding stream to device %s: %s",
                   deviceId.c_str(),
                   [[error description] UTF8String]);
        return;
    }

    m_deviceSources[deviceIdStr] = deviceSource;

    if (self.provider) {
        error = nil;
        [self.provider addDevice: device error: &error];

        if (error)
            AkPrintErr("Error registering device %s with provider: %s",
                       deviceId.c_str(),
                       [[error description] UTF8String]);
    }
}

/* Stops any active stream, removes the device from the provider, and
 * discards the ExtensionDeviceSource for the given device ID.
 */
- (void) destroyDeviceWithId: (const std::string &) deviceId
{
    AkLogFunction();

    NSString *deviceIdStr = @(deviceId.c_str());
    ExtensionDeviceSource *deviceSource = m_deviceSources[deviceIdStr];

    if (!deviceSource) {
        AkLogWarning("Device not found: %s", deviceId.c_str());
        return;
    }

    [deviceSource stopStreaming];

    if (self.provider && deviceSource.device) {
        NSError *error = nil;
        [self.provider removeDevice: deviceSource.device error: &error];

        if (error)
            AkPrintErr("Error removing device %s from provider: %s",
                       deviceId.c_str(),
                       [[error description] UTF8String]);
    }

    [m_deviceSources removeObjectForKey: deviceIdStr];
}

/* Destroys all existing devices and recreates them from the updated device
 * list reported by the daemon.
 */
static void devicesChangedCallback(void *userData,
                                   const std::vector<std::string> &devices)
{
    AkLogFunction();

    auto self = (__bridge ExtensionProviderSource *) userData;

    // Collect IDs currently registered.
    NSArray<NSString *> *existing =
        [self->m_deviceSources.allKeys copy];

    for (NSString *deviceIdStr in existing) {
        std::string deviceId = [deviceIdStr UTF8String];
        [self destroyDeviceWithId: deviceId];
    }

    for (auto &deviceId: self->m_ipcBridge->devices()) {
        auto description = self->m_ipcBridge->description(deviceId);
        auto formats     = self->m_ipcBridge->formats(deviceId);
        [self createDeviceWithId: deviceId
                     description: description
                         formats: formats];
    }
}

/* Forwards an incoming frame to the ExtensionDeviceSource identified by
 * deviceId.
 */
static void frameReadyCallback(void *userData,
                               const std::string &deviceId,
                               const AkVCam::VideoFrame &frame,
                               bool isActive)
{
    AkLogFunction();

    auto self = (__bridge ExtensionProviderSource *) userData;
    NSString *deviceIdStr = @(deviceId.c_str());
    ExtensionDeviceSource *deviceSource = self->m_deviceSources[deviceIdStr];

    if (deviceSource)
        [deviceSource frameReady: frame isActive: isActive];
}

// Forwards the new picture path to every registered device source.
static void pictureChangedCallback(void *userData,
                                   const std::string &picture)
{
    AkLogFunction();

    auto self = (__bridge ExtensionProviderSource *) userData;

    for (ExtensionDeviceSource *deviceSource in self->m_deviceSources.allValues)
        [deviceSource setPicture: picture];
}

// Forwards updated controls to the device source identified by deviceId.
static void controlsChangedCallback(void *userData,
                                    const std::string &deviceId,
                                    const std::map<std::string, int> &controls)
{
    AkLogFunction();

    auto self = (__bridge ExtensionProviderSource *) userData;
    NSString *deviceIdStr = @(deviceId.c_str());
    ExtensionDeviceSource *deviceSource = self->m_deviceSources[deviceIdStr];

    if (deviceSource)
        [deviceSource setControls: controls];
}

// Declares the provider-level properties exposed to the CoreMediaIO framework.
- (NSSet<CMIOExtensionProperty> *) availableProperties
{
    AkLogFunction();

    return [NSSet setWithObject: CMIOExtensionPropertyProviderManufacturer];
}

// Returns the current values for the requested provider properties.
- (nullable CMIOExtensionProviderProperties *) providerPropertiesForProperties: (NSSet<CMIOExtensionProperty> *) properties
                                               error: (NSError **) outError
{
    AkLogFunction();

    CMIOExtensionProviderProperties *providerProperties =
        [CMIOExtensionProviderProperties providerPropertiesWithDictionary: @{}];

    if ([properties containsObject: CMIOExtensionPropertyProviderManufacturer])
        providerProperties.manufacturer = @COMMONS_APPNAME;

    return providerProperties;
}

- (BOOL) setProviderProperties: (CMIOExtensionProviderProperties *) providerProperties
                         error: (NSError **) outError
{
    AkLogFunction();

    return YES;
}

/* Called when a client application connects to the provider.
 * Returns YES to accept all connections.
 */
- (BOOL) connectClient: (CMIOExtensionClient *) client
                 error: (NSError * _Nullable *) outError
{
    AkLogFunction();

    return YES;
}

// Called when a client application disconnects from the provider.
- (void) disconnectClient: (CMIOExtensionClient *) client
{
    AkLogFunction();

    const char *clientDesc = client? [[client description] UTF8String]: "nil";
    AkPrintOut("Client disconnected: %s\n", clientDesc);
}

@end
