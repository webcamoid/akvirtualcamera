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

#ifndef COREFOUNDATION_CFUUID_H
#define COREFOUNDATION_CFUUID_H

#include <cstdint>

#include "Allocators.h"
#include "CFType.h"

using CFUUID = CFType;
using CFUUIDRef = CFTypeRef;

typedef struct _GUID {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} GUID;

using IID = GUID;
using REFIID = IID *;

struct CFUUIDBytes
{
    uint8_t byte0;
    uint8_t byte1;
    uint8_t byte2;
    uint8_t byte3;
    uint8_t byte4;
    uint8_t byte5;
    uint8_t byte6;
    uint8_t byte7;
    uint8_t byte8;
    uint8_t byte9;
    uint8_t byte10;
    uint8_t byte11;
    uint8_t byte12;
    uint8_t byte13;
    uint8_t byte14;
    uint8_t byte15;
};

struct CFRuntimeBase {
    uintptr_t _cfisa;
    uint8_t _cfinfo[4];
    uint32_t _rc;
};

struct _CFUUIDData
{
    CFRuntimeBase base;
    CFUUIDBytes bytes;
};

using _CFUUIDDataRef = _CFUUIDData *;

inline CFTypeID CFUUIDGetTypeID()
{
    return 0x7;
}

inline CFUUIDRef CFUUIDGetConstantUUIDWithBytes(CFAllocatorRef alloc,
                                                UInt8 byte0,
                                                UInt8 byte1,
                                                UInt8 byte2,
                                                UInt8 byte3,
                                                UInt8 byte4,
                                                UInt8 byte5,
                                                UInt8 byte6,
                                                UInt8 byte7,
                                                UInt8 byte8,
                                                UInt8 byte9,
                                                UInt8 byte10,
                                                UInt8 byte11,
                                                UInt8 byte12,
                                                UInt8 byte13,
                                                UInt8 byte14,
                                                UInt8 byte15)
{
    auto uuid = new CFUUID;
    uuid->type = CFUUIDGetTypeID();
    uuid->data = new _CFUUIDData;
    memset(uuid->data, 0, sizeof(_CFUUIDData));
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte0  = byte0 ;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte1  = byte1 ;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte2  = byte2 ;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte3  = byte3 ;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte4  = byte4 ;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte5  = byte5 ;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte6  = byte6 ;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte7  = byte7 ;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte8  = byte8 ;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte9  = byte9 ;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte10 = byte10;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte11 = byte11;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte12 = byte12;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte13 = byte13;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte14 = byte14;
    reinterpret_cast<_CFUUIDDataRef>(uuid->data)->bytes.byte15 = byte15;
    uuid->size = sizeof(_CFUUIDData);
    uuid->deleter = [] (void *data) {
        delete reinterpret_cast<CFUUIDRef>(data);
    };
    uuid->ref = 1;

    return uuid;

    return uuid;
}

inline CFUUIDBytes CFUUIDGetUUIDBytes(CFUUIDRef uuid)
{
    return reinterpret_cast<_CFUUIDDataRef>(uuid)->bytes;
}

#endif // COREFOUNDATION_CFUUID_H
