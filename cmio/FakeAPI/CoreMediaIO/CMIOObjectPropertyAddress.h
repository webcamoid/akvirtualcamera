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

#ifndef COREMEDIAIO_CMIOOBJECTPROPERTYADDRESS_H
#define COREMEDIAIO_CMIOOBJECTPROPERTYADDRESS_H

#include "CoreFoundation/CFType.h"

using CMIOObjectPropertySelector = UInt32;
using CMIOObjectPropertyScope = UInt32;
using CMIOObjectPropertyElement = UInt32;

struct CMIOObjectPropertyAddress
{
    CMIOObjectPropertySelector mSelector;
    CMIOObjectPropertyScope mScope;
    CMIOObjectPropertyElement mElement;
};

enum CMIOObjectPropertyScopes
{
    kCMIOObjectPropertyScopeGlobal = CFTYPE_FOURCC('g', 'l', 'o', 'b'),
    kCMIOObjectPropertyElementMaster = 0,
    kCMIOObjectClassID = CFTYPE_FOURCC('a', 'o', 'b', 'j'),
    kCMIOObjectClassIDWildcard = CFTYPE_FOURCC('*', '*', '*', '*'),
    kCMIOObjectUnknown = 0
};

#endif // COREMEDIAIO_CMIOOBJECTPROPERTYADDRESS_H
