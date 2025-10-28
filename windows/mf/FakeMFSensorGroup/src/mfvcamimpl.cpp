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

#include <atomic>
#include <objbase.h>
#include <initguid.h>

#include "mfvcamimpl.h"
#include "PlatformUtils/src/cunknown.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

DEFINE_GUID(IID_IMFVCam, 0x12345678, 0x1234, 0x5678, 0x90, 0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56, 0x78);

namespace AkVCam
{
    class MFVCamImplPrivate
    {
        public:
            std::atomic<uint64_t> m_refCount {1};
            std::wstring m_friendlyName;
            std::wstring m_sourceId;
    };
}

AkVCam::MFVCamImpl::MFVCamImpl(LPCWSTR friendlyName, LPCWSTR sourceId)
{
    this->d = new MFVCamImplPrivate;

    if (friendlyName)
        this->d->m_friendlyName = friendlyName;

    if (sourceId)
        this->d->m_sourceId = sourceId;
}

AkVCam::MFVCamImpl::~MFVCamImpl()
{
    delete this->d;
}

HRESULT AkVCam::MFVCamImpl::QueryInterface(const IID &riid, void **ppv)
{
    AkLogFunction();
    AkLogDebug("IID: %s", stringFromClsid(riid).c_str());

    if (!ppv)
        return E_POINTER;

    static const struct
    {
        const IID *iid;
        void *ptr;
    } comInterfaceEntryMediaSample[] = {
        COM_INTERFACE(IMFVCam)
        COM_INTERFACE(IMFAttributes)
        COM_INTERFACE(IUnknown)
        COM_INTERFACE_NULL
    };

    for (auto map = comInterfaceEntryMediaSample; map->ptr; ++map)
        if (*map->iid == riid) {
            *ppv = map->ptr;
            this->AddRef();

            return S_OK;
        }

    *ppv = nullptr;
    AkLogDebug("Interface not found");

    return E_NOINTERFACE;
}

ULONG AkVCam::MFVCamImpl::AddRef()
{
    AkLogFunction();

    return ++this->d->m_refCount;
}

ULONG AkVCam::MFVCamImpl::Release()
{
    AkLogFunction();
    ULONG ref = --this->d->m_refCount;

    if (ref == 0)
        delete this;

    return ref;
}

HRESULT AkVCam::MFVCamImpl::GetItem(const GUID &guidKey, PROPVARIANT *pValue)
{
    UNUSED(guidKey);
    UNUSED(pValue);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetItemType(const GUID &guidKey, MF_ATTRIBUTE_TYPE *pType)
{
    UNUSED(guidKey);
    UNUSED(pType);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::CompareItem(const GUID &guidKey,
                                        const PROPVARIANT &Value,
                                        WINBOOL *pbResult)
{
    UNUSED(guidKey);
    UNUSED(Value);
    UNUSED(pbResult);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::Compare(IMFAttributes *pTheirs,
                                    MF_ATTRIBUTES_MATCH_TYPE MatchType,
                                    WINBOOL *pbResult)
{
    UNUSED(pTheirs);
    UNUSED(MatchType);
    UNUSED(pbResult);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetUINT32(const GUID &guidKey, UINT32 *punValue)
{
    UNUSED(guidKey);
    UNUSED(punValue);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetUINT64(const GUID &guidKey, UINT64 *punValue)
{
    UNUSED(guidKey);
    UNUSED(punValue);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetDouble(const GUID &guidKey, double *pfValue)
{
    UNUSED(guidKey);
    UNUSED(pfValue);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetGUID(const GUID &guidKey, GUID *pguidValue)
{
    UNUSED(guidKey);
    UNUSED(pguidValue);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetStringLength(const GUID &guidKey, UINT32 *pcchLength)
{
    UNUSED(guidKey);
    UNUSED(pcchLength);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetString(const GUID &guidKey,
                                      LPWSTR pwszValue,
                                      UINT32 cchBufSize,
                                      UINT32 *pcchLength)
{
    UNUSED(guidKey);
    UNUSED(pwszValue);
    UNUSED(cchBufSize);
    UNUSED(pcchLength);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetAllocatedString(const GUID &guidKey,
                                               LPWSTR *ppwszValue,
                                               UINT32 *pcchLength)
{
    UNUSED(guidKey);
    UNUSED(ppwszValue);
    UNUSED(pcchLength);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetBlobSize(const GUID &guidKey,
                                        UINT32 *pcbBlobSize)
{
    UNUSED(guidKey);
    UNUSED(pcbBlobSize);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetBlob(const GUID &guidKey,
                                    UINT8 *pBuf,
                                    UINT32 cbBufSize,
                                    UINT32 *pcbBlobSize)
{
    UNUSED(guidKey);
    UNUSED(pBuf);
    UNUSED(cbBufSize);
    UNUSED(pcbBlobSize);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetAllocatedBlob(const GUID &guidKey,
                                             UINT8 **ppBuf,
                                             UINT32 *pcbSize)
{
    UNUSED(guidKey);
    UNUSED(ppBuf);
    UNUSED(pcbSize);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetUnknown(const GUID &guidKey,
                                       const IID &riid,
                                       LPVOID *ppv)
{
    UNUSED(guidKey);
    UNUSED(riid);
    UNUSED(ppv);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::SetItem(const GUID &guidKey,
                                    const PROPVARIANT &Value)
{
    UNUSED(guidKey);
    UNUSED(Value);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::DeleteItem(const GUID &guidKey)
{
    UNUSED(guidKey);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::DeleteAllItems()
{
    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::SetUINT32(const GUID &guidKey, UINT32 unValue)
{
    UNUSED(guidKey);
    UNUSED(unValue);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::SetUINT64(const GUID &guidKey, UINT64 unValue)
{
    UNUSED(guidKey);
    UNUSED(unValue);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::SetDouble(const GUID &guidKey, double fValue)
{
    UNUSED(guidKey);
    UNUSED(fValue);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::SetGUID(const GUID &guidKey, const GUID &guidValue)
{
    UNUSED(guidKey);
    UNUSED(guidValue);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::SetString(const GUID &guidKey, LPCWSTR wszValue)
{
    UNUSED(guidKey);
    UNUSED(wszValue);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::SetBlob(const GUID &guidKey,
                                    const UINT8 *pBuf,
                                    UINT32 cbBufSize)
{
    UNUSED(guidKey);
    UNUSED(pBuf);
    UNUSED(cbBufSize);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::SetUnknown(const GUID &guidKey, IUnknown *pUnknown)
{
    UNUSED(guidKey);
    UNUSED(pUnknown);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::LockStore()
{
    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::UnlockStore()
{
    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetCount(UINT32 *pcItems)
{
    UNUSED(pcItems);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetItemByIndex(UINT32 unIndex,
                                           GUID *pguidKey,
                                           PROPVARIANT *pValue)
{
    UNUSED(unIndex);
    UNUSED(pguidKey);
    UNUSED(pValue);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::CopyAllItems(IMFAttributes *pDest)
{
    UNUSED(pDest);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::AddDeviceSourceInfo(LPCWSTR deviceSourceInfo)
{
    UNUSED(deviceSourceInfo);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::AddProperty(const DEVPROPKEY *pKey,
                                        DEVPROPTYPE type,
                                        const BYTE *pbData,
                                        ULONG cbData)
{
    UNUSED(pKey);
    UNUSED(type);
    UNUSED(pbData);
    UNUSED(cbData);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::AddRegistryEntry(LPCWSTR entryName,
                                             LPCWSTR subkeyPath,
                                             DWORD dwRegType,
                                             const BYTE *pbData,
                                             ULONG cbData)
{
    UNUSED(entryName);
    UNUSED(subkeyPath);
    UNUSED(dwRegType);
    UNUSED(pbData);
    UNUSED(cbData);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::Start(IMFAsyncCallback *pCallback)
{
    UNUSED(pCallback);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::Stop()
{
    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::Remove()
{
    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::GetMediaSource(IMFMediaSource **ppMediaSource)
{
    UNUSED(ppMediaSource);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::SendCameraProperty(const GUID &propertySet,
                                               ULONG propertyId,
                                               ULONG propertyFlags,
                                               void *propertyPayload,
                                               ULONG propertyPayloadLength,
                                               void *data,
                                               ULONG dataLength,
                                               ULONG *dataWritten)
{
    UNUSED(propertySet);
    UNUSED(propertyId);
    UNUSED(propertyFlags);
    UNUSED(propertyPayload);
    UNUSED(propertyPayloadLength);
    UNUSED(data);
    UNUSED(dataLength);
    UNUSED(dataWritten);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::CreateSyncEvent(const GUID &kseventSet,
                                            ULONG kseventId,
                                            ULONG kseventFlags,
                                            HANDLE eventHandle,
                                            IMFCamSyncObject **cameraSyncObject)
{
    UNUSED(kseventSet);
    UNUSED(kseventId);
    UNUSED(kseventFlags);
    UNUSED(eventHandle);
    UNUSED(cameraSyncObject);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::CreateSyncSemaphore(const GUID &kseventSet,
                                                ULONG kseventId,
                                                ULONG kseventFlags,
                                                HANDLE semaphoreHandle,
                                                LONG semaphoreAdjustment,
                                                IMFCamSyncObject **cameraSyncObject)
{
    UNUSED(kseventSet);
    UNUSED(kseventId);
    UNUSED(kseventFlags);
    UNUSED(semaphoreHandle);
    UNUSED(semaphoreAdjustment);
    UNUSED(cameraSyncObject);

    return S_OK;
}

HRESULT AkVCam::MFVCamImpl::Shutdown()
{
    return S_OK;
}
