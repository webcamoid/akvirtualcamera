/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2024  Gonzalo Exequiel Pedone
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

#include "sockets.h"

#ifdef _WIN32
static bool akvcamSocketsInitialized = false;
#endif

bool AkVCam::Sockets::init()
{
#ifdef _WIN32
    // Initialize winsocks
    if (!akvcamSocketsInitialized) {
        WSADATA wsaData;

        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            return false;

        akvcamSocketsInitialized = true;
    }
#endif

    return true;
}

void AkVCam::Sockets::uninit()
{
#ifdef _WIN32
    if (akvcamSocketsInitialized) {
        WSACleanup();
        akvcamSocketsInitialized = false;
    }
#endif
}

bool AkVCam::Sockets::send(SocketType socket, const void *data, size_t dataSize)
{
    size_t dataSent = 0;

    while (dataSent < dataSize) {
        auto sent = ::send(socket,
                           reinterpret_cast<const char *>(data) + dataSent,
                           dataSize - dataSent,
                           0);

        if (sent < 1)
            return false;

        dataSent += sent;
    }

    return true;
}

bool AkVCam::Sockets::send(SocketType socket, const std::vector<char> &data)
{
    bool ok = true;
    size_t dataSize = data.size();
    ok &= Sockets::send(socket,
                        reinterpret_cast<char *>(&dataSize),
                        sizeof(size_t));

    if (ok && dataSize > 0)
        ok &= Sockets::send(socket, data.data(), dataSize);

    return ok;
}

bool AkVCam::Sockets::recv(SocketType socket, void *data, size_t dataSize)
{
    size_t dataReceived = 0;

    while (dataReceived < dataSize) {
        auto received = ::recv(socket,
                               reinterpret_cast<char *>(data) + dataReceived,
                               dataSize - dataReceived,
                               0);

        if (received < 1)
            return false;

        dataReceived += received;
    }

    return true;
}

bool AkVCam::Sockets::recv(SocketType socket, std::vector<char> &data)
{
    bool ok = true;
    size_t dataSize = 0;
    ok &= Sockets::recv(socket,
                        reinterpret_cast<char *>(&dataSize),
                        sizeof(size_t));
    data.resize(dataSize);

    if (ok && dataSize > 0)
        ok &= Sockets::recv(socket, data.data(), dataSize);

    return ok;
}

void AkVCam::Sockets::closeSocket(SocketType socket)
{
#ifdef _WIN32
    closesocket(socket);
#else
    close(socket);
#endif
}
