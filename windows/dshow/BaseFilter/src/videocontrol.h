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

#ifndef VIDEOCONTROL_H
#define VIDEOCONTROL_H

#include <strmif.h>

#include "PlatformUtils/src/cunknown.h"

namespace AkVCam
{
    class VideoControlPrivate;
    class EnumPins;

    class VideoControl:
            public IAMVideoControl,
            public CUnknown
    {
        public:
            VideoControl(IEnumPins *enumPins);
            virtual ~VideoControl();

            DECLARE_IUNKNOWN(IID_IAMVideoControl)

            // IAMVideoControl
            HRESULT WINAPI GetCaps(IPin *pPin, LONG *pCapsFlags) override;
            HRESULT WINAPI SetMode(IPin *pPin, LONG Mode) override;
            HRESULT WINAPI GetMode(IPin *pPin, LONG *Mode) override;
            HRESULT WINAPI GetCurrentActualFrameRate(IPin *pPin,
                                                     LONGLONG *ActualFrameRate) override;
            HRESULT WINAPI GetMaxAvailableFrameRate(IPin *pPin,
                                                    LONG iIndex,
                                                    SIZE Dimensions,
                                                    LONGLONG *MaxAvailableFrameRate) override;
            HRESULT WINAPI GetFrameRateList(IPin *pPin,
                                            LONG iIndex,
                                            SIZE Dimensions,
                                            LONG *ListSize,
                                            LONGLONG **FrameRates) override;

        private:
            VideoControlPrivate *d;
    };
}

#endif // VIDEOCONTROL_H
