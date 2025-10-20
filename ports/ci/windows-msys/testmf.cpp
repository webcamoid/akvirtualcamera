/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <stdio.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfuuid.lib")

int main()
{
    auto hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    if (FAILED(hr)) {
        printf("Error initializing COM: 0x%08lX\n", static_cast<unsigned long>(hr));

        return -1;
    }

    hr = MFStartup(MF_VERSION, 0);

    if (FAILED(hr)) {
        printf("Error initializing Media Foundation: 0x%08lX\n", static_cast<unsigned long>(hr));
        CoUninitialize();

        return -1;
    }

    IMFAttributes *pAttributes = nullptr;
    hr = MFCreateAttributes(&pAttributes, 1);

    if (FAILED(hr)) {
        printf("Error creating attributtes: 0x%08lX\n", static_cast<unsigned long>(hr));
        MFShutdown();
        CoUninitialize();

        return -1;
    }

    hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                              MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

    if (FAILED(hr)) {
        printf("Error configuring attributtes: 0x%08lX\n", static_cast<unsigned long>(hr));
        pAttributes->Release();
        MFShutdown();
        CoUninitialize();

        return -1;
    }

    IMFActivate **ppDevices = nullptr;
    UINT32 count = 0;
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);

    if (FAILED(hr)) {
        printf("Error enumerating devices: 0x%08lX\n", static_cast<unsigned long>(hr));
        pAttributes->Release();
        MFShutdown();
        CoUninitialize();

        return -1;
    }

    printf("Found cameras: %u\n\n", count);

    for (UINT32 i = 0; i < count; i++) {
        WCHAR *pName = nullptr;
        hr = ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
                                              &pName,
                                              nullptr);

        if (SUCCEEDED(hr)) {
            printf("\tDevice %u: %S\n", i, pName);
            CoTaskMemFree(pName);
        } else {
            printf("Error reading the device name %u: 0x%08X\n", i, hr);
        }

        ppDevices[i]->Release();
    }

    CoTaskMemFree(ppDevices);
    pAttributes->Release();
    MFShutdown();
    CoUninitialize();

    return 0;
}
