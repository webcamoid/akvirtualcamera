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

#ifndef BASEFILTER_H
#define BASEFILTER_H

#include <string>
#include <vector>

#include "VCamUtils/src/videoformat.h"
#include "mediafilter.h"

namespace AkVCam
{
    class BaseFilterPrivate;

    class BaseFilter:
            public IBaseFilter,
            public MediaFilter
    {
        public:
            BaseFilter(const GUID &clsid);
            virtual ~BaseFilter();

            void addPin(const std::vector<VideoFormat> &formats={},
                        const std::string &pinName={},
                        bool changed=true);
            void removePin(IPin *pin, bool changed=true);
            static BaseFilter *create(const GUID &clsid);
            IFilterGraph *filterGraph() const;
            IReferenceClock *referenceClock() const;
            std::string deviceId();
            bool directMode() const;

            DECLARE_IMEDIAFILTER_NQ

            // IUnknown
            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                                     void **ppvObject) override;

            // IBaseFilter
            HRESULT STDMETHODCALLTYPE EnumPins(IEnumPins **ppEnum) override;
            HRESULT STDMETHODCALLTYPE FindPin(LPCWSTR Id, IPin **ppPin) override;
            HRESULT STDMETHODCALLTYPE QueryFilterInfo(FILTER_INFO *pInfo) override;
            HRESULT STDMETHODCALLTYPE JoinFilterGraph(IFilterGraph *pGraph,
                                                      LPCWSTR pName) override;
            HRESULT STDMETHODCALLTYPE QueryVendorInfo(LPWSTR *pVendorInfo) override;

        private:
            BaseFilterPrivate *d;
    };
}

#endif // BASEFILTER_H
