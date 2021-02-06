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

#ifndef IPCBRIDGE_H
#define IPCBRIDGE_H

#include <string>
#include <map>

#include "image/videoformattypes.h"
#include "image/videoframetypes.h"
#include "utils.h"

namespace AkVCam
{
    class IpcBridgePrivate;
    class VideoFormat;
    class VideoFrame;

    enum ControlType
    {
        ControlTypeInteger,
        ControlTypeBoolean,
        ControlTypeMenu,
    };

    struct DeviceControl
    {
        std::string id;
        std::string description;
        ControlType type;
        int minimum;
        int maximum;
        int step;
        int defaultValue;
        int value;
        std::vector<std::string> menu;
    };

    class IpcBridge
    {
        public:
            enum ServerState
            {
                ServerStateAvailable,
                ServerStateGone
            };

            enum StreamType
            {
                StreamTypeOutput,
                StreamTypeInput
            };

            AKVCAM_SIGNAL(ServerStateChanged,
                          ServerState state)
            AKVCAM_SIGNAL(FrameReady,
                          const std::string &deviceId,
                          const VideoFrame &frame)
            AKVCAM_SIGNAL(PictureChanged,
                          const std::string &picture)
            AKVCAM_SIGNAL(DevicesChanged,
                          const std::vector<std::string> &devices)
            AKVCAM_SIGNAL(ListenerAdded,
                          const std::string &deviceId,
                          const std::string &listener)
            AKVCAM_SIGNAL(ListenerRemoved,
                          const std::string &deviceId,
                          const std::string &listener)
            AKVCAM_SIGNAL(BroadcastingChanged,
                          const std::string &deviceId,
                          const std::string &broadcaster)
            AKVCAM_SIGNAL(ControlsChanged,
                          const std::string &deviceId,
                          const std::map<std::string, int> &controls)

        public:
            IpcBridge();
            ~IpcBridge();

            /* Server & Client */

            std::string picture() const;
            void setPicture(const std::string &picture);
            int logLevel() const;
            void setLogLevel(int logLevel);

            // Register the peer to the global server.
            bool registerPeer();

            // Unregister the peer to the global server.
            void unregisterPeer();

            // List available devices.
            std::vector<std::string> devices() const;

            // Return human readable description of the device.
            std::string description(const std::string &deviceId) const;
            void setDescription(const std::string &deviceId,
                                const std::string &description);

            // Pixel formats supported by the driver.
            std::vector<PixelFormat> supportedPixelFormats(StreamType type) const;

            // Default pixel format of the driver.
            PixelFormat defaultPixelFormat(StreamType type) const;

            // Return supported formats for the device.
            std::vector<VideoFormat> formats(const std::string &deviceId) const;
            void setFormats(const std::string &deviceId,
                            const std::vector<VideoFormat> &formats);

            // Return return the status of the device.
            std::string broadcaster(const std::string &deviceId) const;

            std::vector<DeviceControl> controls(const std::string &deviceId);
            void setControls(const std::string &deviceId,
                             const std::map<std::string, int> &controls);

            // Returns the clients that are capturing from a virtual camera.
            std::vector<std::string> listeners(const std::string &deviceId);

            // Returns clients PIDs using the virtual devices.
            std::vector<uint64_t> clientsPids() const;

            // Returns client executable from PID.
            std::string clientExe(uint64_t pid) const;

            /* Server */

            std::string addDevice(const std::string &description);
            void removeDevice(const std::string &deviceId);
            void addFormat(const std::string &deviceId,
                           const VideoFormat &format,
                           int index=-1);
            void removeFormat(const std::string &deviceId, int index);
            void updateDevices();

            // Start frame transfer to the device.
            bool deviceStart(const std::string &deviceId,
                             const VideoFormat &format);

            // Stop frame transfer to the device.
            void deviceStop(const std::string &deviceId);

            // Transfer a frame to the device.
            bool write(const std::string &deviceId,
                       const VideoFrame &frame);

            /* Client */

            // Increment the count of device listeners
            bool addListener(const std::string &deviceId);

            // Decrement the count of device listeners
            bool removeListener(const std::string &deviceId);

            bool needsRoot(const std::string &operation) const;
            int sudo(int argc, char **argv) const;

        private:
            IpcBridgePrivate *d;

        friend class IpcBridgePrivate;
    };
}

#endif // IPCBRIDGE_H
