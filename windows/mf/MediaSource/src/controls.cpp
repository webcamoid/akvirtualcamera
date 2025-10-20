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

#include <cinttypes>
#include <map>
#include <vector>
#include <ntstatus.h>

#include "controls.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

DEFINE_GUID(IID_VIDEOPROCAMP, 0xc6e13360, 0x30ac, 0x11d0, 0xa1, 0x8c, 0x00, 0xa0,0xc9, 0x11, 0x89, 0x56);

namespace AkVCam
{
    class ControlsPrivate
    {
        public:
            std::map<std::string, LONG> m_control;
    };

    class ProcAmpPrivate
    {
        public:
            const char *name;
            KSPROPERTY_VIDCAP_VIDEOPROCAMP property;
            LONG propertyDS;
            LONG min;
            LONG max;
            LONG step;
            LONG defaultValue;
            LONG flags;
            LONG flagsDS;

            inline static const std::vector<ProcAmpPrivate> &controls()
            {
                static const std::vector<ProcAmpPrivate> controls {
                    {"Brightness" , KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS , VideoProcAmp_Brightness , -255, 255, 1, 0, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL, VideoProcAmp_Flags_Manual},
                    {"Contrast"   , KSPROPERTY_VIDEOPROCAMP_CONTRAST   , VideoProcAmp_Contrast   , -255, 255, 1, 0, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL, VideoProcAmp_Flags_Manual},
                    {"Saturation" , KSPROPERTY_VIDEOPROCAMP_SATURATION , VideoProcAmp_Saturation , -255, 255, 1, 0, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL, VideoProcAmp_Flags_Manual},
                    {"Gamma"      , KSPROPERTY_VIDEOPROCAMP_GAMMA      , VideoProcAmp_Gamma      , -255, 255, 1, 0, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL, VideoProcAmp_Flags_Manual},
                    {"Hue"        , KSPROPERTY_VIDEOPROCAMP_HUE        , VideoProcAmp_Hue        , -359, 359, 1, 0, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL, VideoProcAmp_Flags_Manual},
                    {"ColorEnable", KSPROPERTY_VIDEOPROCAMP_COLORENABLE, VideoProcAmp_ColorEnable,    0,   1, 1, 1, KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL, VideoProcAmp_Flags_Manual}
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

            static inline const ProcAmpPrivate *byPropertyDS(LONG property)
            {
                for (auto &control: controls())
                    if (control.propertyDS == property)
                        return &control;

                return nullptr;
            }

            static inline const ProcAmpPrivate *byName(const std::string &name)
            {
                for (auto &control: controls())
                    if (control.name == name)
                        return &control;

                return nullptr;
            }
    };
}

AkVCam::Controls::Controls():
    CUnknown(this, IID_VCamControl)
{
    AkLogFunction();
    this->d = new ControlsPrivate;
}

AkVCam::Controls::~Controls()
{
    AkLogFunction();
    delete this->d;
}

LONG AkVCam::Controls::value(const std::string &property) const
{
    AkLogFunction();

    auto control = ProcAmpPrivate::byName(property);

    if (!control)
        return 0;

    if (!this->d->m_control.contains(property))
        return control->defaultValue;

    return this->d->m_control[property];
}

HRESULT STDMETHODCALLTYPE AkVCam::Controls::QueryInterface(REFIID riid,
                                                           void **ppvObject)
{
    AkLogFunction();

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_VCamControl)
        || IsEqualIID(riid, IID_IAMVideoProcAmp)) {
        this->AddRef();
        *ppvObject = this;

        return S_OK;
    }

    return CUnknown::QueryInterface(riid, ppvObject);
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
        AkLogError("Invalid parameters");

        return STATUS_INVALID_PARAMETER;
    }

    *bytesReturned = 0;

    if (!IsEqualGUID(property->Set, IID_VIDEOPROCAMP)) {
        AkLogWarning("Unsupported property set");

        return STATUS_NOT_IMPLEMENTED;
    }

    auto control =
            ProcAmpPrivate::byProperty(KSPROPERTY_VIDCAP_VIDEOPROCAMP(property->Id));

    if (!control) {
        AkLogWarning("Unsupported property ID: %" PRIu64, property->Id);

        return STATUS_NOT_FOUND;
    }

    // Handle KSPROPERTY_TYPE_BASICSUPPORT

    if (property->Flags & KSPROPERTY_TYPE_BASICSUPPORT) {
        if (dataLength < sizeof(KSPROPERTY_DESCRIPTION)) {
            AkLogError("Insufficient data length for BASICSUPPORT: %" PRIu64, dataLength);

            return STATUS_BUFFER_TOO_SMALL;
        }

        auto description = static_cast<KSPROPERTY_DESCRIPTION *>(propertyData);
        description->AccessFlags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET;
        description->DescriptionSize = sizeof(KSPROPERTY_DESCRIPTION)
                                     + sizeof(KSPROPERTY_MEMBERSHEADER)
                                     + sizeof(KSPROPERTY_STEPPING_LONG);
        description->PropTypeSet.Set = IID_VIDEOPROCAMP;
        description->PropTypeSet.Id = property->Id;
        description->PropTypeSet.Flags = 0;
        description->MembersListCount = 1;

        if (dataLength >= description->DescriptionSize) {
            auto membersHeader = reinterpret_cast<KSPROPERTY_MEMBERSHEADER *>(description + 1);
            membersHeader->MembersFlags = KSPROPERTY_MEMBER_RANGES;
            membersHeader->MembersSize = sizeof(KSPROPERTY_STEPPING_LONG);
            membersHeader->MembersCount = 1;
            membersHeader->Flags = 0;

            auto stepping = reinterpret_cast<KSPROPERTY_STEPPING_LONG *>(membersHeader + 1);
            stepping->Bounds.SignedMinimum = control->min;
            stepping->Bounds.SignedMaximum = control->max;
            stepping->SteppingDelta = control->step;
            stepping->Reserved = 0;

            *bytesReturned = description->DescriptionSize;
            AkLogInfo("BASICSUPPORT for property %s: Min=%d, Max=%d, Step=%d",
                      control->name,
                      control->min,
                      control->max,
                      control->step);
        } else {
            *bytesReturned = description->DescriptionSize;

            // The buffer is too short

            return STATUS_BUFFER_OVERFLOW;
        }

        return STATUS_SUCCESS;
    }

    // Handle KSPROPERTY_TYPE_DEFAULTVALUES

    if (property->Flags & KSPROPERTY_TYPE_DEFAULTVALUES) {
        if (dataLength < sizeof(KSPROPERTY_VIDEOPROCAMP_S)) {
            AkLogError("Insufficient data length for DEFAULTVALUES: %" PRIu64, dataLength);

            return STATUS_BUFFER_TOO_SMALL;
        }

        auto procAmp = static_cast<KSPROPERTY_VIDEOPROCAMP_S *>(propertyData);
        procAmp->Value = control->defaultValue;
        procAmp->Flags = control->flags;
        procAmp->Capabilities = control->flags;
        *bytesReturned = sizeof(KSPROPERTY_VIDEOPROCAMP_S);
        AkLogInfo("DEFAULTVALUES for property %s: DefaultValue=%d",
                  control->name,
                  procAmp->Value);

        return STATUS_SUCCESS;
    }

    auto currentValue = this->d->m_control.contains(control->name)?
                            this->d->m_control[control->name]:
                            control->defaultValue;

    // Handle KSPROPERTY_TYPE_GET

    if (property->Flags & KSPROPERTY_TYPE_GET) {
        if (dataLength < sizeof(KSPROPERTY_VIDEOPROCAMP_S)) {
            AkLogError("Insufficient data length for GET: %" PRIu64, dataLength);

            return STATUS_BUFFER_TOO_SMALL;
        }

        auto procAmp = static_cast<KSPROPERTY_VIDEOPROCAMP_S *>(propertyData);
        procAmp->Value = currentValue;
        procAmp->Flags = control->flags;
        procAmp->Capabilities = control->flags;
        *bytesReturned = sizeof(KSPROPERTY_VIDEOPROCAMP_S);
        AkLogInfo("Get property %s: Value=%d, Flags=0x%x",
                  control->name,
                  procAmp->Value,
                  procAmp->Flags);

        return STATUS_SUCCESS;
    }

    // Handle KSPROPERTY_TYPE_SET

    if (property->Flags & KSPROPERTY_TYPE_SET) {
        if (dataLength < sizeof(KSPROPERTY_VIDEOPROCAMP_S)) {
            AkLogError("Insufficient data length for SET: %" PRIu64, dataLength);

            return STATUS_BUFFER_TOO_SMALL;
        }

        auto procAmp = static_cast<KSPROPERTY_VIDEOPROCAMP_S *>(propertyData);
        LONG newValue = procAmp->Value;
        LONG newFlags = procAmp->Flags;

        if (newFlags & ~control->flags) {
            AkLogError("Unsupported flags for property %s: 0x%x",
                       control->name,
                       newFlags);

            return STATUS_INVALID_PARAMETER;
        }

        if (newValue < control->min || newValue > control->max) {
            AkLogError("Value out of range for property %s: %d (min=%d, max=%d)",
                       control->name,
                       newValue,
                       control->min,
                       control->max);

            return STATUS_INVALID_PARAMETER;
        }

        if ((newValue - control->min) % control->step != 0) {
            AkLogError("Invalid step for property %s: %d (step=%d)",
                       control->name,
                       newValue,
                       control->step);

            return STATUS_INVALID_PARAMETER;
        }

        if (currentValue != newValue) {
            this->d->m_control[control->name] = newValue;
            AkLogInfo("Set property %s: Value=%d", control->name, newValue);
            AKVCAM_EMIT(this, PropertyChanged,
                        control->property,
                        newValue,
                        property->Flags);
        }

        *bytesReturned = sizeof(KSPROPERTY_VIDEOPROCAMP_S);

        return STATUS_SUCCESS;
    }

    AkLogWarning("Unsupported property flags: 0x%x", property->Flags);

    return STATUS_NOT_IMPLEMENTED;
}

HRESULT AkVCam::Controls::GetRange(LONG property,
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

    auto control = ProcAmpPrivate::byPropertyDS(property);

    if (!control)
        return E_INVALIDARG;

    *pMin = control->min;
    *pMax = control->max;
    *pSteppingDelta = control->step;
    *pDefault = control->defaultValue;
    *pCapsFlags = control->flags;

    return S_OK;
}

HRESULT AkVCam::Controls::Set(LONG property, LONG lValue, LONG flags)
{
    AkLogFunction();

    auto control = ProcAmpPrivate::byPropertyDS(property);

    if (!control)
        return E_INVALIDARG;

    if (lValue < control->min
        || lValue > control->max
        || flags != control->flags) {
        return E_INVALIDARG;
    }

    this->d->m_control[control->name] = lValue;
    AKVCAM_EMIT(this, PropertyChanged, control->property, lValue, flags)

    return S_OK;
}

HRESULT AkVCam::Controls::Get(LONG property, LONG *lValue, LONG *flags)
{
    AkLogFunction();

    if (!lValue || !flags)
        return E_POINTER;

    *lValue = 0;
    *flags = 0;

    auto control = ProcAmpPrivate::byPropertyDS(property);

    if (!control)
        return E_INVALIDARG;

    if (this->d->m_control.count(control->name))
        *lValue = this->d->m_control[control->name];
    else
        *lValue = control->defaultValue;

    *flags = control->flags;

    return S_OK;
}
