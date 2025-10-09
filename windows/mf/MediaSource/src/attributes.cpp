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

#include <mfapi.h>

#include "attributes.h"

namespace AkVCam
{
    class AttributesPrivate
    {
        public:
            IMFAttributes *m_pAttributes {nullptr};
    };
}

AkVCam::Attributes::Attributes(UINT32 cInitialSize)
{
    this->d = new AttributesPrivate;
    MFCreateAttributes(&this->d->m_pAttributes, cInitialSize);
}

AkVCam::Attributes::~Attributes()
{
    this->d->m_pAttributes->Release();
    delete this->d;
}

HRESULT AkVCam::Attributes::GetItem(const GUID &guidKey, PROPVARIANT *pValue)
{
    return this->d->m_pAttributes->GetItem(guidKey, pValue);
}

HRESULT AkVCam::Attributes::GetItemType(const GUID &guidKey,
                                        MF_ATTRIBUTE_TYPE *pType)
{
    return this->d->m_pAttributes->GetItemType(guidKey, pType);
}

HRESULT AkVCam::Attributes::CompareItem(const GUID &guidKey,
                                        const PROPVARIANT &Value,
                                        WINBOOL *pbResult)
{
    return this->d->m_pAttributes->CompareItem(guidKey, Value, pbResult);
}

HRESULT AkVCam::Attributes::Compare(IMFAttributes *pTheirs,
                                    MF_ATTRIBUTES_MATCH_TYPE MatchType,
                                    WINBOOL *pbResult)
{
    return this->d->m_pAttributes->Compare(pTheirs, MatchType, pbResult);
}

HRESULT AkVCam::Attributes::GetUINT32(const GUID &guidKey, UINT32 *punValue)
{
    return this->d->m_pAttributes->GetUINT32(guidKey, punValue);
}

HRESULT AkVCam::Attributes::GetUINT64(const GUID &guidKey, UINT64 *punValue)
{
    return this->d->m_pAttributes->GetUINT64(guidKey, punValue);
}

HRESULT AkVCam::Attributes::GetDouble(const GUID &guidKey, double *pfValue)
{
    return this->d->m_pAttributes->GetDouble(guidKey, pfValue);
}

HRESULT AkVCam::Attributes::GetGUID(const GUID &guidKey, GUID *pguidValue)
{
    return this->d->m_pAttributes->GetGUID(guidKey, pguidValue);
}

HRESULT AkVCam::Attributes::GetStringLength(const GUID &guidKey,
                                            UINT32 *pcchLength)
{
    return this->d->m_pAttributes->GetStringLength(guidKey, pcchLength);
}

HRESULT AkVCam::Attributes::GetString(const GUID &guidKey,
                                      LPWSTR pwszValue,
                                      UINT32 cchBufSize,
                                      UINT32 *pcchLength)
{
    return this->d->m_pAttributes->GetString(guidKey,
                                             pwszValue,
                                             cchBufSize,
                                             pcchLength);
}

HRESULT AkVCam::Attributes::GetAllocatedString(const GUID &guidKey,
                                               LPWSTR *ppwszValue,
                                               UINT32 *pcchLength)
{
    return this->d->m_pAttributes->GetAllocatedString(guidKey,
                                                      ppwszValue,
                                                      pcchLength);
}

HRESULT AkVCam::Attributes::GetBlobSize(const GUID &guidKey,
                                        UINT32 *pcbBlobSize)
{
    return this->d->m_pAttributes->GetBlobSize(guidKey, pcbBlobSize);
}

HRESULT AkVCam::Attributes::GetBlob(const GUID &guidKey,
                                    UINT8 *pBuf,
                                    UINT32 cbBufSize,
                                    UINT32 *pcbBlobSize)
{
    return this->d->m_pAttributes->GetBlob(guidKey,
                                           pBuf,
                                           cbBufSize,
                                           pcbBlobSize);
}

HRESULT AkVCam::Attributes::GetAllocatedBlob(const GUID &guidKey,
                                             UINT8 **ppBuf,
                                             UINT32 *pcbSize)
{
    return this->d->m_pAttributes->GetAllocatedBlob(guidKey, ppBuf, pcbSize);
}

HRESULT AkVCam::Attributes::GetUnknown(const GUID &guidKey,
                                       const IID &riid,
                                       LPVOID *ppv)
{
    return this->d->m_pAttributes->GetUnknown(guidKey, riid, ppv);
}

HRESULT AkVCam::Attributes::SetItem(const GUID &guidKey,
                                    const PROPVARIANT &Value)
{
    return this->d->m_pAttributes->SetItem(guidKey, Value);
}

HRESULT AkVCam::Attributes::DeleteItem(const GUID &guidKey)
{
    return this->d->m_pAttributes->DeleteItem(guidKey);
}

HRESULT AkVCam::Attributes::DeleteAllItems()
{
    return this->d->m_pAttributes->DeleteAllItems();
}

HRESULT AkVCam::Attributes::SetUINT32(const GUID &guidKey, UINT32 unValue)
{
    return this->d->m_pAttributes->SetUINT32(guidKey, unValue);
}

HRESULT AkVCam::Attributes::SetUINT64(const GUID &guidKey, UINT64 unValue)
{
    return this->d->m_pAttributes->SetUINT64(guidKey, unValue);
}

HRESULT AkVCam::Attributes::SetDouble(const GUID &guidKey, double fValue)
{
    return this->d->m_pAttributes->SetDouble(guidKey, fValue);
}

HRESULT AkVCam::Attributes::SetGUID(const GUID &guidKey, const GUID &guidValue)
{
    return this->d->m_pAttributes->SetGUID(guidKey, guidValue);
}

HRESULT AkVCam::Attributes::SetString(const GUID &guidKey, LPCWSTR wszValue)
{
    return this->d->m_pAttributes->SetString(guidKey, wszValue);
}

HRESULT AkVCam::Attributes::SetBlob(const GUID &guidKey,
                                    const UINT8 *pBuf,
                                    UINT32 cbBufSize)
{
    return this->d->m_pAttributes->SetBlob(guidKey, pBuf, cbBufSize);
}

HRESULT AkVCam::Attributes::SetUnknown(const GUID &guidKey, IUnknown *pUnknown)
{
    return this->d->m_pAttributes->SetUnknown(guidKey, pUnknown);
}

HRESULT AkVCam::Attributes::LockStore()
{
    return this->d->m_pAttributes->LockStore();
}

HRESULT AkVCam::Attributes::UnlockStore()
{
    return this->d->m_pAttributes->UnlockStore();
}

HRESULT AkVCam::Attributes::GetCount(UINT32 *pcItems)
{
    return this->d->m_pAttributes->GetCount(pcItems);
}

HRESULT AkVCam::Attributes::GetItemByIndex(UINT32 unIndex,
                                           GUID *pguidKey,
                                           PROPVARIANT *pValue)
{
    return this->d->m_pAttributes->GetItemByIndex(unIndex, pguidKey, pValue);
}

HRESULT AkVCam::Attributes::CopyAllItems(IMFAttributes *pDest)
{
    return this->d->m_pAttributes->CopyAllItems(pDest);
}
