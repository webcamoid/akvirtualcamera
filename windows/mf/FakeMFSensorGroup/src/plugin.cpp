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

#include "mfvcamimpl.h"
#include "VCamUtils/src/utils.h"

extern "C" HRESULT WINAPI MFCreateVirtualCamera(MFVCamType type,
                                                MFVCamLifetime lifetime,
                                                MFVCamAccess access,
                                                LPCWSTR friendlyName,
                                                LPCWSTR sourceId,
                                                const GUID *categories,
                                                ULONG categoryCount,
                                                IMFVCam **virtualCamera)
{
    UNUSED(type);
    UNUSED(lifetime);
    UNUSED(access);
    UNUSED(categories);
    UNUSED(categoryCount);

    if (!virtualCamera)
        return E_POINTER;

    *virtualCamera = nullptr;
    auto vcam = new MFVCamImpl(friendlyName, sourceId);

    if (!vcam)
        return E_OUTOFMEMORY;

    *virtualCamera = vcam;

    return S_OK;
}
