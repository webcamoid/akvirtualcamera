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

#include "utils.h"

std::shared_ptr<CFTypeRef> AkVCam::cfTypeFromStd(const std::string &str)
{
    auto ref =
            new CFTypeRef(CFStringCreateWithCString(kCFAllocatorDefault,
                                                    str.c_str(),
                                                    kCFStringEncodingUTF8));

    return std::shared_ptr<CFTypeRef>(ref, [] (CFTypeRef *ptr) {
        CFRelease(*ptr);
        delete ptr;
    });
}

std::shared_ptr<CFTypeRef> AkVCam::cfTypeFromStd(const std::wstring &str)
{
    auto ref =
            new CFTypeRef(CFStringCreateWithBytes(kCFAllocatorDefault,
                                                  reinterpret_cast<const UInt8 *>(str.c_str()),
                                                  CFIndex(str.size() * sizeof(wchar_t)),
                                                  kCFStringEncodingUTF32LE,
                                                  false));

    return std::shared_ptr<CFTypeRef>(ref, [] (CFTypeRef *ptr) {
        CFRelease(*ptr);
        delete ptr;
    });
}

std::shared_ptr<CFTypeRef> AkVCam::cfTypeFromStd(int num)
{
    auto ref =
            new CFTypeRef(CFNumberCreate(kCFAllocatorDefault,
                                         kCFNumberIntType,
                                         &num));

    return std::shared_ptr<CFTypeRef>(ref, [] (CFTypeRef *ptr) {
        CFRelease(*ptr);
        delete ptr;
    });
}

std::shared_ptr<CFTypeRef> AkVCam::cfTypeFromStd(double num)
{
    auto ref =
            new CFTypeRef(CFNumberCreate(kCFAllocatorDefault,
                                         kCFNumberDoubleType,
                                         &num));

    return std::shared_ptr<CFTypeRef>(ref, [] (CFTypeRef *ptr) {
        CFRelease(*ptr);
        delete ptr;
    });
}

std::string AkVCam::stringFromCFType(CFTypeRef cfType)
{
    auto len = size_t(CFStringGetLength(CFStringRef(cfType)));
    auto data = CFStringGetCStringPtr(CFStringRef(cfType),
                                      kCFStringEncodingUTF8);

    if (data)
        return std::string(data, len);

    auto cstr = new char[len];
    CFStringGetCString(CFStringRef(cfType),
                       cstr,
                       CFIndex(len),
                       kCFStringEncodingUTF8);
    std::string str(cstr, len);
    delete [] cstr;

    return str;
}

std::wstring AkVCam::wstringFromCFType(CFTypeRef cfType)
{
    auto len = CFStringGetLength(CFStringRef(cfType));
    auto range = CFRangeMake(0, len);
    CFIndex bufferLen = 0;
    auto converted = CFStringGetBytes(CFStringRef(cfType),
                                      range,
                                      kCFStringEncodingUTF32LE,
                                      0,
                                      false,
                                      nullptr,
                                      0,
                                      &bufferLen);

    if (converted < 1 || bufferLen < 1)
        return {};

    wchar_t cstr[bufferLen];

    converted = CFStringGetBytes(CFStringRef(cfType),
                                 range,
                                 kCFStringEncodingUTF32LE,
                                 0,
                                 false,
                                 reinterpret_cast<UInt8 *>(cstr),
                                 bufferLen,
                                 nullptr);

    if (converted < 1)
        return {};

    return std::wstring(cstr, size_t(len));
}
