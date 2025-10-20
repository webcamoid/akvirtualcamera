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

#include <cinttypes>
#include <vector>
#include <dshow.h>

#include "enumpins.h"
#include "pin.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class EnumPinsPrivate
    {
        public:
            std::vector<IPin *> m_pins;
            size_t m_position {0};
            bool m_changed {false};
    };
}

AkVCam::EnumPins::EnumPins():
    CUnknown(this, IID_IEnumPins)
{
    this->d = new EnumPinsPrivate;
}

AkVCam::EnumPins::EnumPins(const EnumPins &other):
    CUnknown(this, IID_IEnumPins)
{
    this->d = new EnumPinsPrivate;
    this->d->m_position = other.d->m_position;
    this->d->m_changed = other.d->m_changed;

    for (auto &pin: other.d->m_pins) {
        this->d->m_pins.push_back(pin);
        this->d->m_pins.back()->AddRef();
    }
}

AkVCam::EnumPins::~EnumPins()
{
    for (auto &pin: this->d->m_pins)
        pin->Release();

    delete this->d;
}

size_t AkVCam::EnumPins::count() const
{
    return this->d->m_pins.size();
}

void AkVCam::EnumPins::addPin(IPin *pin, bool changed)
{
    this->d->m_pins.push_back(pin);
    this->d->m_pins.back()->AddRef();
    this->d->m_changed = changed;
}

void AkVCam::EnumPins::removePin(IPin *pin, bool changed)
{
    for (auto it = this->d->m_pins.begin(); it != this->d->m_pins.end(); it++)
        if (*it == pin) {
            this->d->m_pins.erase(it);
            pin->Release();
            this->d->m_changed = changed;

            break;
        }
}

void AkVCam::EnumPins::setBaseFilter(BaseFilter *baseFilter)
{
    for (auto &pin: this->d->m_pins) {
        auto akPin = static_cast<Pin *>(pin);
        akPin->setBaseFilter(baseFilter);
    }
}

HRESULT AkVCam::EnumPins::Next(ULONG cPins, IPin **ppPins, ULONG *pcFetched)
{
    AkLogFunction();

    if (pcFetched)
        *pcFetched = 0;

    if (cPins < 1)
        return E_INVALIDARG;

    if (!ppPins)
        return E_POINTER;

    memset(ppPins, 0, cPins * sizeof(IPin *));

    if (this->d->m_changed) {
        this->d->m_changed = false;

        return VFW_E_ENUM_OUT_OF_SYNC;
    }

    if (this->d->m_pins.empty())
        return S_FALSE;

    ULONG fetched = 0;

    for (;
         fetched < cPins
         && this->d->m_position < this->d->m_pins.size();
         fetched++, this->d->m_position++) {
        auto pin = this->d->m_pins[this->d->m_position];

        if (pin) {
            *ppPins = pin;
            pin->AddRef();
            ppPins++;
        }
    }

    if (pcFetched)
        *pcFetched = fetched;

    return fetched == cPins? S_OK: S_FALSE;
}

HRESULT AkVCam::EnumPins::Skip(ULONG cPins)
{
    AkLogFunction();
    AkLogDebug("Skip %" PRIu64 " pins", cPins);

    if (this->d->m_changed) {
        this->d->m_changed = false;

        return VFW_E_ENUM_OUT_OF_SYNC;
    }

    auto position = this->d->m_position + cPins;

    if (position > this->d->m_pins.size())
        return S_FALSE;

    this->d->m_position = position;

    return S_OK;
}

HRESULT AkVCam::EnumPins::Reset()
{
    AkLogFunction();
    this->d->m_position = 0;

    return S_OK;
}

HRESULT AkVCam::EnumPins::Clone(IEnumPins **ppEnum)
{
    AkLogFunction();

    if (!ppEnum)
        return E_POINTER;

    if (this->d->m_changed) {
        this->d->m_changed = false;

        return VFW_E_ENUM_OUT_OF_SYNC;
    }

    *ppEnum = new EnumPins(*this);
    (*ppEnum)->AddRef();

    return S_OK;
}
