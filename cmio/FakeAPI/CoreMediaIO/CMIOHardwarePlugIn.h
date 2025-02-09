/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#ifndef COREMEDIAIO_CMIOHARDWAREPLUGIN_H
#define COREMEDIAIO_CMIOHARDWAREPLUGIN_H

#include <algorithm>

#include "../CoreFoundation/CFArray.h"
#include "../CoreFoundation/CFUUID.h"
#include "../CoreFoundation/CFType.h"
#include "../CoreMedia/CMSimpleQueue.h"
#include "../CoreMedia/CMSampleBuffer.h"
#include "../CoreMedia/CMVideoFormatDescription.h"
#include "CMIOHardware.h"
#include "CMIOHardwareDevice.h"
#include "CMIOHardwareStream.h"
#include "CMIOCOM.h"

struct CMIOHardwarePlugInInterface;
using CMIOHardwarePlugInRef = CMIOHardwarePlugInInterface **;

#define kCMIOHardwarePlugInTypeID      CFUUIDGetConstantUUIDWithBytes(kCFAllocatorDefault, 0x30, 0x01, 0x0c, 0x1c, 0x93, 0xbf, 0x11, 0xd8, 0x8b, 0x5b, 0x00, 0x0a, 0x95, 0xaf, 0x9c, 0x6a)
#define kCMIOHardwarePlugInInterfaceID CFUUIDGetConstantUUIDWithBytes(kCFAllocatorDefault, 0xb8, 0x9d, 0xfa, 0xba, 0x93, 0xbf, 0x11, 0xd8, 0x8e, 0xa6, 0x00, 0x0a, 0x95, 0xaf, 0x9c, 0x6a)


struct CMIOHardwarePlugInInterface
{
    void *_reserved;
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(void *self,
                                                REFIID uuid,
                                                LPVOID *interface);
    ULONG (STDMETHODCALLTYPE *AddRef)(void *self);
    ULONG (STDMETHODCALLTYPE *Release)(void *self);
    OSStatus (*Initialize)(CMIOHardwarePlugInRef self);
    OSStatus (*InitializeWithObjectID)(CMIOHardwarePlugInRef self,
                                       CMIOObjectID objectID);
    OSStatus (*Teardown)(CMIOHardwarePlugInRef self);
    void (*ObjectShow)(CMIOHardwarePlugInRef self, CMIOObjectID objectID);
    Boolean (*ObjectHasProperty)(CMIOHardwarePlugInRef self,
                                 CMIOObjectID objectID,
                                 const CMIOObjectPropertyAddress *address);
    OSStatus (*ObjectIsPropertySettable)(CMIOHardwarePlugInRef self,
                                         CMIOObjectID objectID,
                                         const CMIOObjectPropertyAddress *address,
                                         Boolean *isSettable);
    OSStatus (*ObjectGetPropertyDataSize)(CMIOHardwarePlugInRef self,
                                          CMIOObjectID objectID,
                                          const CMIOObjectPropertyAddress *address,
                                          UInt32 qualifierDataSize,
                                          const void *qualifierData,
                                          UInt32 *dataSize);
    OSStatus (*ObjectGetPropertyData)(CMIOHardwarePlugInRef self,
                                      CMIOObjectID objectID,
                                      const CMIOObjectPropertyAddress *address,
                                      UInt32 qualifierDataSize,
                                      const void *qualifierData,
                                      UInt32 dataSize,
                                      UInt32 *dataUsed,
                                      void *data);
    OSStatus (*ObjectSetPropertyData)(CMIOHardwarePlugInRef self,
                                      CMIOObjectID objectID,
                                      const CMIOObjectPropertyAddress *address,
                                      UInt32 qualifierDataSize,
                                      const void *qualifierData,
                                      UInt32 dataSize,
                                      const void *data);
    OSStatus (*DeviceSuspend)(CMIOHardwarePlugInRef self, CMIODeviceID device);
    OSStatus (*DeviceResume)(CMIOHardwarePlugInRef self, CMIODeviceID device);
    OSStatus (*DeviceStartStream)(CMIOHardwarePlugInRef self,
                                  CMIODeviceID device,
                                  CMIOStreamID stream);
    OSStatus (*DeviceStopStream)(CMIOHardwarePlugInRef self,
                                 CMIODeviceID device,
                                 CMIOStreamID stream);
    OSStatus (*DeviceProcessAVCCommand)(CMIOHardwarePlugInRef self,
                                        CMIODeviceID device,
                                        CMIODeviceAVCCommand *ioAVCCommand);
    OSStatus (*DeviceProcessRS422Command)(CMIOHardwarePlugInRef self,
                                          CMIODeviceID device,
                                          CMIODeviceRS422Command *ioRS422Command);
    OSStatus (*StreamCopyBufferQueue)(CMIOHardwarePlugInRef self,
                                      CMIOStreamID stream,
                                      CMIODeviceStreamQueueAlteredProc queueAlteredProc,
                                      void *queueAlteredRefCon,
                                      CMSimpleQueueRef *queue);
    OSStatus (*StreamDeckPlay)(CMIOHardwarePlugInRef self, CMIOStreamID stream);
    OSStatus (*StreamDeckStop)(CMIOHardwarePlugInRef self, CMIOStreamID stream);
    OSStatus (*StreamDeckJog)(CMIOHardwarePlugInRef self,
                              CMIOStreamID stream,
                              SInt32 speed);
    OSStatus (*StreamDeckCueTo)(CMIOHardwarePlugInRef self,
                                CMIOStreamID stream,
                                Float64 frameNumber,
                                Boolean playOnCue);

};

struct _CMIOObject
{
    CMIOHardwarePlugInRef owningPlugIn;
    CMIOObjectID owningObjectID;
    CMIOClassID classID;
    CMIOObjectID objectID;
    bool published;
};

#define CMIO_MAX_OBJECTS (1024 * 1024)

static struct _CMIOGlobalObjectsType
{
    UInt32 nobjects {0};
    _CMIOObject objects[CMIO_MAX_OBJECTS];
} _CMIOGlobalObjects;

inline OSStatus CMIOObjectCreate(CMIOHardwarePlugInRef owningPlugIn,
                                 CMIOObjectID owningObjectID,
                                 CMIOClassID classID,
                                 CMIOObjectID *objectID)
{
    static UInt32 cmioNewObjectID = 0;
    auto &object = _CMIOGlobalObjects.objects[_CMIOGlobalObjects.nobjects];
    object.owningPlugIn = owningPlugIn;
    object.owningObjectID = owningObjectID;
    object.classID = classID;
    object.objectID = cmioNewObjectID++;
    object.published = false;
    ++_CMIOGlobalObjects.nobjects;
    *objectID = object.objectID;

    return noErr;
}

inline OSStatus CMIOObjectsPublishedAndDied(CMIOHardwarePlugInRef owningPlugIn,
                                            CMIOObjectID owningObjectID,
                                            UInt32 numberPublishedCMIOObjects,
                                            const CMIOObjectID *publishedCMIOObjects,
                                            UInt32 numberDeadCMIOObjects,
                                            const CMIOObjectID *deadCMIOObjects)
{
    _CMIOGlobalObjectsType newObjects;
    newObjects.nobjects = 0;

    for (size_t object = 0; object < _CMIOGlobalObjects.nobjects; ++object) {
        auto obj = _CMIOGlobalObjects.objects + object;

        if (obj->owningPlugIn == owningPlugIn && obj->owningObjectID == owningObjectID) {
            // Publish the new objects
            for (size_t i = 0; i < numberPublishedCMIOObjects; ++i)
                if (obj->objectID == publishedCMIOObjects[i]) {
                    obj->published = true;

                    break;
                }

            // Remove the dead objects
            bool isDead = false;

            for (size_t i = 0; i < numberDeadCMIOObjects; ++i)
                if (obj->objectID == deadCMIOObjects[i]) {
                    isDead = true;

                    break;
                }

            if (!isDead) {
                memcpy(newObjects.objects + newObjects.nobjects, obj, sizeof(_CMIOObject));
                ++newObjects.nobjects;
            }
        } else {
            memcpy(newObjects.objects + newObjects.nobjects, obj, sizeof(_CMIOObject));
            ++newObjects.nobjects;
        }
    }

    memcpy(&_CMIOGlobalObjects, &newObjects, sizeof(_CMIOGlobalObjectsType));

    return noErr;
}

inline OSStatus CMIOObjectPropertiesChanged(CMIOHardwarePlugInRef owningPlugIn,
                                            CMIOObjectID objectID,
                                            UInt32 numberAddresses,
                                            const CMIOObjectPropertyAddress *addresses)
{
    OSStatus status = noErr;

    for (UInt32 address = 0; address < numberAddresses; ++address) {
        auto propertyAddress = addresses + address;

        if (!(*owningPlugIn)->ObjectHasProperty(owningPlugIn,
                                                objectID,
                                                propertyAddress))
            break;

        Boolean isSettable = false;
        status = (*owningPlugIn)->ObjectIsPropertySettable(owningPlugIn,
                                                           objectID,
                                                           propertyAddress,
                                                           &isSettable);

        if (status != noErr)
            break;

        if (!isSettable)
            continue;

        std::vector<uint8_t> qualifier;
        UInt32 dataSize = 0;
        status = (*owningPlugIn)->ObjectGetPropertyDataSize(owningPlugIn,
                                                            objectID,
                                                            propertyAddress,
                                                            UInt32(qualifier.size()),
                                                            qualifier.data(),
                                                            &dataSize);

        if (status != noErr)
            break;

        UInt32 dataUsed = 0;
        std::vector<uint8_t> data(dataSize);
        status = (*owningPlugIn)->ObjectGetPropertyData(owningPlugIn,
                                                        objectID,
                                                        propertyAddress,
                                                        UInt32(qualifier.size()),
                                                        qualifier.data(),
                                                        data.size(),
                                                        &dataUsed,
                                                        data.data());

        if (status != noErr)
            break;

        status = (*owningPlugIn)->ObjectSetPropertyData(owningPlugIn,
                                                        objectID,
                                                        propertyAddress,
                                                        UInt32(qualifier.size()),
                                                        qualifier.data(),
                                                        dataUsed,
                                                        data.data());

        if (status != noErr)
            break;
    }

    return status;
}

#endif // COREMEDIAIO_CMIOHARDWAREPLUGIN_H
