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

#ifndef COREMEDIAIO_CMIOHARDWAREDEVICE_H
#define COREMEDIAIO_CMIOHARDWAREDEVICE_H

#include "CMIOHardwareObject.h"
#include "CMIOHardwareSystem.h"

using CMIODeviceID = CMIOObjectID;

struct CMIODeviceAVCCommand
{
    UInt8 *mCommand;
    UInt32 mCommandLength;
    UInt8 *mResponse;
    UInt32 mResponseLength;
    UInt32 mResponseUsed;
};

struct CMIODeviceRS422Command
{
    UInt8 *mCommand;
    UInt32 mCommandLength;
    UInt8 *mResponse;
    UInt32 mResponseLength;
    UInt32 mResponseUsed;
};

enum CMIODevicePropertyTypes
{
    kCMIODevicePropertyStreams                           = CFTYPE_FOURCC('s', 't', 'm', '#'),
    kCMIODevicePropertyDeviceIsRunning                   = CFTYPE_FOURCC('g', 'o', 'i', 'n'),
    kCMIODevicePropertyDeviceUID                         = CFTYPE_FOURCC('u', 'i', 'd', ' '),
    kCMIODevicePropertyModelUID                          = CFTYPE_FOURCC('m', 'u', 'i', 'd'),
    kCMIODevicePropertyLinkedCoreAudioDeviceUID          = CFTYPE_FOURCC('p', 'l', 'u', 'd'),
    kCMIODevicePropertyLinkedAndSyncedCoreAudioDeviceUID = CFTYPE_FOURCC('p', 'l', 's', 'd'),
    kCMIODevicePropertySuspendedByUser                   = CFTYPE_FOURCC('s', 'b', 'y', 'u'),
    kCMIODevicePropertyHogMode                           = CFTYPE_FOURCC('o', 'i', 'n', 'k'),
    kCMIODevicePropertyDeviceMaster                      = CFTYPE_FOURCC('p', 'm', 'n', 'h'),
    kCMIODevicePropertyExcludeNonDALAccess               = CFTYPE_FOURCC('i', 'x', 'n', 'a'),
    kCMIODevicePropertyDeviceIsAlive                     = CFTYPE_FOURCC('l', 'i', 'v', 'n'),
    kCMIODevicePropertyTransportType                     = CFTYPE_FOURCC('t', 'r', 'a', 'n'),
    kCMIODevicePropertyDeviceIsRunningSomewhere          = CFTYPE_FOURCC('g', 'o', 'n', 'e'),
};

enum CMIODeviceTypes
{
    kCMIODeviceClassID = CFTYPE_FOURCC('a', 'd', 'e', 'v')
};

#endif // COREMEDIAIO_CMIOHARDWAREDEVICE_H
