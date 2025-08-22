/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2020  Gonzalo Exequiel Pedone
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

#ifndef PROPERTYSET_H
#define PROPERTYSET_H

#include <strmif.h>

#include "PlatformUtils/src/cunknown.h"

namespace AkVCam
{
    class PropertySet:
            public IKsPropertySet,
            public CUnknown
    {
        public:
            PropertySet();
            virtual ~PropertySet();

            DECLARE_IUNKNOWN(IID_IKsPropertySet)

            // IKsPropertySet
            HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet,
                                          DWORD dwPropID,
                                          LPVOID pInstanceData,
                                          DWORD cbInstanceData,
                                          LPVOID pPropData,
                                          DWORD cbPropData) override;
            HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet,
                                          DWORD dwPropID,
                                          LPVOID pInstanceData,
                                          DWORD cbInstanceData,
                                          LPVOID pPropData,
                                          DWORD cbPropData,
                                          DWORD *pcbReturned) override;
            HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet,
                                                     DWORD dwPropID,
                                                     DWORD *pTypeSupport) override;
    };
}

#endif // PROPERTYSET_H
