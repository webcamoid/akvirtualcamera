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

struct CFUUID
{
    CFRuntimeBase base;
    CFUUIDBytes bytes;
};

using CFUUIDRef = CFUUID *;

inline CFUUIDBytes CFUUIDGetUUIDBytes(CFUUIDRef uuid)
{
    return uuid->bytes;
}

#endif // COREFOUNDATION_CFUUID_H
