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

#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include <windows.h>
#include <mfobjects.h>
#include <mfapi.h>

namespace AkVCam
{
    class AttributesPrivate;

    class Attributes: public IMFAttributes
    {
        public:
            Attributes(UINT32 cInitialSize=0);
            virtual ~Attributes();

            // IMFAttributes methods

            HRESULT STDMETHODCALLTYPE GetItem(REFGUID guidKey,
                                              PROPVARIANT *pValue) override;
            HRESULT STDMETHODCALLTYPE GetItemType(REFGUID guidKey,
                                                  MF_ATTRIBUTE_TYPE *pType) override;
            HRESULT STDMETHODCALLTYPE CompareItem(REFGUID guidKey,
                                                  REFPROPVARIANT Value,
                                                  BOOL *pbResult) override;
            HRESULT STDMETHODCALLTYPE Compare(IMFAttributes *pTheirs,
                                              MF_ATTRIBUTES_MATCH_TYPE MatchType,
                                              BOOL *pbResult) override;
            HRESULT STDMETHODCALLTYPE GetUINT32(REFGUID guidKey,
                                                UINT32 *punValue) override;
            HRESULT STDMETHODCALLTYPE GetUINT64(REFGUID guidKey,
                                                UINT64 *punValue) override;
            HRESULT STDMETHODCALLTYPE GetDouble(REFGUID guidKey,
                                                double *pfValue) override;
            HRESULT STDMETHODCALLTYPE GetGUID(REFGUID guidKey,
                                              GUID *pguidValue) override;
            HRESULT STDMETHODCALLTYPE GetStringLength(REFGUID guidKey,
                                                      UINT32 *pcchLength) override;
            HRESULT STDMETHODCALLTYPE GetString(REFGUID guidKey,
                                                LPWSTR pwszValue,
                                                UINT32 cchBufSize,
                                                UINT32 *pcchLength) override;
            HRESULT STDMETHODCALLTYPE GetAllocatedString(REFGUID guidKey,
                                                         LPWSTR *ppwszValue,
                                                         UINT32 *pcchLength) override;
            HRESULT STDMETHODCALLTYPE GetBlobSize(REFGUID guidKey,
                                                  UINT32 *pcbBlobSize) override;
            HRESULT STDMETHODCALLTYPE GetBlob(REFGUID guidKey,
                                              UINT8 *pBuf,
                                              UINT32 cbBufSize,
                                              UINT32 *pcbBlobSize) override;
            HRESULT STDMETHODCALLTYPE GetAllocatedBlob(REFGUID guidKey,
                                                       UINT8 **ppBuf,
                                                       UINT32 *pcbSize) override;
            HRESULT STDMETHODCALLTYPE GetUnknown(REFGUID guidKey,
                                                 REFIID riid,
                                                 LPVOID *ppv) override;
            HRESULT STDMETHODCALLTYPE SetItem(REFGUID guidKey,
                                              REFPROPVARIANT Value) override;
            HRESULT STDMETHODCALLTYPE DeleteItem(REFGUID guidKey) override;
            HRESULT STDMETHODCALLTYPE DeleteAllItems() override;
            HRESULT STDMETHODCALLTYPE SetUINT32(REFGUID guidKey,
                                                UINT32 unValue) override;
            HRESULT STDMETHODCALLTYPE SetUINT64(REFGUID guidKey,
                                                UINT64 unValue) override;
            HRESULT STDMETHODCALLTYPE SetDouble(REFGUID guidKey,
                                                double fValue) override;
            HRESULT STDMETHODCALLTYPE SetGUID(REFGUID guidKey,
                                              REFGUID guidValue) override;
            HRESULT STDMETHODCALLTYPE SetString(REFGUID guidKey,
                                                LPCWSTR wszValue) override;
            HRESULT STDMETHODCALLTYPE SetBlob(REFGUID guidKey,
                                              const UINT8 *pBuf,
                                              UINT32 cbBufSize) override;
            HRESULT STDMETHODCALLTYPE SetUnknown(REFGUID guidKey,
                                                 IUnknown *pUnknown) override;
            HRESULT STDMETHODCALLTYPE LockStore() override;
            HRESULT STDMETHODCALLTYPE UnlockStore() override;
            HRESULT STDMETHODCALLTYPE GetCount(UINT32 *pcItems) override;
            HRESULT STDMETHODCALLTYPE GetItemByIndex(UINT32 unIndex,
                                                     GUID *pguidKey,
                                                     PROPVARIANT *pValue) override;
            HRESULT STDMETHODCALLTYPE CopyAllItems(IMFAttributes *pDest) override;

        private:
            AttributesPrivate *d;

        friend class AttributesPrivate;
    };
}

#define DECLARE_IMFATTRIBUTES \
    HRESULT STDMETHODCALLTYPE GetItem(REFGUID guidKey, \
                                      PROPVARIANT *pValue) override \
    { \
        return Attributes::GetItem(guidKey, pValue); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetItemType(REFGUID guidKey, \
                                          MF_ATTRIBUTE_TYPE *pType) override \
    { \
        return Attributes::GetItemType(guidKey, pType); \
    } \
    \
    HRESULT STDMETHODCALLTYPE CompareItem(REFGUID guidKey, \
                                          REFPROPVARIANT Value, \
                                          WINBOOL *pbResult) override \
    { \
        return Attributes::CompareItem(guidKey, Value, pbResult); \
    } \
    \
    HRESULT STDMETHODCALLTYPE Compare(IMFAttributes *pTheirs, \
                                      MF_ATTRIBUTES_MATCH_TYPE MatchType, \
                                      BOOL *pbResult) override \
    { \
        return Attributes::Compare(pTheirs, MatchType, pbResult); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetUINT32(REFGUID guidKey, \
                                        UINT32 *punValue) override \
    { \
        return Attributes::GetUINT32(guidKey, punValue); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetUINT64(REFGUID guidKey, \
                                        UINT64 *punValue) override \
    { \
        return Attributes::GetUINT64(guidKey, punValue); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetDouble(REFGUID guidKey, \
                                        double *pfValue) override \
    { \
        return Attributes::GetDouble(guidKey, pfValue); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetGUID(REFGUID guidKey, \
                                      GUID *pguidValue) override \
    { \
        return Attributes::GetGUID(guidKey, pguidValue); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetStringLength(REFGUID guidKey, \
                                              UINT32 *pcchLength) override \
    { \
        return Attributes::GetStringLength(guidKey, pcchLength); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetString(REFGUID guidKey, \
                                        LPWSTR pwszValue, \
                                        UINT32 cchBufSize, \
                                        UINT32 *pcchLength) override \
    { \
        return Attributes::GetString(guidKey, \
                                     pwszValue, \
                                     cchBufSize, \
                                     pcchLength); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetAllocatedString(REFGUID guidKey, \
                                                 LPWSTR *ppwszValue, \
                                                 UINT32 *pcchLength) override \
    { \
        return Attributes::GetAllocatedString(guidKey, \
                                              ppwszValue, \
                                              pcchLength); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetBlobSize(REFGUID guidKey, \
                                          UINT32 *pcbBlobSize) override \
    { \
        return Attributes::GetBlobSize(guidKey, pcbBlobSize); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetBlob(REFGUID guidKey, \
                                      UINT8 *pBuf, \
                                      UINT32 cbBufSize, \
                                      UINT32 *pcbBlobSize) override \
    { \
        return Attributes::GetBlob(guidKey, \
                                   pBuf, \
                                   cbBufSize, \
                                   pcbBlobSize); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetAllocatedBlob(REFGUID guidKey, \
                                               UINT8 **ppBuf, \
                                               UINT32 *pcbSize) override \
    { \
        return Attributes::GetAllocatedBlob(guidKey, ppBuf, pcbSize); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetUnknown(REFGUID guidKey, \
                                         REFIID riid, \
                                         LPVOID *ppv) override \
    { \
        return Attributes::GetUnknown(guidKey, riid, ppv); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetItem(REFGUID guidKey, \
                                      REFPROPVARIANT Value) override \
    { \
        return Attributes::SetItem(guidKey, Value); \
    } \
    \
    HRESULT STDMETHODCALLTYPE DeleteItem(REFGUID guidKey) override \
    { \
        return Attributes::DeleteItem(guidKey); \
    } \
    \
    HRESULT STDMETHODCALLTYPE DeleteAllItems() override \
    { \
        return Attributes::DeleteAllItems(); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetUINT32(REFGUID guidKey, \
                                        UINT32 unValue) override \
    { \
        return Attributes::SetUINT32(guidKey, unValue); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetUINT64(REFGUID guidKey, \
                                        UINT64 unValue) override \
    { \
        return Attributes::SetUINT64(guidKey, unValue); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetDouble(REFGUID guidKey, \
                                        double fValue) override \
    { \
        return Attributes::SetDouble(guidKey, fValue); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetGUID(REFGUID guidKey, \
                                      REFGUID guidValue) override \
    { \
        return Attributes::SetGUID(guidKey, guidValue); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetString(REFGUID guidKey, \
                                        LPCWSTR wszValue) override \
    { \
        return Attributes::SetString(guidKey, wszValue); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetBlob(REFGUID guidKey, \
                                      const UINT8 *pBuf, \
                                      UINT32 cbBufSize) override \
    { \
        return Attributes::SetBlob(guidKey, pBuf, cbBufSize); \
    } \
    \
    HRESULT STDMETHODCALLTYPE SetUnknown(REFGUID guidKey, \
                                         IUnknown *pUnknown) override \
    { \
        return Attributes::SetUnknown(guidKey, pUnknown); \
    } \
    \
    HRESULT STDMETHODCALLTYPE LockStore() override \
    { \
        return Attributes::LockStore(); \
    } \
    \
    HRESULT STDMETHODCALLTYPE UnlockStore() override \
    { \
        return Attributes::UnlockStore(); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetCount(UINT32 *pcItems) override \
    { \
        return Attributes::GetCount(pcItems); \
    } \
    \
    HRESULT STDMETHODCALLTYPE GetItemByIndex(UINT32 unIndex, \
                                             GUID *pguidKey, \
                                             PROPVARIANT *pValue) override \
    { \
        return Attributes::GetItemByIndex(unIndex, pguidKey, pValue); \
    } \
    \
    HRESULT STDMETHODCALLTYPE CopyAllItems(IMFAttributes *pDest) override \
    { \
        return Attributes::CopyAllItems(pDest); \
    }

#endif // ATTRIBUTES_H
