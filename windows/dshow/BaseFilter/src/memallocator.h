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

#ifndef MEMALLOCATOR_H
#define MEMALLOCATOR_H

#include <strmif.h>

#include "PlatformUtils/src/cunknown.h"

namespace AkVCam
{
    class MemAllocatorPrivate;

    class MemAllocator: public CUnknown<IMemAllocator>
    {
        public:
            MemAllocator();
            virtual ~MemAllocator();

            BEGIN_COM_MAP(MemAllocator)
                COM_INTERFACE_ENTRY(IMemAllocator)
            END_COM_MAP(MemAllocator)

            // IMemAllocator
            HRESULT STDMETHODCALLTYPE SetProperties(ALLOCATOR_PROPERTIES *pRequest,
                                                    ALLOCATOR_PROPERTIES *pActual) override;
            HRESULT STDMETHODCALLTYPE GetProperties(ALLOCATOR_PROPERTIES *pProps) override;
            HRESULT STDMETHODCALLTYPE Commit() override;
            HRESULT STDMETHODCALLTYPE Decommit() override;
            HRESULT STDMETHODCALLTYPE GetBuffer(IMediaSample **ppBuffer,
                                                REFERENCE_TIME *pStartTime,
                                                REFERENCE_TIME *pEndTime,
                                                DWORD dwFlags) override;
            HRESULT STDMETHODCALLTYPE ReleaseBuffer(IMediaSample *pBuffer) override;

        private:
            MemAllocatorPrivate *d;
    };
}

#endif // MEMALLOCATOR_H
