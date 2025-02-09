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

#ifndef COREMEDIAIO_CMIOHARDWAREOBJECT_H
#define COREMEDIAIO_CMIOHARDWAREOBJECT_H

#include "CoreMediaIO/CMIOObjectPropertyAddress.h"

using CMIOObjectID = UInt32;
using CMIOClassID = UInt32;

enum CMIOObjectProperties
{
    kCMIOObjectPropertyOwnedObjects    = CFTYPE_FOURCC('o', 'w', 'n', 'd'),
    kCMIOObjectPropertyListenerAdded   = CFTYPE_FOURCC('l', 'i', 's', 'a'),
    kCMIOObjectPropertyListenerRemoved = CFTYPE_FOURCC('l', 'i', 's', 'r'),
    kCMIOObjectPropertyName            = CFTYPE_FOURCC('l', 'n', 'a', 'm'),
    kCMIOObjectPropertyManufacturer    = CFTYPE_FOURCC('l', 'm', 'a', 'k'),
};

#endif // COREMEDIAIO_CMIOHARDWAREOBJECT_H
