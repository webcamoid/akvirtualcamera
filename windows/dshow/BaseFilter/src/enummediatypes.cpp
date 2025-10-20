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
#include <dshow.h>

#include "enummediatypes.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class EnumMediaTypesPrivate
    {
        public:
            std::vector<VideoFormat> m_formats;
            size_t m_position {0};
            bool m_changed {false};
    };
}

AkVCam::EnumMediaTypes::EnumMediaTypes(const std::vector<VideoFormat> &formats):
    CUnknown(this, IID_IEnumMediaTypes)
{
    this->d = new EnumMediaTypesPrivate;
    this->d->m_formats = formats;
}

AkVCam::EnumMediaTypes::EnumMediaTypes(const AkVCam::EnumMediaTypes &other):
    CUnknown(this, IID_IEnumMediaTypes)
{
    this->d = new EnumMediaTypesPrivate;
    this->d->m_formats = other.d->m_formats;
    this->d->m_position = other.d->m_position;
    this->d->m_changed = other.d->m_changed;
}

AkVCam::EnumMediaTypes &AkVCam::EnumMediaTypes::operator =(const EnumMediaTypes &other)
{
    if (this != &other) {
        this->d->m_formats = other.d->m_formats;
        this->d->m_position = other.d->m_position;
        this->d->m_changed = other.d->m_changed;
    }

    return *this;
}

AkVCam::EnumMediaTypes::~EnumMediaTypes()
{
    delete this->d;
}

std::vector<AkVCam::VideoFormat> &AkVCam::EnumMediaTypes::formats()
{
    return this->d->m_formats;
}

std::vector<AkVCam::VideoFormat> AkVCam::EnumMediaTypes::formats() const
{
    return this->d->m_formats;
}

void AkVCam::EnumMediaTypes::setFormats(const std::vector<AkVCam::VideoFormat> &formats,
                                        bool changed)
{
    if (this->d->m_formats == formats)
        return;

    this->d->m_formats = formats;
    this->d->m_changed = changed;
}

HRESULT AkVCam::EnumMediaTypes::Next(ULONG cMediaTypes,
                                     AM_MEDIA_TYPE **ppMediaTypes,
                                     ULONG *pcFetched)
{
    AkLogFunction();

    if (pcFetched)
        *pcFetched = 0;

    if (!cMediaTypes)
        return E_INVALIDARG;

    if (!ppMediaTypes)
        return E_POINTER;

    memset(ppMediaTypes, 0, cMediaTypes * sizeof(AM_MEDIA_TYPE *));

    if (this->d->m_changed) {
        this->d->m_changed = false;

        return VFW_E_ENUM_OUT_OF_SYNC;
    }

    ULONG fetched = 0;

    for (;
         fetched < cMediaTypes
         && this->d->m_position < this->d->m_formats.size();
         fetched++, this->d->m_position++) {
        *ppMediaTypes = mediaTypeFromFormat(this->d->m_formats[this->d->m_position]);

        if (*ppMediaTypes)
            ppMediaTypes++;
    }

    if (pcFetched)
        *pcFetched = fetched;

    return fetched == cMediaTypes? S_OK: S_FALSE;
}

HRESULT AkVCam::EnumMediaTypes::Skip(ULONG cMediaTypes)
{
    AkLogFunction();
    AkLogDebug("Skip %" PRIu64 " media types", cMediaTypes);

    if (this->d->m_changed) {
        this->d->m_changed = false;

        return VFW_E_ENUM_OUT_OF_SYNC;
    }

    auto position = this->d->m_position + cMediaTypes;

    if (position > this->d->m_formats.size())
        return S_FALSE;

    this->d->m_position = position;

    return S_OK;
}

HRESULT AkVCam::EnumMediaTypes::Reset()
{
    AkLogFunction();
    this->d->m_position = 0;

    return S_OK;
}

HRESULT AkVCam::EnumMediaTypes::Clone(IEnumMediaTypes **ppEnum)
{
    AkLogFunction();

    if (!ppEnum)
        return E_POINTER;

    if (this->d->m_changed) {
        this->d->m_changed = false;

        return VFW_E_ENUM_OUT_OF_SYNC;
    }

    *ppEnum = new EnumMediaTypes(*this);
    (*ppEnum)->AddRef();

    return S_OK;
}
