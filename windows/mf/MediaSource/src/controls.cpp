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

#include <map>
#include <vector>
#include <ntstatus.h>

#include "controls.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class ControlsPrivate
    {
        public:
            std::map<KSPROPERTY_VIDCAP_VIDEOPROCAMP, LONG> m_control;
    };

    class ProcAmpPrivate
    {
        public:
            KSPROPERTY_VIDCAP_VIDEOPROCAMP property;
            LONG min;
            LONG max;
            LONG step;
            LONG defaultValue;
            LONG flags;

            inline static const std::vector<ProcAmpPrivate> &controls()
            {
                static const std::vector<ProcAmpPrivate> controls {
                    {KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS , -255, 255, 1, 0, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL},
                    {KSPROPERTY_VIDEOPROCAMP_CONTRAST   , -255, 255, 1, 0, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL},
                    {KSPROPERTY_VIDEOPROCAMP_SATURATION , -255, 255, 1, 0, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL},
                    {KSPROPERTY_VIDEOPROCAMP_GAMMA      , -255, 255, 1, 0, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL},
                    {KSPROPERTY_VIDEOPROCAMP_HUE        , -359, 359, 1, 0, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL},
                    {KSPROPERTY_VIDEOPROCAMP_COLORENABLE,    0,   1, 1, 1, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL}
                };

                return controls;
            }

            static inline const ProcAmpPrivate *byProperty(KSPROPERTY_VIDCAP_VIDEOPROCAMP property)
            {
                for (auto &control: controls())
                    if (control.property == property)
                        return &control;

                return nullptr;
            }
    };
}

AkVCam::Controls::Controls():
    CUnknown(this, IID_VCamControl)
{
    this->d = new ControlsPrivate;
}

AkVCam::Controls::~Controls()
{
    delete this->d;
}

LONG AkVCam::Controls::value(KSPROPERTY_VIDCAP_VIDEOPROCAMP property) const
{
    if (!this->d->m_control.contains(property))
        return 0;

    return this->d->m_control[property];
}

NTSTATUS AkVCam::Controls::KsEvent(PKSEVENT event,
                                   ULONG eventLength,
                                   PVOID eventData,
                                   ULONG dataLength,
                                   ULONG *bytesReturned)
{
    AkLogFunction();

    UNUSED(event);
    UNUSED(eventLength);
    UNUSED(eventData);
    UNUSED(dataLength);
    UNUSED(bytesReturned);

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS AkVCam::Controls::KsMethod(PKSMETHOD method,
                                    ULONG methodLength,
                                    PVOID methodData,
                                    ULONG dataLength,
                                    ULONG *bytesReturned)
{
    AkLogFunction();

    UNUSED(method);
    UNUSED(methodLength);
    UNUSED(methodData);
    UNUSED(dataLength);
    UNUSED(bytesReturned);

    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS AkVCam::Controls::KsProperty(PKSPROPERTY property,
                                      ULONG propertyLength,
                                      PVOID propertyData,
                                      ULONG dataLength,
                                      ULONG *bytesReturned)
{
    AkLogFunction();

    if (!property
        || propertyLength < sizeof(KSPROPERTY)
        || !propertyData
        || !bytesReturned) {
        AkLogError() << "Invalid parameters";

        return STATUS_INVALID_PARAMETER;
    }

    *bytesReturned = 0;

    if (!IsEqualGUID(property->Set, PROPSETID_VIDCAP_VIDEOPROCAMP)) {
        AkLogWarning() << "Unsupported property set";

        return STATUS_NOT_IMPLEMENTED;
    }

    auto control = ProcAmpPrivate::byProperty(KSPROPERTY_VIDCAP_VIDEOPROCAMP(property->Id));

    if (!control) {
        AkLogWarning() << "Unsupported property ID: " << property->Id;

        return STATUS_NOT_FOUND;
    }

    if (dataLength < sizeof(KSPROPERTY_VIDEOPROCAMP_S)) {
        AkLogError() << "Insufficient data length: " << dataLength;

        return STATUS_BUFFER_TOO_SMALL;
    }

    auto procAmp = static_cast<KSPROPERTY_VIDEOPROCAMP_S *>(propertyData);

    if (property->Flags & KSPROPERTY_TYPE_GET) {
        procAmp->Value = this->d->m_control[control->property];
        procAmp->Flags = control->flags;
        procAmp->Capabilities = control->flags;
        *bytesReturned = sizeof(KSPROPERTY_VIDEOPROCAMP_S);
        AkLogInfo() << "Get property "
                    << property->Id
                    << ": Value="
                    << procAmp->Value
                    << ", Flags="
                    << std::hex
                    << procAmp->Flags;

        return STATUS_SUCCESS;
    } else if (property->Flags & KSPROPERTY_TYPE_SET) {
        LONG newValue = procAmp->Value;
        LONG newFlags = procAmp->Flags;

        if (newFlags & ~control->flags) {
            AkLogError() << "Unsupported flags for property "
                         << property->Id
                         << ": "
                         << std::hex
                         << newFlags;

            return STATUS_INVALID_PARAMETER;
        }

        if (newValue < control->min || newValue > control->max) {
            AkLogError() << "Value out of range for property "
                         << property->Id
                         << ": "
                         << newValue
                         << " (min="
                         << control->min
                         << ", max="
                         << control->max
                         << ")";

            return STATUS_INVALID_PARAMETER;
        }

        if ((newValue - control->min) % control->step != 0) {
            AkLogError() << "Invalid step for property "
                         << property->Id
                         << ": "
                         << newValue
                         << " (step="
                         << control->step
                         << ")";

            return STATUS_INVALID_PARAMETER;
        }

        if (this->d->m_control[control->property] != newValue) {
            this->d->m_control[control->property] = newValue;
            AkLogInfo() << "Set property "
                        << property->Id
                        << ": Value="
                        << newValue;
            AKVCAM_EMIT(this, PropertyChanged, control->property, newValue, property->Flags)
        }

        *bytesReturned = sizeof(KSPROPERTY_VIDEOPROCAMP_S);

        return STATUS_SUCCESS;
    }

    AkLogWarning() << "Unsupported property flags: "
                   << std::hex
                   << property->Flags;

    return STATUS_NOT_IMPLEMENTED;
}
