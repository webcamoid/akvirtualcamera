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

#include "classfactory.h"
#include "BaseFilter/src/basefilter.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class ClassFactoryPrivate
    {
        public:
            CLSID m_clsid;
            static std::atomic<uint64_t> m_locked;
    };

    std::atomic<uint64_t> ClassFactoryPrivate::m_locked = 0;
}

AkVCam::ClassFactory::ClassFactory(const CLSID &clsid):
    CUnknown()
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

HRESULT AkVCam::ClassFactory::CreateInstance(IUnknown *pUnkOuter,
                                             const IID &riid,
                                             void **ppvObject)
{
    AkLogFunction();
    AkLogDebug("Outer: %p", pUnkOuter);
    AkLogDebug("IID: %s", stringFromClsid(riid).c_str());

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    auto filter = new BaseFilter(this->d->m_clsid);
    auto hr = filter->QueryInterface(riid, ppvObject);
    filter->Release();

    return hr;
}

HRESULT AkVCam::ClassFactory::LockServer(BOOL fLock)
{
    AkLogFunction();

    if (fLock)
        this->d->m_locked++;
    else
        this->d->m_locked--;

    return S_OK;
}
