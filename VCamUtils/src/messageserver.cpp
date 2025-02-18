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
#include <future>
#include <map>
#include <mutex>

#include "messageserver.h"
#include "logger.h"
#include "message.h"
#include "sockets.h"

namespace AkVCam
{
    struct Connection
    {
        std::future<void> thread;
        bool run {true};
    };

    using ConnectionThreadPtr = std::shared_ptr<Connection>;

    class MessageServerPrivate
    {
        public:
            MessageServer *self;
            uint16_t m_port;
            std::map<uint32_t, MessageServer::MessageHandler> m_handlers;
            std::vector<ConnectionThreadPtr> m_clients;
            std::mutex m_handlersMutex;
            std::mutex m_clientsMutex;
            std::mutex m_logsMutex;
            bool m_run {false};

            explicit MessageServerPrivate(MessageServer *self);
            void cleanup(bool wait);
            void connection(SocketType clientSocket,
                            ConnectionThreadPtr connection);
    };
}

AkVCam::MessageServer::MessageServer()
{
    this->d = new MessageServerPrivate(this);
    Sockets::init();
}

AkVCam::MessageServer::~MessageServer()
{
    this->stop();
    Sockets::uninit();
    delete this->d;
}

uint16_t AkVCam::MessageServer::port() const
{
    return this->d->m_port;
}

void AkVCam::MessageServer::setPort(uint16_t port)
{
    this->d->m_port = port;
}

bool AkVCam::MessageServer::subscribe(int messageId,
                                      MessageHandler messageHandlerFunc)
{
    this->d->m_handlersMutex.lock();

    if (this->d->m_handlers.count(messageId) > 0) {
        this->d->m_handlersMutex.unlock();

        return false;
    }

    this->d->m_handlers[messageId] = messageHandlerFunc;
    this->d->m_handlersMutex.unlock();

    return true;
}

bool AkVCam::MessageServer::unsubscribe(int messageId)
{
    this->d->m_handlersMutex.lock();
    auto it = this->d->m_handlers.find(messageId);

    if (it == this->d->m_handlers.end()) {
        this->d->m_handlersMutex.unlock();

        return false;
    }

    this->d->m_handlers.erase(it);
    this->d->m_handlersMutex.unlock();

    return true;
}

int AkVCam::MessageServer::run()
{
    AkLogFunction();
    AkLogInfo() << "Starting server" << std::endl;

    auto serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0) {
        AkLogCritical() << "Failed to create the socket" << std::endl;

        return -EXIT_FAILURE;
    }

    // Set the port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(this->d->m_port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(serverSocket,
             reinterpret_cast<struct sockaddr *>(&serverAddress),
             sizeof(sockaddr_in))) {
        AkLogCritical() << "Failed to bind the socket" << std::endl;

        return -EXIT_FAILURE;
    }

    // Start listening for connected clients
    if (listen(serverSocket, SOMAXCONN) != 0) {
        AkLogCritical() << "Failed listening to the socket" << std::endl;

        return -EXIT_FAILURE;
    }

    AkLogInfo() << "Server running at http://localhost:" << this->d->m_port << '/' << std::endl;
    this->d->m_run = true;

    while (this->d->m_run) {
        // accepting connection request
        auto clientSocket = accept(serverSocket, nullptr, nullptr);

        this->d->m_clientsMutex.lock();
        auto connection = std::make_shared<Connection>();
        connection->thread = std::async(&MessageServerPrivate::connection,
                                        this->d,
                                        clientSocket,
                                        connection);
        this->d->m_clients.push_back(connection);
        this->d->m_clientsMutex.unlock();

        this->d->cleanup(false);
    }

    AkLogInfo() << "Stopping the server." << std::endl;
    this->d->cleanup(true);

    Sockets::closeSocket(serverSocket);

    AkLogInfo() << "Server stopped." << std::endl;

    return EXIT_SUCCESS;
}

void AkVCam::MessageServer::stop()
{
    AkLogFunction();

    this->d->m_run = false;
}

AkVCam::MessageServerPrivate::MessageServerPrivate(MessageServer *self):
    self(self)
{
}

void AkVCam::MessageServerPrivate::cleanup(bool wait)
{
    bool run;

    do {
        run = false;

        this->m_clientsMutex.lock();

        for (auto it = this->m_clients.begin(); it != this->m_clients.end(); ++it)
            if (wait) {
                (*it)->thread.wait();
                this->m_clients.erase(it);
                run = true;

                break;
            } else if ((*it)->thread.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                this->m_clients.erase(it);
                run = true;

                break;
            }

        this->m_clientsMutex.unlock();
    } while (run);
}

void AkVCam::MessageServerPrivate::connection(SocketType clientSocket,
                                              ConnectionThreadPtr connection)
{
    auto clientId = AkVCam::id();

    this->m_logsMutex.lock();
    AkLogDebug() << "Client connected: " << clientId << std::endl;
    this->m_logsMutex.unlock();

    bool ok = true;

    while (connection->run && ok) {
        int messageId = 0;

        if (!Sockets::recv(clientSocket, messageId))
            break;

        uint64_t queryId = 0;

        if (!Sockets::recv(clientSocket, queryId))
            break;

        std::vector<char> inData;

        if (!Sockets::recv(clientSocket, inData))
            break;

        this->m_logsMutex.lock();
        AkLogDebug() << "Received message:" << std::endl;
        AkLogDebug() << "    Client ID: " << clientId << std::endl;
        AkLogDebug() << "    Message ID: " << stringFromMessageId(messageId) << std::endl;
        AkLogDebug() << "    Query ID: " << queryId << std::endl;
        AkLogDebug() << "    Data size: " << inData.size() << std::endl;
        this->m_logsMutex.unlock();

        Message outMessage;

        this->m_handlersMutex.lock();
        auto hnd = this->m_handlers.find(messageId);

        if (hnd != this->m_handlers.end())
            ok &= hnd->second(clientId, {messageId, queryId, inData}, outMessage);

        this->m_handlersMutex.unlock();

        if (!ok)
            break;

        this->m_logsMutex.lock();
        AkLogDebug() << "Send message:" << std::endl;
        AkLogDebug() << "    Client ID: " << clientId << std::endl;
        AkLogDebug() << "    Message ID: " << stringFromMessageId(outMessage.id()) << std::endl;
        AkLogDebug() << "    Query ID: " << outMessage.queryId() << std::endl;
        AkLogDebug() << "    Data size: " << outMessage.data().size() << std::endl;
        this->m_logsMutex.unlock();

        if (!Sockets::send(clientSocket, outMessage.id()))
            break;

        if (!Sockets::send(clientSocket, outMessage.queryId()))
            break;

        if (!Sockets::send(clientSocket, outMessage.data()))
            break;
    }

    Sockets::closeSocket(clientSocket);
    this->m_logsMutex.lock();
    AkLogDebug() << "Client disconnected: " << clientId << std::endl;
    this->m_logsMutex.unlock();
    AKVCAM_EMIT(self, ConnectionClosed, clientId)
}
