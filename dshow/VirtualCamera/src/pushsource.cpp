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

#include "pushsource.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

AkVCam::PushSource::PushSource(IAMStreamConfig *streamConfig):
    Latency(streamConfig)
{
    this->setParent(this, &IID_IAMPushSource);
}

AkVCam::PushSource::~PushSource()
{
}

HRESULT AkVCam::PushSource::GetPushSourceFlags(ULONG *pFlags)
{
    AkLogFunction();

    if (!pFlags)
        return E_POINTER;

    *pFlags = 0;

    return S_OK;
}

HRESULT AkVCam::PushSource::SetPushSourceFlags(ULONG Flags)
{
    UNUSED(Flags)
    AkLogFunction();

    return E_NOTIMPL;
}

HRESULT AkVCam::PushSource::SetStreamOffset(REFERENCE_TIME rtOffset)
{
    UNUSED(rtOffset)
    AkLogFunction();

    return E_NOTIMPL;
}

HRESULT AkVCam::PushSource::GetStreamOffset(REFERENCE_TIME *prtOffset)
{
    UNUSED(prtOffset)
    AkLogFunction();

    return E_NOTIMPL;
}

HRESULT AkVCam::PushSource::GetMaxStreamOffset(REFERENCE_TIME *prtMaxOffset)
{
    UNUSED(prtMaxOffset)
    AkLogFunction();

    return E_NOTIMPL;
}

HRESULT AkVCam::PushSource::SetMaxStreamOffset(REFERENCE_TIME rtMaxOffset)
{
    UNUSED(rtMaxOffset)
    AkLogFunction();

    return E_NOTIMPL;
}
