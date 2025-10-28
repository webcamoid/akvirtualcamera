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
#include <dshow.h>
#include <stdio.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32")
#pragma comment(lib, "strmiids.lib")

int main()
{
    auto hr = CoInitialize(nullptr);

    if (FAILED(hr)) {
        printf("Error initializing COM: 0x%08lX\n", static_cast<unsigned long>(hr));

        return -11;
    }

    ICreateDevEnum *pDevEnum = nullptr;
    hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                          nullptr,
                          CLSCTX_INPROC_SERVER,
                          IID_ICreateDevEnum,
                          reinterpret_cast<void **>(&pDevEnum));

    if (FAILED(hr)) {
        printf("Error creating the device enumerator: 0x%08lX\n", static_cast<unsigned long>(hr));
        CoUninitialize();

        return -1;
    }

    IEnumMoniker *pEnum = nullptr;
    hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
                                         &pEnum,
                                         0);

    if (hr == S_FALSE) {
        printf("Camaras not found.\n");
        pDevEnum->Release();
        CoUninitialize();

        return 0;
    } else if (FAILED(hr)) {
        printf("Error enumerating devices: 0x%08lX\n", static_cast<unsigned long>(hr));
        pDevEnum->Release();
        CoUninitialize();

        return -1;
    }

    IMoniker *pMoniker = nullptr;
    ULONG count = 0;
    printf("Found cameras:\n\n");

    while (pEnum->Next(1, &pMoniker, nullptr) == S_OK) {
        IPropertyBag *pPropBag = nullptr;
        hr = pMoniker->BindToStorage(0,
                                     0,
                                     IID_IPropertyBag,
                                     reinterpret_cast<void **>(&pPropBag));

        if (SUCCEEDED(hr)) {
            VARIANT var;
            VariantInit(&var);
            hr = pPropBag->Read(L"FriendlyName", &var, 0);

            if (SUCCEEDED(hr)) {
                printf("\tDevice %lu: %S\n", count, var.bstrVal);
                VariantClear(&var);
            } else {
                printf("Error reading the device name %lu: 0x%08lX\n", count, hr);
            }

            pPropBag->Release();
        } else {
            printf("Error reading the device properties %lu: 0x%08lX\n", count, hr);
        }

        pMoniker->Release();
        count++;
    }

    pEnum->Release();
    pDevEnum->Release();
    CoUninitialize();

    if (count == 0)
        printf("No camaras found.\n");

    return 0;
}
