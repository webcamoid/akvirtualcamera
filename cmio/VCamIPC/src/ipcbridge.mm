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
#include <codecvt>
#include <fstream>
#include <locale>
#include <map>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include <libgen.h>
#include <Foundation/Foundation.h>
#include <IOSurface/IOSurface.h>
#include <CoreMedia/CMFormatDescription.h>
#include <xpc/connection.h>
#include <libproc.h>

#include "Assistant/src/assistantglobals.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/logger/logger.h"
#include "VCamUtils/src/utils.h"

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1, std::placeholders::_2)

namespace AkVCam
{
    class IpcBridgePrivate
    {
        public:
            IpcBridge *self;
            std::string m_portName;
            xpc_connection_t m_messagePort;
            xpc_connection_t m_serverMessagePort;
            std::map<int64_t, XpcMessage> m_messageHandlers;
            std::vector<std::string> m_broadcasting;

            IpcBridgePrivate(IpcBridge *self=nullptr);
            ~IpcBridgePrivate();

            inline void add(IpcBridge *bridge);
            void remove(IpcBridge *bridge);
            inline std::vector<IpcBridge *> &bridges();
            inline const std::vector<DeviceControl> &controls() const;

            // Message handling methods
            void isAlive(xpc_connection_t client, xpc_object_t event);
            void deviceCreate(xpc_connection_t client, xpc_object_t event);
            void deviceDestroy(xpc_connection_t client, xpc_object_t event);
            void deviceUpdate(xpc_connection_t client, xpc_object_t event);
            void frameReady(xpc_connection_t client, xpc_object_t event);
            void pictureUpdated(xpc_connection_t client, xpc_object_t event);
            void setBroadcasting(xpc_connection_t client, xpc_object_t event);
            void controlsUpdated(xpc_connection_t client, xpc_object_t event);
            void listenerAdd(xpc_connection_t client, xpc_object_t event);
            void listenerRemove(xpc_connection_t client, xpc_object_t event);
            void messageReceived(xpc_connection_t client, xpc_object_t event);
            void connectionInterrupted();

            // Utility methods
            bool fileExists(const std::string &path) const;
            static std::string locatePluginPath();

        private:
            std::vector<IpcBridge *> m_bridges;
    };

    inline IpcBridgePrivate &ipcBridgePrivate()
    {
        static IpcBridgePrivate ipcBridgePrivate;

        return ipcBridgePrivate;
    }
}

AkVCam::IpcBridge::IpcBridge()
{
    AkLogFunction();
    this->d = new IpcBridgePrivate(this);
    auto loglevel = AkVCam::Preferences::logLevel();
    AkVCam::Logger::setLogLevel(loglevel);
    ipcBridgePrivate().add(this);
    this->registerPeer();
}

AkVCam::IpcBridge::~IpcBridge()
{
    this->unregisterPeer();
    ipcBridgePrivate().remove(this);
    delete this->d;
}

std::wstring AkVCam::IpcBridge::picture() const
{
    return Preferences::picture();
}

void AkVCam::IpcBridge::setPicture(const std::wstring &picture)
{
    AkLogFunction();
    Preferences::setPicture(picture);

    if (!this->d->m_serverMessagePort)
        return;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cv;
    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_PICTURE_UPDATED);
    xpc_dictionary_set_string(dictionary, "picture", cv.to_bytes(picture).c_str());
    xpc_connection_send_message(this->d->m_serverMessagePort, dictionary);
    xpc_release(dictionary);
}

int AkVCam::IpcBridge::logLevel() const
{
    return Preferences::logLevel();
}

void AkVCam::IpcBridge::setLogLevel(int logLevel)
{
    Preferences::setLogLevel(logLevel);
    Logger::setLogLevel(logLevel);
}

bool AkVCam::IpcBridge::registerPeer()
{
    AkLogFunction();

    if (this->d->m_serverMessagePort)
        return true;

    xpc_object_t dictionary = nullptr;
    xpc_object_t reply = nullptr;
    std::string portName;
    xpc_connection_t messagePort = nullptr;
    xpc_type_t replyType;
    bool status = false;

    auto serverMessagePort =
            xpc_connection_create_mach_service(CMIO_ASSISTANT_NAME,
                                               nullptr,
                                               0);

    if (!serverMessagePort)
        goto registerEndPoint_failed;

    xpc_connection_set_event_handler(serverMessagePort, ^(xpc_object_t event) {
        if (event == XPC_ERROR_CONNECTION_INTERRUPTED) {
            ipcBridgePrivate().connectionInterrupted();
        }
    });
    xpc_connection_resume(serverMessagePort);

    dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_REQUEST_PORT);
    reply = xpc_connection_send_message_with_reply_sync(serverMessagePort,
                                                        dictionary);
    xpc_release(dictionary);
    replyType = xpc_get_type(reply);

    if (replyType == XPC_TYPE_DICTIONARY)
        portName = xpc_dictionary_get_string(reply, "port");

    xpc_release(reply);

    if (replyType != XPC_TYPE_DICTIONARY)
        goto registerEndPoint_failed;

    messagePort = xpc_connection_create(nullptr, nullptr);

    if (!messagePort)
        goto registerEndPoint_failed;

    xpc_connection_set_event_handler(messagePort, ^(xpc_object_t event) {
        auto type = xpc_get_type(event);

        if (type == XPC_TYPE_ERROR)
            return;

        auto client = reinterpret_cast<xpc_connection_t>(event);

        xpc_connection_set_event_handler(client, ^(xpc_object_t event) {
            ipcBridgePrivate().messageReceived(client, event);
        });

        xpc_connection_resume(client);
    });

    xpc_connection_resume(messagePort);

    dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_ADD_PORT);
    xpc_dictionary_set_string(dictionary, "port", portName.c_str());
    xpc_dictionary_set_connection(dictionary, "connection", messagePort);
    reply = xpc_connection_send_message_with_reply_sync(serverMessagePort,
                                                        dictionary);
    xpc_release(dictionary);
    replyType = xpc_get_type(reply);

    if (replyType == XPC_TYPE_DICTIONARY)
        status = xpc_dictionary_get_bool(reply, "status");

    xpc_release(reply);

    if (replyType != XPC_TYPE_DICTIONARY || !status)
        goto registerEndPoint_failed;

    this->d->m_portName = portName;
    this->d->m_messagePort = messagePort;
    this->d->m_serverMessagePort = serverMessagePort;

    AkLogInfo() << "SUCCESSFUL" << std::endl;

    return true;

registerEndPoint_failed:
    if (messagePort)
        xpc_release(messagePort);

    if (serverMessagePort)
        xpc_release(serverMessagePort);

    AkLogError() << "FAILED" << std::endl;

    return false;
}

void AkVCam::IpcBridge::unregisterPeer()
{
    AkLogFunction();

    if (this->d->m_messagePort) {
        xpc_release(this->d->m_messagePort);
        this->d->m_messagePort = nullptr;
    }

    if (this->d->m_serverMessagePort) {
        if (!this->d->m_portName.empty()) {
            auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
            xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_REMOVE_PORT);
            xpc_dictionary_set_string(dictionary, "port", this->d->m_portName.c_str());
            xpc_connection_send_message(this->d->m_serverMessagePort,
                                        dictionary);
            xpc_release(dictionary);
        }

        xpc_release(this->d->m_serverMessagePort);
        this->d->m_serverMessagePort = nullptr;
    }

    this->d->m_portName.clear();
}

std::vector<std::string> AkVCam::IpcBridge::devices() const
{
    AkLogFunction();
    auto nCameras = Preferences::camerasCount();
    std::vector<std::string> devices;
    AkLogInfo() << "Devices:" << std::endl;

    for (size_t i = 0; i < nCameras; i++) {
        auto deviceId = Preferences::cameraPath(i);
        devices.push_back(deviceId);
        AkLogInfo() << "    " << deviceId << std::endl;
    }

    return devices;
}

std::wstring AkVCam::IpcBridge::description(const std::string &deviceId) const
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex < 0)
        return {};

    return Preferences::cameraDescription(size_t(cameraIndex));
}

void AkVCam::IpcBridge::setDescription(const std::string &deviceId,
                                       const std::wstring &description)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex >= 0)
        Preferences::cameraSetDescription(size_t(cameraIndex), description);
}

std::vector<AkVCam::PixelFormat> AkVCam::IpcBridge::supportedPixelFormats(StreamType type) const
{
    if (type == StreamTypeInput)
        return {PixelFormatRGB24};

    return {
        PixelFormatRGB32,
        PixelFormatRGB24,
        PixelFormatUYVY,
        PixelFormatYUY2
    };
}

AkVCam::PixelFormat AkVCam::IpcBridge::defaultPixelFormat(StreamType type) const
{
    return type == StreamTypeInput?
                PixelFormatRGB24:
                PixelFormatYUY2;
}

std::vector<AkVCam::VideoFormat> AkVCam::IpcBridge::formats(const std::string &deviceId) const
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex < 0)
        return {};

    return Preferences::cameraFormats(size_t(cameraIndex));
}

void AkVCam::IpcBridge::setFormats(const std::string &deviceId,
                                   const std::vector<VideoFormat> &formats)
{
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex >= 0)
        Preferences::cameraSetFormats(size_t(cameraIndex), formats);
}

std::string AkVCam::IpcBridge::broadcaster(const std::string &deviceId) const
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return {};

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return {};
    }

    std::string broadcaster = xpc_dictionary_get_string(reply, "broadcaster");
    xpc_release(reply);

    AkLogInfo() << "Device: " << deviceId << std::endl;
    AkLogInfo() << "Broadcaster: " << broadcaster << std::endl;

    return broadcaster;
}

std::vector<AkVCam::DeviceControl> AkVCam::IpcBridge::controls(const std::string &deviceId)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex < 0)
        return {};

    std::vector<DeviceControl> controls;

    for (auto &control: this->d->controls()) {
        controls.push_back(control);
        controls.back().value =
                Preferences::cameraControlValue(size_t(cameraIndex), control.id);
    }

    return controls;
}

void AkVCam::IpcBridge::setControls(const std::string &deviceId,
                                    const std::map<std::string, int> &controls)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex < 0)
        return;

    bool updated = false;

    for (auto &control: this->d->controls()) {
        auto oldValue =
                Preferences::cameraControlValue(size_t(cameraIndex),
                                                control.id);

        if (controls.count(control.id)) {
            auto newValue = controls.at(control.id);

            if (newValue != oldValue) {
                Preferences::cameraSetControlValue(size_t(cameraIndex),
                                                   control.id,
                                                   newValue);
                updated = true;
            }
        }
    }

    if (!this->d->m_serverMessagePort || !updated)
        return;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_CONTROLS_UPDATED);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_connection_send_message(this->d->m_serverMessagePort, dictionary);
    xpc_release(dictionary);
}

std::vector<std::string> AkVCam::IpcBridge::listeners(const std::string &deviceId)
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return {};

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_LISTENERS);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return {};
    }

    auto listenersList = xpc_dictionary_get_array(reply, "listeners");
    std::vector<std::string> listeners;

    for (size_t i = 0; i < xpc_array_get_count(listenersList); i++)
        listeners.push_back(xpc_array_get_string(listenersList, i));

    xpc_release(reply);

    AkLogInfo() << "Device: " << deviceId << std::endl;
    AkLogInfo() << "Listeners: " << listeners.size() << std::endl;

    return listeners;
}

std::vector<uint64_t> AkVCam::IpcBridge::clientsPids() const
{
    AkLogFunction();
    auto pluginPath = this->d->locatePluginPath();
    AkLogDebug() << "Plugin path: " << pluginPath << std::endl;

    if (pluginPath.empty())
        return {};

    auto path = pluginPath + "/Contents/MacOS/" CMIO_PLUGIN_NAME;
    AkLogDebug() << "Plugin binary: " << path << std::endl;

    if (!this->d->fileExists(path))
        return {};

    auto npids = proc_listpidspath(PROC_ALL_PIDS,
                                   0,
                                   path.c_str(),
                                   0,
                                   nullptr,
                                   0);
    pid_t pidsvec[npids];
    memset(pidsvec, 0, npids * sizeof(pid_t));
    proc_listpidspath(PROC_ALL_PIDS,
                      0,
                      path.c_str(),
                      0,
                      pidsvec,
                      npids * sizeof(pid_t));
    auto currentPid = getpid();
    std::vector<uint64_t> pids;

    for (int i = 0; i < npids; i++) {
        auto it = std::find(pids.begin(), pids.end(), pidsvec[i]);

        if (pidsvec[i] > 0 && it == pids.end() && pidsvec[i] != currentPid)
            pids.push_back(pidsvec[i]);
    }

    return pids;
}

std::string AkVCam::IpcBridge::clientExe(uint64_t pid) const
{
    char path[4096];
    memset(path, 0, 4096);
    proc_pidpath(pid, path, 4096);

    return {path};
}

std::string AkVCam::IpcBridge::addDevice(const std::wstring &description)
{
    return Preferences::addDevice(description);
}

void AkVCam::IpcBridge::removeDevice(const std::string &deviceId)
{
    Preferences::removeCamera(deviceId);
}

void AkVCam::IpcBridge::addFormat(const std::string &deviceId,
                                  const AkVCam::VideoFormat &format,
                                  int index)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex >= 0)
        Preferences::cameraAddFormat(size_t(cameraIndex),
                                     format,
                                     index);
}

void AkVCam::IpcBridge::removeFormat(const std::string &deviceId, int index)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex >= 0)
        Preferences::cameraRemoveFormat(size_t(cameraIndex),
                                        index);
}

void AkVCam::IpcBridge::updateDevices()
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary,
                             "message",
                             AKVCAM_ASSISTANT_MSG_DEVICE_UPDATE);
    xpc_connection_send_message(this->d->m_serverMessagePort, dictionary);
    xpc_release(dictionary);
}

bool AkVCam::IpcBridge::deviceStart(const std::string &deviceId,
                                    const VideoFormat &format)
{
    UNUSED(format);
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return false;

    auto it = std::find(this->d->m_broadcasting.begin(),
                        this->d->m_broadcasting.end(),
                        deviceId);

    if (it != this->d->m_broadcasting.end())
        return false;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_string(dictionary, "broadcaster", this->d->m_portName.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return false;
    }

    bool status = xpc_dictionary_get_bool(reply, "status");
    xpc_release(reply);
    this->d->m_broadcasting.push_back(deviceId);

    return status;
}

void AkVCam::IpcBridge::deviceStop(const std::string &deviceId)
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return;

    auto it = std::find(this->d->m_broadcasting.begin(),
                        this->d->m_broadcasting.end(),
                        deviceId);

    if (it == this->d->m_broadcasting.end())
        return;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_string(dictionary, "broadcaster", "");
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);
    this->d->m_broadcasting.erase(it);
}

bool AkVCam::IpcBridge::write(const std::string &deviceId,
                              const VideoFrame &frame)
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return false;

    auto it = std::find(this->d->m_broadcasting.begin(),
                        this->d->m_broadcasting.end(),
                        deviceId);

    if (it == this->d->m_broadcasting.end())
        return false;

    std::vector<CFStringRef> keys {
        kIOSurfacePixelFormat,
        kIOSurfaceWidth,
        kIOSurfaceHeight,
        kIOSurfaceAllocSize
    };

    auto fourcc = frame.format().fourcc();
    auto width = frame.format().width();
    auto height = frame.format().height();
    auto dataSize = int64_t(frame.data().size());

    std::vector<CFNumberRef> values {
        CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &fourcc),
        CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &width),
        CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &height),
        CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &dataSize)
    };

    auto surfaceProperties =
            CFDictionaryCreate(kCFAllocatorDefault,
                               reinterpret_cast<const void **>(keys.data()),
                               reinterpret_cast<const void **>(values.data()),
                               CFIndex(values.size()),
                               nullptr,
                               nullptr);
    auto surface = IOSurfaceCreate(surfaceProperties);

    for (auto &value: values)
        CFRelease(value);

    CFRelease(surfaceProperties);

    if (!surface)
        return false;

    uint32_t surfaceSeed = 0;
    IOSurfaceLock(surface, 0, &surfaceSeed);
    auto data = IOSurfaceGetBaseAddress(surface);
    memcpy(data, frame.data().data(), frame.data().size());
    IOSurfaceUnlock(surface, 0, &surfaceSeed);
    auto surfaceObj = IOSurfaceCreateXPCObject(surface);

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_FRAME_READY);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_value(dictionary, "frame", surfaceObj);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);
    xpc_release(surfaceObj);
    CFRelease(surface);

    return true;
}

bool AkVCam::IpcBridge::addListener(const std::string &deviceId)
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return false;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_string(dictionary, "listener", this->d->m_portName.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return false;
    }

    bool status = xpc_dictionary_get_bool(reply, "status");
    xpc_release(reply);

    return status;
}

bool AkVCam::IpcBridge::removeListener(const std::string &deviceId)
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return true;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_string(dictionary, "listener", this->d->m_portName.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return true;
    }

    bool status = xpc_dictionary_get_bool(reply, "status");
    xpc_release(reply);

    return status;
}

AkVCam::IpcBridgePrivate::IpcBridgePrivate(IpcBridge *self):
    self(self),
    m_messagePort(nullptr),
    m_serverMessagePort(nullptr)
{
    this->m_messageHandlers = {
        {AKVCAM_ASSISTANT_MSG_ISALIVE                , AKVCAM_BIND_FUNC(IpcBridgePrivate::isAlive)        },
        {AKVCAM_ASSISTANT_MSG_FRAME_READY            , AKVCAM_BIND_FUNC(IpcBridgePrivate::frameReady)     },
        {AKVCAM_ASSISTANT_MSG_PICTURE_UPDATED        , AKVCAM_BIND_FUNC(IpcBridgePrivate::pictureUpdated) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_CREATE          , AKVCAM_BIND_FUNC(IpcBridgePrivate::deviceCreate)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY         , AKVCAM_BIND_FUNC(IpcBridgePrivate::deviceDestroy)  },
        {AKVCAM_ASSISTANT_MSG_DEVICE_UPDATE          , AKVCAM_BIND_FUNC(IpcBridgePrivate::deviceUpdate)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD    , AKVCAM_BIND_FUNC(IpcBridgePrivate::listenerAdd)    },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE , AKVCAM_BIND_FUNC(IpcBridgePrivate::listenerRemove) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING , AKVCAM_BIND_FUNC(IpcBridgePrivate::setBroadcasting)},
        {AKVCAM_ASSISTANT_MSG_DEVICE_CONTROLS_UPDATED, AKVCAM_BIND_FUNC(IpcBridgePrivate::controlsUpdated)},
    };
}

AkVCam::IpcBridgePrivate::~IpcBridgePrivate()
{

}

void AkVCam::IpcBridgePrivate::add(IpcBridge *bridge)
{
    this->m_bridges.push_back(bridge);
}

void AkVCam::IpcBridgePrivate::remove(IpcBridge *bridge)
{
    for (size_t i = 0; i < this->m_bridges.size(); i++)
        if (this->m_bridges[i] == bridge) {
            this->m_bridges.erase(this->m_bridges.begin() + long(i));

            break;
        }
}

std::vector<AkVCam::IpcBridge *> &AkVCam::IpcBridgePrivate::bridges()
{
    return this->m_bridges;
}

const std::vector<AkVCam::DeviceControl> &AkVCam::IpcBridgePrivate::controls() const
{
    static const std::vector<std::string> scalingMenu {
        "Fast",
        "Linear"
    };
    static const std::vector<std::string> aspectRatioMenu {
        "Ignore",
        "Keep",
        "Expanding"
    };
    static const auto scalingMax = int(scalingMenu.size()) - 1;
    static const auto aspectRatioMax = int(aspectRatioMenu.size()) - 1;

    static const std::vector<DeviceControl> controls {
        {"hflip"       , "Horizontal Mirror", ControlTypeBoolean, 0, 1             , 1, 0, 0, {}             },
        {"vflip"       , "Vertical Mirror"  , ControlTypeBoolean, 0, 1             , 1, 0, 0, {}             },
        {"scaling"     , "Scaling"          , ControlTypeMenu   , 0, scalingMax    , 1, 0, 0, scalingMenu    },
        {"aspect_ratio", "Aspect Ratio"     , ControlTypeMenu   , 0, aspectRatioMax, 1, 0, 0, aspectRatioMenu},
        {"swap_rgb"    , "Swap RGB"         , ControlTypeBoolean, 0, 1             , 1, 0, 0, {}             },
    };

    return controls;
}

void AkVCam::IpcBridgePrivate::isAlive(xpc_connection_t client,
                                       xpc_object_t event)
{
    AkLogFunction();

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "alive", true);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::IpcBridgePrivate::deviceCreate(xpc_connection_t client,
                                            xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();
    std::string device = xpc_dictionary_get_string(event, "device");

    for (auto bridge: this->m_bridges)
        AKVCAM_EMIT(bridge, DeviceAdded, device)
}

void AkVCam::IpcBridgePrivate::deviceDestroy(xpc_connection_t client,
                                             xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();

    std::string device = xpc_dictionary_get_string(event, "device");

    for (auto bridge: this->m_bridges)
        AKVCAM_EMIT(bridge, DeviceRemoved, device)
}

void AkVCam::IpcBridgePrivate::deviceUpdate(xpc_connection_t client,
                                            xpc_object_t event)
{
    UNUSED(client);
    UNUSED(event);
    AkLogFunction();

    for (auto bridge: this->m_bridges)
        AKVCAM_EMIT(bridge, DevicesUpdated, nullptr)
}

void AkVCam::IpcBridgePrivate::frameReady(xpc_connection_t client,
                                          xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();

    std::string deviceId =
            xpc_dictionary_get_string(event, "device");
    auto frame = xpc_dictionary_get_value(event, "frame");
    auto surface = IOSurfaceLookupFromXPCObject(frame);

    if (surface) {
        uint32_t surfaceSeed = 0;
        IOSurfaceLock(surface, kIOSurfaceLockReadOnly, &surfaceSeed);
        FourCC fourcc = IOSurfaceGetPixelFormat(surface);
        int width = int(IOSurfaceGetWidth(surface));
        int height = int(IOSurfaceGetHeight(surface));
        size_t size = IOSurfaceGetAllocSize(surface);
        auto data = reinterpret_cast<uint8_t *>(IOSurfaceGetBaseAddress(surface));
        VideoFormat videoFormat(fourcc, width, height);
        VideoFrame videoFrame(videoFormat);
        memcpy(videoFrame.data().data(), data, size);
        IOSurfaceUnlock(surface, kIOSurfaceLockReadOnly, &surfaceSeed);
        CFRelease(surface);

        for (auto bridge: this->m_bridges)
            AKVCAM_EMIT(bridge, FrameReady, deviceId, videoFrame)
    }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", surface? true: false);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::IpcBridgePrivate::pictureUpdated(xpc_connection_t client,
                                              xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();

    std::string picture = xpc_dictionary_get_string(event, "picture");

    for (auto bridge: this->m_bridges)
        AKVCAM_EMIT(bridge, PictureChanged, picture)
}

void AkVCam::IpcBridgePrivate::setBroadcasting(xpc_connection_t client,
                                               xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();

    std::string deviceId =
            xpc_dictionary_get_string(event, "device");
    std::string broadcaster =
            xpc_dictionary_get_string(event, "broadcaster");

    for (auto bridge: this->m_bridges)
        AKVCAM_EMIT(bridge, BroadcastingChanged, deviceId, broadcaster)
}

void AkVCam::IpcBridgePrivate::controlsUpdated(xpc_connection_t client,
                                               xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();

    std::string deviceId =
            xpc_dictionary_get_string(event, "device");
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex < 0)
        return;

    std::map<std::string, int> controls;

    for (auto &control: this->controls())
        controls[control.id] =
                Preferences::cameraControlValue(size_t(cameraIndex), control.id);

    for (auto bridge: this->m_bridges)
        AKVCAM_EMIT(bridge,
                    ControlsChanged,
                    deviceId,
                    controls)
}

void AkVCam::IpcBridgePrivate::listenerAdd(xpc_connection_t client,
                                           xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();

    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::string listener = xpc_dictionary_get_string(event, "listener");

    for (auto bridge: this->m_bridges)
        AKVCAM_EMIT(bridge, ListenerAdded, deviceId, listener)
}

void AkVCam::IpcBridgePrivate::listenerRemove(xpc_connection_t client,
                                              xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();

    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::string listener = xpc_dictionary_get_string(event, "listener");

    for (auto bridge: this->m_bridges)
        AKVCAM_EMIT(bridge, ListenerRemoved, deviceId, listener)
}

void AkVCam::IpcBridgePrivate::messageReceived(xpc_connection_t client,
                                               xpc_object_t event)
{
    auto type = xpc_get_type(event);

    if (type == XPC_TYPE_ERROR) {
        auto description = xpc_copy_description(event);
        AkLogError() << description << std::endl;
        free(description);
    } else if (type == XPC_TYPE_DICTIONARY) {
        auto message = xpc_dictionary_get_int64(event, "message");

        if (this->m_messageHandlers.count(message))
            this->m_messageHandlers[message](client, event);
    }
}

void AkVCam::IpcBridgePrivate::connectionInterrupted()
{
    for (auto bridge: this->m_bridges) {
        AKVCAM_EMIT(bridge, ServerStateChanged, IpcBridge::ServerStateGone)
        bridge->unregisterPeer();
    }

    // Restart service
    for (auto bridge: this->m_bridges)
        if (bridge->registerPeer()) {
            AKVCAM_EMIT(bridge,
                        ServerStateChanged,
                        IpcBridge::ServerStateAvailable)
        }
}

bool AkVCam::IpcBridgePrivate::fileExists(const std::string &path) const
{
    struct stat stats;
    memset(&stats, 0, sizeof(struct stat));

    return stat(path.c_str(), &stats) == 0;
}

std::string AkVCam::IpcBridgePrivate::locatePluginPath()
{
    AkLogFunction();
    Dl_info info;
    memset(&info, 0, sizeof(Dl_info));
    dladdr(reinterpret_cast<void *>(&AkVCam::IpcBridgePrivate::locatePluginPath),
           &info);
    std::string dirName = dirname(const_cast<char *>(info.dli_fname));

    return realPath(dirName + "/../..");
}
