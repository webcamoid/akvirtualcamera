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

            // Picture update
            std::string m_picture;
            bool m_pictUpdated {false};
            std::condition_variable_any m_pictureUpdated;
            std::mutex m_pictureMutex;

            // Devices update
            bool m_devsUpdated {false};
            std::condition_variable_any m_devicesUpdated;
            std::mutex m_devicesMutex;

            // Controls update
            std::string m_deviceControls;
            bool m_devControlsUpdated {false};
            std::condition_variable_any m_deviceControlsUpdated;
            std::mutex m_deviceControlsMutex;

            ServicePrivate();
            static void removeClientById(void *userData, uint64_t clientId);
            bool clients(uint64_t clientId,
                         const Message &inMessage,
                         Message &outMessage);
            bool updateDevices(uint64_t clientId,
                               const Message &inMessage,
                               Message &outMessage);
            bool devicesUpdated(uint64_t clientId,
                                const Message &inMessage,
                                Message &outMessage);
            bool updatePicture(uint64_t clientId,
                               const Message &inMessage,
                               Message &outMessage);
            bool pictureUpdated(uint64_t clientId,
                                const Message &inMessage,
                                Message &outMessage);
            bool updateControls(uint64_t clientId,
                                const Message &inMessage,
                                Message &outMessage);
            bool controlsUpdated(uint64_t clientId,
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

    this->m_messageServer.subscribe(AKVCAM_SERVICE_MSG_CLIENTS         , BIND(ServicePrivate::clients)        );
    this->m_messageServer.subscribe(AKVCAM_SERVICE_MSG_UPDATE_PICTURE  , BIND(ServicePrivate::updatePicture)  );
    this->m_messageServer.subscribe(AKVCAM_SERVICE_MSG_PICTURE_UPDATED , BIND(ServicePrivate::pictureUpdated) );
    this->m_messageServer.subscribe(AKVCAM_SERVICE_MSG_UPDATE_DEVICES  , BIND(ServicePrivate::updateDevices)  );
    this->m_messageServer.subscribe(AKVCAM_SERVICE_MSG_DEVICES_UPDATED , BIND(ServicePrivate::devicesUpdated) );
    this->m_messageServer.subscribe(AKVCAM_SERVICE_MSG_UPDATE_CONTROLS , BIND(ServicePrivate::updateControls) );
    this->m_messageServer.subscribe(AKVCAM_SERVICE_MSG_CONTROLS_UPDATED, BIND(ServicePrivate::controlsUpdated));
    this->m_messageServer.subscribe(AKVCAM_SERVICE_MSG_BROADCAST       , BIND(ServicePrivate::broadcast)      );
    this->m_messageServer.subscribe(AKVCAM_SERVICE_MSG_LISTEN          , BIND(ServicePrivate::listen)         );

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
                            inMessage.queryId());

    return true;
}

bool AkVCam::ServicePrivate::updateDevices(uint64_t clientId,
                                           const Message &inMessage,
                                           Message &outMessage)
{
    AkLogFunction();
    UNUSED(clientId);
    MsgUpdateDevices updateDevices(inMessage);

    this->m_devicesMutex.lock();
    this->m_devsUpdated = true;
    this->m_devicesUpdated.notify_all();
    this->m_devicesMutex.unlock();

    outMessage = MsgStatus(0, inMessage.queryId());

    return true;
}

bool AkVCam::ServicePrivate::devicesUpdated(uint64_t clientId,
                                           const Message &inMessage,
                                           Message &outMessage)
{
    AkLogFunction();
    UNUSED(clientId);

    this->m_devicesMutex.lock();

    if (!this->m_devsUpdated)
        this->m_devicesUpdated.wait_for(this->m_devicesMutex,
                                        std::chrono::seconds(1));

    outMessage = MsgStatus(this->m_devsUpdated? 0: -1, inMessage.queryId());
    this->m_devsUpdated = false;
    this->m_devicesMutex.unlock();

    return true;
}

bool AkVCam::ServicePrivate::updatePicture(uint64_t clientId,
                                           const Message &inMessage,
                                           Message &outMessage)
{
    AkLogFunction();
    UNUSED(clientId);
    MsgUpdatePicture updatePicture(inMessage);

    this->m_pictureMutex.lock();
    this->m_picture = updatePicture.picture();
    this->m_pictUpdated = true;
    this->m_pictureUpdated.notify_all();
    this->m_pictureMutex.unlock();

    outMessage = MsgStatus(0, inMessage.queryId());

    return true;
}

bool AkVCam::ServicePrivate::pictureUpdated(uint64_t clientId,
                                            const Message &inMessage,
                                            Message &outMessage)
{
    AkLogFunction();
    UNUSED(clientId);

    this->m_pictureMutex.lock();

    if (!this->m_pictUpdated)
        this->m_pictureUpdated.wait_for(this->m_pictureMutex,
                                        std::chrono::seconds(1));

    outMessage = MsgPictureUpdated(this->m_picture,
                                   this->m_pictUpdated,
                                   inMessage.queryId());
    this->m_pictUpdated = false;
    this->m_pictureMutex.unlock();

    return true;
}

bool AkVCam::ServicePrivate::updateControls(uint64_t clientId,
                                            const Message &inMessage,
                                            Message &outMessage)
{
    AkLogFunction();
    UNUSED(clientId);
    MsgUpdateControls updateControls(inMessage);

    this->m_deviceControlsMutex.lock();
    this->m_deviceControls = updateControls.device();
    this->m_devControlsUpdated = true;
    this->m_deviceControlsUpdated.notify_all();
    this->m_deviceControlsMutex.unlock();

    outMessage = MsgStatus(0, inMessage.queryId());

    return true;
}

bool AkVCam::ServicePrivate::controlsUpdated(uint64_t clientId,
                                             const Message &inMessage,
                                             Message &outMessage)
{
    AkLogFunction();
    UNUSED(clientId);

    this->m_deviceControlsMutex.lock();

    if (!this->m_devControlsUpdated)
        this->m_deviceControlsUpdated.wait_for(this->m_deviceControlsMutex,
                                               std::chrono::seconds(1));

    outMessage = MsgControlsUpdated(this->m_deviceControls,
                                    this->m_devControlsUpdated,
                                    inMessage.queryId());
    this->m_devControlsUpdated = false;
    this->m_deviceControlsMutex.unlock();

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

    if (this->m_broadcasts.count(msgBroadcast.device()) < 1)
        this->m_broadcasts[msgBroadcast.device()] =
            {{clientId, msgBroadcast.pid()}, {}, {}};

    auto &slot = this->m_broadcasts[msgBroadcast.device()];

    if (slot.broadcaster.pid == 0)
        slot.broadcaster = {clientId, msgBroadcast.pid()};

    if (slot.broadcaster.pid == msgBroadcast.pid()
        && slot.broadcaster.clientId == clientId) {
        slot.frame = msgBroadcast.frame();
        status = MsgStatus(0, inMessage.queryId());
        this->m_frameAvailable.notify_all();
    }

    this->m_peerMutex.unlock();
    outMessage = status;

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
                               inMessage.queryId());
    slot.frame = {};
    ok = true;
    this->m_peerMutex.unlock();

    return ok;
}
