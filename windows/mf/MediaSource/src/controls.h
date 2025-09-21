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

#ifndef CONTROLS_H
#define CONTROLS_H

#include <windows.h>
#include <initguid.h>
#include <ks.h>
#include <ksmedia.h>
#include <strmif.h>

#include "PlatformUtils/src/cunknown.h"
#include "VCamUtils/src/utils.h"

DEFINE_GUID(IID_VCamControl, 0x28f54685, 0x06fd, 0x11d2, 0xb2, 0x7a, 0x00, 0xa0, 0xc9, 0x22, 0x31, 0x96);

namespace AkVCam
{
    class ControlsPrivate;

    class Controls:
        public IAMVideoProcAmp,
        public CUnknown
    {
        AKVCAM_SIGNAL(PropertyChanged,
                      KSPROPERTY_VIDCAP_VIDEOPROCAMP Property,
                      LONG lValue,
                      LONG Flags)

        public:
            Controls();
            virtual ~Controls();

            DECLARE_IUNKNOWN_NQ

            LONG value(const std::string &property) const;

            // IUnknown

            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                                     void **ppvObject);

            // IKsControl

            NTSTATUS STDMETHODCALLTYPE KsEvent(PKSEVENT event,
                                               ULONG eventLength,
                                               PVOID eventData,
                                               ULONG dataLength,
                                               ULONG *bytesReturned);
            NTSTATUS STDMETHODCALLTYPE KsMethod(PKSMETHOD method,
                                                ULONG methodLength,
                                                PVOID methodData,
                                                ULONG dataLength,
                                                ULONG *bytesReturned);
            NTSTATUS STDMETHODCALLTYPE KsProperty(PKSPROPERTY property,
                                                  ULONG propertyLength,
                                                  PVOID propertyData,
                                                  ULONG dataLength,
                                                  ULONG *bytesReturned);

            // IAMVideoProcAmp

            HRESULT STDMETHODCALLTYPE GetRange(LONG property,
                                               LONG *pMin,
                                               LONG *pMax,
                                               LONG *pSteppingDelta,
                                               LONG *pDefault,
                                               LONG *pCapsFlags) override;
            HRESULT STDMETHODCALLTYPE Set(LONG property,
                                          LONG lValue,
                                          LONG flags) override;
            HRESULT STDMETHODCALLTYPE Get(LONG property,
                                          LONG *lValue,
                                          LONG *flags) override;

        private:
            ControlsPrivate *d;

        friend class ControlsPrivate;
    };
}

#endif // CONTROLS_H
