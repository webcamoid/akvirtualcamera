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

#ifndef COREMEDIAIO_CMIOCOM_H
#define COREMEDIAIO_CMIOCOM_H

#include "CoreFoundation/Allocators.h"
#include "CoreFoundation/CFType.h"

#define STDMETHODCALLTYPE

using LPVOID = void *;
using HRESULT = SInt32;
using ULONG = UInt32;

#define S_OK           0
#define E_FAIL        -1
#define E_POINTER     -2
#define E_NOINTERFACE -3

#define IUnknownUUID CFUUIDGetConstantUUIDWithBytes(kCFAllocatorDefault, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46)

#endif // COREMEDIAIO_CMIOCOM_H
