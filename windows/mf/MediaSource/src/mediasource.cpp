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

#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfobjects.h>
#include <mfreadwrite.h>
#include <propvarutil.h>
#include <dbt.h>
#include <mutex>

#include "mediasource.h"
#include "mediastream.h"
#include "mfvcam.h"
#include "controls.h"
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
    class MediaSourcePrivate
    {
        public:
            MediaSource *self;
            MediaStream *m_pStream {nullptr};
            MediaSourceState m_state {MediaSourceState_Stopped};
            IMFStreamDescriptor *m_pStreamDesc {nullptr};
            Controls *m_controls {nullptr};
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

AkVCam::MediaSource::MediaSource(const GUID &clsid):
    Attributes(),
    MediaEventGenerator()
{
    AkLogFunction();
    this->d = new MediaSourcePrivate(this);
    this->d->m_clsid = clsid;

    AkLogDebug() << "CLSID: " << stringFromClsidMF(clsid) << std::endl;

    auto cameraIndex = Preferences::cameraFromCLSID(clsid);
    AkLogDebug() << "Camera index: " << cameraIndex << std::endl;
    std::vector<IMFMediaType *> mediaTypes;

    if (cameraIndex >= 0) {
        this->d->m_directMode = Preferences::cameraDirectMode(cameraIndex);

        AkLogDebug() << "Virtual camera formats:";
        std::vector<AkVCam::VideoFormat> formats;

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
                AkLogDebug() << "    " << format;
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
}

AkVCam::MediaSource::~MediaSource()
{
    this->Shutdown();
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

HRESULT AkVCam::MediaSource::QueryInterface(REFIID riid, void **ppvObject)
{
    AkLogFunction();
    AkLogDebug() << "IID: " << stringFromClsidMF(riid) << std::endl;

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = nullptr;

    if (IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IMFMediaSource)
        || IsEqualIID(riid, IID_IMFAttributes)
        || IsEqualIID(riid, IID_IMFGetService)
        || IsEqualIID(riid, IID_IMFMediaEventGenerator)) {
        AkLogInterface(IMFMediaSource, this);
        this->AddRef();
        *ppvObject = this;

        return S_OK;
    } else if (!this->d->m_directMode
               && (IsEqualIID(riid, IID_VCamControl)
                   || IsEqualIID(riid, IID_IAMVideoProcAmp))) {
        auto controls = this->d->m_controls;
        AkLogInterface(IAMVideoProcAmp, controls);
        controls->AddRef();
        *ppvObject = controls;

        return S_OK;
    }

    return MediaEventGenerator::QueryInterface(riid, ppvObject);
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

    if (FAILED(hr))
        return hr;

    *presentationDescriptor = pPresentationDesc;

    return S_OK;
}

HRESULT AkVCam::MediaSource::Start(IMFPresentationDescriptor *pPresentationDescriptor,
                                   const GUID *pguidTimeFormat,
                                   const PROPVARIANT *pvarStartPosition)
{
    AkLogFunction();

    if (!pPresentationDescriptor) {
        AkLogError() << "Invalid pointer" << std::endl;

        return E_POINTER;
    }

    HRESULT hr = S_OK;
    std::lock_guard<std::mutex> lock(this->d->m_mutex);

    if (this->d->m_state != MediaSourceState_Stopped) {
        AkLogError() << "Invalid state transition" << std::endl;

        return MF_E_INVALID_STATE_TRANSITION;
    }

    if (pguidTimeFormat && *pguidTimeFormat != GUID_NULL) {
        AkLogError() << "Unsuported time format" << std::endl;

        return MF_E_UNSUPPORTED_TIME_FORMAT;
    }

    // Validate the presentation descriptor
    DWORD streamCount = 0;
    hr = pPresentationDescriptor->GetStreamDescriptorCount(&streamCount);

    if (FAILED(hr) || streamCount < 1) {
        AkLogError() << "Invalid request: " << hr << std::endl;

        return MF_E_INVALIDREQUEST;
    }

    BOOL selected = FALSE;
    IMFStreamDescriptor *pStreamDesc = nullptr;
    hr = pPresentationDescriptor->GetStreamDescriptorByIndex(0,
                                                             &selected,
                                                             &pStreamDesc);

    if (FAILED(hr)) {
        AkLogError() << "Failed getting stream descriptor by index: " << hr << std::endl;

        return hr;
    }

    if (!selected) {
        AkLogError() << "Stream not selected" << std::endl;
        pStreamDesc->Release();

        return MF_E_INVALIDREQUEST;
    }

    IMFMediaTypeHandler *mediaTypeHandler = nullptr;
    hr = pStreamDesc->GetMediaTypeHandler(&mediaTypeHandler);
    pStreamDesc->Release();

    if (FAILED(hr)) {
        AkLogError() << "Failed to get the media type handler: " << hr << std::endl;

        return hr;
    }

    IMFMediaType *mediaType = nullptr;
    hr = mediaTypeHandler->GetCurrentMediaType(&mediaType);
    mediaTypeHandler->Release();

    if (FAILED(hr)) {
        AkLogError() << "Failed to get the current media type: " << hr << std::endl;

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
        AkLogError() << "Failed to start the stream: " << hr << std::endl;
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
        AkLogError() << "Failed to queue MESourceStarted: " << hr << std::endl;
        this->d->m_state = MediaSourceState_Stopped;
        this->d->m_pStream->stop();

        return hr;
    }

    AkLogDebug() << "MediaSource started" << std::endl;

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

HRESULT AkVCam::MediaSource::GetService(REFGUID service,
                                        REFIID riid,
                                        LPVOID *ppvObject)
{
    UNUSED(service);
    UNUSED(riid);
    UNUSED(ppvObject);

    return MF_E_UNSUPPORTED_SERVICE;
}

AkVCam::MediaSourcePrivate::MediaSourcePrivate(MediaSource *self):
    self(self),
    m_controls(new Controls)
{
    this->m_controls->AddRef();

    this->m_ipcBridge = std::make_shared<IpcBridge>();
    this->m_ipcBridge->connectDevicesChanged(this,
                                             &MediaSourcePrivate::devicesChanged);
    this->m_ipcBridge->connectFrameReady(this,
                                         &MediaSourcePrivate::frameReady);
    this->m_ipcBridge->connectPictureChanged(this,
                                             &MediaSourcePrivate::pictureChanged);
    this->m_ipcBridge->connectControlsChanged(this,
                                              &MediaSourcePrivate::setControls);
}

AkVCam::MediaSourcePrivate::~MediaSourcePrivate()
{
    AkLogFunction();
    this->m_ipcBridge->stopNotifications();
    this->m_controls->Release();
}

void AkVCam::MediaSourcePrivate::frameReady(void *userData,
                                            const std::string &deviceId,
                                            const VideoFrame &frame,
                                            bool isActive)
{
    AkLogFunction();
    auto self = reinterpret_cast<MediaSourcePrivate *>(userData);
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

    if (!self->m_directMode)
        self->m_pStream->setControls(controls);
}

BOOL AkVCamEnumWindowsProc(HWND handler, LPARAM userData)
{
    auto handlers = reinterpret_cast<std::vector<HWND> *>(userData);
    handlers->push_back(handler);

    return TRUE;
}
