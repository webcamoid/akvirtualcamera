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

#include <map>
#include <sstream>
#include <CoreFoundation/CoreFoundation.h>
#include <xpc/xpc.h>
#include <xpc/connection.h>

#include "assistant.h"
#include "assistantglobals.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/image/videoformat.h"
#include "VCamUtils/src/image/videoframe.h"
#include "VCamUtils/src/logger/logger.h"

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1, std::placeholders::_2)

#define PREFERENCES_ID CFSTR(CMIO_ASSISTANT_NAME)

namespace AkVCam
{
    struct AssistantDevice
    {
        std::wstring description;
        std::vector<VideoFormat> formats;
        std::string broadcaster;
        std::vector<std::string> listeners;
        bool horizontalMirror {false};
        bool verticalMirror {false};
        Scaling scaling {ScalingFast};
        AspectRatio aspectRatio {AspectRatioIgnore};
        bool swapRgb {false};
    };

    using AssistantPeers = std::map<std::string, xpc_connection_t>;
    using DeviceConfigs = std::map<std::string, AssistantDevice>;

    class AssistantPrivate
    {
        public:
            AssistantPeers m_servers;
            AssistantPeers m_clients;
            DeviceConfigs m_deviceConfigs;
            std::map<int64_t, XpcMessage> m_messageHandlers;
            CFRunLoopTimerRef m_timer {nullptr};
            double m_timeout {0.0};

            AssistantPrivate();
            ~AssistantPrivate();
            inline static uint64_t id();
            bool startTimer();
            void stopTimer();
            static void timerTimeout(CFRunLoopTimerRef timer, void *info);
            void loadCameras();
            void releaseDevicesFromPeer(const std::string &portName);
            void peerDied();
            void requestPort(xpc_connection_t client, xpc_object_t event);
            void addPort(xpc_connection_t client, xpc_object_t event);
            void removePortByName(const std::string &portName);
            void removePort(xpc_connection_t client, xpc_object_t event);
            void deviceCreate(xpc_connection_t client, xpc_object_t event);
            void deviceDestroyById(const std::string &deviceId);
            void deviceDestroy(xpc_connection_t client, xpc_object_t event);
            void setBroadcasting(xpc_connection_t client, xpc_object_t event);
            void setMirroring(xpc_connection_t client, xpc_object_t event);
            void setScaling(xpc_connection_t client, xpc_object_t event);
            void setAspectRatio(xpc_connection_t client, xpc_object_t event);
            void setSwapRgb(xpc_connection_t client, xpc_object_t event);
            void frameReady(xpc_connection_t client, xpc_object_t event);
            void listeners(xpc_connection_t client, xpc_object_t event);
            void listener(xpc_connection_t client, xpc_object_t event);
            void devices(xpc_connection_t client, xpc_object_t event);
            void description(xpc_connection_t client, xpc_object_t event);
            void formats(xpc_connection_t client, xpc_object_t event);
            void broadcasting(xpc_connection_t client, xpc_object_t event);
            void mirroring(xpc_connection_t client, xpc_object_t event);
            void scaling(xpc_connection_t client, xpc_object_t event);
            void aspectRatio(xpc_connection_t client, xpc_object_t event);
            void swapRgb(xpc_connection_t client, xpc_object_t event);
            void listenerAdd(xpc_connection_t client, xpc_object_t event);
            void listenerRemove(xpc_connection_t client, xpc_object_t event);
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
        {AKVCAM_ASSISTANT_MSG_FRAME_READY           , AKVCAM_BIND_FUNC(AssistantPrivate::frameReady)     },
        {AKVCAM_ASSISTANT_MSG_REQUEST_PORT          , AKVCAM_BIND_FUNC(AssistantPrivate::requestPort)    },
        {AKVCAM_ASSISTANT_MSG_ADD_PORT              , AKVCAM_BIND_FUNC(AssistantPrivate::addPort)        },
        {AKVCAM_ASSISTANT_MSG_REMOVE_PORT           , AKVCAM_BIND_FUNC(AssistantPrivate::removePort)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_CREATE         , AKVCAM_BIND_FUNC(AssistantPrivate::deviceCreate)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY        , AKVCAM_BIND_FUNC(AssistantPrivate::deviceDestroy)  },
        {AKVCAM_ASSISTANT_MSG_DEVICES               , AKVCAM_BIND_FUNC(AssistantPrivate::devices)        },
        {AKVCAM_ASSISTANT_MSG_DEVICE_DESCRIPTION    , AKVCAM_BIND_FUNC(AssistantPrivate::description)    },
        {AKVCAM_ASSISTANT_MSG_DEVICE_FORMATS        , AKVCAM_BIND_FUNC(AssistantPrivate::formats)        },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD   , AKVCAM_BIND_FUNC(AssistantPrivate::listenerAdd)    },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE, AKVCAM_BIND_FUNC(AssistantPrivate::listenerRemove) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENERS      , AKVCAM_BIND_FUNC(AssistantPrivate::listeners)      },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER       , AKVCAM_BIND_FUNC(AssistantPrivate::listener)       },
        {AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING   , AKVCAM_BIND_FUNC(AssistantPrivate::broadcasting)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING, AKVCAM_BIND_FUNC(AssistantPrivate::setBroadcasting)},
        {AKVCAM_ASSISTANT_MSG_DEVICE_MIRRORING      , AKVCAM_BIND_FUNC(AssistantPrivate::mirroring)      },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETMIRRORING   , AKVCAM_BIND_FUNC(AssistantPrivate::setMirroring)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SCALING        , AKVCAM_BIND_FUNC(AssistantPrivate::scaling)        },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETSCALING     , AKVCAM_BIND_FUNC(AssistantPrivate::setScaling)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_ASPECTRATIO    , AKVCAM_BIND_FUNC(AssistantPrivate::aspectRatio)    },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETASPECTRATIO , AKVCAM_BIND_FUNC(AssistantPrivate::setAspectRatio) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SWAPRGB        , AKVCAM_BIND_FUNC(AssistantPrivate::swapRgb)        },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETSWAPRGB     , AKVCAM_BIND_FUNC(AssistantPrivate::setSwapRgb)     },
    };

    this->loadCameras();
    this->startTimer();
}

AkVCam::AssistantPrivate::~AssistantPrivate()
{
    std::vector<AssistantPeers *> allPeers {
        &this->m_clients,
        &this->m_servers
    };

    for (auto &device: this->m_deviceConfigs) {
        auto notification = xpc_dictionary_create(NULL, NULL, 0);
        xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY);
        xpc_dictionary_set_string(notification, "device", device.first.c_str());

        for (auto peers: allPeers)
            for (auto &peer: *peers)
                xpc_connection_send_message(peer.second, notification);

        xpc_release(notification);
    }
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
    UNUSED(timer)
    UNUSED(info)
    AkLogFunction();

    CFRunLoopStop(CFRunLoopGetMain());
}

void AkVCam::AssistantPrivate::loadCameras()
{
    AkLogFunction();

    for (size_t i = 0; i < Preferences::camerasCount(); i++) {
        this->m_deviceConfigs[Preferences::cameraPath(i)] = {};
        this->m_deviceConfigs[Preferences::cameraPath(i)].description =
                Preferences::cameraDescription(i);
        this->m_deviceConfigs[Preferences::cameraPath(i)].formats =
                Preferences::cameraFormats(i);
    }
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

            for (auto &client: this->m_clients) {
                auto reply =
                        xpc_connection_send_message_with_reply_sync(client.second,
                                                                    dictionary);
                xpc_release(reply);
            }

            xpc_release(dictionary);
        } else {
            auto it = std::find(config.second.listeners.begin(),
                                config.second.listeners.end(),
                                portName);

            if (it != config.second.listeners.end())
                config.second.listeners.erase(it);
        }
}

void AkVCam::AssistantPrivate::peerDied()
{
    AkLogFunction();

    std::vector<AssistantPeers *> allPeers {
        &this->m_clients,
        &this->m_servers
    };

    for (auto peers: allPeers) {
        for (auto &peer: *peers) {
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
                this->removePortByName(peer.first);
        }
    }
}

void AkVCam::AssistantPrivate::requestPort(xpc_connection_t client,
                                           xpc_object_t event)
{
    AkLogFunction();

    bool asClient = xpc_dictionary_get_bool(event, "client");
    std::string portName = asClient?
                AKVCAM_ASSISTANT_CLIENT_NAME:
                AKVCAM_ASSISTANT_SERVER_NAME;
    portName += std::to_string(this->id());

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
    AssistantPeers *peers;

    if (portName.find(AKVCAM_ASSISTANT_CLIENT_NAME) != std::string::npos)
        peers = &this->m_clients;
    else
        peers = &this->m_servers;

    for (auto &peer: *peers)
        if (peer.first == portName) {
            ok = false;

            break;
        }

    if (ok) {
        AkLogInfo() << "Adding Peer: " << portName << std::endl;
        (*peers)[portName] = connection;
        this->stopTimer();
    }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::removePortByName(const std::string &portName)
{
    AkLogFunction();
    AkLogInfo() << "Port: " << portName << std::endl;

    std::vector<AssistantPeers *> allPeers {
        &this->m_clients,
        &this->m_servers
    };

    bool breakLoop = false;

    for (auto peers: allPeers) {
        for (auto &peer: *peers)
            if (peer.first == portName) {
                xpc_release(peer.second);
                peers->erase(portName);
                breakLoop = true;

                break;
            }

        if (breakLoop)
            break;
    }

    bool peersEmpty = this->m_servers.empty() && this->m_clients.empty();

    if (peersEmpty)
        this->startTimer();

    this->releaseDevicesFromPeer(portName);
}

void AkVCam::AssistantPrivate::removePort(xpc_connection_t client,
                                          xpc_object_t event)
{
    UNUSED(client)
    AkLogFunction();

    this->removePortByName(xpc_dictionary_get_string(event, "port"));
}

void AkVCam::AssistantPrivate::deviceCreate(xpc_connection_t client,
                                            xpc_object_t event)
{
    AkLogFunction();
    std::string portName = xpc_dictionary_get_string(event, "port");
    AkLogInfo() << "Port Name: " << portName << std::endl;
    size_t len = 0;
    auto data = reinterpret_cast<const wchar_t *>(xpc_dictionary_get_data(event,
                                                                          "description",
                                                                          &len));
    std::wstring description(data, len / sizeof(wchar_t));
    auto formatsArray = xpc_dictionary_get_array(event, "formats");
    std::vector<VideoFormat> formats;

    for (size_t i = 0; i < xpc_array_get_count(formatsArray); i++) {
        auto format = xpc_array_get_dictionary(formatsArray, i);
        auto fourcc = FourCC(xpc_dictionary_get_uint64(format, "fourcc"));
        auto width = int(xpc_dictionary_get_int64(format, "width"));
        auto height = int(xpc_dictionary_get_int64(format, "height"));
        auto frameRate = Fraction(xpc_dictionary_get_string(format, "fps"));
        formats.push_back(VideoFormat {fourcc, width, height, {frameRate}});
    }

    auto deviceId = Preferences::addCamera(description, formats);
    this->m_deviceConfigs[deviceId] = {};
    this->m_deviceConfigs[deviceId].description = description;
    this->m_deviceConfigs[deviceId].formats = formats;

    auto notification = xpc_copy(event);
    xpc_dictionary_set_string(notification, "device", deviceId.c_str());

    for (auto &client: this->m_clients)
        xpc_connection_send_message(client.second, notification);

    xpc_release(notification);
    AkLogInfo() << "Device created: " << deviceId << std::endl;

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_string(reply, "device", deviceId.c_str());
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::deviceDestroyById(const std::string &deviceId)
{
    AkLogFunction();
    auto it = this->m_deviceConfigs.find(deviceId);

    if (it != this->m_deviceConfigs.end()) {
        this->m_deviceConfigs.erase(it);

        auto notification = xpc_dictionary_create(nullptr, nullptr, 0);
        xpc_dictionary_set_int64(notification, "message", AKVCAM_ASSISTANT_MSG_DEVICE_DESTROY);
        xpc_dictionary_set_string(notification, "device", deviceId.c_str());

        for (auto &client: this->m_clients)
            xpc_connection_send_message(client.second, notification);

        xpc_release(notification);
    }
}

void AkVCam::AssistantPrivate::deviceDestroy(xpc_connection_t client,
                                             xpc_object_t event)
{
    UNUSED(client)
    AkLogFunction();

    std::string deviceId = xpc_dictionary_get_string(event, "device");
    this->deviceDestroyById(deviceId);
    Preferences::removeCamera(deviceId);
}

void AkVCam::AssistantPrivate::setBroadcasting(xpc_connection_t client,
                                               xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::string broadcaster = xpc_dictionary_get_string(event, "broadcaster");
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        if (this->m_deviceConfigs[deviceId].broadcaster != broadcaster) {
            AkLogInfo() << "Device: " << deviceId << std::endl;
            AkLogInfo() << "Broadcaster: " << broadcaster << std::endl;
            this->m_deviceConfigs[deviceId].broadcaster = broadcaster;
            auto notification = xpc_copy(event);

            for (auto &client: this->m_clients)
                xpc_connection_send_message(client.second, notification);

            xpc_release(notification);
            ok = true;
        }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::setMirroring(xpc_connection_t client,
                                            xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool horizontalMirror = xpc_dictionary_get_bool(event, "hmirror");
    bool verticalMirror = xpc_dictionary_get_bool(event, "vmirror");
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        if (this->m_deviceConfigs[deviceId].horizontalMirror != horizontalMirror
            || this->m_deviceConfigs[deviceId].verticalMirror != verticalMirror) {
            this->m_deviceConfigs[deviceId].horizontalMirror = horizontalMirror;
            this->m_deviceConfigs[deviceId].verticalMirror = verticalMirror;
            auto notification = xpc_copy(event);

            for (auto &client: this->m_clients)
                xpc_connection_send_message(client.second, notification);

            xpc_release(notification);
            ok = true;
        }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::setScaling(xpc_connection_t client,
                                          xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    auto scaling = Scaling(xpc_dictionary_get_int64(event, "scaling"));
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        if (this->m_deviceConfigs[deviceId].scaling != scaling) {
            this->m_deviceConfigs[deviceId].scaling = scaling;
            auto notification = xpc_copy(event);

            for (auto &client: this->m_clients)
                xpc_connection_send_message(client.second, notification);

            xpc_release(notification);
            ok = true;
        }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::setAspectRatio(xpc_connection_t client,
                                              xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    auto aspectRatio = AspectRatio(xpc_dictionary_get_int64(event, "aspect"));
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        if (this->m_deviceConfigs[deviceId].aspectRatio != aspectRatio) {
            this->m_deviceConfigs[deviceId].aspectRatio = aspectRatio;
            auto notification = xpc_copy(event);

            for (auto &client: this->m_clients)
                xpc_connection_send_message(client.second, notification);

            xpc_release(notification);
            ok = true;
        }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::setSwapRgb(xpc_connection_t client,
                                          xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    auto swapRgb = xpc_dictionary_get_bool(event, "swap");
    bool ok = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        if (this->m_deviceConfigs[deviceId].swapRgb != swapRgb) {
            this->m_deviceConfigs[deviceId].swapRgb = swapRgb;
            auto notification = xpc_copy(event);

            for (auto &client: this->m_clients)
                xpc_connection_send_message(client.second, notification);

            xpc_release(notification);
            ok = true;
        }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::frameReady(xpc_connection_t client,
                                          xpc_object_t event)
{
    UNUSED(client)
    AkLogFunction();
    auto reply = xpc_dictionary_create_reply(event);
    bool ok = true;

    for (auto &client: this->m_clients) {
        auto reply = xpc_connection_send_message_with_reply_sync(client.second,
                                                                 event);
        auto replyType = xpc_get_type(reply);
        bool isOk = false;

        if (replyType == XPC_TYPE_DICTIONARY)
            isOk = xpc_dictionary_get_bool(reply, "status");

        ok &= isOk;
        xpc_release(reply);
    }

    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
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

void AkVCam::AssistantPrivate::devices(xpc_connection_t client,
                                       xpc_object_t event)
{
    AkLogFunction();
    auto devices = xpc_array_create(nullptr, 0);

    for (auto &device: this->m_deviceConfigs) {
        auto deviceObj = xpc_string_create(device.first.c_str());
        xpc_array_append_value(devices, deviceObj);
    }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_value(reply, "devices", devices);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::description(xpc_connection_t client,
                                           xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    std::wstring description;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        description = this->m_deviceConfigs[deviceId].description;

    AkLogInfo() << "Description for device "
                << deviceId
                << ": "
                << std::string(description.begin(), description.end())
                << std::endl;
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_data(reply,
                            "description",
                            description.c_str(),
                            description.size() * sizeof(wchar_t));
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::formats(xpc_connection_t client,
                                       xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    auto formats = xpc_array_create(nullptr, 0);

    if (this->m_deviceConfigs.count(deviceId) > 0)
        for (auto &format: this->m_deviceConfigs[deviceId].formats) {
            auto dictFormat = xpc_dictionary_create(nullptr, nullptr, 0);
            xpc_dictionary_set_uint64(dictFormat, "fourcc", format.fourcc());
            xpc_dictionary_set_int64(dictFormat, "width", format.width());
            xpc_dictionary_set_int64(dictFormat, "height", format.height());
            xpc_dictionary_set_string(dictFormat, "fps", format.minimumFrameRate().toString().c_str());
            xpc_array_append_value(formats, dictFormat);
        }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_value(reply, "formats", formats);
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

void AkVCam::AssistantPrivate::mirroring(xpc_connection_t client,
                                         xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool horizontalMirror = false;
    bool verticalMirror = false;

    if (this->m_deviceConfigs.count(deviceId) > 0) {
        horizontalMirror = this->m_deviceConfigs[deviceId].horizontalMirror;
        verticalMirror = this->m_deviceConfigs[deviceId].verticalMirror;
    }

    AkLogInfo() << "Device: " << deviceId << std::endl;
    AkLogInfo() << "Horizontal mirror: " << horizontalMirror << std::endl;
    AkLogInfo() << "Vertical mirror: " << verticalMirror << std::endl;
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "hmirror", horizontalMirror);
    xpc_dictionary_set_bool(reply, "vmirror", verticalMirror);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::scaling(xpc_connection_t client, xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    Scaling scaling = ScalingFast;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        scaling = this->m_deviceConfigs[deviceId].scaling;

    AkLogInfo() << "Device: " << deviceId << std::endl;
    AkLogInfo() << "Scaling: " << scaling << std::endl;
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_int64(reply, "scaling", scaling);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::aspectRatio(xpc_connection_t client,
                                           xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    AspectRatio aspectRatio = AspectRatioIgnore;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        aspectRatio = this->m_deviceConfigs[deviceId].aspectRatio;

    AkLogInfo() << "Device: " << deviceId << std::endl;
    AkLogInfo() << "Aspect ratio: " << aspectRatio << std::endl;
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_int64(reply, "aspect", aspectRatio);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}

void AkVCam::AssistantPrivate::swapRgb(xpc_connection_t client,
                                       xpc_object_t event)
{
    AkLogFunction();
    std::string deviceId = xpc_dictionary_get_string(event, "device");
    bool swapRgb = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        swapRgb = this->m_deviceConfigs[deviceId].swapRgb;

    AkLogInfo() << "Device: " << deviceId << std::endl;
    AkLogInfo() << "Swap RGB: " << swapRgb << std::endl;
    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "swap", swapRgb);
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

            for (auto &client: this->m_clients)
                xpc_connection_send_message(client.second, notification);

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

            for (auto &client: this->m_clients)
                xpc_connection_send_message(client.second, notification);

            xpc_release(notification);
            ok = true;
        }
    }

    auto reply = xpc_dictionary_create_reply(event);
    xpc_dictionary_set_bool(reply, "status", ok);
    xpc_connection_send_message(client, reply);
    xpc_release(reply);
}
