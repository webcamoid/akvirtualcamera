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

#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <IOKit/audio/IOAudioTypes.h>

#include "plugininterface.h"
#include "utils.h"
#include "Assistant/src/assistantglobals.h"
#include "PlatformUtils/src/preferences.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/logger/logger.h"

namespace AkVCam
{
    struct PluginInterfacePrivate
    {
        public:
            CMIOHardwarePlugInInterface *pluginInterface;
            PluginInterface *self;
            ULONG m_ref;
            ULONG m_reserved;
            IpcBridge m_ipcBridge;

            void updateDevices();
            static HRESULT QueryInterface(void *self,
                                          REFIID uuid,
                                          LPVOID *interface);
            static ULONG AddRef(void *self);
            static ULONG Release(void *self);
            static OSStatus Initialize(CMIOHardwarePlugInRef self);
            static OSStatus InitializeWithObjectID(CMIOHardwarePlugInRef self,
                                                   CMIOObjectID objectID);
            static OSStatus Teardown(CMIOHardwarePlugInRef self);
            static void ObjectShow(CMIOHardwarePlugInRef self,
                                   CMIOObjectID objectID);
            static Boolean ObjectHasProperty(CMIOHardwarePlugInRef self,
                                             CMIOObjectID objectID,
                                             const CMIOObjectPropertyAddress *address);
            static OSStatus ObjectIsPropertySettable(CMIOHardwarePlugInRef self,
                                                     CMIOObjectID objectID,
                                                     const CMIOObjectPropertyAddress *address,
                                                     Boolean *isSettable);
            static OSStatus ObjectGetPropertyDataSize(CMIOHardwarePlugInRef self,
                                                      CMIOObjectID objectID,
                                                      const CMIOObjectPropertyAddress *address,
                                                      UInt32 qualifierDataSize,
                                                      const void *qualifierData,
                                                      UInt32 *dataSize);
            static OSStatus ObjectGetPropertyData(CMIOHardwarePlugInRef self,
                                                  CMIOObjectID objectID,
                                                  const CMIOObjectPropertyAddress *address,
                                                  UInt32 qualifierDataSize,
                                                  const void *qualifierData,
                                                  UInt32 dataSize,
                                                  UInt32 *dataUsed,
                                                  void *data);
            static OSStatus ObjectSetPropertyData(CMIOHardwarePlugInRef self,
                                                  CMIOObjectID objectID,
                                                  const CMIOObjectPropertyAddress *address,
                                                  UInt32 qualifierDataSize,
                                                  const void *qualifierData,
                                                  UInt32 dataSize,
                                                  const void *data);
            static OSStatus DeviceSuspend(CMIOHardwarePlugInRef self,
                                          CMIODeviceID device);
            static OSStatus DeviceResume(CMIOHardwarePlugInRef self,
                                         CMIODeviceID device);
            static OSStatus DeviceStartStream(CMIOHardwarePlugInRef self,
                                              CMIODeviceID device,
                                              CMIOStreamID stream);
            static OSStatus DeviceStopStream(CMIOHardwarePlugInRef self,
                                             CMIODeviceID device,
                                             CMIOStreamID stream);
            static OSStatus DeviceProcessAVCCommand(CMIOHardwarePlugInRef self,
                                                    CMIODeviceID device,
                                                    CMIODeviceAVCCommand *ioAVCCommand);
            static OSStatus DeviceProcessRS422Command(CMIOHardwarePlugInRef self,
                                                      CMIODeviceID device,
                                                      CMIODeviceRS422Command *ioRS422Command);
            static OSStatus StreamCopyBufferQueue(CMIOHardwarePlugInRef self,
                                                  CMIOStreamID stream,
                                                  CMIODeviceStreamQueueAlteredProc queueAlteredProc,
                                                  void *queueAlteredRefCon,
                                                  CMSimpleQueueRef *queue);
            static OSStatus StreamDeckPlay(CMIOHardwarePlugInRef self,
                                           CMIOStreamID stream);
            static OSStatus StreamDeckStop(CMIOHardwarePlugInRef self,
                                           CMIOStreamID stream);
            static OSStatus StreamDeckJog(CMIOHardwarePlugInRef self,
                                          CMIOStreamID stream,
                                          SInt32 speed);
            static OSStatus StreamDeckCueTo(CMIOHardwarePlugInRef self,
                                            CMIOStreamID stream,
                                            Float64 frameNumber,
                                            Boolean playOnCue);
    };
}

AkVCam::PluginInterface::PluginInterface():
    ObjectInterface(),
    m_objectID(0)
{
    this->m_className = "PluginInterface";
    this->d = new PluginInterfacePrivate;
    this->d->self = this;
    this->d->pluginInterface = new CMIOHardwarePlugInInterface {
        //	Padding for COM
        NULL,

        // IUnknown Routines
        PluginInterfacePrivate::QueryInterface,
        PluginInterfacePrivate::AddRef,
        PluginInterfacePrivate::Release,

        // DAL Plug-In Routines
        PluginInterfacePrivate::Initialize,
        PluginInterfacePrivate::InitializeWithObjectID,
        PluginInterfacePrivate::Teardown,
        PluginInterfacePrivate::ObjectShow,
        PluginInterfacePrivate::ObjectHasProperty,
        PluginInterfacePrivate::ObjectIsPropertySettable,
        PluginInterfacePrivate::ObjectGetPropertyDataSize,
        PluginInterfacePrivate::ObjectGetPropertyData,
        PluginInterfacePrivate::ObjectSetPropertyData,
        PluginInterfacePrivate::DeviceSuspend,
        PluginInterfacePrivate::DeviceResume,
        PluginInterfacePrivate::DeviceStartStream,
        PluginInterfacePrivate::DeviceStopStream,
        PluginInterfacePrivate::DeviceProcessAVCCommand,
        PluginInterfacePrivate::DeviceProcessRS422Command,
        PluginInterfacePrivate::StreamCopyBufferQueue,
        PluginInterfacePrivate::StreamDeckPlay,
        PluginInterfacePrivate::StreamDeckStop,
        PluginInterfacePrivate::StreamDeckJog,
        PluginInterfacePrivate::StreamDeckCueTo
    };
    this->d->m_ref = 0;
    this->d->m_reserved = 0;

    this->d->m_ipcBridge.connectService();
    this->d->m_ipcBridge.connectServerStateChanged(this, &PluginInterface::serverStateChanged);
    this->d->m_ipcBridge.connectDeviceAdded(this, &PluginInterface::deviceAdded);
    this->d->m_ipcBridge.connectDeviceRemoved(this, &PluginInterface::deviceRemoved);
    this->d->m_ipcBridge.connectDevicesUpdated(this, &PluginInterface::devicesUpdated);
    this->d->m_ipcBridge.connectFrameReady(this, &PluginInterface::frameReady);
    this->d->m_ipcBridge.connectBroadcastingChanged(this, &PluginInterface::setBroadcasting);
    this->d->m_ipcBridge.connectMirrorChanged(this, &PluginInterface::setMirror);
    this->d->m_ipcBridge.connectScalingChanged(this, &PluginInterface::setScaling);
    this->d->m_ipcBridge.connectAspectRatioChanged(this, &PluginInterface::setAspectRatio);
    this->d->m_ipcBridge.connectSwapRgbChanged(this, &PluginInterface::setSwapRgb);
}

AkVCam::PluginInterface::~PluginInterface()
{
    this->d->m_ipcBridge.disconnectService();
    delete this->d->pluginInterface;
    delete this->d;
}

CMIOObjectID AkVCam::PluginInterface::objectID() const
{
    return this->m_objectID;
}

CMIOHardwarePlugInRef AkVCam::PluginInterface::create()
{
    AkLogFunction();

    auto pluginInterface = new PluginInterface();
    pluginInterface->d->AddRef(pluginInterface->d);

    return reinterpret_cast<CMIOHardwarePlugInRef>(pluginInterface->d);
}

AkVCam::Object *AkVCam::PluginInterface::findObject(CMIOObjectID objectID)
{
    for (auto device: this->m_devices)
        if (auto object = device->findObject(objectID))
            return object;

    return nullptr;
}

HRESULT AkVCam::PluginInterface::QueryInterface(REFIID uuid, LPVOID *interface)
{
    AkLogFunction();

    if (!interface)
        return E_POINTER;

    if (uuidEqual(uuid, kCMIOHardwarePlugInInterfaceID)
        || uuidEqual(uuid, IUnknownUUID)) {
        AkLogInfo() << "Found plugin interface." << std::endl;
        this->d->AddRef(this->d);
        *interface = this->d;

        return S_OK;
    }

    return E_NOINTERFACE;
}

OSStatus AkVCam::PluginInterface::Initialize()
{
    AkLogFunction();

    return this->InitializeWithObjectID(kCMIOObjectUnknown);
}

OSStatus AkVCam::PluginInterface::InitializeWithObjectID(CMIOObjectID objectID)
{
    AkLogFunction();
    AkLogInfo() << objectID << std::endl;

    this->m_objectID = objectID;

    for (auto &deviceId: this->d->m_ipcBridge.devices())
        this->deviceAdded(this, deviceId);

    return kCMIOHardwareNoError;
}

OSStatus AkVCam::PluginInterface::Teardown()
{
    AkLogFunction();

    return kCMIOHardwareNoError;
}

void AkVCam::PluginInterface::serverStateChanged(void *userData,
                                                 IpcBridge::ServerState state)
{
    AkLogFunction();
    auto self = reinterpret_cast<PluginInterface *>(userData);

    for (auto device: self->m_devices)
        device->serverStateChanged(state);

    if (state == IpcBridge::ServerStateAvailable)
        self->d->updateDevices();
}

void AkVCam::PluginInterface::deviceAdded(void *userData,
                                          const std::string &deviceId)
{
    AkLogFunction();
    AkLogInfo() << "Device Added: " << deviceId << std::endl;

    auto self = reinterpret_cast<PluginInterface *>(userData);
    auto description = self->d->m_ipcBridge.description(deviceId);
    auto formats = self->d->m_ipcBridge.formats(deviceId);
    self->createDevice(deviceId, description, formats);
}

void AkVCam::PluginInterface::deviceRemoved(void *userData,
                                            const std::string &deviceId)
{
    AkLogFunction();
    AkLogInfo() << "Device Removed: " << deviceId << std::endl;

    auto self = reinterpret_cast<PluginInterface *>(userData);
    self->destroyDevice(deviceId);
}

void AkVCam::PluginInterface::devicesUpdated(void *userData, void *unused)
{
    UNUSED(unused);
    AkLogFunction();
    auto self = reinterpret_cast<PluginInterface *>(userData);
    std::vector<std::string> devices;

    for (auto &device: self->m_devices) {
        std::string deviceId;
        device->properties().getProperty(kCMIODevicePropertyDeviceUID,
                                         &deviceId);
        devices.push_back(deviceId);
    }

    for (auto &deviceId: devices)
        self->destroyDevice(deviceId);

    for (auto &deviceId: self->d->m_ipcBridge.devices()) {
        auto description = self->d->m_ipcBridge.description(deviceId);
        auto formats = self->d->m_ipcBridge.formats(deviceId);
        self->createDevice(deviceId, description, formats);
    }
}

void AkVCam::PluginInterface::frameReady(void *userData,
                                         const std::string &deviceId,
                                         const VideoFrame &frame)
{
    AkLogFunction();
    auto self = reinterpret_cast<PluginInterface *>(userData);

    for (auto device: self->m_devices)
        if (device->deviceId() == deviceId)
            device->frameReady(frame);
}

void AkVCam::PluginInterface::setBroadcasting(void *userData,
                                              const std::string &deviceId,
                                              const std::string &broadcaster)
{
    AkLogFunction();
    AkLogInfo() << "Device: " << deviceId << std::endl;
    AkLogInfo() << "Broadcaster: " << broadcaster << std::endl;
    auto self = reinterpret_cast<PluginInterface *>(userData);

    for (auto device: self->m_devices)
        if (device->deviceId() == deviceId)
            device->setBroadcasting(broadcaster);
}

void AkVCam::PluginInterface::setMirror(void *userData,
                                        const std::string &deviceId,
                                        bool horizontalMirror,
                                        bool verticalMirror)
{
    AkLogFunction();
    auto self = reinterpret_cast<PluginInterface *>(userData);

    for (auto device: self->m_devices)
        if (device->deviceId() == deviceId)
            device->setMirror(horizontalMirror, verticalMirror);
}

void AkVCam::PluginInterface::setScaling(void *userData,
                                         const std::string &deviceId,
                                         Scaling scaling)
{
    AkLogFunction();
    auto self = reinterpret_cast<PluginInterface *>(userData);

    for (auto device: self->m_devices)
        if (device->deviceId() == deviceId)
            device->setScaling(scaling);
}

void AkVCam::PluginInterface::setAspectRatio(void *userData,
                                             const std::string &deviceId,
                                             AspectRatio aspectRatio)
{
    AkLogFunction();
    auto self = reinterpret_cast<PluginInterface *>(userData);

    for (auto device: self->m_devices)
        if (device->deviceId() == deviceId)
            device->setAspectRatio(aspectRatio);
}

void AkVCam::PluginInterface::setSwapRgb(void *userData,
                                         const std::string &deviceId,
                                         bool swap)
{
    AkLogFunction();
    auto self = reinterpret_cast<PluginInterface *>(userData);

    for (auto device: self->m_devices)
        if (device->deviceId() == deviceId)
            device->setSwapRgb(swap);
}

void AkVCam::PluginInterface::addListener(void *userData,
                                          const std::string &deviceId)
{
    AkLogFunction();
    auto self = reinterpret_cast<PluginInterface *>(userData);
    self->d->m_ipcBridge.addListener(deviceId);
}

void AkVCam::PluginInterface::removeListener(void *userData,
                                             const std::string &deviceId)
{
    AkLogFunction();
    auto self = reinterpret_cast<PluginInterface *>(userData);
    self->d->m_ipcBridge.removeListener(deviceId);
}

bool AkVCam::PluginInterface::createDevice(const std::string &deviceId,
                                           const std::wstring &description,
                                           const std::vector<VideoFormat> &formats)
{
    AkLogFunction();
    StreamPtr stream;

    // Create one device.
    auto pluginRef = reinterpret_cast<CMIOHardwarePlugInRef>(this->d);
    auto device = std::make_shared<Device>(pluginRef, false);
    device->setDeviceId(deviceId);
    device->connectAddListener(this, &PluginInterface::addListener);
    device->connectRemoveListener(this, &PluginInterface::removeListener);
    this->m_devices.push_back(device);

    // Define device properties.
    device->properties().setProperty(kCMIOObjectPropertyName,
                                     description);
    device->properties().setProperty(kCMIOObjectPropertyManufacturer,
                                     CMIO_PLUGIN_VENDOR);
    device->properties().setProperty(kCMIODevicePropertyModelUID,
                                     CMIO_PLUGIN_PRODUCT);
    device->properties().setProperty(kCMIODevicePropertyLinkedCoreAudioDeviceUID,
                                     "");
    device->properties().setProperty(kCMIODevicePropertyLinkedAndSyncedCoreAudioDeviceUID,
                                     "");
    device->properties().setProperty(kCMIODevicePropertySuspendedByUser,
                                     UInt32(0));
    device->properties().setProperty(kCMIODevicePropertyHogMode,
                                     pid_t(-1),
                                     false);
    device->properties().setProperty(kCMIODevicePropertyDeviceMaster,
                                     pid_t(-1));
    device->properties().setProperty(kCMIODevicePropertyExcludeNonDALAccess,
                                     UInt32(0));
    device->properties().setProperty(kCMIODevicePropertyDeviceIsAlive,
                                     UInt32(1));
    device->properties().setProperty(kCMIODevicePropertyDeviceUID,
                                     deviceId);
    device->properties().setProperty(kCMIODevicePropertyTransportType,
                                     UInt32(kIOAudioDeviceTransportTypePCI));
    device->properties().setProperty(kCMIODevicePropertyDeviceIsRunningSomewhere,
                                     UInt32(0));

    if (device->createObject() != kCMIOHardwareNoError)
        goto createDevice_failed;

    stream = device->addStream();

    // Register one stream for this device.
    if (!stream)
        goto createDevice_failed;

    stream->setBridge(&this->d->m_ipcBridge);
    stream->setFormats(formats);
    stream->properties().setProperty(kCMIOStreamPropertyDirection, 0);

    if (device->registerStreams() != kCMIOHardwareNoError) {
        device->registerStreams(false);

        goto createDevice_failed;
    }

    // Register the device.
    if (device->registerObject() != kCMIOHardwareNoError) {
        device->registerObject(false);
        device->registerStreams(false);

        goto createDevice_failed;
    }

    device->setBroadcasting(this->d->m_ipcBridge.broadcaster(deviceId));
    device->setMirror(this->d->m_ipcBridge.isHorizontalMirrored(deviceId),
                      this->d->m_ipcBridge.isVerticalMirrored(deviceId));
    device->setScaling(this->d->m_ipcBridge.scalingMode(deviceId));
    device->setAspectRatio(this->d->m_ipcBridge.aspectRatioMode(deviceId));
    device->setSwapRgb(this->d->m_ipcBridge.swapRgb(deviceId));

    return true;

createDevice_failed:
    this->m_devices.erase(std::prev(this->m_devices.end()));

    return false;
}

void AkVCam::PluginInterface::destroyDevice(const std::string &deviceId)
{
    AkLogFunction();

    for (auto it = this->m_devices.begin(); it != this->m_devices.end(); it++) {
        auto device = *it;

        std::string curDeviceId;
        device->properties().getProperty(kCMIODevicePropertyDeviceUID,
                                         &curDeviceId);

        if (curDeviceId == deviceId) {
            device->stopStreams();
            device->registerObject(false);
            device->registerStreams(false);
            this->m_devices.erase(it);

            break;
        }
    }
}

void AkVCam::PluginInterfacePrivate::updateDevices()
{
    for (auto &device: this->self->m_devices) {
        device->setBroadcasting(this->m_ipcBridge.broadcaster(device->deviceId()));
        device->setMirror(this->m_ipcBridge.isHorizontalMirrored(device->deviceId()),
                          this->m_ipcBridge.isVerticalMirrored(device->deviceId()));
        device->setScaling(this->m_ipcBridge.scalingMode(device->deviceId()));
        device->setAspectRatio(this->m_ipcBridge.aspectRatioMode(device->deviceId()));
        device->setSwapRgb(this->m_ipcBridge.swapRgb(device->deviceId()));
    }
}

HRESULT AkVCam::PluginInterfacePrivate::QueryInterface(void *self,
                                                       REFIID uuid,
                                                       LPVOID *interface)
{
    AkLogFunction();

    if (!self)
        return E_FAIL;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

    return _self->self->QueryInterface(uuid, interface);
}

ULONG AkVCam::PluginInterfacePrivate::AddRef(void *self)
{
    AkLogFunction();

    if (!self)
        return 0;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
    _self->m_ref++;

    return _self->m_ref;
}

ULONG AkVCam::PluginInterfacePrivate::Release(void *self)
{
    AkLogFunction();

    if (!self)
        return 0;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

    if (_self->m_ref > 0) {
        _self->m_ref--;

        if (_self->m_ref < 1) {
            delete _self->self;

            return 0UL;
        }
    }

    return _self->m_ref;
}

OSStatus AkVCam::PluginInterfacePrivate::Initialize(CMIOHardwarePlugInRef self)
{
    AkLogFunction();

    if (!self)
        return kCMIOHardwareUnspecifiedError;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

    return _self->self->Initialize();
}

OSStatus AkVCam::PluginInterfacePrivate::InitializeWithObjectID(CMIOHardwarePlugInRef self,
                                                                CMIOObjectID objectID)
{
    AkLogFunction();

    if (!self)
        return kCMIOHardwareUnspecifiedError;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

    return _self->self->InitializeWithObjectID(objectID);
}

OSStatus AkVCam::PluginInterfacePrivate::Teardown(CMIOHardwarePlugInRef self)
{
    AkLogFunction();

    if (!self)
        return kCMIOHardwareUnspecifiedError;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

    return _self->self->Teardown();
}

void AkVCam::PluginInterfacePrivate::ObjectShow(CMIOHardwarePlugInRef self,
                                                CMIOObjectID objectID)
{
    AkLogFunction();
    AkLogInfo() << "ObjectID " << objectID << std::endl;

    if (!self)
        return;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

    if (_self->self->objectID() == objectID)
        _self->self->show();
    else if (auto object = _self->self->findObject(objectID))
        object->show();
}

Boolean AkVCam::PluginInterfacePrivate::ObjectHasProperty(CMIOHardwarePlugInRef self,
                                                          CMIOObjectID objectID,
                                                          const CMIOObjectPropertyAddress *address)
{
    AkLogFunction();
    AkLogInfo() << "ObjectID " << objectID << std::endl;
    Boolean result = false;

    if (!self)
        return result;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

    if (_self->self->objectID() == objectID)
        result = _self->self->hasProperty(address);
    else if (auto object = _self->self->findObject(objectID))
        result = object->hasProperty(address);

    return result;
}

OSStatus AkVCam::PluginInterfacePrivate::ObjectIsPropertySettable(CMIOHardwarePlugInRef self,
                                                                  CMIOObjectID objectID,
                                                                  const CMIOObjectPropertyAddress *address,
                                                                  Boolean *isSettable)
{
    AkLogFunction();
    AkLogInfo() << "ObjectID " << objectID << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

    if (_self->self->objectID() == objectID)
        status = _self->self->isPropertySettable(address,
                                                 isSettable);
    else if (auto object = _self->self->findObject(objectID))
        status = object->isPropertySettable(address,
                                            isSettable);

    return status;
}

OSStatus AkVCam::PluginInterfacePrivate::ObjectGetPropertyDataSize(CMIOHardwarePlugInRef self,
                                                                   CMIOObjectID objectID,
                                                                   const CMIOObjectPropertyAddress *address,
                                                                   UInt32 qualifierDataSize,
                                                                   const void *qualifierData,
                                                                   UInt32 *dataSize)
{
    AkLogFunction();
    AkLogInfo() << "ObjectID " << objectID << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

    if (_self->self->objectID() == objectID)
        status = _self->self->getPropertyDataSize(address,
                                                  qualifierDataSize,
                                                  qualifierData,
                                                  dataSize);
    else if (auto object = _self->self->findObject(objectID))
        status = object->getPropertyDataSize(address,
                                             qualifierDataSize,
                                             qualifierData,
                                             dataSize);

    return status;
}

OSStatus AkVCam::PluginInterfacePrivate::ObjectGetPropertyData(CMIOHardwarePlugInRef self,
                                                               CMIOObjectID objectID,
                                                               const CMIOObjectPropertyAddress *address,
                                                               UInt32 qualifierDataSize,
                                                               const void *qualifierData,
                                                               UInt32 dataSize,
                                                               UInt32 *dataUsed,
                                                               void *data)
{
    AkLogFunction();
    AkLogInfo() << "ObjectID " << objectID << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

    if (_self->self->objectID() == objectID)
        status = _self->self->getPropertyData(address,
                                              qualifierDataSize,
                                              qualifierData,
                                              dataSize,
                                              dataUsed,
                                              data);
    else if (auto object = _self->self->findObject(objectID))
        status = object->getPropertyData(address,
                                         qualifierDataSize,
                                         qualifierData,
                                         dataSize,
                                         dataUsed,
                                         data);

    return status;
}

OSStatus AkVCam::PluginInterfacePrivate::ObjectSetPropertyData(CMIOHardwarePlugInRef self,
                                                               CMIOObjectID objectID,
                                                               const CMIOObjectPropertyAddress *address,
                                                               UInt32 qualifierDataSize,
                                                               const void *qualifierData,
                                                               UInt32 dataSize,
                                                               const void *data)
{
    AkLogFunction();
    AkLogInfo() << "ObjectID " << objectID << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);

    if (_self->self->objectID() == objectID)
        status = _self->self->setPropertyData(address,
                                              qualifierDataSize,
                                              qualifierData,
                                              dataSize,
                                              data);
    else if (auto object = _self->self->findObject(objectID))
        status = object->setPropertyData(address,
                                         qualifierDataSize,
                                         qualifierData,
                                         dataSize,
                                         data);

    return status;
}

OSStatus AkVCam::PluginInterfacePrivate::DeviceSuspend(CMIOHardwarePlugInRef self,
                                                       CMIODeviceID device)
{
    AkLogFunction();
    AkLogInfo() << "DeviceID " << device << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
    auto object = reinterpret_cast<Device *>(_self->self->findObject(device));

    if (object)
        status = object->suspend();

    return status;
}

OSStatus AkVCam::PluginInterfacePrivate::DeviceResume(CMIOHardwarePlugInRef self,
                                                      CMIODeviceID device)
{
    AkLogFunction();
    AkLogInfo() << "DeviceID " << device << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
    auto object = reinterpret_cast<Device *>(_self->self->findObject(device));

    if (object)
        status = object->resume();

    return status;
}

OSStatus AkVCam::PluginInterfacePrivate::DeviceStartStream(CMIOHardwarePlugInRef self,
                                                           CMIODeviceID device,
                                                           CMIOStreamID stream)
{
    AkLogFunction();
    AkLogInfo() << "DeviceID " << device << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
    auto object = reinterpret_cast<Device *>(_self->self->findObject(device));

    if (object)
        status = object->startStream(stream);

    return status;
}

OSStatus AkVCam::PluginInterfacePrivate::DeviceStopStream(CMIOHardwarePlugInRef self,
                                                          CMIODeviceID device,
                                                          CMIOStreamID stream)
{
    AkLogFunction();
    AkLogInfo() << "DeviceID " << device << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
    auto object = reinterpret_cast<Device *>(_self->self->findObject(device));

    if (object)
        status = object->stopStream(stream);

    return status;
}

OSStatus AkVCam::PluginInterfacePrivate::DeviceProcessAVCCommand(CMIOHardwarePlugInRef self,
                                                                 CMIODeviceID device,
                                                                 CMIODeviceAVCCommand *ioAVCCommand)
{
    AkLogFunction();
    AkLogInfo() << "DeviceID " << device << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
    auto object = reinterpret_cast<Device *>(_self->self->findObject(device));

    if (object)
        status = object->processAVCCommand(ioAVCCommand);

    return status;
}

OSStatus AkVCam::PluginInterfacePrivate::DeviceProcessRS422Command(CMIOHardwarePlugInRef self,
                                                                   CMIODeviceID device,
                                                                   CMIODeviceRS422Command *ioRS422Command)
{
    AkLogFunction();
    AkLogInfo() << "DeviceID " << device << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
    auto object = reinterpret_cast<Device *>(_self->self->findObject(device));

    if (object)
        status = object->processRS422Command(ioRS422Command);

    return status;
}

OSStatus AkVCam::PluginInterfacePrivate::StreamCopyBufferQueue(CMIOHardwarePlugInRef self,
                                                               CMIOStreamID stream,
                                                               CMIODeviceStreamQueueAlteredProc queueAlteredProc,
                                                               void *queueAlteredRefCon,
                                                               CMSimpleQueueRef *queue)
{
    AkLogFunction();
    AkLogInfo() << "StreamID " << stream << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
    auto object = reinterpret_cast<Stream *>(_self->self->findObject(stream));

    if (object)
        status = object->copyBufferQueue(queueAlteredProc,
                                         queueAlteredRefCon,
                                         queue);

    return status;
}

OSStatus AkVCam::PluginInterfacePrivate::StreamDeckPlay(CMIOHardwarePlugInRef self,
                                                        CMIOStreamID stream)
{
    AkLogFunction();
    AkLogInfo() << "StreamID " << stream << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
    auto object = reinterpret_cast<Stream *>(_self->self->findObject(stream));

    if (object)
        status = object->deckPlay();

    return status;
}

OSStatus AkVCam::PluginInterfacePrivate::StreamDeckStop(CMIOHardwarePlugInRef self,
                                                        CMIOStreamID stream)
{
    AkLogFunction();
    AkLogInfo() << "StreamID " << stream << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
    auto object = reinterpret_cast<Stream *>(_self->self->findObject(stream));

    if (object)
        status = object->deckStop();

    return status;
}

OSStatus AkVCam::PluginInterfacePrivate::StreamDeckJog(CMIOHardwarePlugInRef self,
                                                       CMIOStreamID stream,
                                                       SInt32 speed)
{
    AkLogFunction();
    AkLogInfo() << "StreamID " << stream << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
    auto object = reinterpret_cast<Stream *>(_self->self->findObject(stream));

    if (object)
        status = object->deckJog(speed);

    return status;
}

OSStatus AkVCam::PluginInterfacePrivate::StreamDeckCueTo(CMIOHardwarePlugInRef self,
                                                         CMIOStreamID stream,
                                                         Float64 frameNumber,
                                                         Boolean playOnCue)
{
    AkLogFunction();
    AkLogInfo() << "StreamID " << stream << std::endl;
    OSStatus status = kCMIOHardwareUnspecifiedError;

    if (!self)
        return status;

    auto _self = reinterpret_cast<PluginInterfacePrivate *>(self);
    auto object = reinterpret_cast<Stream *>(_self->self->findObject(stream));

    if (object)
        status = object->deckCueTo(frameNumber, playOnCue);

    return status;
}
