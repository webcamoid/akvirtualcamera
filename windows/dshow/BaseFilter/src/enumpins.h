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

#ifndef ENUMPINS_H
#define ENUMPINS_H

#include <strmif.h>

#include "PlatformUtils/src/cunknown.h"

namespace AkVCam
{
    class EnumPinsPrivate;
    class BaseFilter;
    class Pin;

    class EnumPins: public CUnknown<IEnumPins>

    {
        public:
            EnumPins(BaseFilter *baseFilter=nullptr);
            EnumPins(const EnumPins &other);
            virtual ~EnumPins();

            void addPin(const std::vector<VideoFormat> &formats,
                        const std::string &pinName);
            size_t size() const;
            bool pin(size_t index, IPin **pPin) const;
            bool contains(IPin *pin) const;
            HRESULT stop();
            HRESULT pause();
            HRESULT run(REFERENCE_TIME tStart);

            BEGIN_COM_MAP(EnumPins)
                COM_INTERFACE_ENTRY(IEnumPins)
            END_COM_MAP(EnumPins)

            // IEnumPins
            HRESULT STDMETHODCALLTYPE Next(ULONG cPins,
                                           IPin **ppPins,
                                           ULONG *pcFetched) override;
            HRESULT STDMETHODCALLTYPE Skip(ULONG cPins) override;
            HRESULT STDMETHODCALLTYPE Reset() override;
            HRESULT STDMETHODCALLTYPE Clone(IEnumPins **ppEnum) override;

        private:
            EnumPinsPrivate *d;
    };
}

#endif // ENUMPINS_H
