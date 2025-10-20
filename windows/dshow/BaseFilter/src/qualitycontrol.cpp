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

#include "qualitycontrol.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

AkVCam::QualityControl::QualityControl():
    CUnknown(this, IID_IQualityControl)
{

}

AkVCam::QualityControl::~QualityControl()
{

}

HRESULT AkVCam::QualityControl::Notify(IBaseFilter *pSelf, Quality q)
{
    UNUSED(q);
    AkLogFunction();

    if (!pSelf)
        return E_POINTER;

    AkLogInfo("Type: %s", q.Type == Famine? "Famine": "Flood");
    AkLogInfo("Proportion: %ll", q.Proportion);
    AkLogInfo("Late: %" PRIu64, q.Late);
    AkLogInfo("TimeStamp: %" PRIu64, q.TimeStamp);

    return S_OK;
}

HRESULT AkVCam::QualityControl::SetSink(IQualityControl *piqc)
{
    UNUSED(piqc);
    AkLogFunction();

    return E_NOTIMPL;
}
