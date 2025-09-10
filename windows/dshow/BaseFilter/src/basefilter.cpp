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
#include <dshow.h>
#include <dbt.h>

#include "basefilter.h"
#include "enumpins.h"
#include "filtermiscflags.h"
#include "pin.h"
#include "referenceclock.h"
#include "specifypropertypages.h"
#include "videocontrol.h"
#include "videoprocamp.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/utils.h"

#define AkVCamPinCall(pins, func, ...) \
    pins->Reset(); \
    Pin *pin = nullptr; \
    \
    while (pins->Next(1, reinterpret_cast<IPin **>(&pin), nullptr) == S_OK) { \
        pin->func(__VA_ARGS__); \
        pin->Release(); \
    }

#define AkVCamDevicePinCall(deviceId, where, func, ...) \
    if (auto pins = where->pinsForDevice(deviceId)) { \
        AkVCamPinCall(pins, func, __VA_ARGS__) \
        pins->Release(); \
    }

namespace AkVCam
{
    class BaseFilterPrivate
    {
        public:
            BaseFilter *self;
            EnumPins *m_pins {nullptr};
            VideoProcAmp *m_videoProcAmp {nullptr};
            ReferenceClock *m_referenceClock {nullptr};
            std::string m_vendor {DSHOW_PLUGIN_VENDOR};
            std::string m_filterName;
            IFilterGraph *m_filterGraph {nullptr};
            IpcBridgePtr m_ipcBridge;

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

AkVCam::BaseFilter::BaseFilter(const GUID &clsid):
    MediaFilter(clsid, this)
{
    this->setParent(this, &IID_IBaseFilter);
    this->d = new BaseFilterPrivate(this);
    auto camera = Preferences::cameraFromCLSID(clsid);

    if (camera >= 0) {
        this->d->m_filterName = Preferences::cameraDescription(size_t(camera));
        auto formats = Preferences::cameraFormats(size_t(camera));
        this->addPin(formats, "Video", false);
    }
}

AkVCam::BaseFilter::~BaseFilter()
{
    AkLogFunction();

    delete this->d;
}

void AkVCam::BaseFilter::addPin(const std::vector<AkVCam::VideoFormat> &formats,
                                const std::string &pinName,
                                bool changed)
{
    AkLogFunction();
    auto pin = new Pin(this, formats, pinName);
    pin->setBridge(this->d->m_ipcBridge);
    this->d->m_pins->addPin(pin, changed);
}

void AkVCam::BaseFilter::removePin(IPin *pin, bool changed)
{
    AkLogFunction();
    this->d->m_pins->removePin(pin, changed);
}

AkVCam::BaseFilter *AkVCam::BaseFilter::create(const GUID &clsid)
{
    AkLogFunction();
    AkLogInfo() << "CLSID: " << stringFromIid(clsid) << std::endl;

    return new BaseFilter(clsid);
}

IFilterGraph *AkVCam::BaseFilter::filterGraph() const
{
    return this->d->m_filterGraph;
}

IReferenceClock *AkVCam::BaseFilter::referenceClock() const
{
    return this->d->m_referenceClock;
}

std::string AkVCam::BaseFilter::deviceId()
{
    CLSID clsid;
    this->GetClassID(&clsid);
    auto cameraIndex = Preferences::cameraFromCLSID(clsid);

    if (cameraIndex < 0)
        return {};

    return Preferences::cameraId(size_t(cameraIndex));
}

HRESULT AkVCam::BaseFilter::QueryInterface(const IID &riid, void **ppvObject)
{
    AkLogFunction();
    AkLogInfo() << "IID: " << AkVCam::stringFromClsid(riid) << std::endl;

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IBaseFilter)
        || IsEqualIID(riid, IID_IMediaFilter)) {
        AkLogInterface(IBaseFilter, this);
        this->AddRef();
        *ppvObject = this;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IAMFilterMiscFlags)) {
        auto filterMiscFlags = new FilterMiscFlags;
        AkLogInterface(IAMFilterMiscFlags, filterMiscFlags);
        filterMiscFlags->AddRef();
        *ppvObject = filterMiscFlags;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IAMVideoControl)) {
        IEnumPins *pins = nullptr;
        this->d->m_pins->Clone(&pins);
        auto videoControl = new VideoControl(pins);
        pins->Release();
        AkLogInterface(IAMVideoControl, videoControl);
        videoControl->AddRef();
        *ppvObject = videoControl;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IAMVideoProcAmp)) {
        auto videoProcAmp = this->d->m_videoProcAmp;
        AkLogInterface(IAMVideoProcAmp, videoProcAmp);
        videoProcAmp->AddRef();
        *ppvObject = videoProcAmp;

        return S_OK;
    } else if (IsEqualIID(riid, IID_IReferenceClock)) {
        auto referenceClock = this->d->m_referenceClock;
        AkLogInterface(IReferenceClock, referenceClock);
        referenceClock->AddRef();
        *ppvObject = referenceClock;

        return S_OK;
    } else if (IsEqualIID(riid, IID_ISpecifyPropertyPages)) {
        this->d->m_pins->Reset();
        IPin *pin = nullptr;
        this->d->m_pins->Next(1, &pin, nullptr);
        auto specifyPropertyPages = new SpecifyPropertyPages(pin);
        pin->Release();
        AkLogInterface(ISpecifyPropertyPages, specifyPropertyPages);
        specifyPropertyPages->AddRef();
        *ppvObject = specifyPropertyPages;

        return S_OK;
    } else {
        this->d->m_pins->Reset();
        IPin *pin = nullptr;

        if (this->d->m_pins->Next(1, &pin, nullptr) == S_OK && pin) {
            auto result = pin->QueryInterface(riid, ppvObject);
            pin->Release();

            if (SUCCEEDED(result))
                return result;
        }
    }

    return MediaFilter::QueryInterface(riid, ppvObject);
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
    this->d->m_filterName = pName? stringFromWSTR(pName): "";

    AkLogInfo() << "Filter graph: " << this->d->m_filterGraph << std::endl;
    AkLogInfo() << "Name: " << this->d->m_filterName << std::endl;

    return S_OK;
}

HRESULT AkVCam::BaseFilter::QueryVendorInfo(LPWSTR *pVendorInfo)
{
    AkLogFunction();

    if (this->d->m_vendor.size() < 1)
        return E_NOTIMPL;

    if (!pVendorInfo)
        return E_POINTER;

    *pVendorInfo = wstrFromString(this->d->m_vendor);

    return S_OK;
}

AkVCam::BaseFilterPrivate::BaseFilterPrivate(AkVCam::BaseFilter *self):
    self(self),
    m_pins(new AkVCam::EnumPins),
    m_videoProcAmp(new VideoProcAmp),
    m_referenceClock(new ReferenceClock)
{
    this->m_pins->AddRef();
    this->m_videoProcAmp->AddRef();
    this->m_referenceClock->AddRef();

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
    this->m_pins->setBaseFilter(nullptr);
    this->m_pins->Release();
    this->m_videoProcAmp->Release();
    this->m_referenceClock->Release();
}

IEnumPins *AkVCam::BaseFilterPrivate::pinsForDevice(const std::string &deviceId)
{
    AkLogFunction();
    CLSID clsid;
    self->GetClassID(&clsid);
    auto cameraIndex = Preferences::cameraFromCLSID(clsid);

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
    CLSID clsid;
    this->self->GetClassID(&clsid);
    auto cameraIndex = Preferences::cameraFromCLSID(clsid);

    if (cameraIndex < 0)
        return;

    auto deviceId = Preferences::cameraId(size_t(cameraIndex));

    auto controlsList = this->m_ipcBridge->controls(deviceId);
    std::map<std::string, int> controls;

    for (auto &control: controlsList)
        controls[control.id] = control.value;

    AkVCamDevicePinCall(deviceId, this, setControls, controls)
}

void AkVCam::BaseFilterPrivate::frameReady(void *userData,
                                           const std::string &deviceId,
                                           const VideoFrame &frame,
                                           bool isActive)
{
    AkLogFunction();
    auto self = reinterpret_cast<BaseFilterPrivate *>(userData);
    AkVCamDevicePinCall(deviceId, self, frameReady, frame, isActive)
}

void AkVCam::BaseFilterPrivate::pictureChanged(void *userData,
                                               const std::string &picture)
{
    AkLogFunction();
    auto self = reinterpret_cast<BaseFilterPrivate *>(userData);
    AkVCamDevicePinCall(self->self->deviceId(), self, setPicture, picture)
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
    AkVCamDevicePinCall(deviceId, self, setControls, controls)
}

BOOL AkVCamEnumWindowsProc(HWND handler, LPARAM userData)
{
    auto handlers = reinterpret_cast<std::vector<HWND> *>(userData);
    handlers->push_back(handler);

    return TRUE;
}
