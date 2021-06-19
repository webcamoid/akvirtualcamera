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
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/logger.h"
#include "VCamUtils/src/utils.h"

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1, std::placeholders::_2)

#define AKVCAM_BIND_HACK_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1)

namespace AkVCam
{
    class Hack
    {
        public:
            using HackFunc = std::function<int (const std::vector<std::string> &args)>;

            std::string name;
            std::string description;
            bool isSafe {false};
            bool needsRoot {false};
            HackFunc func;

            Hack();
            Hack(const std::string &name,
                 const std::string &description,
                 bool isSafe,
                 bool needsRoot,
                 const HackFunc &func);
            Hack(const Hack &other);
            Hack &operator =(const Hack &other);
    };

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
            void updateDevices(xpc_connection_t port, bool propagate);

            // Message handling methods
            void isAlive(xpc_connection_t client, xpc_object_t event);
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
            bool isRoot() const;
            std::vector<std::string> listServices() const;
            std::vector<std::string> listDisabledServices() const;
            inline bool isServiceLoaded(const std::string &service) const;
            inline bool isServiceDisabled(const std::string &service) const;
            bool readEntitlements(const std::string &app,
                                  const std::string &output) const;

            // Hacks
            const std::vector<Hack> &hacks();
            int setServiceUp(const std::vector<std::string> &args);
            int setServiceDown(const std::vector<std::string> &args);
            int disableLibraryValidation(const std::vector<std::string> &args);
            int codeResign(const std::vector<std::string> &args);
            int unsign(const std::vector<std::string> &args);
            int disableSIP(const std::vector<std::string> &args);

        private:
            std::vector<IpcBridge *> m_bridges;
    };

    inline IpcBridgePrivate &ipcBridgePrivate()
    {
        static IpcBridgePrivate ipcBridgePrivate;

        return ipcBridgePrivate;
    }
}

AkVCam::IpcBridge::IpcBridge(bool isVCam)
{
    AkLogFunction();
    this->d = new IpcBridgePrivate(this);
    auto loglevel = AkVCam::Preferences::logLevel();
    AkVCam::Logger::setLogLevel(loglevel);
    ipcBridgePrivate().add(this);
    this->registerPeer(isVCam);
}

AkVCam::IpcBridge::~IpcBridge()
{
    this->unregisterPeer();
    ipcBridgePrivate().remove(this);
    delete this->d;
}

std::string AkVCam::IpcBridge::picture() const
{
    return Preferences::picture();
}

void AkVCam::IpcBridge::setPicture(const std::string &picture)
{
    AkLogFunction();
    Preferences::setPicture(picture);

    if (!this->d->m_serverMessagePort)
        return;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_PICTURE_UPDATED);
    xpc_dictionary_set_string(dictionary, "picture", picture.c_str());
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

std::string AkVCam::IpcBridge::logPath(const std::string &logName) const
{
    if (logName.empty())
        return {};

    return AkVCam::Preferences::readString("logfile",
                                           "/tmp/" + logName + ".log");
}

bool AkVCam::IpcBridge::registerPeer(bool isVCam)
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

    AkLogDebug() << "Requesting service." << std::endl;

    auto serverMessagePort =
            xpc_connection_create_mach_service(CMIO_ASSISTANT_NAME,
                                               nullptr,
                                               0);

    if (!serverMessagePort)
        goto registerEndPoint_failed;

    AkLogDebug() << "Setting event handler." << std::endl;

    xpc_connection_set_event_handler(serverMessagePort, ^(xpc_object_t event) {
        if (event == XPC_ERROR_CONNECTION_INTERRUPTED) {
            ipcBridgePrivate().connectionInterrupted();
        }
    });
    xpc_connection_resume(serverMessagePort);

    AkLogDebug() << "Requesting port." << std::endl;

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

    AkLogDebug() << "Creating message port." << std::endl;

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

    AkLogDebug() << "Adding port to the server." << std::endl;

    dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_ADD_PORT);
    xpc_dictionary_set_string(dictionary, "port", portName.c_str());
    xpc_dictionary_set_connection(dictionary, "connection", messagePort);
    xpc_dictionary_set_bool(dictionary, "isvcam", isVCam);
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
    this->d->updateDevices(serverMessagePort, false);

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
    std::vector<std::string> devices;
    auto nCameras = Preferences::camerasCount();
    AkLogInfo() << "Devices:" << std::endl;

    for (size_t i = 0; i < nCameras; i++) {
        auto deviceId = Preferences::cameraPath(i);
        devices.push_back(deviceId);
        AkLogInfo() << "    " << deviceId << std::endl;
    }

    return devices;
}

std::string AkVCam::IpcBridge::description(const std::string &deviceId) const
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex < 0)
        return {};

    return Preferences::cameraDescription(size_t(cameraIndex));
}

void AkVCam::IpcBridge::setDescription(const std::string &deviceId,
                                       const std::string &description)
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

std::string AkVCam::IpcBridge::addDevice(const std::string &description,
                                         const std::string &deviceId)
{
    AkLogFunction();

    return Preferences::addDevice(description, deviceId);
}

void AkVCam::IpcBridge::removeDevice(const std::string &deviceId)
{
    AkLogFunction();

    Preferences::removeCamera(deviceId);
}

void AkVCam::IpcBridge::addFormat(const std::string &deviceId,
                                  const VideoFormat &format,
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
    this->d->updateDevices(this->d->m_serverMessagePort, true);
}

bool AkVCam::IpcBridge::deviceStart(const std::string &deviceId,
                                    const VideoFormat &format)
{
    UNUSED(format);
    AkLogFunction();

    if (!this->d->m_serverMessagePort) {
        AkLogError() << "Service not ready." << std::endl;

        return false;
    }

    auto it = std::find(this->d->m_broadcasting.begin(),
                        this->d->m_broadcasting.end(),
                        deviceId);

    if (it != this->d->m_broadcasting.end()) {
        AkLogError() << '\'' << deviceId << "' is busy." << std::endl;

        return false;
    }

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_string(dictionary, "broadcaster", this->d->m_portName.c_str());
    xpc_connection_send_message(this->d->m_serverMessagePort, dictionary);
    xpc_release(dictionary);

    this->d->m_broadcasting.push_back(deviceId);

    return true;
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
    xpc_connection_send_message(this->d->m_serverMessagePort, dictionary);
    xpc_release(dictionary);

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

bool AkVCam::IpcBridge::isBusyFor(const std::string &operation) const
{
    static const std::vector<std::string> operations {
        "add-device",
        "add-format",
        "load",
        "remove-device",
        "remove-devices",
        "remove-format",
        "remove-formats",
        "set-description",
        "update",
        "hack"
    };

    auto it = std::find(operations.begin(), operations.end(), operation);

    return it != operations.end() && !this->clientsPids().empty();
}

bool AkVCam::IpcBridge::needsRoot(const std::string &operation) const
{
    static const std::vector<std::string> operations;
    auto it = std::find(operations.begin(), operations.end(), operation);

    return it != operations.end() && !this->d->isRoot();
}

int AkVCam::IpcBridge::sudo(int argc, char **argv) const
{
    UNUSED(argc);
    UNUSED(argv);
    std::cerr << "You must run this command with administrator privileges." << std::endl;

    return -1;
}

std::vector<std::string> AkVCam::IpcBridge::hacks() const
{
    std::vector<std::string> hacks;

    for (auto &hack: this->d->hacks())
        hacks.push_back(hack.name);

    return hacks;
}

std::string AkVCam::IpcBridge::hackDescription(const std::string &hack) const
{
    for (auto &hck: this->d->hacks())
        if (hck.name == hack)
            return hck.description;

    return {};
}

bool AkVCam::IpcBridge::hackIsSafe(const std::string &hack) const
{
    for (auto &hck: this->d->hacks())
        if (hck.name == hack)
            return hck.isSafe;

    return true;
}

bool AkVCam::IpcBridge::hackNeedsRoot(const std::string &hack) const
{
    for (auto &hck: this->d->hacks())
        if (hck.name == hack)
            return hck.needsRoot && !this->d->isRoot();

    return false;
}

int AkVCam::IpcBridge::execHack(const std::string &hack,
                                const std::vector<std::string> &args)
{
    for (auto &hck: this->d->hacks())
        if (hck.name == hack)
            return hck.func(args);

    return 0;
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

void AkVCam::IpcBridgePrivate::updateDevices(xpc_connection_t port, bool propagate)
{
    AkLogFunction();

    if (!port)
        return;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary,
                             "message",
                             AKVCAM_ASSISTANT_MSG_DEVICE_UPDATE);
    auto devices = xpc_array_create(nullptr, 0);

    for (size_t i = 0; i < Preferences::camerasCount(); i++) {
        auto path = Preferences::cameraPath(i);
        auto pathObj = xpc_string_create(path.c_str());
        xpc_array_append_value(devices, pathObj);
        AkLogDebug() << "Device " << i << ": " << path << std::endl;
    }

    xpc_dictionary_set_value(dictionary, "devices", devices);
    xpc_dictionary_set_bool(dictionary, "propagate", propagate);
    xpc_connection_send_message(port, dictionary);
    xpc_release(dictionary);
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

void AkVCam::IpcBridgePrivate::deviceUpdate(xpc_connection_t client,
                                            xpc_object_t event)
{
    UNUSED(client);
    UNUSED(event);
    AkLogFunction();
    std::vector<std::string> devices;
    auto nCameras = Preferences::camerasCount();

    for (size_t i = 0; i < nCameras; i++)
        devices.push_back(Preferences::cameraPath(i));

    for (auto bridge: this->m_bridges)
        AKVCAM_EMIT(bridge, DevicesChanged, devices)
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

bool AkVCam::IpcBridgePrivate::isRoot() const
{
    AkLogFunction();

    return getuid() == 0;
}

std::vector<std::string> AkVCam::IpcBridgePrivate::listServices() const
{
    std::vector<std::string> services;
    auto proc = popen("launchctl list", "r");

    if (proc) {
        while (!feof(proc)) {
            char line[1024];

            if (!fgets(line, 1024, proc))
                break;

            if (strncmp(line, "PID", 3) == 0)
                continue;

            char *pline = strtok(line, " \n\r\t");

            for (size_t i = 0; i < 3 && pline != nullptr; i++) {
                if (i == 2)
                    services.push_back(pline);

                pline = strtok(nullptr, " \n\r\t");
            }
        }

        pclose(proc);
    }

    return services;
}

std::vector<std::string> AkVCam::IpcBridgePrivate::listDisabledServices() const
{
    std::vector<std::string> services;
    auto proc = popen("launchctl print-disabled system/", "r");

    if (proc) {
        while (!feof(proc)) {
            char line[1024];

            if (!fgets(line, 1024, proc))
                break;

            if (strncmp(line, "\t", 1) != 0)
                continue;

            std::string service;
            char *pline = strtok(line, " ");

            for (size_t i = 0; i < 3 && pline; i++) {
                if (i == 0)
                    service = std::string(pline).substr(1, strlen(pline) - 2);

                if (i == 2 && strncmp(pline, "true", 4) == 0)
                    services.push_back(service);

                pline = strtok(nullptr, " ");
            }
        }

        pclose(proc);
    }

    return services;
}

bool AkVCam::IpcBridgePrivate::isServiceLoaded(const std::string &service) const
{
    auto services = this->listServices();

    return std::find(services.begin(), services.end(), service) != services.end();
}

bool AkVCam::IpcBridgePrivate::isServiceDisabled(const std::string &service) const
{
    auto services = this->listDisabledServices();

    return std::find(services.begin(), services.end(), service) != services.end();
}

bool AkVCam::IpcBridgePrivate::readEntitlements(const std::string &app,
                                                const std::string &output) const
{
    bool writen = false;
    std::string cmd = "codesign -d --entitlements - \"" + app + "\"";
    auto proc = popen(cmd.c_str(), "r");

    if (proc) {
        auto entitlements = fopen(output.c_str(), "w");

        if (entitlements) {
            for (size_t i = 0; !feof(proc); i++) {
                char data[1024];
                auto len = fread(data, 1, 1024, proc);

                if (len < 1)
                    break;

                size_t offset = 0;

                if (i == 0)
                    offset = std::string(data, len).find("<?xml");

                fwrite(data + offset, 1, len - offset, entitlements);
                writen = true;
            }

            fclose(entitlements);
        }

        pclose(proc);
    }

    return writen;
}

const std::vector<AkVCam::Hack> &AkVCam::IpcBridgePrivate::hacks()
{
    static const std::vector<AkVCam::Hack> hacks {
        {"set-service-up",
         "Setup and start virtual camera service if isn't working",
         true,
         true,
         AKVCAM_BIND_HACK_FUNC(IpcBridgePrivate::setServiceUp)},
        {"set-service-down",
         "Stop and unregister virtual camera service",
         true,
         true,
         AKVCAM_BIND_HACK_FUNC(IpcBridgePrivate::setServiceDown)},
        {"disable-library-validation",
         "Disable external plugins validation in app bundle",
         false,
         false,
         AKVCAM_BIND_HACK_FUNC(IpcBridgePrivate::disableLibraryValidation)},
        {"code-re-sign",
         "Remove app code signature and re-sign it with a developer signature",
         false,
         false,
         AKVCAM_BIND_HACK_FUNC(IpcBridgePrivate::codeResign)},
        {"unsign",
         "Remove app code signature",
         false,
         false,
         AKVCAM_BIND_HACK_FUNC(IpcBridgePrivate::unsign)},
        {"disable-sip",
         "Disable System Integrity Protection",
         false,
         false,
         AKVCAM_BIND_HACK_FUNC(IpcBridgePrivate::disableSIP)}
    };

    return hacks;
}

int AkVCam::IpcBridgePrivate::setServiceUp(const std::vector<std::string> &args)
{
    UNUSED(args);
    AkLogFunction();
    std::string pluginPath = this->locatePluginPath();
    static const std::string dstPluginPath =
            "/Library/CoreMediaIO/Plug-Ins/DAL/" CMIO_PLUGIN_NAME ".plugin";

    if (!fileExists(dstPluginPath))
        if (symlink(pluginPath.c_str(), dstPluginPath.c_str()) != 0) {
            std::cerr << strerror(errno) << std::endl;

            return -1;
        }

    static const std::string daemonPlist =
            "/Library/LaunchDaemons/" CMIO_ASSISTANT_NAME ".plist";

    if (!fileExists(daemonPlist)) {
        std::ofstream plist(daemonPlist);

        if (!plist.is_open())
            return -1;

        plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
        plist << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" ";
        plist << "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">" << std::endl;
        plist << "<plist version=\"1.0\">" << std::endl;
        plist << "    <dict>" << std::endl;
        plist << "        <key>Label</key>" << std::endl;
        plist << "        <string>" CMIO_ASSISTANT_NAME "</string>" << std::endl;
        plist << "        <key>ProgramArguments</key>" << std::endl;
        plist << "        <array>" << std::endl;
        plist << "            <string>";
        plist <<                  pluginPath;
        plist <<                  "/Contents/Resources/AkVCamAssistant";
        plist <<             "</string>" << std::endl;
        plist << "            <string>--timeout</string>" << std::endl;
        plist << "            <string>300.0</string>" << std::endl;
        plist << "        </array>" << std::endl;
        plist << "        <key>MachServices</key>" << std::endl;
        plist << "        <dict>" << std::endl;
        plist << "            <key>" CMIO_ASSISTANT_NAME "</key>" << std::endl;
        plist << "            <true/>" << std::endl;
        plist << "        </dict>" << std::endl;
        plist << "        <key>StandardOutPath</key>" << std::endl;
        plist << "        <string>/tmp/AkVCamAssistant.log</string>" << std::endl;
        plist << "        <key>StandardErrorPath</key>" << std::endl;
        plist << "        <string>/tmp/AkVCamAssistant.log</string>" << std::endl;
        plist << "    </dict>" << std::endl;
        plist << "</plist>" << std::endl;
        plist.close();
    }

    if (this->isServiceDisabled(CMIO_ASSISTANT_NAME)) {
        int result = system("launchctl enable system/" CMIO_ASSISTANT_NAME);

        if (result)
            return result;
    }

    if (!this->isServiceLoaded(CMIO_ASSISTANT_NAME)) {
        auto cmd = "launchctl bootstrap system " + daemonPlist;
        int result = system(cmd.c_str());

        if (result)
            return result;
    }

    return 0;
}

int AkVCam::IpcBridgePrivate::setServiceDown(const std::vector<std::string> &args)
{
    UNUSED(args);
    AkLogFunction();

    static const std::string daemonPlist =
            "/Library/LaunchDaemons/" CMIO_ASSISTANT_NAME ".plist";

    if (fileExists(daemonPlist)) {
        if (this->isServiceLoaded(CMIO_ASSISTANT_NAME)) {
            auto cmd = "launchctl bootout system " + daemonPlist;
            int result = system(cmd.c_str());

            if (result)
                return result;
        }

        std::remove(daemonPlist.c_str());
    }

    static const std::string dstPluginPath =
            "/Library/CoreMediaIO/Plug-Ins/DAL/" CMIO_PLUGIN_NAME ".plugin";

    if (this->fileExists(dstPluginPath))
        if (unlink(dstPluginPath.c_str()) != 0) {
            std::cerr << strerror(errno) << std::endl;

            return -1;
        }

    return 0;
}

int AkVCam::IpcBridgePrivate::disableLibraryValidation(const std::vector<std::string> &args)
{
    if (args.size() < 1) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    if (!this->fileExists(args[0])) {
        std::cerr << "No such file or directory." << std::endl;

        return -1;
    }

    static const std::string entitlementsXml = "/tmp/entitlements.xml";
    static const std::string dlv =
            "com.apple.security.cs.disable-library-validation";

    if (this->readEntitlements(args[0], entitlementsXml)) {
        auto nsdlv = [NSString
                      stringWithCString: dlv.c_str()
                      encoding: [NSString defaultCStringEncoding]];
        auto nsEntitlementsXml = [NSString
                                  stringWithCString: entitlementsXml.c_str()
                                  encoding: [NSString defaultCStringEncoding]];
        auto nsEntitlementsXmlUrl = [NSURL
                                    fileURLWithPath: nsEntitlementsXml];
        NSError *error = nil;
        auto entitlements =
                [[NSXMLDocument alloc]
                 initWithContentsOfURL: nsEntitlementsXmlUrl
                 options: 0
                 error: &error];
        [nsEntitlementsXmlUrl release];
        const std::string xpath =
                "/plist/dict/key[contains(text(), \"" + dlv + "\")]";
        auto nsxpath = [NSString
                        stringWithCString: xpath.c_str()
                        encoding: [NSString defaultCStringEncoding]];
        auto nodes = [entitlements nodesForXPath: nsxpath error: &error];
        [nsxpath release];

        if ([nodes count] < 1) {
            auto key = [[NSXMLElement alloc] initWithName: @"key" stringValue: nsdlv];
            [(NSXMLElement *) entitlements.rootElement.children[0] addChild: key];
            [key release];
            auto value = [[NSXMLElement alloc] initWithName: @"true"];
            [(NSXMLElement *) entitlements.rootElement.children[0] addChild: value];
            [value release];
        } else {
            for (NSXMLNode *node: nodes) {
                auto value = [[NSXMLElement alloc] initWithName: @"true"];
                [(NSXMLElement *) node.parent replaceChildAtIndex: node.nextSibling.index withNode: value];
                [value release];
            }
        }

        auto data = [entitlements XMLDataWithOptions: NSXMLNodePrettyPrint
                                                    | NSXMLNodeCompactEmptyElement];
        [data writeToFile: nsEntitlementsXml atomically: YES];
        [data release];
        [entitlements release];
        [nsEntitlementsXml release];
        [nsdlv release];
    } else {
        std::ofstream entitlements(entitlementsXml);

        if (entitlements.is_open()) {
            entitlements << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
            entitlements << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">" << std::endl;
            entitlements << "<plist version=\"1.0\">" << std::endl;
            entitlements << "\t<dict>" << std::endl;
            entitlements << "\t\t<key>" << dlv << "</key>" << std::endl;
            entitlements << "\t\t<true/>" << std::endl;
            entitlements << "\t</dict>" << std::endl;
            entitlements << "</plist>" << std::endl;
            entitlements.close();
        }
    }

    auto cmd = "codesign --entitlements \""
               + entitlementsXml
               + "\" -f -s - \""
               + args[0]
               + "\"";
    int result = system(cmd.c_str());
    std::remove(entitlementsXml.c_str());

    return result;
}

int AkVCam::IpcBridgePrivate::codeResign(const std::vector<std::string> &args)
{
    if (args.size() < 1) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    if (!this->fileExists(args[0])) {
        std::cerr << "No such file or directory." << std::endl;

        return -1;
    }

    static const std::string entitlementsXml = "/tmp/entitlements.xml";
    std::string cmd;

    if (this->readEntitlements(args[0], entitlementsXml))
        cmd = "codesign --entitlements \""
                       + entitlementsXml
                       + "\" -f -s - \""
                       + args[0]
                       + "\"";
    else
        cmd = "codesign -f -s - \"" + args[0] + "\"";

    int result = system(cmd.c_str());
    std::remove(entitlementsXml.c_str());

    return result;
}

int AkVCam::IpcBridgePrivate::unsign(const std::vector<std::string> &args)
{
    if (args.size() < 1) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    if (!this->fileExists(args[0])) {
        std::cerr << "No such file or directory." << std::endl;

        return -1;
    }

    auto cmd = "codesign --remove-signature \"" + args[0] + "\"";

    return system(cmd.c_str());
}

int AkVCam::IpcBridgePrivate::disableSIP(const std::vector<std::string> &args)
{
    std::cerr << "SIP (System Integrity Protection) can't be disbled from "
                 "inside the system, you must reboot your system and then "
                 "press and hold Command + R keys on boot to enter to the "
                 "recovery mode, then go to Utilities > Terminal and run:"
              << std::endl;
    std::cerr << std::endl;
    std::cerr << "csrutil enable --without fs" << std::endl;
    std::cerr << std::endl;
    std::cerr << "If that does not works, then run:" << std::endl;
    std::cerr << std::endl;
    std::cerr << "csrutil disable" << std::endl;
    std::cerr << std::endl;

    return -1;
}

AkVCam::Hack::Hack()
{

}

AkVCam::Hack::Hack(const std::string &name,
                   const std::string &description,
                   bool isSafe,
                   bool needsRoot,
                   const Hack::HackFunc &func):
    name(name),
    description(description),
    isSafe(isSafe),
    needsRoot(needsRoot),
    func(func)
{

}

AkVCam::Hack::Hack(const Hack &other):
    name(other.name),
    description(other.description),
    isSafe(other.isSafe),
    needsRoot(other.needsRoot),
    func(other.func)
{
}

AkVCam::Hack &AkVCam::Hack::operator =(const Hack &other)
{
    if (this != &other) {
        this->name = other.name;
        this->description = other.description;
        this->isSafe = other.isSafe;
        this->needsRoot = other.needsRoot;
        this->func = other.func;
    }

    return *this;
}
