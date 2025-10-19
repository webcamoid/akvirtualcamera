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

#include "activate.h"
#include "mediasource.h"
#include "mfvcam.h"
#include "MFUtils/src/utils.h"
#include "PlatformUtils/src/utils.h"
#include "PlatformUtils/src/preferences.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class ActivatePrivate
    {
        public:
            CLSID m_clsid;
            MediaSource *m_mediaSource {nullptr};
    };
}

AkVCam::Activate::Activate(const CLSID &clsid):
    CUnknown(this, IID_IMFActivate),
    Attributes()
{
    this->d = new ActivatePrivate;
    this->d->m_clsid = clsid;
    this->SetUINT32(MF_VIRTUALCAMERA_PROVIDE_ASSOCIATED_CAMERA_SOURCES, 1);
    this->SetGUID(MFT_TRANSFORM_CLSID_Attribute, clsid);

    AkLogDebug("Created Activate for CLSID: %s", stringFromClsidMF(clsid).c_str());
}

AkVCam::Activate::~Activate()
{
    if (this->d->m_mediaSource)
        this->d->m_mediaSource->Release();

    delete this->d;
}

HRESULT AkVCam::Activate::QueryInterface(REFIID riid, void **ppvObject)
{
    AkLogFunction();
    AkLogDebug("IID: %s", stringFromClsidMF(riid).c_str());

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualGUID(riid, IID_IUnknown)
        || IsEqualGUID(riid, IID_IMFActivate)
        || IsEqualGUID(riid, IID_IMFAttributes)) {
        *ppvObject = this;
        this->AddRef();

        return S_OK;
    }

    return CUnknown::QueryInterface(riid, ppvObject);
}

HRESULT AkVCam::Activate::ActivateObject(REFIID riid, void **ppv)
{
    AkLogFunction();
    AkLogInfo("Activating for IID: %s", stringFromClsidMF(riid).c_str());

    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (!this->d->m_mediaSource) {
        this->d->m_mediaSource = new MediaSource(this->d->m_clsid);
        this->CopyAllItems(this->d->m_mediaSource);
    }

    return this->d->m_mediaSource->QueryInterface(riid, ppv);
}

HRESULT AkVCam::Activate::DetachObject()
{
    AkLogFunction();

    if (this->d->m_mediaSource) {
        this->d->m_mediaSource->Release();
        this->d->m_mediaSource = nullptr;
    }

    return S_OK;
}

HRESULT AkVCam::Activate::ShutdownObject()
{
    AkLogFunction();

    if (this->d->m_mediaSource) {
        this->d->m_mediaSource->Shutdown();
        this->d->m_mediaSource->Release();
        this->d->m_mediaSource = nullptr;
    }

    return S_OK;
}
