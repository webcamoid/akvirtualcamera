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

#ifndef AKVCAMUTILS_IPCBRIDGE_H
#define AKVCAMUTILS_IPCBRIDGE_H

#include <string>
#include <map>
#include <memory>

#include "videoformattypes.h"
#include "videoframetypes.h"
#include "utils.h"

namespace AkVCam
{
    class IpcBridge;
    class IpcBridgePrivate;
    class VideoFormat;
    class VideoFrame;

    enum ControlType
    {
        ControlTypeUnknown = -1,
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

    using IpcBridgePtr = std::shared_ptr<IpcBridge>;

    class IpcBridge
    {
        public:
            enum StreamType
            {
                StreamType_Output,
                StreamType_Input
            };

            AKVCAM_SIGNAL(FrameReady,
                          const std::string &deviceId,
                          const VideoFrame &frame,
                          bool isActive)
            AKVCAM_SIGNAL(PictureChanged,
                          const std::string &picture)
            AKVCAM_SIGNAL(DevicesChanged,
                          const std::vector<std::string> &devices)
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
            std::string logPath(const std::string &logName={}) const;
            void stopNotifications();

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

            std::vector<DeviceControl> controls(const std::string &deviceId);
            void setControls(const std::string &deviceId,
                             const std::map<std::string, int> &controls);

            // Returns clients PIDs using the virtual devices.
            std::vector<uint64_t> clientsPids() const;

            // Returns client executable from PID.
            std::string clientExe(uint64_t pid) const;

            /* Server */

            std::string addDevice(const std::string &description,
                                  const std::string &deviceId={});
            void removeDevice(const std::string &deviceId);
            void addFormat(const std::string &deviceId,
                           const VideoFormat &format,
                           int index=-1);
            void removeFormat(const std::string &deviceId, int index);
            void updateDevices();

            // Start frame transfer to the device.
            bool deviceStart(StreamType type, const std::string &deviceId);

            // Stop frame transfer to the device.
            void deviceStop(const std::string &deviceId);

            // Transfer a frame to the device.
            bool write(const std::string &deviceId, const VideoFrame &frame);

            /* Client */

            bool isBusyFor(const std::string &operation) const;
            bool needsRoot(const std::string &operation) const;

            /* Hacks */

            std::vector<std::string> hacks() const;
            std::string hackDescription(const std::string &hack) const;
            bool hackIsSafe(const std::string &hack) const;
            bool hackNeedsRoot(const std::string &hack) const;
            int execHack(const std::string &hack,
                         const std::vector<std::string> &args);

        private:
            IpcBridgePrivate *d;

        friend class IpcBridgePrivate;
    };
}

#endif // AKVCAMUTILS_IPCBRIDGE_H
