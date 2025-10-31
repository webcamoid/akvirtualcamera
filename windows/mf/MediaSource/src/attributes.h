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
            auto hr = attributes()->GetItem(guidKey, pValue); \
            \
            if (FAILED(hr)) \
                AkLogError("GetItem failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetItemType(REFGUID guidKey, \
                                              MF_ATTRIBUTE_TYPE *pType) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->GetItemType(guidKey, pType); \
            \
            if (FAILED(hr)) \
                AkLogError("GetItemType failed: 0x%x", hr); \
            else if (pType) \
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
            auto hr = attributes()->CompareItem(guidKey, Value, pbResult); \
            \
            if (FAILED(hr)) \
                AkLogError("CompareItem failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE Compare(IMFAttributes *pTheirs, \
                                          MF_ATTRIBUTES_MATCH_TYPE MatchType, \
                                          BOOL *pbResult) override \
        { \
            AkLogFunction(); \
            auto hr = attributes()->Compare(pTheirs, MatchType, pbResult); \
            \
            if (FAILED(hr)) \
                AkLogError("Compare failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetUINT32(REFGUID guidKey, \
                                            UINT32 *punValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->GetUINT32(guidKey, punValue); \
            \
            if (FAILED(hr)) \
                AkLogError("GetUINT32 failed: 0x%x", hr); \
            else if (punValue) \
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
            if (FAILED(hr)) \
                AkLogError("GetUINT64 failed: 0x%x", hr); \
            else if (punValue) \
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
            if (FAILED(hr)) \
                AkLogError("GetDouble failed: 0x%x", hr); \
            else if (pfValue) \
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
            if (FAILED(hr)) \
                AkLogError("GetGUID failed: 0x%x", hr); \
            else if (pguidValue) \
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
            if (FAILED(hr)) \
                AkLogError("GetStringLength failed: 0x%x", hr); \
            else if (pcchLength) \
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
            if (FAILED(hr)) \
                AkLogError("GetString failed: 0x%x", hr); \
            else if (pwszValue) \
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
            auto hr = attributes()->GetAllocatedString(guidKey, \
                                                       ppwszValue, \
                                                       pcchLength); \
            \
            if (FAILED(hr)) \
                AkLogError("GetAllocatedString failed: 0x%x", hr); \
            else if (ppwszValue) \
                AkLogDebug("Value: %s", stringFromWSTR(*ppwszValue).c_str()); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetBlobSize(REFGUID guidKey, \
                                              UINT32 *pcbBlobSize) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->GetBlobSize(guidKey, pcbBlobSize); \
            \
            if (FAILED(hr)) \
                AkLogError("GetBlobSize failed: 0x%x", hr); \
            else if (pcbBlobSize) \
                AkLogDebug("Value: %u", *pcbBlobSize); \
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
            auto hr = attributes()->GetBlob(guidKey, \
                                            pBuf, \
                                            cbBufSize, \
                                            pcbBlobSize); \
            \
            if (FAILED(hr)) \
                AkLogError("GetBlob failed: 0x%x", hr); \
            else if (pcbBlobSize) \
                AkLogDebug("Value: %u", *pcbBlobSize); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetAllocatedBlob(REFGUID guidKey, \
                                                   UINT8 **ppBuf, \
                                                   UINT32 *pcbSize) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->GetAllocatedBlob(guidKey, ppBuf, pcbSize);\
            \
            if (FAILED(hr)) \
                AkLogError("GetAllocatedBlob failed: 0x%x", hr); \
            else if (pcbSize) \
                AkLogDebug("Value: %u", *pcbSize); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetUnknown(REFGUID guidKey, \
                                             REFIID riid, \
                                             LPVOID *ppv) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            AkLogDebug("REFIID: %s", stringFromClsidMF(riid).c_str()); \
            auto hr = attributes()->GetUnknown(guidKey, riid, ppv); \
            \
            if (FAILED(hr)) \
                AkLogError("GetUnknown failed: 0x%x", hr); \
            else if (ppv) \
                AkLogDebug("Value: 0x%p", *ppv); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetItem(REFGUID guidKey, \
                                          REFPROPVARIANT Value) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->SetItem(guidKey, Value); \
            \
            if (FAILED(hr)) \
                AkLogError("SetItem failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE DeleteItem(REFGUID guidKey) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->DeleteItem(guidKey); \
            \
            if (FAILED(hr)) \
                AkLogError("DeleteItem failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE DeleteAllItems() override \
        { \
            AkLogFunction(); \
            auto hr = attributes()->DeleteAllItems(); \
            \
            if (FAILED(hr)) \
                AkLogError("DeleteAllItems failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetUINT32(REFGUID guidKey, \
                                            UINT32 unValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            AkLogDebug("Value: %u", unValue); \
            auto hr = attributes()->SetUINT32(guidKey, unValue); \
            \
            if (FAILED(hr)) \
                AkLogError("SetUINT32 failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetUINT64(REFGUID guidKey, \
                                            UINT64 unValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            AkLogDebug("Value: %" PRIu64, unValue); \
            auto hr = attributes()->SetUINT64(guidKey, unValue); \
            \
            if (FAILED(hr)) \
                AkLogError("SetUINT64 failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetDouble(REFGUID guidKey, \
                                            double fValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            AkLogDebug("Value: %f", fValue); \
            auto hr = attributes()->SetDouble(guidKey, fValue); \
            \
            if (FAILED(hr)) \
                AkLogError("SetDouble failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetGUID(REFGUID guidKey, \
                                          REFGUID guidValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            AkLogDebug("Value: %s", stringFromClsidMF(guidValue).c_str()); \
            auto hr = attributes()->SetGUID(guidKey, guidValue); \
            \
            if (FAILED(hr)) \
                AkLogError("SetGUID failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetString(REFGUID guidKey, \
                                            LPCWSTR wszValue) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            AkLogDebug("Value: %s", stringFromWSTR(wszValue).c_str()); \
            auto hr = attributes()->SetString(guidKey, wszValue); \
            \
            if (FAILED(hr)) \
                AkLogError("SetString failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetBlob(REFGUID guidKey, \
                                          const UINT8 *pBuf, \
                                          UINT32 cbBufSize) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            auto hr = attributes()->SetBlob(guidKey, pBuf, cbBufSize); \
            \
            if (FAILED(hr)) \
                AkLogError("SetBlob failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE SetUnknown(REFGUID guidKey, \
                                             IUnknown *pUnknown) override \
        { \
            AkLogFunction(); \
            AkLogDebug("GUID: %s", stringFromClsidMF(guidKey).c_str()); \
            AkLogDebug("Value: 0x%p", pUnknown); \
            auto hr = attributes()->SetUnknown(guidKey, pUnknown); \
            \
            if (FAILED(hr)) \
                AkLogError("SetUnknown failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE LockStore() override \
        { \
            AkLogFunction(); \
            auto hr = attributes()->LockStore(); \
            \
            if (FAILED(hr)) \
                AkLogError("LockStore failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE UnlockStore() override \
        { \
            AkLogFunction(); \
            auto hr = attributes()->UnlockStore(); \
            \
            if (FAILED(hr)) \
                AkLogError("UnlockStore failed: 0x%x", hr); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE GetCount(UINT32 *pcItems) override \
        { \
            AkLogFunction(); \
            auto hr = attributes()->GetCount(pcItems); \
            \
            if (FAILED(hr)) \
                AkLogError("GetCount failed: 0x%x", hr); \
            else if (pcItems) \
                AkLogDebug("Value: %u", *pcItems); \
            \
            return hr; \
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
            if (FAILED(hr)) \
                AkLogError("GetItemByIndex failed: 0x%x", hr); \
            else if (pguidKey) \
                AkLogDebug("GUID: %s", stringFromClsidMF(*pguidKey).c_str()); \
            \
            return hr; \
        } \
        \
        HRESULT STDMETHODCALLTYPE CopyAllItems(IMFAttributes *pDest) override \
        { \
            AkLogFunction(); \
            auto hr = attributes()->CopyAllItems(pDest); \
            \
            if (FAILED(hr)) \
                AkLogError("CopyAllItems failed: 0x%x", hr); \
            \
            return hr; \
        }

#endif // ATTRIBUTES_H
