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

#include <algorithm>
#include <cinttypes>
#include <condition_variable>
#include <thread>
#include <dshow.h>
#include <dbt.h>
#include <amvideo.h>
#include <dvdmedia.h>
#include <uuids.h>

#include "basefilter.h"
#include "enumpins.h"
#include "pin.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/utils.h"

#define TIME_BASE 1.0e7

namespace AkVCam
{
    class ProcAmpPrivate
    {
        public:
            LONG property;
            LONG min;
            LONG max;
            LONG step;
            LONG defaultValue;
            LONG flags;

            inline static const std::vector<ProcAmpPrivate> &controls()
            {
                static const std::vector<ProcAmpPrivate> controls {
                    {VideoProcAmp_Brightness , -255, 255, 1, 0, VideoProcAmp_Flags_Manual},
                    {VideoProcAmp_Contrast   , -255, 255, 1, 0, VideoProcAmp_Flags_Manual},
                    {VideoProcAmp_Saturation , -255, 255, 1, 0, VideoProcAmp_Flags_Manual},
                    {VideoProcAmp_Gamma      , -255, 255, 1, 0, VideoProcAmp_Flags_Manual},
                    {VideoProcAmp_Hue        , -359, 359, 1, 0, VideoProcAmp_Flags_Manual},
                    {VideoProcAmp_ColorEnable,    0,   1, 1, 1, VideoProcAmp_Flags_Manual}
                };

                return controls;
            }

            static inline const ProcAmpPrivate *byProperty(LONG property)
            {
                for (auto &control: controls())
                    if (control.property == property)
                        return &control;

                return nullptr;
            }
    };

    class BaseFilterPrivate
    {
        public:
            BaseFilter *self;
            CLSID m_clsid;
            EnumPins *m_pins {nullptr};
            std::string m_vendor {DSHOW_PLUGIN_VENDOR};
            std::string m_filterName;
            std::string m_deviceId;
            IFilterGraph *m_filterGraph {nullptr};
            IReferenceClock *m_clock {nullptr};
            FILTER_STATE m_state {State_Stopped};
            REFERENCE_TIME m_start {0};
            IpcBridgePtr m_ipcBridge;
            std::map<LONG, LONG> m_controls;
            bool m_directMode {false};

            BaseFilterPrivate(BaseFilter *self);
            BaseFilterPrivate(const BaseFilterPrivate &other) = delete;
            ~BaseFilterPrivate();
            IEnumPins *pinsForDevice(const std::string &deviceId);
            void updatePins();
            static void frameReady(void *userData,
                                   const std::string &deviceId,
                                   const VideoFrame &frame,
                                   bool isActive);
            static void pictureChanged(void *userData,
                                       const std::string &picture);
            static void devicesChanged(void *userData,
                                       const std::vector<std::string> &devices);
            static void setControls(void *userData,
                                    const std::string &deviceId,
                                    const std::map<std::string, int> &controls);
    };
}

BOOL AkVCamEnumWindowsProc(HWND handler, LPARAM userData);

AkVCam::BaseFilter::BaseFilter(const GUID &clsid)
{
    this->d = new BaseFilterPrivate(this);
    this->d->m_clsid = clsid;
    auto camera = Preferences::cameraFromCLSID(clsid);

    if (camera >= 0) {
        this->d->m_deviceId = Preferences::cameraId(size_t(camera));
        this->d->m_filterName = Preferences::cameraDescription(size_t(camera));
        this->d->m_directMode = Preferences::cameraDirectMode(camera);
        auto formats = Preferences::cameraFormats(size_t(camera));

        if (this->d->m_directMode) {
            std::vector<AkVCam::VideoFormat> fmts;

            if (!formats.empty())
                fmts.push_back(formats.front());

            this->d->m_pins->addPin(fmts, "Video");
        } else {
            this->d->m_pins->addPin(formats, "Video");
        }
    }
}

AkVCam::BaseFilter::~BaseFilter()
{
    AkLogFunction();

    if (this->d->m_clock)
        this->d->m_clock->Release();

    delete this->d;
}

AkVCam::IpcBridgePtr AkVCam::BaseFilter::ipcBridge() const
{
    return this->d->m_ipcBridge;
}

AkVCam::BaseFilter *AkVCam::BaseFilter::create(const GUID &clsid)
{
    AkLogFunction();
    AkLogDebug("CLSID: %s", stringFromIid(clsid).c_str());

    return new BaseFilter(clsid);
}

IReferenceClock *AkVCam::BaseFilter::referenceClock() const
{
    return this->d->m_clock;
}

std::string AkVCam::BaseFilter::deviceId()
{
    return this->d->m_deviceId;
}

bool AkVCam::BaseFilter::directMode() const
{
    return this->d->m_directMode;
}

ULONG AkVCam::BaseFilter::GetMiscFlags()
{
    AkLogFunction();

    return AM_FILTER_MISC_FLAGS_IS_SOURCE;
}

HRESULT AkVCam::BaseFilter::GetCaps(IPin *pPin, LONG *pCapsFlags)
{
    AkLogFunction();

    if (!pPin || !pCapsFlags)
        return E_POINTER;

    *pCapsFlags = 0;

    if (!this->d->m_pins->contains(pPin))
        return E_FAIL;

    *pCapsFlags = VideoControlFlag_FlipHorizontal
                | VideoControlFlag_FlipVertical;

    return S_OK;
}

HRESULT AkVCam::BaseFilter::SetMode(IPin *pPin, LONG Mode)
{
    AkLogFunction();

    if (!pPin)
        return E_POINTER;

    if (!this->d->m_pins->contains(pPin))
        return E_FAIL;

    auto cpin = dynamic_cast<Pin *>(pPin);
    cpin->setHorizontalFlip((Mode & VideoControlFlag_FlipHorizontal) != 0);
    cpin->setVerticalFlip((Mode & VideoControlFlag_FlipVertical) != 0);

    return S_OK;
}

HRESULT AkVCam::BaseFilter::GetMode(IPin *pPin, LONG *Mode)
{
    AkLogFunction();

    if (!pPin || !Mode)
        return E_POINTER;

    *Mode = 0;

    if (!this->d->m_pins->contains(pPin))
        return E_FAIL;

    auto cpin = dynamic_cast<Pin *>(pPin);

    if (cpin->horizontalFlip())
        *Mode |= VideoControlFlag_FlipHorizontal;

    if (cpin->verticalFlip())
        *Mode |= VideoControlFlag_FlipVertical;

    return S_OK;
}

HRESULT AkVCam::BaseFilter::GetCurrentActualFrameRate(IPin *pPin,
                                                      LONGLONG *ActualFrameRate)
{
    AkLogFunction();

    if (!pPin || !ActualFrameRate)
        return E_POINTER;

    *ActualFrameRate = 0;

    if (!this->d->m_pins->contains(pPin))
        return E_FAIL;

    auto cpin = dynamic_cast<Pin *>(pPin);
    AM_MEDIA_TYPE *mediaType = nullptr;
    auto result = cpin->GetFormat(&mediaType);

    if (FAILED(result))
        return result;

    if (IsEqualGUID(mediaType->formattype, FORMAT_VideoInfo)) {
        auto format = reinterpret_cast<VIDEOINFOHEADER *>(mediaType->pbFormat);
        *ActualFrameRate = format->AvgTimePerFrame;
    } else if (IsEqualGUID(mediaType->formattype, FORMAT_VideoInfo2)) {
        auto format = reinterpret_cast<VIDEOINFOHEADER2 *>(mediaType->pbFormat);
        *ActualFrameRate = format->AvgTimePerFrame;
    } else {
        deleteMediaType(&mediaType);

        return E_FAIL;
    }

    deleteMediaType(&mediaType);

    return S_OK;
}

HRESULT AkVCam::BaseFilter::GetMaxAvailableFrameRate(IPin *pPin,
                                                     LONG iIndex,
                                                     SIZE Dimensions,
                                                     LONGLONG *MaxAvailableFrameRate)
{
    AkLogFunction();

    if (!pPin || !MaxAvailableFrameRate)
        return E_POINTER;

    *MaxAvailableFrameRate = 0;

    if (!this->d->m_pins->contains(pPin))
        return E_FAIL;

    auto cpin = dynamic_cast<Pin *>(pPin);
    AM_MEDIA_TYPE *mediaType = nullptr;
    VIDEO_STREAM_CONFIG_CAPS configCaps;
    auto result = cpin->GetStreamCaps(iIndex,
                                      &mediaType,
                                      reinterpret_cast<BYTE *>(&configCaps));

    if (SUCCEEDED(result)) {
        if (configCaps.MaxOutputSize.cx == Dimensions.cx
            && configCaps.MaxOutputSize.cy == Dimensions.cy) {
            *MaxAvailableFrameRate = configCaps.MaxFrameInterval;
        } else {
            result = E_FAIL;
        }

        deleteMediaType(&mediaType);
    }

    return result;
}

HRESULT AkVCam::BaseFilter::GetFrameRateList(IPin *pPin,
                                             LONG iIndex,
                                             SIZE Dimensions,
                                             LONG *ListSize,
                                             LONGLONG **FrameRates)
{
    AkLogFunction();

    if (!pPin || !ListSize || !FrameRates)
        return E_POINTER;

    *ListSize = 0;
    *FrameRates = nullptr;

    if (!this->d->m_pins->contains(pPin))
        return E_FAIL;

    auto cpin = dynamic_cast<Pin *>(pPin);
    AM_MEDIA_TYPE *mediaType = nullptr;
    VIDEO_STREAM_CONFIG_CAPS configCaps;
    auto result = cpin->GetStreamCaps(iIndex,
                                      &mediaType,
                                      reinterpret_cast<BYTE *>(&configCaps));

    if (SUCCEEDED(result)) {
        if (configCaps.MaxOutputSize.cx == Dimensions.cx
            && configCaps.MaxOutputSize.cy == Dimensions.cy) {
            *ListSize = 1;
            *FrameRates = reinterpret_cast<LONGLONG *>(CoTaskMemAlloc(sizeof(LONGLONG)));
            **FrameRates = configCaps.MaxFrameInterval;
        } else {
            result = E_FAIL;
        }

        deleteMediaType(&mediaType);
    }

    return result;
}

HRESULT AkVCam::BaseFilter::GetRange(LONG property,
                                     LONG *pMin,
                                     LONG *pMax,
                                     LONG *pSteppingDelta,
                                     LONG *pDefault,
                                     LONG *pCapsFlags)
{
    AkLogFunction();

    if (!pMin || !pMax || !pSteppingDelta || !pDefault || !pCapsFlags)
        return E_POINTER;

    *pMin = 0;
    *pMax = 0;
    *pSteppingDelta = 0;
    *pDefault = 0;
    *pCapsFlags = 0;

    auto control = ProcAmpPrivate::byProperty(property);

    if (!control)
        return E_INVALIDARG;

    *pMin = control->min;
    *pMax = control->max;
    *pSteppingDelta = control->step;
    *pDefault = control->defaultValue;
    *pCapsFlags = control->flags;

    return S_OK;
}

HRESULT AkVCam::BaseFilter::Set(LONG property, LONG lValue, LONG flags)
{
    AkLogFunction();

    auto control = ProcAmpPrivate::byProperty(property);

    if (!control)
        return E_INVALIDARG;

    if (lValue < control->min
        || lValue > control->max
        || flags != control->flags) {
        return E_INVALIDARG;
    }

    this->d->m_controls[property] = lValue;
    AKVCAM_EMIT(this, PropertyChanged, property, lValue, flags)

    return S_OK;
}

HRESULT AkVCam::BaseFilter::Get(LONG property, LONG *lValue, LONG *flags)
{
    AkLogFunction();

    if (!lValue || !flags)
        return E_POINTER;

    *lValue = 0;
    *flags = 0;

    auto control = ProcAmpPrivate::byProperty(property);

    if (!control)
        return E_INVALIDARG;

    if (this->d->m_controls.count(property))
        *lValue = this->d->m_controls[property];
    else
        *lValue = control->defaultValue;

    *flags = control->flags;

    return S_OK;
}

HRESULT AkVCam::BaseFilter::GetPages(CAUUID *pPages)
{
    AkLogFunction();

    if (!pPages)
        return E_POINTER;

    std::vector<GUID> pages {
        CLSID_VideoProcAmpPropertyPage,
    };

    IPin *connectedPin = nullptr;
    IPin *pin = nullptr;
    this->d->m_pins->pin(0, &pin);

    if (SUCCEEDED(pin->ConnectedTo(&connectedPin))) {
        if (this->d->m_state == State_Stopped)
            pages.push_back(CLSID_VideoStreamConfigPropertyPage);

        connectedPin->Release();
    }

    pPages->cElems = static_cast<ULONG>(pages.size());
    pPages->pElems =
            reinterpret_cast<GUID *>(CoTaskMemAlloc(sizeof(GUID) * pages.size()));
    AkLogInfo("Returning property pages:");

    for (size_t i = 0; i < pages.size(); i++) {
        memcpy(&pPages->pElems[i], &pages[i], sizeof(GUID));
        AkLogInfo("    %s", stringFromClsid(pages[i]).c_str());
    }

    return S_OK;
}

HRESULT AkVCam::BaseFilter::GetClassID(CLSID *pClassID)
{
    AkLogFunction();

    if (!pClassID)
        return E_POINTER;

    *pClassID = this->d->m_clsid;

    return S_OK;
}

HRESULT AkVCam::BaseFilter::Stop()
{
    AkLogFunction();
    this->d->m_state = State_Stopped;

    return this->d->m_pins->stop();
}

HRESULT AkVCam::BaseFilter::Pause()
{
    AkLogFunction();
    this->d->m_state = State_Paused;

    return this->d->m_pins->pause();
}

HRESULT AkVCam::BaseFilter::Run(REFERENCE_TIME tStart)
{
    AkLogFunction();
    this->d->m_start = tStart;
    this->d->m_state = State_Running;

    return this->d->m_pins->run(tStart);
}

HRESULT AkVCam::BaseFilter::GetState(DWORD dwMilliSecsTimeout,
                                     FILTER_STATE *State)
{
    UNUSED(dwMilliSecsTimeout);
    AkLogFunction();

    if (!State)
        return E_POINTER;

    *State = this->d->m_state;
    AkLogDebug("State: %d", *State);

    return S_OK;
}

HRESULT AkVCam::BaseFilter::SetSyncSource(IReferenceClock *pClock)
{
    AkLogFunction();

    if (this->d->m_clock)
        this->d->m_clock->Release();

    this->d->m_clock = pClock;

    if (this->d->m_clock)
        this->d->m_clock->AddRef();

    return S_OK;
}

HRESULT AkVCam::BaseFilter::GetSyncSource(IReferenceClock **pClock)
{
    AkLogFunction();

    if (!pClock)
        return E_POINTER;

    *pClock = this->d->m_clock;

    if (*pClock)
        (*pClock)->AddRef();

    return S_OK;
}

HRESULT AkVCam::BaseFilter::EnumPins(IEnumPins **ppEnum)
{
    AkLogFunction();

    if (!this->d->m_pins)
        return E_FAIL;

    auto result = this->d->m_pins->Clone(ppEnum);

    if (SUCCEEDED(result))
        (*ppEnum)->Reset();

    return result;
}

HRESULT AkVCam::BaseFilter::FindPin(LPCWSTR Id, IPin **ppPin)
{
    AkLogFunction();

    if (!ppPin)
        return E_POINTER;

    *ppPin = nullptr;

    if (!Id)
        return VFW_E_NOT_FOUND;

    IPin *pin = nullptr;
    HRESULT result = VFW_E_NOT_FOUND;
    this->d->m_pins->Reset();

    while (this->d->m_pins->Next(1, &pin, nullptr) == S_OK) {
        WCHAR *pinId = nullptr;
        auto ok = pin->QueryId(&pinId);

        if (ok == S_OK && wcscmp(pinId, Id) == 0) {
            *ppPin = pin;
            (*ppPin)->AddRef();
            result = S_OK;
        }

        CoTaskMemFree(pinId);
        pin->Release();
        pin = nullptr;

        if (result == S_OK)
            break;
    }

    return result;
}

HRESULT AkVCam::BaseFilter::QueryFilterInfo(FILTER_INFO *pInfo)
{
    AkLogFunction();

    if (!pInfo)
        return E_POINTER;

    memset(pInfo->achName, 0, MAX_FILTER_NAME * sizeof(WCHAR));

    if (!this->d->m_filterName.empty()) {
        auto filterName = wstrFromString(this->d->m_filterName);
        memcpy(pInfo->achName,
               filterName,
               (std::min<size_t>)(wcsnlen(filterName, MAX_FILTER_NAME)
                                  * sizeof(WCHAR),
                                  MAX_FILTER_NAME));
        CoTaskMemFree(filterName);
    }

    pInfo->pGraph = this->d->m_filterGraph;

    if (pInfo->pGraph)
        pInfo->pGraph->AddRef();

    return S_OK;
}

HRESULT AkVCam::BaseFilter::JoinFilterGraph(IFilterGraph *pGraph, LPCWSTR pName)
{
    AkLogFunction();

    this->d->m_filterGraph = pGraph;
    this->d->m_filterName = stringFromWSTR(pName);

    AkLogDebug("Filter graph: %p", this->d->m_filterGraph);
    AkLogDebug("Name: %s", this->d->m_filterName.c_str());

    return S_OK;
}

HRESULT AkVCam::BaseFilter::QueryVendorInfo(LPWSTR *pVendorInfo)
{
    AkLogFunction();

    if (!pVendorInfo)
        return E_POINTER;

    if (this->d->m_vendor.size() < 1)
        return E_NOTIMPL;

    *pVendorInfo = wstrFromString(this->d->m_vendor);

    return S_OK;
}

bool AkVCam::BaseFilter::isInterfaceDisabled(REFIID riid) const
{
    if (this->d->m_directMode
        && (IsEqualIID(riid, IID_IAMVideoControl)
            || IsEqualIID(riid, IID_IAMVideoProcAmp))) {
        return true;
    }

    return false;
}

AkVCam::BaseFilterPrivate::BaseFilterPrivate(AkVCam::BaseFilter *self):
    self(self)
{
    this->m_pins = new EnumPins(self),
    this->m_ipcBridge = std::make_shared<IpcBridge>();
    this->m_ipcBridge->connectDevicesChanged(this,
                                             &BaseFilterPrivate::devicesChanged);
    this->m_ipcBridge->connectFrameReady(this,
                                         &BaseFilterPrivate::frameReady);
    this->m_ipcBridge->connectPictureChanged(this,
                                             &BaseFilterPrivate::pictureChanged);
    this->m_ipcBridge->connectControlsChanged(this,
                                              &BaseFilterPrivate::setControls);
}

AkVCam::BaseFilterPrivate::~BaseFilterPrivate()
{
    AkLogFunction();

    for (auto &device: this->m_ipcBridge->devices())
        this->m_ipcBridge->deviceStop(device);

    this->m_ipcBridge->stopNotifications();
    this->m_pins->Release();
}

IEnumPins *AkVCam::BaseFilterPrivate::pinsForDevice(const std::string &deviceId)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromCLSID(this->m_clsid);

    if (cameraIndex < 0)
        return nullptr;

    auto id = Preferences::cameraId(size_t(cameraIndex));

    if (id.empty() || id != deviceId)
        return nullptr;

    IEnumPins *pins = nullptr;
    self->EnumPins(&pins);

    return pins;
}

void AkVCam::BaseFilterPrivate::updatePins()
{
    AkLogFunction();
    auto controlsList = this->m_ipcBridge->controls(this->m_deviceId);
    std::map<std::string, int> controls;

    for (auto &control: controlsList)
        controls[control.id] = control.value;

    for (size_t i = 0; i < this->m_pins->size(); ++i) {
        IPin *pin = nullptr;
        this->m_pins->pin(i, &pin);
        dynamic_cast<Pin *>(pin)->setControls(controls);
        pin->Release();
    }
}

void AkVCam::BaseFilterPrivate::frameReady(void *userData,
                                           const std::string &deviceId,
                                           const VideoFrame &frame,
                                           bool isActive)
{
    AkLogFunction();
    auto self = reinterpret_cast<BaseFilterPrivate *>(userData);

    if (deviceId != self->m_deviceId)
        return;

    for (size_t i = 0; i < self->m_pins->size(); ++i) {
        IPin *pin = nullptr;
        self->m_pins->pin(i, &pin);
        dynamic_cast<Pin *>(pin)->frameReady(frame, isActive);
        pin->Release();
    }
}

void AkVCam::BaseFilterPrivate::pictureChanged(void *userData,
                                               const std::string &picture)
{
    AkLogFunction();
    auto self = reinterpret_cast<BaseFilterPrivate *>(userData);

    for (size_t i = 0; i < self->m_pins->size(); ++i) {
        IPin *pin = nullptr;
        self->m_pins->pin(i, &pin);
        dynamic_cast<Pin *>(pin)->setPicture(picture);
        pin->Release();
    }
}

void AkVCam::BaseFilterPrivate::devicesChanged(void *userData,
                                               const std::vector<std::string> &devices)
{
    AkLogFunction();
    UNUSED(userData);
    UNUSED(devices);
    std::vector<HWND> handlers;
    EnumWindows(WNDENUMPROC(AkVCamEnumWindowsProc), LPARAM(&handlers));

    for (auto &handler: handlers)
        SendNotifyMessage(handler, WM_DEVICECHANGE, DBT_DEVNODES_CHANGED, 0);
}

void AkVCam::BaseFilterPrivate::setControls(void *userData,
                                            const std::string &deviceId,
                                            const std::map<std::string, int> &controls)
{
    AkLogFunction();
    auto self = reinterpret_cast<BaseFilterPrivate *>(userData);

    if (deviceId != self->m_deviceId)
        return;

    for (size_t i = 0; i < self->m_pins->size(); ++i) {
        IPin *pin = nullptr;
        self->m_pins->pin(i, &pin);
        dynamic_cast<Pin *>(pin)->setControls(controls);
        pin->Release();
    }
}

BOOL AkVCamEnumWindowsProc(HWND handler, LPARAM userData)
{
    auto handlers = reinterpret_cast<std::vector<HWND> *>(userData);
    handlers->push_back(handler);

    return TRUE;
}
