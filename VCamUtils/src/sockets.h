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

#ifndef AKVCAMUTILS_SOCKETS_H
#define AKVCAMUTILS_SOCKETS_H

#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

using SocketType = SOCKET;
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using SocketType = int;
#endif

namespace AkVCam
{
    namespace Sockets
    {
        bool init();
        void uninit();
        bool send(SocketType socket, const void *data, size_t dataSize);
        bool send(SocketType socket, const std::vector<char> &data);

        template <typename T>
        inline bool send(SocketType socket, T value)
        {
            return Sockets::send(socket, &value, sizeof(T));
        }

        bool recv(SocketType socket, void *data, size_t dataSize);
        bool recv(SocketType socket, std::vector<char> &data);

        template <typename T>
        inline bool recv(SocketType socket, T &value)
        {
            return Sockets::recv(socket, &value, sizeof(T));
        }

        void closeSocket(SocketType socket);
    }
}

#endif // AKVCAMUTILS_SOCKETS_H
