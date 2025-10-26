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
#include "basefilter.h"
#include "pin.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class EnumPinsPrivate
    {
        public:
            BaseFilter *m_baseFilter {nullptr};
            std::vector<Pin *> m_pins;
            size_t m_position {0};
    };
}

AkVCam::EnumPins::EnumPins(BaseFilter *baseFilter):
    CUnknown()
{
    this->d = new EnumPinsPrivate;
    this->d->m_baseFilter = baseFilter;
}

AkVCam::EnumPins::EnumPins(const EnumPins &other):
    CUnknown()
{
    this->d = new EnumPinsPrivate;
    this->d->m_position = other.d->m_position;

    for (auto &pin: other.d->m_pins) {
        this->d->m_pins.push_back(pin);
        pin->AddRef();
    }
}

AkVCam::EnumPins::~EnumPins()
{
    for (auto &pin: this->d->m_pins)
        pin->Release();

    delete this->d;
}

void AkVCam::EnumPins::addPin(const std::vector<VideoFormat> &formats,
                              const std::string &pinName)
{
    this->d->m_pins.push_back(new Pin(this->d->m_baseFilter, formats, pinName));
}

size_t AkVCam::EnumPins::size() const
{
    return this->d->m_pins.size();
}

bool AkVCam::EnumPins::pin(size_t index, IPin **pPin) const
{
    if (index >= this->d->m_pins.size())
        return false;

    *pPin = this->d->m_pins[index];
    (*pPin)->AddRef();

    return true;
}

bool AkVCam::EnumPins::contains(IPin *pin) const
{
    for (auto p: this->d->m_pins)
        if (p == pin)
            return true;

    return false;
}

HRESULT AkVCam::EnumPins::stop()
{
    HRESULT result = S_OK;

    for (auto &pin: this->d->m_pins) {
        result = pin->stop();

        if (FAILED(result))
            return result;
    }

    return result;
}

HRESULT AkVCam::EnumPins::pause()
{
    HRESULT result = S_OK;

    for (auto &pin: this->d->m_pins) {
        result = pin->pause();

        if (FAILED(result))
            return result;
    }

    return result;
}

HRESULT AkVCam::EnumPins::run(REFERENCE_TIME tStart)
{
    HRESULT result = S_OK;

    for (auto &pin: this->d->m_pins) {
        result = pin->run(tStart);

        if (FAILED(result))
            return result;
    }

    return result;
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

    *ppEnum = new EnumPins(*this);

    return S_OK;
}
