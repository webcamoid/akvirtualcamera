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

#ifndef MFVCAM_H
#define MFVCAM_H

#include <devpropdef.h>
#include <mfobjects.h>

enum MFVCamType
{
    MFVCamType_SoftwareCameraSource
};

enum MFVCamLifetime
{
    MFVCamLifetime_Session,
    MFVCamLifetime_System
};

enum MFVCamAccess
{
    MFVCamAccess_CurrentUser,
    MFVCamAccess_AllUsers
};

class IMFCamSyncObject: public IUnknown
{
    public:
        virtual HRESULT WaitOnSignal(DWORD timeOutInMs) = 0;
        virtual void Shutdown() = 0;
};

class IMFVCam: public IMFAttributes
{
    public:
        virtual HRESULT STDMETHODCALLTYPE AddDeviceSourceInfo(LPCWSTR deviceSourceInfo) = 0;
        virtual HRESULT STDMETHODCALLTYPE AddProperty(const DEVPROPKEY *pKey,
                                                      DEVPROPTYPE type,
                                                      const BYTE *pbData,
                                                      ULONG cbData) = 0;
        virtual HRESULT STDMETHODCALLTYPE AddRegistryEntry(LPCWSTR entryName,
                                                           LPCWSTR subkeyPath,
                                                           DWORD dwRegType,
                                                           const BYTE *pbData,
                                                           ULONG cbData) = 0;
        virtual HRESULT STDMETHODCALLTYPE Start(IMFAsyncCallback *pCallback) = 0;
        virtual HRESULT STDMETHODCALLTYPE Stop() = 0;
        virtual HRESULT STDMETHODCALLTYPE Remove() = 0;
        virtual HRESULT STDMETHODCALLTYPE GetMediaSource(IMFMediaSource **ppMediaSource) = 0;
        virtual HRESULT STDMETHODCALLTYPE SendCameraProperty(REFGUID propertySet,
                                                             ULONG propertyId,
                                                             ULONG propertyFlags,
                                                             void *propertyPayload,
                                                             ULONG propertyPayloadLength,
                                                             void *data,
                                                             ULONG dataLength,
                                                             ULONG *dataWritten) = 0;
        virtual HRESULT STDMETHODCALLTYPE CreateSyncEvent(REFGUID kseventSet,
                                                          ULONG kseventId,
                                                          ULONG kseventFlags,
                                                          HANDLE eventHandle,
                                                          IMFCamSyncObject **cameraSyncObject) = 0;
        virtual HRESULT STDMETHODCALLTYPE CreateSyncSemaphore(REFGUID kseventSet,
                                                              ULONG kseventId,
                                                              ULONG kseventFlags,
                                                              HANDLE semaphoreHandle,
                                                              LONG semaphoreAdjustment,
                                                              IMFCamSyncObject **cameraSyncObject) = 0;
        virtual HRESULT STDMETHODCALLTYPE Shutdown() = 0;
};

#endif // MFVCAM_H
