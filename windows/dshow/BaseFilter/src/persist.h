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

#ifndef PERSIST_H
#define PERSIST_H

#include "PlatformUtils/src/cunknown.h"

namespace AkVCam
{
    class PersistPrivate;

    class Persist:
            public IPersist,
            public CUnknown
    {
        public:
            Persist(REFIID classCLSID);
            virtual ~Persist();

            DECLARE_IUNKNOWN(IID_IPersist)

            // IPersist
            HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID) override;

        private:
            PersistPrivate *d;
    };
}

#define DECLARE_IPERSIST_NQ \
    DECLARE_IUNKNOWN_NQ \
    \
    HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID) override \
    { \
        return Persist::GetClassID(pClassID); \
    }

#define DECLARE_IPERSIST(interfaceIid) \
    DECLARE_IUNKNOWN_Q(interfaceIid) \
    DECLARE_IPERSIST_NQ

#endif // PERSIST_H
