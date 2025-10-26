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

#include <cinttypes>
#include <memory>
#include <mfapi.h>

#include "PlatformUtils/src/utils.h"
#include "MFUtils/src/utils.h"

#define DECLARE_ATTRIBUTES \
    private: \
        std::shared_ptr<IMFAttributes> m_attributes; \
        \
        std::shared_ptr<IMFAttributes> attributes() \
        { \
            if (!this->m_attributes) {\
                IMFAttributes *attributes = nullptr; \
                MFCreateAttributes(&attributes, 0); \
                \
                this->m_attributes = \
                    std::shared_ptr<IMFAttributes>(attributes, \
                                                   [] (IMFAttributes *attributes) { \
                        attributes->Release(); \
                    }); \
            } \
            \
            return this->m_attributes; \
        } \
    \
    public: \
        HRESULT STDMETHODCALLTYPE GetItem(REFGUID guidKey, \
                                          PROPVARIANT *pValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            \
            return attributes()->GetItem(guidKey, pValue); \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetItemType(REFGUID guidKey, \
                                              MF_ATTRIBUTE_TYPE *pType) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->GetItemType(guidKey, pType); \
            \
            if (pType) \
                AkLogDebug("Type: %d", *pType); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE CompareItem(REFGUID guidKey, \
                                              REFPROPVARIANT Value, \
                                              BOOL *pbResult) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            \
            return attributes()->CompareItem(guidKey, Value, pbResult); \
        } \
        \
        HRESULT STDMETHODCALLTYPE Compare(IMFAttributes *pTheirs, \
                                          MF_ATTRIBUTES_MATCH_TYPE MatchType, \
                                          BOOL *pbResult) override \
        { \
            AkLogFunction(); \
            \
            return attributes()->Compare(pTheirs, MatchType, pbResult); \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetUINT32(REFGUID guidKey, \
                                            UINT32 *punValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->GetUINT32(guidKey, punValue); \
            \
            if (punValue) \
                AkLogDebug("Value: %u", *punValue); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetUINT64(REFGUID guidKey, \
                                            UINT64 *punValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->GetUINT64(guidKey, punValue); \
            \
            if (punValue) \
                AkLogDebug("Value: %" PRIu64, *punValue); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetDouble(REFGUID guidKey, \
                                            double *pfValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->GetDouble(guidKey, pfValue); \
            \
            if (pfValue) \
                AkLogDebug("Value: %f", *pfValue); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetGUID(REFGUID guidKey, \
                                          GUID *pguidValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->GetGUID(guidKey, pguidValue); \
            \
            if (pguidValue) \
                AkLogDebug("Value: %s", stringFromClsidMF(*pguidValue).c_str()); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetStringLength(REFGUID guidKey, \
                                                  UINT32 *pcchLength) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->GetStringLength(guidKey, pcchLength); \
            \
            if (pcchLength) \
                AkLogDebug("Value: %u", *pcchLength); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetString(REFGUID guidKey, \
                                            LPWSTR pwszValue, \
                                            UINT32 cchBufSize, \
                                            UINT32 *pcchLength) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->GetString(guidKey, \
                                              pwszValue, \
                                              cchBufSize, \
                                              pcchLength); \
            \
            if (pwszValue) \
                AkLogDebug("Value: %s", stringFromWSTR(pwszValue).c_str()); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetAllocatedString(REFGUID guidKey, \
                                                     LPWSTR *ppwszValue, \
                                                     UINT32 *pcchLength) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            \
            return attributes()->GetAllocatedString(guidKey, \
                                                    ppwszValue, \
                                                    pcchLength); \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetBlobSize(REFGUID guidKey, \
                                              UINT32 *pcbBlobSize) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->GetBlobSize(guidKey, pcbBlobSize); \
            \
            if (pcbBlobSize) \
                AkLogDebug("Value: %d", *pcbBlobSize); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetBlob(REFGUID guidKey, \
                                          UINT8 *pBuf, \
                                          UINT32 cbBufSize, \
                                          UINT32 *pcbBlobSize) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            \
            return attributes()->GetBlob(guidKey, \
                                         pBuf, \
                                         cbBufSize, \
                                         pcbBlobSize); \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetAllocatedBlob(REFGUID guidKey, \
                                                   UINT8 **ppBuf, \
                                                   UINT32 *pcbSize) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            \
            return attributes()->GetAllocatedBlob(guidKey, ppBuf, pcbSize); \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetUnknown(REFGUID guidKey, \
                                             REFIID riid, \
                                             LPVOID *ppv) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            \
            return attributes()->GetUnknown(guidKey, riid, ppv); \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetItem(REFGUID guidKey, \
                                          REFPROPVARIANT Value) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            \
            return attributes()->SetItem(guidKey, Value); \
        } \
        \
        HRESULT STDMETHODCALLTYPE DeleteItem(REFGUID guidKey) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            \
            return attributes()->DeleteItem(guidKey); \
        } \
        \
        HRESULT STDMETHODCALLTYPE DeleteAllItems() override \
        { \
            AkLogFunction(); \
            \
            return attributes()->DeleteAllItems(); \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetUINT32(REFGUID guidKey, \
                                            UINT32 unValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            AkLogDebug("Value: %u", unValue); \
            \
            return attributes()->SetUINT32(guidKey, unValue); \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetUINT64(REFGUID guidKey, \
                                            UINT64 unValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            AkLogDebug("Value: %" PRIu64, unValue); \
            \
            return attributes()->SetUINT64(guidKey, unValue); \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetDouble(REFGUID guidKey, \
                                            double fValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            AkLogDebug("Value: %f", fValue); \
            \
            return attributes()->SetDouble(guidKey, fValue); \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetGUID(REFGUID guidKey, \
                                          REFGUID guidValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            AkLogDebug("Value: %s", stringFromClsidMF(guidValue).c_str()); \
            \
            return attributes()->SetGUID(guidKey, guidValue); \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetString(REFGUID guidKey, \
                                            LPCWSTR wszValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            AkLogDebug("Value: %s", stringFromWSTR(wszValue).c_str()); \
            \
            return attributes()->SetString(guidKey, wszValue); \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetBlob(REFGUID guidKey, \
                                          const UINT8 *pBuf, \
                                          UINT32 cbBufSize) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            \
            return attributes()->SetBlob(guidKey, pBuf, cbBufSize); \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetUnknown(REFGUID guidKey, \
                                             IUnknown *pUnknown) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            \
            return attributes()->SetUnknown(guidKey, pUnknown); \
        } \
        \
        HRESULT STDMETHODCALLTYPE LockStore() override \
        { \
            AkLogFunction(); \
            \
            return attributes()->LockStore(); \
        } \
        \
        HRESULT STDMETHODCALLTYPE UnlockStore() override \
        { \
            AkLogFunction(); \
            \
            return attributes()->UnlockStore(); \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetCount(UINT32 *pcItems) override \
        { \
            AkLogFunction(); \
            \
            return attributes()->GetCount(pcItems); \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetItemByIndex(UINT32 unIndex, \
                                                 GUID *pguidKey, \
                                                 PROPVARIANT *pValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("Index: %u", unIndex); \
            auto hr = attributes()->GetItemByIndex(unIndex, pguidKey, pValue); \
            \
            if (pguidKey) \
                AkLogDebug("GUID: %s", stringFromClsidMF(*pguidKey).c_str()); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE CopyAllItems(IMFAttributes *pDest) override \
        { \
            AkLogFunction(); \
            \
            return attributes()->CopyAllItems(pDest); \
        }

#endif // ATTRIBUTES_H
