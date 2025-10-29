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

#include <initguid.h>
#include <devpropdef.h>
#include <mfobjects.h>
#include <mfidl.h>

DEFINE_GUID(AKVCAM_PINNAME_VIDEO_CAPTURE, 0xfb6c4281, 0x353, 0x11d1, 0x90, 0x5f, 0x0, 0x0, 0xc0, 0xcc, 0x16, 0xba);
DEFINE_GUID(AKVCAM_KSCAMERAPROFILE_Legacy, 0xb4894d81, 0x62b7, 0x4eec, 0x87, 0x40, 0x80, 0x65, 0x8c, 0x4a, 0x9d, 0x3e);
DEFINE_GUID(AKVCAM_KSCAMERAPROFILE_HighFrameRate, 0x566e6113, 0x8c35, 0x48e7, 0xb8, 0x9f, 0xd2, 0x3f, 0xdc, 0x12, 0x19, 0xdc);
DEFINE_GUID(AKVCAM_MF_DEVICEMFT_SENSORPROFILE_COLLECTION, 0x36ebdc44, 0xb12c, 0x441b, 0x89, 0xf4, 0x08, 0xb2, 0xf4, 0x1a, 0x9c, 0xfc);
DEFINE_GUID(AKVCAM_MF_DEVICESTREAM_ATTRIBUTE_FRAMESOURCE_TYPES, 0x17145fd1, 0x1b2b, 0x423c, 0x80, 0x1, 0x2b, 0x68, 0x33, 0xed, 0x35, 0x88);
DEFINE_GUID(AKVCAM_MF_DEVICESTREAM_FRAMESERVER_SHARED, 0x1cb378e9, 0xb279, 0x41d4, 0xaf, 0x97, 0x34, 0xa2, 0x43, 0xe6, 0x83, 0x20);
DEFINE_GUID(AKVCAM_MF_DEVICESTREAM_STREAM_CATEGORY, 0x2939e7b8, 0xa62e, 0x4579, 0xb6, 0x74, 0xd4, 0x7, 0x3d, 0xfa, 0xbb, 0xba);
DEFINE_GUID(AKVCAM_MF_DEVICESTREAM_STREAM_ID, 0x11bd5120, 0xd124, 0x446b, 0x88, 0xe6, 0x17, 0x6, 0x2, 0x57, 0xff, 0xf9);
DEFINE_GUID(AKVCAM_MF_VIRTUALCAMERA_CONFIGURATION_APP_PACKAGE_FAMILY_NAME, 0x658abe51, 0x8044, 0x462e, 0x97, 0xea, 0xe6, 0x76, 0xfd, 0x72, 0x05, 0x5f);
DEFINE_GUID(AKVCAM_MF_VIRTUALCAMERA_PROVIDE_ASSOCIATED_CAMERA_SOURCES, 0xf0273718, 0x4a4d, 0x4ac5, 0xa1, 0x5d, 0x30, 0x5e, 0xb5, 0xe9, 0x06, 0x67);
DEFINE_GUID(IID_IMFMediaSrcEx, 0x3c9b2eb9, 0x86d5, 0x4514, 0xa3, 0x94, 0xf5, 0x66, 0x64, 0xf9, 0xf0, 0xd8);

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

struct SENSORPROFILEID
{
    GUID Type;
    UINT32 Index;
    UINT32 Unused;
};

class IMFSensorProf: public IUnknown
{
    public:
        virtual HRESULT STDMETHODCALLTYPE GetProfileId(SENSORPROFILEID *pId) = 0;
        virtual HRESULT STDMETHODCALLTYPE AddProfileFilter(UINT32 StreamId,
                                                           LPCWSTR wzFilterSetString) = 0;
        virtual HRESULT STDMETHODCALLTYPE IsMediaTypeSupported(UINT32 StreamId,
                                                               IMFMediaType *pMediaType,
                                                               BOOL *pfSupported) = 0;
        virtual HRESULT STDMETHODCALLTYPE AddBlockedControl(LPCWSTR wzBlockedControl) = 0;
};

class IMFSensorProfCollection: public IUnknown
{
    public:
        virtual DWORD STDMETHODCALLTYPE GetProfileCount() = 0;
        virtual HRESULT STDMETHODCALLTYPE GetProfile(DWORD Index,
                                                     IMFSensorProf **ppProfile) = 0;
        virtual HRESULT STDMETHODCALLTYPE AddProfile(IMFSensorProf *pProfile) = 0;
        virtual HRESULT STDMETHODCALLTYPE FindProfile(SENSORPROFILEID *ProfileId,
                                                      IMFSensorProf **ppProfile) = 0;
        virtual void STDMETHODCALLTYPE RemoveProfileByIndex(DWORD Index) = 0;
        virtual void STDMETHODCALLTYPE RemoveProfile(SENSORPROFILEID *ProfileId) = 0;
};

class IMFMediaSrcEx: public IMFMediaSource
{
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSourceAttributes(IMFAttributes **ppAttributes) = 0;
        virtual HRESULT STDMETHODCALLTYPE GetStreamAttributes(DWORD dwStreamIdentifier,
                                                              IMFAttributes **ppAttributes) = 0;
        virtual HRESULT STDMETHODCALLTYPE SetD3DManager(IUnknown *pManager) = 0;
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
