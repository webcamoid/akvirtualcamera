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

#include <memory>

#include "classfactory.h"
#include "activate.h"
#include "mediasource.h"
#include "MFUtils/src/utils.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class ClassFactoryPrivate
    {
        public:
            CLSID m_clsid;
            static int m_locked;
    };

    int ClassFactoryPrivate::m_locked = 0;
}

AkVCam::ClassFactory::ClassFactory(const CLSID &clsid):
    CUnknown(this, IID_IClassFactory)
{
    this->d = new ClassFactoryPrivate;
    this->d->m_clsid = clsid;
}

AkVCam::ClassFactory::~ClassFactory()
{
    delete this->d;
}

bool AkVCam::ClassFactory::locked()
{
    return ClassFactoryPrivate::m_locked > 0;
}

HRESULT AkVCam::ClassFactory::QueryInterface(const IID &riid, void **ppvObject)
{
    AkLogFunction();
    AkLogInfo() << "IID: " << stringFromClsidMF(riid) << std::endl;

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IClassFactory)) {
        AkLogInterface(IClassFactory, this);
        this->AddRef();
        *ppvObject = this;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IMFMediaSource)) {
        auto mediaSource = new MediaSource(this->d->m_clsid);
        AkLogInterface(IMFMediaSource, mediaSource);

        if (!mediaSource)
            return E_FAIL;

        mediaSource->AddRef();
        *ppvObject = mediaSource;

        return S_OK;
    }

    return CUnknown::QueryInterface(riid, ppvObject);
}

HRESULT AkVCam::ClassFactory::CreateInstance(IUnknown *pUnkOuter,
                                             const IID &riid,
                                             void **ppvObject)
{
    AkLogFunction();
    AkLogInfo() << "Outer: " << ULONG_PTR(pUnkOuter) << std::endl;
    AkLogInfo() << "IID: " << stringFromClsidMF(riid) << std::endl;

    if (!ppvObject)
        return E_INVALIDARG;

    *ppvObject = nullptr;

    if (pUnkOuter && !IsEqualIID(riid, IID_IUnknown))
        return E_NOINTERFACE;

    auto activate = std::shared_ptr<Activate>(new Activate(this->d->m_clsid),
                                              [] (Activate *activate) {
        activate->Release();
    });

    return activate->QueryInterface(riid, ppvObject);
}

HRESULT AkVCam::ClassFactory::LockServer(BOOL fLock)
{
    AkLogFunction();
    this->d->m_locked += fLock? 1: -1;

    return S_OK;
}
