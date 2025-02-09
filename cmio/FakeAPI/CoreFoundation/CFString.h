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

#ifndef CFSTRING_H
#define CFSTRING_H

#include <cstring>
#include <cstdio>

#include "Allocators.h"
#include "CFIndex.h"
#include "CFType.h"
#include "CFStringBuiltInEncodings.h"

#define CFSTR(str) str

using CFString = CFType;
using CFStringRef = CFTypeRef;

inline CFTypeID CFStringGetTypeID()
{
    return 0x1;
}

inline CFStringRef CFStringCreateWithCString(CFAllocatorRef alloc,
                                             const char *str,
                                             CFStringEncoding encoding)
{
    auto strType = new CFString;
    strType->type = CFStringGetTypeID();
    strType->size = strlen(str) + 1;
    strType->data = new char[strType->size];
    strcpy(reinterpret_cast<char *>(strType->data), str);
    strType->deleter = [] (void *data) {
        delete [] reinterpret_cast<char *>(data);
    };
    strType->ref = 1;

    return strType;
}

inline CFIndex CFStringGetLength(CFStringRef str)
{
    return str->size - 1;
}

inline const char *CFStringGetCStringPtr(CFStringRef str, CFStringEncoding encoding)
{
    return reinterpret_cast<char *>(str->data);
}

inline bool CFStringGetCString(CFStringRef str,
                               char *buffer,
                               CFIndex bufferSize,
                               CFStringEncoding encoding)
{
    snprintf(buffer, size_t(bufferSize), "%s", CFStringGetCStringPtr(str, encoding));

    return true;
}

inline CFIndex CFStringGetMaximumSizeForEncoding(CFIndex length,
                                                 CFStringEncoding encoding)
{
    return length;
}

#endif // CFSTRING_H
