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
#include <strmif.h>

#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/utils.h"

namespace AkVCam
{
    class BaseFilterPrivate;
    class Pin;

    class BaseFilter:
            public virtual IBaseFilter,
            public virtual IAMFilterMiscFlags,
            public virtual IAMVideoControl,
            public virtual IAMVideoProcAmp,
            public virtual ISpecifyPropertyPages
    {
        AKVCAM_SIGNAL(PropertyChanged, LONG Property, LONG lValue, LONG Flags)

        public:
            BaseFilter(const GUID &clsid);
            virtual ~BaseFilter();

            IpcBridgePtr ipcBridge() const;
            static BaseFilter *create(const GUID &clsid);
            IReferenceClock *referenceClock() const;
            std::string deviceId();
            bool directMode() const;

            // IUnknown

            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                                     void **ppv) override;
            ULONG STDMETHODCALLTYPE AddRef() override;
            ULONG STDMETHODCALLTYPE Release() override;

            // IAMFilterMiscFlags

            ULONG STDMETHODCALLTYPE GetMiscFlags() override;

            // IAMVideoControl

            HRESULT STDMETHODCALLTYPE GetCaps(IPin *pPin, LONG *pCapsFlags) override;
            HRESULT STDMETHODCALLTYPE SetMode(IPin *pPin, LONG Mode) override;
            HRESULT STDMETHODCALLTYPE GetMode(IPin *pPin, LONG *Mode) override;
            HRESULT STDMETHODCALLTYPE GetCurrentActualFrameRate(IPin *pPin,
                                                                LONGLONG *ActualFrameRate) override;
            HRESULT STDMETHODCALLTYPE GetMaxAvailableFrameRate(IPin *pPin,
                                                               LONG iIndex,
                                                               SIZE Dimensions,
                                                               LONGLONG *MaxAvailableFrameRate) override;
            HRESULT STDMETHODCALLTYPE GetFrameRateList(IPin *pPin,
                                                       LONG iIndex,
                                                       SIZE Dimensions,
                                                       LONG *ListSize,
                                                       LONGLONG **FrameRates) override;

            // IAMVideoProcAmp

            HRESULT STDMETHODCALLTYPE GetRange(LONG property,
                                               LONG *pMin,
                                               LONG *pMax,
                                               LONG *pSteppingDelta,
                                               LONG *pDefault,
                                               LONG *pCapsFlags) override;
            HRESULT STDMETHODCALLTYPE Set(LONG property,
                                          LONG lValue,
                                          LONG flags) override;
            HRESULT STDMETHODCALLTYPE Get(LONG property,
                                          LONG *lValue,
                                          LONG *flags) override;

            // ISpecifyPropertyPages
            HRESULT STDMETHODCALLTYPE GetPages(CAUUID *pPages) override;

            // IPersist

            HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID) override;

            // IMediaFilter

            HRESULT STDMETHODCALLTYPE Stop() override;
            HRESULT STDMETHODCALLTYPE Pause() override;
            HRESULT STDMETHODCALLTYPE Run(REFERENCE_TIME tStart) override;
            HRESULT STDMETHODCALLTYPE GetState(DWORD dwMilliSecsTimeout,
                                               FILTER_STATE *State) override;
            HRESULT STDMETHODCALLTYPE SetSyncSource(IReferenceClock *pClock) override;
            HRESULT STDMETHODCALLTYPE GetSyncSource(IReferenceClock **pClock) override;

            // IBaseFilter

            HRESULT STDMETHODCALLTYPE EnumPins(IEnumPins **ppEnum) override;
            HRESULT STDMETHODCALLTYPE FindPin(LPCWSTR Id, IPin **ppPin) override;
            HRESULT STDMETHODCALLTYPE QueryFilterInfo(FILTER_INFO *pInfo) override;
            HRESULT STDMETHODCALLTYPE JoinFilterGraph(IFilterGraph *pGraph,
                                                      LPCWSTR pName) override;
            HRESULT STDMETHODCALLTYPE QueryVendorInfo(LPWSTR *pVendorInfo) override;

        private:
            BaseFilterPrivate *d;

        friend class BaseFilterPrivate;
    };
}

#endif // BASEFILTER_H
