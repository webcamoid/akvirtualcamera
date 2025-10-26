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

#ifndef ENUMMEDIATYPES_H
#define ENUMMEDIATYPES_H

#include <vector>
#include <strmif.h>

#include "PlatformUtils/src/cunknown.h"
#include "VCamUtils/src/videoformat.h"

namespace AkVCam
{
    class EnumMediaTypesPrivate;

    class EnumMediaTypes: public virtual CUnknown<IEnumMediaTypes>
    {
        public:
            EnumMediaTypes(const std::vector<VideoFormat> &formats={});
            EnumMediaTypes(const EnumMediaTypes &other);
            EnumMediaTypes &operator =(const EnumMediaTypes &other);
            virtual ~EnumMediaTypes();

            size_t size() const;
            bool mediaType(size_t index, AM_MEDIA_TYPE **ppMediaType) const;
            bool contains(const AM_MEDIA_TYPE *pMediaType) const;

            BEGIN_COM_MAP(EnumMediaTypes)
                COM_INTERFACE_ENTRY(IEnumMediaTypes)
            END_COM_MAP(EnumMediaTypes)

            // IEnumMediaTypes
            HRESULT STDMETHODCALLTYPE Next(ULONG cMediaTypes,
                                           AM_MEDIA_TYPE **ppMediaTypes,
                                           ULONG *pcFetched) override;
            HRESULT STDMETHODCALLTYPE Skip(ULONG cMediaTypes) override;
            HRESULT STDMETHODCALLTYPE Reset() override;
            HRESULT STDMETHODCALLTYPE Clone(IEnumMediaTypes **ppEnum) override;

        private:
            EnumMediaTypesPrivate *d;
    };
}

#endif // ENUMMEDIATYPES_H
