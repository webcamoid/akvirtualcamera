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

#ifndef VIDEOPROCAMP_H
#define VIDEOPROCAMP_H

#include <strmif.h>

#include "cunknown.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class VideoProcAmpPrivate;

    class VideoProcAmp:
            public IAMVideoProcAmp,
            public CUnknown
    {
        AKVCAM_SIGNAL(PropertyChanged, LONG Property,  LONG lValue, LONG Flags)

        public:
            VideoProcAmp();
            virtual ~VideoProcAmp();

            DECLARE_IUNKNOWN(IID_IAMVideoProcAmp)

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
            VideoProcAmpPrivate *d;

        friend class VideoProcAmpPrivate;
    };
}

#endif // VIDEOPROCAMP_H
