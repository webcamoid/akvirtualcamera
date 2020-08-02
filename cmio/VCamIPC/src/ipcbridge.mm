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

#include <fstream>
#include <map>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <dirent.h>
#include <Foundation/Foundation.h>
#include <IOSurface/IOSurface.h>
#include <CoreMedia/CMFormatDescription.h>
#include <xpc/connection.h>
#include <libproc.h>

#include "Assistant/src/assistantglobals.h"
#include "PlatformUtils/src/preferences.h"
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
            std::map<std::string, std::string> m_options;
            std::wstring m_error;

            IpcBridgePrivate(IpcBridge *self=nullptr);
            ~IpcBridgePrivate();

            static inline std::vector<std::wstring> *driverPaths();
            inline void add(IpcBridge *bridge);
            void remove(IpcBridge *bridge);
            inline std::vector<IpcBridge *> &bridges();

            // Message handling methods
            void isAlive(xpc_connection_t client, xpc_object_t event);
            void deviceCreate(xpc_connection_t client, xpc_object_t event);
            void deviceDestroy(xpc_connection_t client, xpc_object_t event);
            void deviceUpdate(xpc_connection_t client, xpc_object_t event);
            void frameReady(xpc_connection_t client, xpc_object_t event);
            void setBroadcasting(xpc_connection_t client,
                                     xpc_object_t event);
            void setMirror(xpc_connection_t client, xpc_object_t event);
            void setScaling(xpc_connection_t client, xpc_object_t event);
            void setAspectRatio(xpc_connection_t client, xpc_object_t event);
            void setSwapRgb(xpc_connection_t client, xpc_object_t event);
            void listenerAdd(xpc_connection_t client, xpc_object_t event);
            void listenerRemove(xpc_connection_t client, xpc_object_t event);
            void messageReceived(xpc_connection_t client, xpc_object_t event);
            void connectionInterrupted();

            // Utility methods
            std::string homePath() const;
            bool fileExists(const std::wstring &path) const;
            bool fileExists(const std::string &path) const;
            std::wstring fileName(const std::wstring &path) const;
            bool mkpath(const std::string &path) const;
            bool rm(const std::string &path) const;
            std::wstring locateDriverPath() const;

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
    ipcBridgePrivate().add(this);
    this->registerPeer();
}

AkVCam::IpcBridge::~IpcBridge()
{
    this->unregisterPeer();
    ipcBridgePrivate().remove(this);
    delete this->d;
}

std::wstring AkVCam::IpcBridge::errorMessage() const
{
    return this->d->m_error;
}

void AkVCam::IpcBridge::setOption(const std::string &key,
                                  const std::string &value)
{
    AkLogFunction();

    if (value.empty())
        this->d->m_options.erase(key);
    else
        this->d->m_options[key] = value;
}

std::vector<std::wstring> AkVCam::IpcBridge::driverPaths() const
{
    AkLogFunction();

    return *this->d->driverPaths();
}

void AkVCam::IpcBridge::setDriverPaths(const std::vector<std::wstring> &driverPaths)
{
    AkLogFunction();
    *this->d->driverPaths() = driverPaths;
}

std::vector<std::string> AkVCam::IpcBridge::availableDrivers() const
{
    return {"AkVirtualCamera"};
}

std::string AkVCam::IpcBridge::driver() const
{
    return {"AkVirtualCamera"};
}

bool AkVCam::IpcBridge::setDriver(const std::string &driver)
{
    return driver == "AkVirtualCamera";
}

std::vector<std::string> AkVCam::IpcBridge::availableRootMethods() const
{
    return {"osascript"};
}

std::string AkVCam::IpcBridge::rootMethod() const
{
    return {"osascript"};
}

bool AkVCam::IpcBridge::setRootMethod(const std::string &rootMethod)
{
    return rootMethod == "osascript";
}

void AkVCam::IpcBridge::connectService()
{
    AkLogFunction();
    this->registerPeer();
}

void AkVCam::IpcBridge::disconnectService()
{
    AkLogFunction();
    this->unregisterPeer();
}

bool AkVCam::IpcBridge::registerPeer()
{
    AkLogFunction();

    std::string plistFile =
            CMIO_DAEMONS_PATH "/" CMIO_ASSISTANT_NAME ".plist";

    auto daemon = replace(plistFile, "~", this->d->homePath());

    if (!this->d->fileExists(daemon))
        return false;

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

    return Preferences::cameraDescription(cameraIndex);
}

void AkVCam::IpcBridge::setDescription(const std::string &deviceId,
                                       const std::wstring &description)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    return Preferences::cameraSetDescription(cameraIndex, description);
}

std::vector<AkVCam::PixelFormat> AkVCam::IpcBridge::supportedPixelFormats(DeviceType type) const
{
    if (type == DeviceTypeInput)
        return {PixelFormatRGB24};

    return {
        PixelFormatRGB32,
        PixelFormatRGB24,
        PixelFormatUYVY,
        PixelFormatYUY2
    };
}

AkVCam::PixelFormat AkVCam::IpcBridge::defaultPixelFormat(DeviceType type) const
{
    return type == DeviceTypeInput?
                PixelFormatRGB24:
                PixelFormatYUY2;
}

std::vector<AkVCam::VideoFormat> AkVCam::IpcBridge::formats(const std::string &deviceId) const
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    return Preferences::cameraFormats(cameraIndex);
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

bool AkVCam::IpcBridge::isHorizontalMirrored(const std::string &deviceId)
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return false;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return false;
    }

    bool horizontalMirror = xpc_dictionary_get_bool(reply, "hmirror");
    xpc_release(reply);

    return horizontalMirror;
}

bool AkVCam::IpcBridge::isVerticalMirrored(const std::string &deviceId)
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return false;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return false;
    }

    bool verticalMirror = xpc_dictionary_get_bool(reply, "vmirror");
    xpc_release(reply);

    return verticalMirror;
}

AkVCam::Scaling AkVCam::IpcBridge::scalingMode(const std::string &deviceId)
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return ScalingFast;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SCALING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return ScalingFast;
    }

    auto scaling = Scaling(xpc_dictionary_get_int64(reply, "scaling"));
    xpc_release(reply);

    return scaling;
}

AkVCam::AspectRatio AkVCam::IpcBridge::aspectRatioMode(const std::string &deviceId)
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return AspectRatioIgnore;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_ASPECTRATIO);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return AspectRatioIgnore;
    }

    auto aspectRatio = AspectRatio(xpc_dictionary_get_int64(reply, "aspect"));
    xpc_release(reply);

    return aspectRatio;
}

bool AkVCam::IpcBridge::swapRgb(const std::string &deviceId)
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return false;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SWAPRGB);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    auto replyType = xpc_get_type(reply);

    if (replyType != XPC_TYPE_DICTIONARY) {
        xpc_release(reply);

        return false;
    }

    auto swap = xpc_dictionary_get_bool(reply, "swap");
    xpc_release(reply);

    return swap;
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
    auto driverPath = this->d->locateDriverPath();

    if (driverPath.empty())
        return {};

    auto plugin = this->d->fileName(driverPath);
    std::wstring pluginPath =
            CMIO_PLUGINS_DAL_PATH_L L"/"
            + plugin
            + L"/Contents/MacOS/" CMIO_PLUGIN_NAME_L;
    std::string path(pluginPath.begin(), pluginPath.end());
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

AkVCam::IpcBridge::DeviceType AkVCam::IpcBridge::deviceType(const std::string &deviceId)
{
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    return Preferences::cameraIsInput(cameraIndex)?
                DeviceTypeInput:
                DeviceTypeOutput;
}

std::string AkVCam::IpcBridge::addDevice(const std::wstring &description,
                                         DeviceType type)
{
    return Preferences::addDevice(description, type);
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
    Preferences::cameraAddFormat(Preferences::cameraFromPath(deviceId),
                                 format,
                                 index);
}

void AkVCam::IpcBridge::removeFormat(const std::string &deviceId, int index)
{
    AkLogFunction();
    Preferences::cameraRemoveFormat(Preferences::cameraFromPath(deviceId),
                                    index);
}

void AkVCam::IpcBridge::update()
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

std::vector<std::string> AkVCam::IpcBridge::connections(const std::string &deviceId)
{
    return Preferences::cameraConnections(Preferences::cameraFromPath(deviceId));
}

void AkVCam::IpcBridge::setConnections(const std::string &deviceId,
                                       const std::vector<std::string> &connectedDevices)
{
    Preferences::cameraSetConnections(Preferences::cameraFromPath(deviceId),
                                      connectedDevices);
}

void AkVCam::IpcBridge::updateDevices()
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_UPDATE);
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

void AkVCam::IpcBridge::setMirroring(const std::string &deviceId,
                                     bool horizontalMirrored,
                                     bool verticalMirrored)
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETMIRRORING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_bool(dictionary, "hmirror", horizontalMirrored);
    xpc_dictionary_set_bool(dictionary, "vmirror", verticalMirrored);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);
}

void AkVCam::IpcBridge::setScaling(const std::string &deviceId,
                                   Scaling scaling)
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETSCALING);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_int64(dictionary, "scaling", scaling);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);
}

void AkVCam::IpcBridge::setAspectRatio(const std::string &deviceId,
                                       AspectRatio aspectRatio)
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETASPECTRATIO);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_int64(dictionary, "aspect", aspectRatio);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);
}

void AkVCam::IpcBridge::setSwapRgb(const std::string &deviceId, bool swap)
{
    AkLogFunction();

    if (!this->d->m_serverMessagePort)
        return;

    auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETSWAPRGB);
    xpc_dictionary_set_string(dictionary, "device", deviceId.c_str());
    xpc_dictionary_set_bool(dictionary, "swap", swap);
    auto reply = xpc_connection_send_message_with_reply_sync(this->d->m_serverMessagePort,
                                                             dictionary);
    xpc_release(dictionary);
    xpc_release(reply);
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
        {AKVCAM_ASSISTANT_MSG_ISALIVE               , AKVCAM_BIND_FUNC(IpcBridgePrivate::isAlive)        },
        {AKVCAM_ASSISTANT_MSG_FRAME_READY           , AKVCAM_BIND_FUNC(IpcBridgePrivate::frameReady)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_CREATE         , AKVCAM_BIND_FUNC(IpcBridgePrivate::deviceCreate)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY        , AKVCAM_BIND_FUNC(IpcBridgePrivate::deviceDestroy)  },
        {AKVCAM_ASSISTANT_MSG_DEVICE_UPDATE         , AKVCAM_BIND_FUNC(IpcBridgePrivate::deviceUpdate)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD   , AKVCAM_BIND_FUNC(IpcBridgePrivate::listenerAdd)    },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE, AKVCAM_BIND_FUNC(IpcBridgePrivate::listenerRemove) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING, AKVCAM_BIND_FUNC(IpcBridgePrivate::setBroadcasting)},
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETMIRRORING   , AKVCAM_BIND_FUNC(IpcBridgePrivate::setMirror)      },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETSCALING     , AKVCAM_BIND_FUNC(IpcBridgePrivate::setScaling)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETASPECTRATIO , AKVCAM_BIND_FUNC(IpcBridgePrivate::setAspectRatio) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETSWAPRGB     , AKVCAM_BIND_FUNC(IpcBridgePrivate::setSwapRgb)     },
    };
}

AkVCam::IpcBridgePrivate::~IpcBridgePrivate()
{

}

std::vector<std::wstring> *AkVCam::IpcBridgePrivate::driverPaths()
{
    static std::vector<std::wstring> paths;

    return &paths;
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

void AkVCam::IpcBridgePrivate::setMirror(xpc_connection_t client,
                                         xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();

    std::string deviceId =
            xpc_dictionary_get_string(event, "device");
    bool horizontalMirror =
            xpc_dictionary_get_bool(event, "hmirror");
    bool verticalMirror =
            xpc_dictionary_get_bool(event, "vmirror");

    for (auto bridge: this->m_bridges)
        AKVCAM_EMIT(bridge,
                    MirrorChanged,
                    deviceId,
                    horizontalMirror,
                    verticalMirror)
}

void AkVCam::IpcBridgePrivate::setScaling(xpc_connection_t client,
                                          xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();

    std::string deviceId =
            xpc_dictionary_get_string(event, "device");
    auto scaling =
            Scaling(xpc_dictionary_get_int64(event, "scaling"));

    for (auto bridge: this->m_bridges)
        AKVCAM_EMIT(bridge, ScalingChanged, deviceId, scaling)
}

void AkVCam::IpcBridgePrivate::setAspectRatio(xpc_connection_t client,
                                              xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();

    std::string deviceId =
            xpc_dictionary_get_string(event, "device");
    auto aspectRatio =
            AspectRatio(xpc_dictionary_get_int64(event, "aspect"));

    for (auto bridge: this->m_bridges)
        AKVCAM_EMIT(bridge, AspectRatioChanged, deviceId, aspectRatio)
}

void AkVCam::IpcBridgePrivate::setSwapRgb(xpc_connection_t client,
                                          xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();

    std::string deviceId =
            xpc_dictionary_get_string(event, "device");
    auto swap = xpc_dictionary_get_bool(event, "swap");

    for (auto bridge: this->m_bridges)
        AKVCAM_EMIT(bridge, SwapRgbChanged, deviceId, swap)
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

std::string AkVCam::IpcBridgePrivate::homePath() const
{
    auto homePath = NSHomeDirectory();

    if (!homePath)
        return {};

    return std::string(homePath.UTF8String);
}

bool AkVCam::IpcBridgePrivate::fileExists(const std::wstring &path) const
{
    return this->fileExists(std::string(path.begin(), path.end()));
}

bool AkVCam::IpcBridgePrivate::fileExists(const std::string &path) const
{
    struct stat stats;
    memset(&stats, 0, sizeof(struct stat));

    return stat(path.c_str(), &stats) == 0;
}

std::wstring AkVCam::IpcBridgePrivate::fileName(const std::wstring &path) const
{
    return path.substr(path.rfind(L'/') + 1);
}

bool AkVCam::IpcBridgePrivate::mkpath(const std::string &path) const
{
    if (path.empty())
        return false;

    if (this->fileExists(path))
        return true;

    // Create parent folders
    for (auto pos = path.find('/');
         pos != std::string::npos;
         pos = path.find('/', pos + 1)) {
        auto path_ = path.substr(0, pos);

        if (path_.empty() || this->fileExists(path_))
            continue;

        if (mkdir(path_.c_str(),
                  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
            return false;
    }

    return !mkdir(path.c_str(),
                  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}

bool AkVCam::IpcBridgePrivate::rm(const std::string &path) const
{
    if (path.empty())
        return false;

    struct stat stats;
    memset(&stats, 0, sizeof(struct stat));

    if (stat(path.c_str(), &stats))
        return false;

    bool ok = true;

    if (S_ISDIR(stats.st_mode)) {
        auto dir = opendir(path.c_str());

        while (auto entry = readdir(dir))
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
                this->rm(entry->d_name);

        closedir(dir);

        ok &= !rmdir(path.c_str());
    } else {
        ok &= !::remove(path.c_str());
    }

    return ok;
}

std::wstring AkVCam::IpcBridgePrivate::locateDriverPath() const
{
    AkLogFunction();
    std::wstring driverPath;

    for (auto it = this->driverPaths()->rbegin();
         it != this->driverPaths()->rend();
         it++) {
        auto path = *it;
        path = replace(path, L"\\", L"/");

        if (path.back() != L'/')
            path += L'/';

        path += CMIO_PLUGIN_NAME_L L".plugin";

        if (!this->fileExists(path + L"/Contents/MacOS/" CMIO_PLUGIN_NAME_L))
            continue;

        if (!this->fileExists(path + L"/Contents/Resources/" CMIO_PLUGIN_ASSISTANT_NAME_L))
            continue;

        driverPath = path;

        break;
    }

    return driverPath;
}
