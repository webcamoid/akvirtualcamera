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

#include <mfidl.h>

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

#endif ATTRIBUTES_H
