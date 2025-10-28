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

#include <atomic>
#include <mutex>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
#include <dbt.h>

#include "mediasource.h"
#include "mediastream.h"
#include "mfvcam.h"
#include "PlatformUtils/src/cunknown.h"
#include "PlatformUtils/src/utils.h"
#include "PlatformUtils/src/preferences.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/videoformat.h"
#include "MFUtils/src/utils.h"

enum MediaSourceState
{
    MediaSourceState_Stopped,
    MediaSourceState_Started,
    MediaSourceState_Paused
};

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

    class MediaSourcePrivate
    {
        public:
            MediaSource *self;
            std::atomic<uint64_t> m_refCount {1};
            MediaStream *m_pStream {nullptr};
            MediaSourceState m_state {MediaSourceState_Stopped};
            IMFStreamDescriptor *m_pStreamDesc {nullptr};
            std::map<LONG, LONG> m_controls;
            IpcBridgePtr m_ipcBridge;
            std::string m_deviceId;
            CLSID m_clsid;
            std::mutex m_mutex;
            bool m_directMode {false};

            MediaSourcePrivate(MediaSource *self);
            ~MediaSourcePrivate();
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

AkVCam::MediaSource::MediaSource(const GUID &clsid)
{
    AkLogFunction();
    this->d = new MediaSourcePrivate(this);
    this->d->m_clsid = clsid;
    AkLogDebug("CLSID: %s", stringFromClsidMF(clsid).c_str());

    auto cameraIndex = Preferences::cameraFromCLSID(clsid);
    AkLogDebug("Camera index: %d", cameraIndex);
    std::vector<IMFMediaType *> mediaTypes;

    if (cameraIndex >= 0) {
        this->d->m_directMode = Preferences::cameraDirectMode(cameraIndex);

        AkLogDebug("Virtual camera formats:");
        std::vector<VideoFormat> formats;

        if (this->d->m_directMode) {
            auto fmts = Preferences::cameraFormats(size_t(cameraIndex));

            if (!fmts.empty())
                formats.push_back(fmts.front());
        } else {
            formats = Preferences::cameraFormats(size_t(cameraIndex));
        }

        for (auto &format: formats) {
            auto mediaType = mfMediaTypeFromFormat(format);

            if (mediaType) {
                AkLogDebug("    %s", format.toString().c_str());
                mediaTypes.push_back(mediaType);
            }
        }

        MFCreateStreamDescriptor(0,
                                 mediaTypes.size(),
                                 mediaTypes.data(),
                                 &this->d->m_pStreamDesc);

        if (!mediaTypes.empty()) {
            IMFMediaTypeHandler *mediaTypeHandler = nullptr;

            if (SUCCEEDED(this->d->m_pStreamDesc->GetMediaTypeHandler(&mediaTypeHandler))) {
                mediaTypeHandler->SetCurrentMediaType(mediaTypes.front());
                mediaTypeHandler->Release();
            }
        }
    }

    this->d->m_ipcBridge = std::make_shared<IpcBridge>();
    this->d->m_pStream = new MediaStream(this, this->d->m_pStreamDesc);
    this->d->m_pStream->setBridge(this->d->m_ipcBridge);

    if (cameraIndex >= 0) {
        for (auto mediaType: mediaTypes)
            mediaType->Release();

        auto deviceId = Preferences::cameraId(size_t(cameraIndex));

        if (!deviceId.empty()) {
            auto controlsList = this->d->m_ipcBridge->controls(deviceId);
            std::map<std::string, int> controls;

            for (auto &control: controlsList)
                controls[control.id] = control.value;

            this->d->m_pStream->setControls(controls);
            this->d->m_deviceId = deviceId;
        }
    }

    this->d->m_ipcBridge->connectDevicesChanged(this->d,
                                                &MediaSourcePrivate::devicesChanged);
    this->d->m_ipcBridge->connectFrameReady(this->d,
                                            &MediaSourcePrivate::frameReady);
    this->d->m_ipcBridge->connectPictureChanged(this->d,
                                                &MediaSourcePrivate::pictureChanged);
    this->d->m_ipcBridge->connectControlsChanged(this->d,
                                                 &MediaSourcePrivate::setControls);
}

AkVCam::MediaSource::~MediaSource()
{
    AkLogFunction();
    this->Shutdown();

    this->d->m_ipcBridge->stopNotifications();

    if (this->d->m_pStream)
        this->d->m_pStream->Release();

    if (this->d->m_pStreamDesc)
        this->d->m_pStreamDesc->Release();

    delete this->d;
}

std::string AkVCam::MediaSource::deviceId() const
{
    return this->d->m_deviceId;
}

bool AkVCam::MediaSource::directMode() const
{
    return this->d->m_directMode;
}

HRESULT AkVCam::MediaSource::QueryInterface(const IID &riid, void **ppv)
{
    AkLogFunction();
    AkLogDebug("IID: %s", stringFromClsid(riid).c_str());

    if (!ppv)
        return E_POINTER;

    static const struct
    {
        const IID *iid;
        void *ptr;
    } comInterfaceEntryMediaSample[] = {
        COM_INTERFACE(IMFMediaEventGenerator)
        COM_INTERFACE(IMFMediaSource)
        COM_INTERFACE(IMFAttributes)
        COM_INTERFACE(IMFGetService)
        COM_INTERFACE(IAMVideoProcAmp)
        COM_INTERFACE2(IUnknown, IMFMediaSource)
        COM_INTERFACE_NULL
    };

    for (auto map = comInterfaceEntryMediaSample; map->ptr; ++map) {
        if (this->d->m_directMode && *map->iid == IID_IAMVideoProcAmp)
            continue;

        if (*map->iid == riid) {
            *ppv = map->ptr;
            this->AddRef();

            return S_OK;
        }
    }

    *ppv = nullptr;
    AkLogDebug("Interface not found");

    return E_NOINTERFACE;
}

ULONG AkVCam::MediaSource::AddRef()
{
    AkLogFunction();

    return ++this->d->m_refCount;
}

ULONG AkVCam::MediaSource::Release()
{
    AkLogFunction();
    ULONG ref = --this->d->m_refCount;

    if (ref == 0)
        delete this;

    return ref;
}

HRESULT AkVCam::MediaSource::GetService(REFGUID service,
                                        REFIID riid,
                                        LPVOID *ppvObject)
{
    UNUSED(service);
    UNUSED(riid);
    UNUSED(ppvObject);

    return MF_E_UNSUPPORTED_SERVICE;
}

HRESULT AkVCam::MediaSource::GetRange(LONG property,
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

HRESULT AkVCam::MediaSource::Set(LONG property, LONG lValue, LONG flags)
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

HRESULT AkVCam::MediaSource::Get(LONG property, LONG *lValue, LONG *flags)
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

HRESULT AkVCam::MediaSource::GetCharacteristics(DWORD *pdwCharacteristics)
{
    AkLogFunction();

    if (!pdwCharacteristics)
        return E_POINTER;

    *pdwCharacteristics = MFMEDIASOURCE_IS_LIVE;

    return S_OK;
}

HRESULT AkVCam::MediaSource::CreatePresentationDescriptor(IMFPresentationDescriptor **presentationDescriptor)
{
    AkLogFunction();

    if (!presentationDescriptor)
        return E_POINTER;

    if (!this->d->m_pStreamDesc)
        return E_UNEXPECTED;

    *presentationDescriptor = nullptr;

    IMFPresentationDescriptor *pPresentationDesc = nullptr;
    auto hr = MFCreatePresentationDescriptor(1,
                                             &this->d->m_pStreamDesc,
                                             &pPresentationDesc);

    if (FAILED(hr))
        return hr;

    hr = pPresentationDesc->SelectStream(0);

    if (FAILED(hr)) {
        pPresentationDesc->Release();

        return hr;
    }

    *presentationDescriptor = pPresentationDesc;

    return S_OK;
}

HRESULT AkVCam::MediaSource::Start(IMFPresentationDescriptor *pPresentationDescriptor,
                                   const GUID *pguidTimeFormat,
                                   const PROPVARIANT *pvarStartPosition)
{
    AkLogFunction();

    if (!pPresentationDescriptor) {
        AkLogError("Invalid pointer");

        return E_POINTER;
    }

    HRESULT hr = S_OK;
    std::lock_guard<std::mutex> lock(this->d->m_mutex);

    if (this->d->m_state != MediaSourceState_Stopped) {
        AkLogError("Invalid state transition");

        return MF_E_INVALID_STATE_TRANSITION;
    }

    if (pguidTimeFormat && *pguidTimeFormat != GUID_NULL) {
        AkLogError("Unsuported time format");

        return MF_E_UNSUPPORTED_TIME_FORMAT;
    }

    // Validate the presentation descriptor
    DWORD streamCount = 0;
    hr = pPresentationDescriptor->GetStreamDescriptorCount(&streamCount);

    if (FAILED(hr) || streamCount < 1) {
        AkLogError("Invalid request: 0x%x", hr);

        return MF_E_INVALIDREQUEST;
    }

    BOOL selected = FALSE;
    IMFStreamDescriptor *pStreamDesc = nullptr;
    hr = pPresentationDescriptor->GetStreamDescriptorByIndex(0,
                                                             &selected,
                                                             &pStreamDesc);

    if (FAILED(hr)) {
        AkLogError("Failed getting stream descriptor by index: 0x%x", hr);

        return hr;
    }

    if (!selected) {
        AkLogError("Stream not selected");
        pStreamDesc->Release();

        return MF_E_INVALIDREQUEST;
    }

    IMFMediaTypeHandler *mediaTypeHandler = nullptr;
    hr = pStreamDesc->GetMediaTypeHandler(&mediaTypeHandler);
    pStreamDesc->Release();

    if (FAILED(hr)) {
        AkLogError("Failed to get the media type handler: 0x%x", hr);

        return hr;
    }

    IMFMediaType *mediaType = nullptr;
    hr = mediaTypeHandler->GetCurrentMediaType(&mediaType);
    mediaTypeHandler->Release();

    if (FAILED(hr)) {
        AkLogError("Failed to get the current media type: 0x%x", hr);

        return hr;
    }

    // Change the state and start the stream
    this->d->m_state = MediaSourceState_Started;
    this->eventQueue()->QueueEventParamUnk(MENewStream,
                                           GUID_NULL,
                                           S_OK,
                                           reinterpret_cast<IUnknown *>(this->d->m_pStream));

    hr = this->d->m_pStream->start(mediaType);
    mediaType->Release();

    if (FAILED(hr)) {
        AkLogError("Failed to start the stream: 0x%x", hr);
        this->d->m_state = MediaSourceState_Stopped;

        return hr;
    }

    PROPVARIANT time;
    InitPropVariantFromInt64(MFGetSystemTime(), &time);

    // Enqueue the MESourceStarted event
    hr = this->eventQueue()->QueueEventParamVar(MESourceStarted,
                                                   GUID_NULL,
                                                   S_OK,
                                                   &time);

    if (FAILED(hr)) {
        AkLogError("Failed to queue MESourceStarted: 0x%x", hr);
        this->d->m_state = MediaSourceState_Stopped;
        this->d->m_pStream->stop();

        return hr;
    }

    AkLogDebug("MediaSource started");

    return S_OK;
}

HRESULT AkVCam::MediaSource::Stop()
{
    AkLogFunction();
    std::lock_guard<std::mutex> lock(this->d->m_mutex);

    if (this->d->m_state != MediaSourceState_Started
        && this->d->m_state != MediaSourceState_Paused) {
        return MF_E_INVALID_STATE_TRANSITION;
    }

    this->d->m_state = MediaSourceState_Stopped;
    auto hr = this->d->m_pStream->stop();

    if (FAILED(hr))
        return hr;

    return this->eventQueue()->QueueEventParamVar(MESourceStopped,
                                                     GUID_NULL,
                                                     S_OK,
                                                     nullptr);
}

HRESULT AkVCam::MediaSource::Pause()
{
    AkLogFunction();
    std::lock_guard<std::mutex> lock(this->d->m_mutex);

    if (this->d->m_state != MediaSourceState_Started)
        return MF_E_INVALID_STATE_TRANSITION;

    this->d->m_state = MediaSourceState_Paused;

    return this->eventQueue()->QueueEventParamVar(MESourcePaused,
                                                     GUID_NULL,
                                                     S_OK,
                                                     nullptr);
}

HRESULT AkVCam::MediaSource::Shutdown()
{
    AkLogFunction();
    std::lock_guard<std::mutex> lock(this->d->m_mutex);

    if (this->d->m_state == MediaSourceState_Stopped) {
        this->d->m_pStream->stop();
        this->eventQueue()->Shutdown();

        return S_OK;
    }

    this->d->m_state = MediaSourceState_Stopped;
    auto hr = this->d->m_pStream->stop();

    if (FAILED(hr)) {
        this->eventQueue()->Shutdown();

        return hr;
    }

    return this->eventQueue()->Shutdown();
}

AkVCam::MediaSourcePrivate::MediaSourcePrivate(MediaSource *self):
    self(self)
{
}

AkVCam::MediaSourcePrivate::~MediaSourcePrivate()
{
}

void AkVCam::MediaSourcePrivate::frameReady(void *userData,
                                            const std::string &deviceId,
                                            const VideoFrame &frame,
                                            bool isActive)
{
    AkLogFunction();
    auto self = reinterpret_cast<MediaSourcePrivate *>(userData);

    if (deviceId != self->m_deviceId)
        return;

    self->m_pStream->frameReady(frame, isActive);
}

void AkVCam::MediaSourcePrivate::pictureChanged(void *userData,
                                                const std::string &picture)
{
    AkLogFunction();
    auto self = reinterpret_cast<MediaSourcePrivate *>(userData);
    self->m_pStream->setPicture(picture);
}

void AkVCam::MediaSourcePrivate::devicesChanged(void *userData,
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

void AkVCam::MediaSourcePrivate::setControls(void *userData,
                                             const std::string &deviceId,
                                             const std::map<std::string, int> &controls)
{
    UNUSED(deviceId);
    AkLogFunction();
    auto self = reinterpret_cast<MediaSourcePrivate *>(userData);

    if (deviceId != self->m_deviceId || self->m_directMode)
        return;

    self->m_pStream->setControls(controls);
}

BOOL AkVCamEnumWindowsProc(HWND handler, LPARAM userData)
{
    auto handlers = reinterpret_cast<std::vector<HWND> *>(userData);
    handlers->push_back(handler);

    return TRUE;
}
