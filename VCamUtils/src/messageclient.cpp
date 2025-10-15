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
#include <cstring>
#include <map>
#include <mutex>

#include "messageclient.h"
#include "logger.h"
#include "message.h"
#include "sockets.h"

namespace AkVCam
{
    class MessageClientPrivate
    {
        public:
            MessageClient *self;
            uint16_t m_port;
            std::mutex m_logsMutex;

            explicit MessageClientPrivate(MessageClient *self);
            static std::string getLastError();
            bool connection(uint16_t port,
                            MessageClient::InMessageHandler readData,
                            MessageClient::OutMessageHandler writeData);
    };
}

AkVCam::MessageClient::MessageClient()
{
    this->d = new MessageClientPrivate(this);
    Sockets::init();
}

AkVCam::MessageClient::~MessageClient()
{
    Sockets::uninit();
    delete this->d;
}

uint16_t AkVCam::MessageClient::port() const
{
    return this->d->m_port;
}

void AkVCam::MessageClient::setPort(uint16_t port)
{
    this->d->m_port = port;
}

bool AkVCam::MessageClient::isUp(uint16_t port)
{
    AkLogFunction();
    AkLogDebug() << "Port: " << port << std::endl;
    auto clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket < 0) {
        AkLogCritical() << "Failed creating the socket: "
                        << std::system_error(errno, std::system_category()).what()
                        << std::endl;

        return false;
    }

    // Set the port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Connect to the server
    if (connect(clientSocket,
                reinterpret_cast<sockaddr *>(&serverAddress),
                sizeof(sockaddr_in)) != 0) {
        AkLogCritical() << "Failed connecting to the socket: "
                        << MessageClientPrivate::getLastError()
                        << std::endl;

        return false;
    }

    Sockets::closeSocket(clientSocket);

    return true;
}

bool AkVCam::MessageClient::send(const Message &inMessage,
                                 Message &outMessage) const
{
    AkLogFunction();

    return this->d->connection(this->d->m_port,
                               [&inMessage] (Message &message) -> bool {
                                   message = inMessage;

                                   return true;
                               },
                               [&outMessage] (const Message &message) -> bool {
                                   outMessage = message;

                                   return false;
                               });
}

bool AkVCam::MessageClient::send(const Message &inMessage) const
{
    Message outMessage;

    return this->send(inMessage, outMessage);
}

std::future<bool> AkVCam::MessageClient::send(InMessageHandler inData,
                                              OutMessageHandler outData)
{
    return std::async(&MessageClientPrivate::connection,
                      this->d,
                      this->d->m_port,
                      inData,
                      outData);
}

std::future<bool> AkVCam::MessageClient::send(InMessageHandler inData)
{
    return this->send(inData,
                      [] (const Message &message) -> bool {
                          UNUSED(message);

                          return true;
                      });
}

std::future<bool> AkVCam::MessageClient::send(const Message &inMessage,
                                              OutMessageHandler outData)
{
    return this->send([inMessage] (Message &message) -> bool {
                          message = inMessage;

                          return true;
                      },
                      outData);
}

AkVCam::MessageClientPrivate::MessageClientPrivate(MessageClient *self):
    self(self)
{
}

std::string AkVCam::MessageClientPrivate::getLastError()
{
#ifdef _WIN32
        int errorCode = WSAGetLastError();

        if (errorCode == 0)
            return {"No error"};

        LPVOID msgBuffer = nullptr;

        // Pide a Windows que traduzca el código de error en un texto legible
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER
                       | FORMAT_MESSAGE_FROM_SYSTEM
                       | FORMAT_MESSAGE_IGNORE_INSERTS,
                       nullptr,
                       errorCode,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       reinterpret_cast<LPSTR>(&msgBuffer),
                       0,
                       nullptr);

        std::string message =
                msgBuffer != nullptr?
                                 std::string(reinterpret_cast<LPSTR>(msgBuffer)):
                                 "Unknown error";

        if (msgBuffer)
            LocalFree(msgBuffer);

        while (!message.empty()
               && (message.back() == '\r' || message.back() == '\n'))
            message.pop_back();

        return message + " (code " + std::to_string(errorCode) + ")";
#else
        return std::system_error(errno, std::system_category()).what();
#endif
}

bool AkVCam::MessageClientPrivate::connection(uint16_t port,
                                              MessageClient::InMessageHandler readData,
                                              MessageClient::OutMessageHandler writeData)
{
    AkLogFunction();
    AkLogDebug() << "Port: " << port << std::endl;
    auto clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket < 0) {
        AkLogDebug() << "Failed to create the socket" << std::endl;

        return false;
    }

    // Configure the socket operations timeout

#ifdef _WIN32
    DWORD timeout = 5000; // 5 seconds
    setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 seconds
    timeout.tv_usec = 0;
    setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
#endif

    // Set the port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Connect to the server
    if (connect(clientSocket,
                reinterpret_cast<sockaddr *>(&serverAddress),
                sizeof(sockaddr_in)) != 0) {
        AkLogDebug() << "Failed to connect with the server" << std::endl;
        Sockets::closeSocket(clientSocket);

        return false;
    }

    auto connectionId = AkVCam::id();

    this->m_logsMutex.lock();
    AkLogDebug() << "Connection ready: " << connectionId << std::endl;
    this->m_logsMutex.unlock();

    bool ok = true;

    // Send the response
    for (;;) {
        bool more = true;

        Message inMessage;
        more &= readData(inMessage);

        this->m_logsMutex.lock();
        AkLogDebug() << "Send message:" << std::endl;
        AkLogDebug() << "    Connection ID: " << connectionId << std::endl;
        AkLogDebug() << "    Message ID: " << stringFromMessageId(inMessage.id()) << std::endl;
        AkLogDebug() << "    Query ID: " << inMessage.queryId() << std::endl;
        AkLogDebug() << "    Data size: " << inMessage.data().size() << std::endl;
        this->m_logsMutex.unlock();

        if (!Sockets::send(clientSocket, inMessage.id())) {
            ok = false;

            break;
        }

        if (!Sockets::send(clientSocket, inMessage.queryId())) {
            ok = false;

            break;
        }

        if (!Sockets::send(clientSocket, inMessage.data())) {
            ok = false;

            break;
        }

        int messageId = 0;

        if (!Sockets::recv(clientSocket, messageId)) {
            ok = false;

            break;
        }

        uint64_t queryId = 0;

        if (!Sockets::recv(clientSocket, queryId)) {
            ok = false;

            break;
        }

        std::vector<char> outData;

        if (!Sockets::recv(clientSocket, outData)) {
            ok = false;

            break;
        }

        this->m_logsMutex.lock();
        AkLogDebug() << "Received message:" << std::endl;
        AkLogDebug() << "    Connection ID: " << connectionId << std::endl;
        AkLogDebug() << "    Message ID: " << stringFromMessageId(messageId) << std::endl;
        AkLogDebug() << "    Query ID: " << queryId << std::endl;
        AkLogDebug() << "    Data size: " << outData.size() << std::endl;
        this->m_logsMutex.unlock();

        more &= writeData({messageId, queryId, outData});

        if (!more)
            break;
    }

    this->m_logsMutex.lock();
    AkLogDebug() << "Connection closed: " << connectionId << std::endl;
    this->m_logsMutex.unlock();

    Sockets::closeSocket(clientSocket);

    return ok;
}
