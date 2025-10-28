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

#ifndef CLASSFACTORY_H
#define CLASSFACTORY_H

#include "PlatformUtils/src/cunknown.h"

namespace AkVCam
{
    class ClassFactoryPrivate;

    class ClassFactory: public CUnknown<IClassFactory>
    {
        public:
            ClassFactory(const CLSID &clsid);
            virtual ~ClassFactory();

            static bool locked();

            BEGIN_COM_MAP(ClassFactory)
                COM_INTERFACE(IClassFactory)
            END_COM_MAP(ClassFactory)

            // IClassFactory
            HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *pUnkOuter,
                                                     REFIID riid,
                                                     void **ppvObject) override;
            HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock) override;

        private:
            ClassFactoryPrivate *d;
    };
}

#endif // CLASSFACTORY_H
