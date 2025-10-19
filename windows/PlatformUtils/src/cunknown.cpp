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

#include <atomic>

#include "cunknown.h"
#include "utils.h"
#include "VCamUtils/src/utils.h"

#define AkCUnknownLogMethod() \
    AkLogFunction(); \
    AkLogDebug("Object: %s (0x%x)", \
               this->d->m_parent? \
                    stringFromClsid(this->d->m_parentCLSID).c_str(): \
                    "CUnknown", \
               this->d->m_parent? this->d->m_parent: this)

#define AkCUnknownLogThis() \
    AkLogDebug("Returning %s (0x%x)", \
               this->d->m_parent? \
                    stringFromClsid(this->d->m_parentCLSID).c_str(): \
                    "CUnknown", \
               this->d->m_parent? this->d->m_parent: this)

namespace AkVCam
{
    class CUnknownPrivate
    {
        public:
            std::atomic<ULONG> m_ref {0};
            CUnknown *m_parent {nullptr};
            CLSID m_parentCLSID;
    };
}

AkVCam::CUnknown::CUnknown(CUnknown *parent, REFIID parentCLSID)
{
    this->d = new CUnknownPrivate;
    this->d->m_parent = parent;
    this->d->m_parentCLSID = parentCLSID;
}

AkVCam::CUnknown::~CUnknown()
{

}

void AkVCam::CUnknown::setParent(AkVCam::CUnknown *parent,
                                 const IID *parentCLSID)
{
    this->d->m_parent = parent;
    this->d->m_parentCLSID = parentCLSID? *parentCLSID: GUID_NULL;
}

ULONG AkVCam::CUnknown::ref() const
{
    return this->d->m_ref;
}

HRESULT AkVCam::CUnknown::QueryInterface(const IID &riid, void **ppvObject)
{
    AkCUnknownLogMethod();
    AkLogDebug("IID: %s", stringFromClsid(riid).c_str());

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, this->d->m_parentCLSID)) {
        AkCUnknownLogThis();
        this->d->m_parent->AddRef();
        *ppvObject = this->d->m_parent;

        return S_OK;
    } else {
        AkLogWarning("Unknown interface");
    }

    return E_NOINTERFACE;
}

ULONG AkVCam::CUnknown::AddRef()
{
    AkCUnknownLogMethod();
    this->d->m_ref++;
    AkLogDebug("REF: %ull", static_cast<ULONG>(this->d->m_ref));

    return this->d->m_ref;
}

ULONG AkVCam::CUnknown::Release()
{
    AkCUnknownLogMethod();
    AkLogDebug("REF: %ull", static_cast<ULONG>(this->d->m_ref));

    if (this->d->m_ref)
        this->d->m_ref--;

    return this->d->m_ref;
}
