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
#include "PlatformUtils/src/utils.h"
#include "PlatformUtils/src/preferences.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class ActivatePrivate
    {
        public:
            CLSID m_clsid;
            IMFMediaSource *m_mediaSource {nullptr};
    };
}

AkVCam::Activate::Activate(const CLSID &clsid):
    CUnknown(this, IID_IMFActivate),
    Attributes()
{
    this->d = new ActivatePrivate;
    this->d->m_clsid = clsid;

    HRESULT hr = E_FAIL;
    auto cameraIndex = Preferences::cameraFromCLSID(clsid);

    if (cameraIndex >= 0) {
        auto description = Preferences::cameraDescription(cameraIndex);

        if (!description.empty()) {
            auto descriptionWStr = wstrFromString(description);

            if (descriptionWStr) {
                hr = this->SetString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
                                     descriptionWStr);
                CoTaskMemFree(descriptionWStr);
            }
        }
    }

    if (SUCCEEDED(hr))
        hr = this->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                           MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

    AkLogDebug() << "Created Activate for CLSID: " << stringFromClsid(clsid) << std::endl;
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

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualGUID(riid, IID_IUnknown)
        || IsEqualGUID(riid, IID_IMFActivate)
        || IsEqualGUID(riid, IID_IMFAttributes)) {
        *ppvObject = static_cast<IMFActivate *>(this);
        this->AddRef();

        return S_OK;
    }

    return CUnknown::QueryInterface(riid, ppvObject);
}

HRESULT AkVCam::Activate::ActivateObject(REFIID riid, void **ppv)
{
    AkLogFunction();
    AkLogInfo() << "Activating for IID: " << stringFromClsid(riid) << std::endl;

    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (!IsEqualGUID(riid, IID_IMFMediaSource))
        return E_NOINTERFACE;

    if (this->d->m_mediaSource)
        return this->d->m_mediaSource->QueryInterface(riid, ppv);

    auto mediaSource = new MediaSource(this->d->m_clsid);
    auto hr = mediaSource->QueryInterface(riid, ppv);

    if (SUCCEEDED(hr)) {
        this->d->m_mediaSource = mediaSource;
        this->d->m_mediaSource->AddRef();
    } else {
        mediaSource->Release();
    }

    return hr;
}

HRESULT AkVCam::Activate::DetachObject()
{
    AkLogFunction();

    if (!this->d->m_mediaSource)
        return S_OK;

    this->d->m_mediaSource->Release();
    this->d->m_mediaSource = nullptr;

    return S_OK;
}

HRESULT AkVCam::Activate::ShutdownObject()
{
    AkLogFunction();

    if (!this->d->m_mediaSource)
        return S_OK;

    this->d->m_mediaSource->Shutdown();
    this->d->m_mediaSource->Release();
    this->d->m_mediaSource = nullptr;

    return S_OK;
}
