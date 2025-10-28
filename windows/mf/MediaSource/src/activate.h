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

#ifndef ACTIVATE_H
#define ACTIVATE_H

#include <mfobjects.h>

#include "attributes.h"
#include "PlatformUtils/src/cunknown.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class ActivatePrivate;

    class Activate: IMFActivate
    {
        public:
            Activate(const CLSID &clsid);
            virtual ~Activate();

            DECLARE_ATTRIBUTES

            // IUnknown

            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                                     void **ppv) override;
            ULONG STDMETHODCALLTYPE AddRef() override;
            ULONG STDMETHODCALLTYPE Release() override;

            // IMFActivate

            HRESULT STDMETHODCALLTYPE ActivateObject(REFIID riid,
                                                     void **ppv) override;
            HRESULT STDMETHODCALLTYPE DetachObject() override;
            HRESULT STDMETHODCALLTYPE ShutdownObject() override;

        private:
            ActivatePrivate *d;
    };
}

#endif // ACTIVATE_H
