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

#include <initguid.h>

#include "mfvcamimpl.h"
#include "VCamUtils/src/utils.h"

DEFINE_GUID(IID_IMFVCam, 0x12345678, 0x1234, 0x5678, 0x90, 0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56, 0x78);

class MFVCamImplPrivate
{
    public:
        ULONG m_refCount {1};
        std::wstring m_friendlyName;
        std::wstring m_sourceId;
};

MFVCamImpl::MFVCamImpl(LPCWSTR friendlyName, LPCWSTR sourceId)
{
    this->d = new MFVCamImplPrivate;

    if (friendlyName)
        this->d->m_friendlyName = friendlyName;

    if (sourceId)
        this->d->m_sourceId = sourceId;
}

MFVCamImpl::~MFVCamImpl()
{
    delete this->d;
}

HRESULT MFVCamImpl::QueryInterface(const IID &riid, void **ppv)
{
    if (!ppv)
        return E_POINTER;

    if (riid == IID_IUnknown || riid == IID_IMFVCam) {
        *ppv = static_cast<IMFVCam *>(this);
        AddRef();

        return S_OK;
    }

    *ppv = nullptr;

    return E_NOINTERFACE;
}

ULONG MFVCamImpl::AddRef()
{
    return InterlockedIncrement(&this->d->m_refCount);
}

ULONG MFVCamImpl::Release()
{
    auto refCount = InterlockedDecrement(&this->d->m_refCount);

    if (refCount == 0)
        delete this;

    return refCount;
}

HRESULT MFVCamImpl::GetItem(const GUID &guidKey, PROPVARIANT *pValue)
{
    UNUSED(guidKey);
    UNUSED(pValue);

    return S_OK;
}

HRESULT MFVCamImpl::GetItemType(const GUID &guidKey, MF_ATTRIBUTE_TYPE *pType)
{
    UNUSED(guidKey);
    UNUSED(pType);

    return S_OK;
}

HRESULT MFVCamImpl::CompareItem(const GUID &guidKey,
                                const PROPVARIANT &Value,
                                WINBOOL *pbResult)
{
    UNUSED(guidKey);
    UNUSED(Value);
    UNUSED(pbResult);

    return S_OK;
}

HRESULT MFVCamImpl::Compare(IMFAttributes *pTheirs,
                            MF_ATTRIBUTES_MATCH_TYPE MatchType,
                            WINBOOL *pbResult)
{
    UNUSED(pTheirs);
    UNUSED(MatchType);
    UNUSED(pbResult);

    return S_OK;
}

HRESULT MFVCamImpl::GetUINT32(const GUID &guidKey, UINT32 *punValue)
{
    UNUSED(guidKey);
    UNUSED(punValue);

    return S_OK;
}

HRESULT MFVCamImpl::GetUINT64(const GUID &guidKey, UINT64 *punValue)
{
    UNUSED(guidKey);
    UNUSED(punValue);

    return S_OK;
}

HRESULT MFVCamImpl::GetDouble(const GUID &guidKey, double *pfValue)
{
    UNUSED(guidKey);
    UNUSED(pfValue);

    return S_OK;
}

HRESULT MFVCamImpl::GetGUID(const GUID &guidKey, GUID *pguidValue)
{
    UNUSED(guidKey);
    UNUSED(pguidValue);

    return S_OK;
}

HRESULT MFVCamImpl::GetStringLength(const GUID &guidKey, UINT32 *pcchLength)
{
    UNUSED(guidKey);
    UNUSED(pcchLength);

    return S_OK;
}

HRESULT MFVCamImpl::GetString(const GUID &guidKey,
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

HRESULT MFVCamImpl::GetAllocatedString(const GUID &guidKey,
                                       LPWSTR *ppwszValue,
                                       UINT32 *pcchLength)
{
    UNUSED(guidKey);
    UNUSED(ppwszValue);
    UNUSED(pcchLength);

    return S_OK;
}

HRESULT MFVCamImpl::GetBlobSize(const GUID &guidKey, UINT32 *pcbBlobSize)
{
    UNUSED(guidKey);
    UNUSED(pcbBlobSize);

    return S_OK;
}

HRESULT MFVCamImpl::GetBlob(const GUID &guidKey,
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

HRESULT MFVCamImpl::GetAllocatedBlob(const GUID &guidKey,
                                     UINT8 **ppBuf,
                                     UINT32 *pcbSize)
{
    UNUSED(guidKey);
    UNUSED(ppBuf);
    UNUSED(pcbSize);

    return S_OK;
}

HRESULT MFVCamImpl::GetUnknown(const GUID &guidKey,
                               const IID &riid,
                               LPVOID *ppv)
{
    UNUSED(guidKey);
    UNUSED(riid);
    UNUSED(ppv);

    return S_OK;
}

HRESULT MFVCamImpl::SetItem(const GUID &guidKey, const PROPVARIANT &Value)
{
    UNUSED(guidKey);
    UNUSED(Value);

    return S_OK;
}

HRESULT MFVCamImpl::DeleteItem(const GUID &guidKey)
{
    UNUSED(guidKey);

    return S_OK;
}

HRESULT MFVCamImpl::DeleteAllItems()
{
    return S_OK;
}

HRESULT MFVCamImpl::SetUINT32(const GUID &guidKey, UINT32 unValue)
{
    UNUSED(guidKey);
    UNUSED(unValue);

    return S_OK;
}

HRESULT MFVCamImpl::SetUINT64(const GUID &guidKey, UINT64 unValue)
{
    UNUSED(guidKey);
    UNUSED(unValue);

    return S_OK;
}

HRESULT MFVCamImpl::SetDouble(const GUID &guidKey, double fValue)
{
    UNUSED(guidKey);
    UNUSED(fValue);

    return S_OK;
}

HRESULT MFVCamImpl::SetGUID(const GUID &guidKey, const GUID &guidValue)
{
    UNUSED(guidKey);
    UNUSED(guidValue);

    return S_OK;
}

HRESULT MFVCamImpl::SetString(const GUID &guidKey, LPCWSTR wszValue)
{
    UNUSED(guidKey);
    UNUSED(wszValue);

    return S_OK;
}

HRESULT MFVCamImpl::SetBlob(const GUID &guidKey,
                            const UINT8 *pBuf,
                            UINT32 cbBufSize)
{
    UNUSED(guidKey);
    UNUSED(pBuf);
    UNUSED(cbBufSize);

    return S_OK;
}

HRESULT MFVCamImpl::SetUnknown(const GUID &guidKey, IUnknown *pUnknown)
{
    UNUSED(guidKey);
    UNUSED(pUnknown);

    return S_OK;
}

HRESULT MFVCamImpl::LockStore()
{
    return S_OK;
}

HRESULT MFVCamImpl::UnlockStore()
{
    return S_OK;
}

HRESULT MFVCamImpl::GetCount(UINT32 *pcItems)
{
    UNUSED(pcItems);

    return S_OK;
}

HRESULT MFVCamImpl::GetItemByIndex(UINT32 unIndex,
                                   GUID *pguidKey,
                                   PROPVARIANT *pValue)
{
    UNUSED(unIndex);
    UNUSED(pguidKey);
    UNUSED(pValue);

    return S_OK;
}

HRESULT MFVCamImpl::CopyAllItems(IMFAttributes *pDest)
{
    UNUSED(pDest);

    return S_OK;
}

HRESULT MFVCamImpl::AddDeviceSourceInfo(LPCWSTR deviceSourceInfo)
{
    UNUSED(deviceSourceInfo);

    return S_OK;
}

HRESULT MFVCamImpl::AddProperty(const DEVPROPKEY *pKey,
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

HRESULT MFVCamImpl::AddRegistryEntry(LPCWSTR entryName,
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

HRESULT MFVCamImpl::Start(IMFAsyncCallback *pCallback)
{
    UNUSED(pCallback);

    return S_OK;
}

HRESULT MFVCamImpl::Stop()
{
    return S_OK;
}

HRESULT MFVCamImpl::Remove()
{
    return S_OK;
}

HRESULT MFVCamImpl::GetMediaSource(IMFMediaSource **ppMediaSource)
{
    UNUSED(ppMediaSource);

    return S_OK;
}

HRESULT MFVCamImpl::SendCameraProperty(const GUID &propertySet,
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

HRESULT MFVCamImpl::CreateSyncEvent(const GUID &kseventSet,
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

HRESULT MFVCamImpl::CreateSyncSemaphore(const GUID &kseventSet,
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

HRESULT MFVCamImpl::Shutdown()
{
    return S_OK;
}
