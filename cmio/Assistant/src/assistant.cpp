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
#include <map>
#include <sstream>
#include <CoreFoundation/CoreFoundation.h>
#include <xpc/xpc.h>
#include <xpc/connection.h>

#include "assistant.h"
#include "assistantglobals.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/logger.h"

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1, std::placeholders::_2)

#define PREFERENCES_ID CFSTR(CMIO_ASSISTANT_NAME)

namespace AkVCam
{
    struct AssistantDevice
    {
        std::string broadcaster;
        std::vector<std::string> listeners;
    };

    using AssistantPeers = std::map<std::string, xpc_connection_t>;
    using DeviceConfigs = std::map<std::string, AssistantDevice>;

    class AssistantPrivate
    {
        public:
            AssistantPeers m_peers;
            DeviceConfigs m_deviceConfigs;
            std::map<int64_t, XpcMessage> m_messageHandlers;
            CFRunLoopTimerRef m_timer {nullptr};
            double m_timeout {0.0};

            AssistantPrivate();
            inline static uint64_t id();
            bool startTimer();
            void stopTimer();
            static void timerTimeout(CFRunLoopTimerRef timer, void *info);
            void peerDied();
            void removePortByName(const std::string &portName);
            void releaseDevicesFromPeer(const std::string &portName);
            void requestPort(xpc_connection_t client, xpc_object_t event);
            void addPort(xpc_connection_t client, xpc_object_t event);
            void removePort(xpc_connection_t client, xpc_object_t event);
            void devicesUpdate(xpc_connection_t client, xpc_object_t event);
            void setBroadcasting(xpc_connection_t client, xpc_object_t event);
            void frameReady(xpc_connection_t client, xpc_object_t event);
            void pictureUpdated(xpc_connection_t client, xpc_object_t event);
            void listeners(xpc_connection_t client, xpc_object_t event);
            void listener(xpc_connection_t client, xpc_object_t event);
            void broadcasting(xpc_connection_t client, xpc_object_t event);
            void listenerAdd(xpc_connection_t client, xpc_object_t event);
            void listenerRemove(xpc_connection_t client, xpc_object_t event);
            void controlsUpdated(xpc_connection_t client, xpc_object_t event);
    };
}

AkVCam::Assistant::Assistant()
{
    this->d = new AssistantPrivate;
}

AkVCam::Assistant::~Assistant()
{
    delete this->d;
}

void AkVCam::Assistant::setTimeout(double timeout)
{
    this->d->m_timeout = timeout;
}

void AkVCam::Assistant::messageReceived(xpc_connection_t client,
                                        xpc_object_t event)
{
    AkLogFunction();
    auto type = xpc_get_type(event);

    if (type == XPC_TYPE_ERROR) {
        if (event == XPC_ERROR_CONNECTION_INVALID) {
            this->d->peerDied();
        } else {
            auto description = xpc_copy_description(event);
            AkLogError() << description << std::endl;
            free(description);
        }
    } else if (type == XPC_TYPE_DICTIONARY) {
        int64_t message = xpc_dictionary_get_int64(event, "message");

        if (this->d->m_messageHandlers.count(message))
            this->d->m_messageHandlers[message](client, event);
    }
}

AkVCam::AssistantPrivate::AssistantPrivate()
{
    this->m_messageHandlers = {
        {AKVCAM_ASSISTANT_MSG_FRAME_READY            , AKVCAM_BIND_FUNC(AssistantPrivate::frameReady)     },
        {AKVCAM_ASSISTANT_MSG_PICTURE_UPDATED        , AKVCAM_BIND_FUNC(AssistantPrivate::pictureUpdated) },
        {AKVCAM_ASSISTANT_MSG_REQUEST_PORT           , AKVCAM_BIND_FUNC(AssistantPrivate::requestPort)    },
        {AKVCAM_ASSISTANT_MSG_ADD_PORT               , AKVCAM_BIND_FUNC(AssistantPrivate::addPort)        },
        {AKVCAM_ASSISTANT_MSG_REMOVE_PORT            , AKVCAM_BIND_FUNC(AssistantPrivate::removePort)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_UPDATE          , AKVCAM_BIND_FUNC(AssistantPrivate::devicesUpdate)  },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD    , AKVCAM_BIND_FUNC(AssistantPrivate::listenerAdd)    },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE , AKVCAM_BIND_FUNC(AssistantPrivate::listenerRemove) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENERS       , AKVCAM_BIND_FUNC(AssistantPrivate::listeners)      },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER        , AKVCAM_BIND_FUNC(AssistantPrivate::listener)       },
        {AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING    , AKVCAM_BIND_FUNC(AssistantPrivate::broadcasting)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING , AKVCAM_BIND_FUNC(AssistantPrivate::setBroadcasting)},
        {AKVCAM_ASSISTANT_MSG_DEVICE_CONTROLS_UPDATED, AKVCAM_BIND_FUNC(AssistantPrivate::controlsUpdated)},
    };

    this->startTimer();
}

uint64_t AkVCam::AssistantPrivate::id()
{
    static uint64_t id = 0;

    return id++;
}

bool AkVCam::AssistantPrivate::startTimer()
{
    AkLogFunction();

    if (this->m_timer || this->m_timeout <= 0.0)
        return false;

    // If no peer has been connected for 5 minutes shutdown the assistant.
    CFRunLoopTimerContext context {0, this, nullptr, nullptr, nullptr};
    this->m_timer =
            CFRunLoopTimerCreate(kCFAllocatorDefault,
                                 CFAbsoluteTimeGetCurrent() + this->m_timeout,
                                 0,
                                 0,
                                 0,
                                 AssistantPrivate::timerTimeout,
                                 &context);

    if (!this->m_timer)
        return false;

    CFRunLoopAddTimer(CFRunLoopGetMain(),
                      this->m_timer,
                      kCFRunLoopCommonModes);

    return true;
}

void AkVCam::AssistantPrivate::stopTimer()
{
    AkLogFunction();

    if (!this->m_timer)
        return;

    CFRunLoopTimerInvalidate(this->m_timer);
    CFRunLoopRemoveTimer(CFRunLoopGetMain(),
                         this->m_timer,
                         kCFRunLoopCommonModes);
    CFRelease(this->m_timer);
    this->m_timer = nullptr;
}

void AkVCam::AssistantPrivate::timerTimeout(CFRunLoopTimerRef timer, void *info)
{
    UNUSED(timer);
    UNUSED(info);
    AkLogFunction();

    CFRunLoopStop(CFRunLoopGetMain());
}

void AkVCam::AssistantPrivate::peerDied()
{
    AkLogFunction();
    std::vector<std::string> deadPeers;

    for (auto &peer: this->m_peers) {
        auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
        xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_ISALIVE);
        auto reply = xpc_connection_send_message_with_reply_sync(peer.second,
                                                                 dictionary);
        xpc_release(dictionary);
        auto replyType = xpc_get_type(reply);
        bool alive = false;

        if (replyType == XPC_TYPE_DICTIONARY)
            alive = xpc_dictionary_get_bool(reply, "alive");

        xpc_release(reply);

        if (!alive)
            deadPeers.push_back(peer.first);
    }

    for (auto &peer: deadPeers)
        this->removePortByName(peer);
}

void AkVCam::AssistantPrivate::removePortByName(const std::string &portName)
{
    AkLogFunction();
    AkLogInfo() << "Port: " << portName << std::endl;

    for (auto &peer: this->m_peers)
        if (peer.first == portName) {
            xpc_release(peer.second);
            this->m_peers.erase(portName);

            break;
        }

    if (this->m_peers.empty())
        this->startTimer();

    this->releaseDevicesFromPeer(portName);
}

void AkVCam::AssistantPrivate::releaseDevicesFromPeer(const std::string &portName)
{
    AkLogFunction();

    for (auto &config: this->m_deviceConfigs)
        if (config.second.broadcaster == portName) {
            config.second.broadcaster.clear();

            auto dictionary = xpc_dictionary_create(nullptr, nullptr, 0);
            xpc_dictionary_set_int64(dictionary, "message", AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING);
            xpc_dictionary_set_string(dictionary, "device", config.first.c_str());
            xpc_dictionary_set_string(dictionary, "broadcaster", "");

            for (auto &peer: this->m_peers)
                xpc_connection_send_message(peer.second, dictionary);

            xpc_release(dictionary);
        } else {
            auto it = std::find(config.second.listeners.begin(),
                                config.second.listeners.end(),
                                portName);

            if (it != config.second.listeners.end())
                config.second.listeners.erase(it);
        }
}

void AkVCam::AssistantPrivate::requestPort(xpc_connection_t client,
                                           xpc_object_t event)
{
    AkLogFunction();
    auto portName = AKVCAM_ASSISTANT_CLIENT_NAME
                  + std::to_string(this->id());

    AkLogInfo() << "Returning Port: " << portName << std::endl;

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_string(reply, "port", portName.c_str());
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::addPort(xpc_connection_t client,
                                       xpc_object_t event)
{
    AkLogFunction();

    std::string portName = xpc_dictionary_get_string(event, "port");
    auto endpoint = xpc_dictionary_get_value(event, "connection");
    auto connection = xpc_connection_create_from_endpoint(reinterpret_cast<xpc_endpoint_t>(endpoint));
    xpc_connection_set_event_handler(connection, ^(xpc_object_t) {});
    xpc_connection_resume(connection);
    bool ok = true;

    for (auto &peer: this->m_peers)
        if (peer.first == portName) {
            ok = false;

            break;
        }

    if (ok) {
        AkLogInfo() << "Adding Peer: " << portName << std::endl;
        this->m_peers[portName] = connection;
        this->stopTimer();
    }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::removePort(xpc_connection_t client,
                                          xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();

    this->removePortByName(xpc_dictionary_get_string(event, "port"));
}

void AkVCam::AssistantPrivate::devicesUpdate(xpc_connection_t client,
                                             xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();

    auto devicesList = xpc_dictionary_get_array(event, "devices");
    DeviceConfigs configs;

    for (size_t i = 0; i < xpc_array_get_count(devicesList); i++) {
        std::string device = xpc_array_get_string(devicesList, i);

        if (this->m_deviceConfigs.count(device) > 0)
            configs[device] = this->m_deviceConfigs[device];
        else
            configs[device] = {};
    }

    this->m_deviceConfigs = configs;

    if (xpc_dictionary_get_bool(event, "propagate")) {
        auto notification = xpc_copy(event);

        for (auto &peer: this->m_peers)
            xpc_connection_send_message(peer.second, notification);

        xpc_release(notification);
    }
}

void AkVCam::AssistantPrivate::setBroadcasting(xpc_connection_t client,
                                               xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::string broadcaster = xpc_dictionary_get_string(event, "broadcaster");

    if (this->m_deviceConfigs.count(deviceId) > 0)
        if (this->m_deviceConfigs[deviceId].broadcaster != broadcaster) {
            AkLogInfo() << "Device: " << deviceId << std::endl;
            AkLogInfo() << "Broadcaster: " << broadcaster << std::endl;
            this->m_deviceConfigs[deviceId].broadcaster = broadcaster;
            auto notification = xpc_copy(event);

            for (auto &peer: this->m_peers)
                xpc_connection_send_message(peer.second, notification);

            xpc_release(notification);
        }
}

void AkVCam::AssistantPrivate::frameReady(xpc_connection_t client,
                                          xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();

    auto reply = xpc_dictionary_create_reply(event);
    bool ok = true;

    for (auto &peer: this->m_peers) {
        AkLogDebug() << "Sending frame to " << peer.first << std::endl;
        auto reply = xpc_connection_send_message_with_reply_sync(peer.second,
                                                                 event);
        auto replyType = xpc_get_type(reply);
        bool isOk = false;

        if (replyType == XPC_TYPE_DICTIONARY)
            isOk = xpc_dictionary_get_bool(reply, "status");

        ok &= isOk;

        if (!isOk)
            AkLogError() << "Failed sending frame." << std::endl;

        xpc_release(reply);
    }

    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::pictureUpdated(xpc_connection_t client,
                                              xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();
    auto notification = xpc_copy(event);

    for (auto &peer: this->m_peers)
        xpc_connection_send_message(peer.second, notification);

    xpc_release(notification);
}

void AkVCam::AssistantPrivate::listeners(xpc_connection_t client,
                                         xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    auto listeners = xpc_array_create(nullptr, 0);

    if (this->m_deviceConfigs.count(deviceId) > 0)
        for (auto &listener: this->m_deviceConfigs[deviceId].listeners) {
            auto listenerObj = xpc_string_create(listener.c_str());
            xpc_array_append_value(listeners, listenerObj);
        }

    AkLogInfo() << "Device: " << deviceId << std::endl;
    AkLogInfo() << "Listeners: " << xpc_array_get_count(listeners) << std::endl;
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_value(reply, "listeners", listeners);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::listener(xpc_connection_t client,
                                        xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    auto index = xpc_dictionary_get_uint64(event, "index");
    std::string listener;
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        if (index < this->m_deviceConfigs[deviceId].listeners.size()) {
            listener = this->m_deviceConfigs[deviceId].listeners[index];
            ok = true;
        }

    AkLogInfo() << "Device: " << deviceId << std::endl;
    AkLogInfo() << "Listener: " << listener << std::endl;
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_string(reply, "listener", listener.c_str());
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::broadcasting(xpc_connection_t client,
                                            xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::string broadcaster;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        broadcaster = this->m_deviceConfigs[deviceId].broadcaster;

    AkLogInfo() << "Device: " << deviceId << std::endl;
    AkLogInfo() << "Broadcaster: " << broadcaster << std::endl;
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_string(reply, "broadcaster", broadcaster.c_str());
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::listenerAdd(xpc_connection_t client,
                                           xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::string listener = xpc_dictionary_get_string(event, "listener");
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0) {
        auto &listeners = this->m_deviceConfigs[deviceId].listeners;
        auto it = std::find(listeners.begin(), listeners.end(), listener);

        if (it == listeners.end()) {
            listeners.push_back(listener);
            auto notification = xpc_copy(event);

            for (auto &peer: this->m_peers)
                xpc_connection_send_message(peer.second, notification);

            xpc_release(notification);
            ok = true;
        }
    }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::listenerRemove(xpc_connection_t client,
                                              xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::string listener = xpc_dictionary_get_string(event, "listener");
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0) {
        auto &listeners = this->m_deviceConfigs[deviceId].listeners;
        auto it = std::find(listeners.begin(), listeners.end(), listener);

        if (it != listeners.end()) {
            listeners.erase(it);
            auto notification = xpc_copy(event);

            for (auto &peer: this->m_peers)
                xpc_connection_send_message(peer.second, notification);

            xpc_release(notification);
            ok = true;
        }
    }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::controlsUpdated(xpc_connection_t client,
                                               xpc_object_t event)
{
    UNUSED(client);
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");

    if (this->m_deviceConfigs.count(deviceId) < 1) {
        AkLogError() << "'"
                     << deviceId
                     << "' device in not in the devices list."
                     << std::endl;

        return;
    }

    auto notification = xpc_copy(event);

    for (auto &peer: this->m_peers)
        xpc_connection_send_message(peer.second, notification);

    xpc_release(notification);
}
