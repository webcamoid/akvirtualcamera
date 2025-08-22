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

#include <map>
#include <vector>

#include "videoprocamp.h"
#include "utils.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class VideoProcAmpPrivate
    {
        public:
            std::map<LONG, LONG> m_control;
    };

    class ProcAmpPrivate
    {
        public:
            LONG property;
            LONG min;
            LONG max;
            LONG step;
            LONG defaultValue;
            LONG flags;

            inline static const std::vector<ProcAmpPrivate> &controls()
            {
                static const std::vector<ProcAmpPrivate> controls {
                    {VideoProcAmp_Brightness , -255, 255, 1, 0, VideoProcAmp_Flags_Manual},
                    {VideoProcAmp_Contrast   , -255, 255, 1, 0, VideoProcAmp_Flags_Manual},
                    {VideoProcAmp_Saturation , -255, 255, 1, 0, VideoProcAmp_Flags_Manual},
                    {VideoProcAmp_Gamma      , -255, 255, 1, 0, VideoProcAmp_Flags_Manual},
                    {VideoProcAmp_Hue        , -359, 359, 1, 0, VideoProcAmp_Flags_Manual},
                    {VideoProcAmp_ColorEnable,    0,   1, 1, 1, VideoProcAmp_Flags_Manual}
                };

                return controls;
            }

            static inline const ProcAmpPrivate *byProperty(LONG property)
            {
                for (auto &control: controls())
                    if (control.property == property)
                        return &control;

                return nullptr;
            }
    };
}

AkVCam::VideoProcAmp::VideoProcAmp():
    CUnknown(this, IID_IAMVideoProcAmp)
{
    this->d = new VideoProcAmpPrivate;
}

AkVCam::VideoProcAmp::~VideoProcAmp()
{
    delete this->d;
}

HRESULT AkVCam::VideoProcAmp::GetRange(LONG property,
                                       LONG *pMin,
                                       LONG *pMax,
                                       LONG *pSteppingDelta,
                                       LONG *pDefault,
                                       LONG *pCapsFlags)
{
    AkLogFunction();

    if (!pMin || !pMax || !pSteppingDelta || !pDefault || !pCapsFlags)
        return E_POINTER;

    *pMin = 0;
    *pMax = 0;
    *pSteppingDelta = 0;
    *pDefault = 0;
    *pCapsFlags = 0;

    for (auto &control: ProcAmpPrivate::controls())
        if (control.property == property) {
            *pMin = control.min;
            *pMax = control.max;
            *pSteppingDelta = control.step;
            *pDefault = control.defaultValue;
            *pCapsFlags = control.flags;

            return S_OK;
        }

    return E_FAIL;
}

HRESULT AkVCam::VideoProcAmp::Set(LONG property, LONG lValue, LONG flags)
{
    AkLogFunction();

    for (auto &control: ProcAmpPrivate::controls())
        if (control.property == property) {
            if (lValue < control.min
                || lValue > control.max
                || flags != control.flags)
                return E_INVALIDARG;

            this->d->m_control[property] = lValue;
            AKVCAM_EMIT(this, PropertyChanged, property, lValue, flags)

            return S_OK;
        }

    return E_FAIL;
}

HRESULT AkVCam::VideoProcAmp::Get(LONG property, LONG *lValue, LONG *flags)
{
    AkLogFunction();

    if (!lValue || !flags)
        return E_POINTER;

    *lValue = 0;
    *flags = 0;

    for (auto &control: ProcAmpPrivate::controls())
        if (control.property == property) {
            if (this->d->m_control.count(property))
                *lValue = this->d->m_control[property];
            else
                *lValue = control.defaultValue;

            *flags = control.flags;

            return S_OK;
        }

    return E_FAIL;
}
