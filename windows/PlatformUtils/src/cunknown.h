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

#ifndef CUNKNOWN_H
#define CUNKNOWN_H

#include <atomic>
#include <cstdint>
#include <unknwn.h>

#include "utils.h"
#include "VCamUtils/src/logger.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    struct ComInterfaceEntry
    {
        const IID *iid;
        void *ptr;
    };

    template<typename... Interfaces>
    class CUnknown: public virtual Interfaces...
    {
        public:
            CUnknown() = default;
            virtual ~CUnknown() = default;

            inline uint64_t ref() const
            {
                return static_cast<uint64_t>(this->m_refCount);
            }

            // IUnknown

            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override
            {
                AkLogFunction();
                AkLogDebug("IID: %s", stringFromClsid(riid).c_str());

                if (!ppv)
                    return E_POINTER;

                size_t n = 0;
                auto map = comEntriesMap(n);

                for (size_t i = 0; i < n; ++i) {
                    if (isInterfaceDisabled(riid))
                        continue;

                    if (*map[i].iid == riid) {
                        *ppv = map[i].ptr;
                        AddRef();

                        return S_OK;
                    }
                }

                *ppv = nullptr;
                AkLogDebug("Interface not found");

                return E_NOINTERFACE;
            }

            ULONG STDMETHODCALLTYPE AddRef() override
            {
                AkLogFunction();

                return ++this->m_refCount;
            }

            ULONG STDMETHODCALLTYPE Release() override
            {
                AkLogFunction();
                ULONG ref = --this->m_refCount;

                if (ref == 0)
                    delete this;

                return ref;
            }

        private:
            std::atomic<uint64_t> m_refCount {1};

        protected:
            virtual const ComInterfaceEntry *comEntriesMap(size_t &count) = 0;
            virtual bool isInterfaceDisabled(REFIID riid) const
            {
                UNUSED(riid);

                return false;
            }
   };
}

#define DECLARE_IUNKNOWN_Q \
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv) override \
    { \
        AkLogFunction(); \
        AkLogDebug("IID: %s", stringFromClsid(riid).c_str()); \
        \
        if (!ppv) \
            return E_POINTER; \
        \
        size_t n = 0; \
        auto map = comEntriesMap(n); \
        \
        for (size_t i = 0; i < n; ++i) { \
            if (isInterfaceDisabled(riid)) \
                continue; \
            \
            if (*map[i].iid == riid) { \
                *ppv = map[i].ptr; \
                AddRef(); \
                \
                return S_OK; \
            } \
        } \
        \
        *ppv = nullptr; \
        AkLogDebug("Interface not found"); \
        \
        return E_NOINTERFACE; \
    }

#define DECLARE_IUNKNOWN_R \
    public: \
        ULONG STDMETHODCALLTYPE Release() override \
        { \
            AkLogFunction(); \
            ULONG ref = --this->m_refCount; \
            \
            if (ref == 0) \
                delete this; \
            \
            return ref; \
        }

#define DECLARE_IUNKNOWN_NQR \
    private: \
        std::atomic<uint64_t> m_refCount {1}; \
    \
    public: \
        inline uint64_t ref() const \
        { \
            return static_cast<uint64_t>(this->m_refCount); \
        } \
        \
        ULONG STDMETHODCALLTYPE AddRef() override \
        { \
            AkLogFunction(); \
            \
            return ++this->m_refCount; \
        }

#define DECLARE_IUNKNOWN_NQ \
    DECLARE_IUNKNOWN_R \
    DECLARE_IUNKNOWN_NQR

#define BEGIN_COM_MAP_NR(object) \
    DECLARE_IUNKNOWN_Q \
    DECLARE_IUNKNOWN_NQR \
    \
    inline bool isInterfaceDisabled(REFIID riid) const \
    { \
        UNUSED(riid); \
        \
        return false; \
    } \
    \
    const AkVCam::ComInterfaceEntry *comEntriesMap(size_t &count) \
    { \
        static const AkVCam::ComInterfaceEntry comInterfaceEntry##object[] = {

#define BEGIN_COM_MAP_NO(object) \
    DECLARE_IUNKNOWN_Q \
    DECLARE_IUNKNOWN_NQ \
    \
    inline bool isInterfaceDisabled(REFIID riid) const \
    { \
        UNUSED(riid); \
        \
        return false; \
    } \
    \
    const AkVCam::ComInterfaceEntry *comEntriesMap(size_t &count) \
    { \
        static const AkVCam::ComInterfaceEntry comInterfaceEntry##object[] = {

#define BEGIN_COM_MAP_NOD(object) \
    DECLARE_IUNKNOWN_Q \
    DECLARE_IUNKNOWN_NQ \
    \
    const AkVCam::ComInterfaceEntry *comEntriesMap(size_t &count) \
    { \
        static const AkVCam::ComInterfaceEntry comInterfaceEntry##object[] = {

#define BEGIN_COM_MAP_NQO(object) \
    DECLARE_IUNKNOWN_NQ \
    \
    const AkVCam::ComInterfaceEntry *comEntriesMap(size_t &count) \
    { \
        static const AkVCam::ComInterfaceEntry comInterfaceEntry##object[] = {

#define BEGIN_COM_MAP(object) \
    const AkVCam::ComInterfaceEntry *comEntriesMap(size_t &count) override \
    { \
        static const AkVCam::ComInterfaceEntry comInterfaceEntry##object[] = {

#define COM_INTERFACE_ENTRY(interface) \
            {&IID_##interface, static_cast<interface *>(this)},

#define COM_INTERFACE_ENTRY2(interface, base) \
            {&IID_##interface, static_cast<interface *>(static_cast<base *>(this))},

#define END_COM_MAP_NU(object) \
        }; \
        count = sizeof(comInterfaceEntry##object) / sizeof(comInterfaceEntry##object[0]); \
        \
        return comInterfaceEntry##object; \
    }

#define END_COM_MAP(object) \
            {&IID_IUnknown, static_cast<IUnknown *>(this)}, \
    END_COM_MAP_NU(object)

#endif // CUNKNOWN_H
