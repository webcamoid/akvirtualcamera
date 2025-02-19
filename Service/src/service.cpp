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
#include <condition_variable>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <thread>

#include "service.h"
#include "PlatformUtils/src/preferences.h"
#include "VCamUtils/src/logger.h"
#include "VCamUtils/src/message.h"
#include "VCamUtils/src/messageserver.h"
#include "VCamUtils/src/servicemsg.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"

namespace AkVCam
{
    struct Peer
    {
        uint64_t clientId {0};
        uint64_t pid {0};

        Peer(uint64_t clientId=0, uint64_t pid=0):
            clientId(clientId),
            pid(pid)
        {

        }
    };

    struct BroadcastSlot
    {
        Peer broadcaster;
        std::vector<Peer> listeners;
        VideoFrame frame;
    };

    typedef std::map<std::string, BroadcastSlot> Broadcasts;

    class ServicePrivate
    {
        public:
            MessageServer m_messageServer;

            // Broadcasting and listen
            Broadcasts m_broadcasts;
            std::condition_variable_any m_frameAvailable;
            std::mutex m_peerMutex;

            ServicePrivate();
            static void removeClientById(void *userData, uint64_t clientId);
            bool clients(uint64_t clientId,
                         const Message &inMessage,
                         Message &outMessage);
            bool broadcast(uint64_t clientId,
                           const Message &inMessage,
                           Message &outMessage);
            bool listen(uint64_t clientId,
                        const Message &inMessage,
                        Message &outMessage);
    };
}

AkVCam::Service::Service()
{
    this->d = new ServicePrivate;
}

AkVCam::Service::~Service()
{
    delete this->d;
}

int AkVCam::Service::run()
{
    AkLogFunction();

    return this->d->m_messageServer.run();
}

void AkVCam::Service::stop()
{
    AkLogFunction();
    this->d->m_messageServer.stop();
}

#define BIND(member) \
    std::bind(&member, \
              this, \
              std::placeholders::_1, \
              std::placeholders::_2, \
              std::placeholders::_3)

AkVCam::ServicePrivate::ServicePrivate()
{
    AkLogFunction();

    this->m_messageServer.setPort(Preferences::servicePort());

    this->m_messageServer.subscribe(AKVCAM_SERVICE_MSG_CLIENTS  , BIND(ServicePrivate::clients)  );
    this->m_messageServer.subscribe(AKVCAM_SERVICE_MSG_BROADCAST, BIND(ServicePrivate::broadcast));
    this->m_messageServer.subscribe(AKVCAM_SERVICE_MSG_LISTEN   , BIND(ServicePrivate::listen)   );

    this->m_messageServer.connectConnectionClosed(this, &ServicePrivate::removeClientById);
}

void AkVCam::ServicePrivate::removeClientById(void *userData,
                                              uint64_t clientId)
{
    AkLogFunction();
    AkLogDebug() << "Removing client: " << clientId << std::endl;
    auto self = reinterpret_cast<ServicePrivate *>(userData);

    self->m_peerMutex.lock();
    std::string removeDevice;

    for (auto &slot: self->m_broadcasts) {
        if (slot.second.broadcaster.clientId == clientId) {
            slot.second.broadcaster = {0, 0};

            if (slot.second.listeners.empty())
                removeDevice = slot.first;

            break;
        } else {
            auto it = std::find_if(slot.second.listeners.begin(),
                                   slot.second.listeners.end(),
                                   [&clientId] (const Peer &peer) -> bool {
                return peer.clientId == clientId;
            });

            if (it != slot.second.listeners.end()) {
                slot.second.listeners.erase(it);

                if (slot.second.broadcaster.pid == 0
                    && slot.second.listeners.empty()) {
                    removeDevice = slot.first;
                }

                break;
            }
        }
    }

    if (!removeDevice.empty())
        self->m_broadcasts.erase(removeDevice);

    self->m_peerMutex.unlock();
}

bool AkVCam::ServicePrivate::clients(uint64_t clientId,
                                     const Message &inMessage,
                                     Message &outMessage)
{
    AkLogFunction();
    UNUSED(clientId);
    MsgClients msgClients(inMessage);
    std::vector<uint64_t> clients;

    this->m_peerMutex.lock();

    for (auto &slot: this->m_broadcasts) {
        if (msgClients.clientType() == MsgClients::ClientType_Any
            && slot.second.broadcaster.pid
            && std::find(clients.begin(),
                         clients.end(),
                         slot.second.broadcaster.pid) == clients.end()) {
            clients.push_back(slot.second.broadcaster.pid);
        }

        for (auto &client: slot.second.listeners)
            if (std::find(clients.begin(),
                          clients.end(),
                          client.pid) == clients.end())
                clients.push_back(client.pid);
    }

    this->m_peerMutex.unlock();
    outMessage = MsgClients(msgClients.clientType(),
                            clients,
                            inMessage.queryId()).toMessage();

    return true;
}

bool AkVCam::ServicePrivate::broadcast(uint64_t clientId,
                                       const Message &inMessage,
                                       Message &outMessage)
{
    AkLogFunction();
    MsgBroadcast msgBroadcast(inMessage);
    MsgStatus status(-1, inMessage.queryId());
    this->m_peerMutex.lock();

    bool isBroadcasting = this->m_broadcasts.count(msgBroadcast.device()) < 1;
    AkLogDebug() << "Device" << msgBroadcast.device() << "is broadcasting?:" << (isBroadcasting? "YES": "NO") << std::endl;

    if (isBroadcasting) {
        AkLogDebug() << "Adding device slot:" << std::endl;
        AkLogDebug() << "    Device ID:" << msgBroadcast.device() << std::endl;
        AkLogDebug() << "    Client ID:" << clientId << std::endl;
        AkLogDebug() << "    Client PID:" << msgBroadcast.pid() << std::endl;

        this->m_broadcasts[msgBroadcast.device()] =
            {{clientId, msgBroadcast.pid()}, {}, {}};
    }

    AkLogDebug() << "Get slot" << std::endl;
    auto &slot = this->m_broadcasts[msgBroadcast.device()];

    if (slot.broadcaster.pid == 0) {
        AkLogDebug() << "Set client as broadcaster" << std::endl;
        slot.broadcaster = {clientId, msgBroadcast.pid()};
    }

    if (slot.broadcaster.pid == msgBroadcast.pid()
        && slot.broadcaster.clientId == clientId) {
        AkLogDebug() << "Save frame" << std::endl;
        slot.frame = msgBroadcast.frame();
        status = MsgStatus(0, inMessage.queryId());
        this->m_frameAvailable.notify_all();
    }

    this->m_peerMutex.unlock();

    AkLogDebug() << "Sending the response" << std::endl;
    outMessage = status.toMessage();

    return status.status() == 0;
}

bool AkVCam::ServicePrivate::listen(uint64_t clientId,
                                    const Message &inMessage,
                                    Message &outMessage)
{
    AkLogFunction();
    MsgListen msgListen(inMessage);
    bool ok = false;

    this->m_peerMutex.lock();

    if (this->m_broadcasts.count(msgListen.device()) < 1)
        this->m_broadcasts[msgListen.device()] = {};

    auto &slot = this->m_broadcasts[msgListen.device()];
    slot.listeners.push_back({clientId, msgListen.pid()});

    if (!slot.frame)
        this->m_frameAvailable.wait_for(this->m_peerMutex,
                                        std::chrono::seconds(1));

    outMessage = MsgFrameReady(msgListen.device(),
                               slot.frame,
                               slot.broadcaster.pid != 0,
                               inMessage.queryId()).toMessage();
    slot.frame = {};
    ok = true;
    this->m_peerMutex.unlock();

    return ok;
}
