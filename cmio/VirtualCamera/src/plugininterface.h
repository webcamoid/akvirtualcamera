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

#ifndef PLUGININTERFACE_H
#define PLUGININTERFACE_H

#include "VCamUtils/src/ipcbridge.h"
#include "device.h"

namespace AkVCam
{
    struct PluginInterfacePrivate;

    class PluginInterface: public ObjectInterface
    {
        public:
            PluginInterface();
            PluginInterface(const PluginInterface &other) = delete;
            ~PluginInterface();

            CMIOObjectID objectID() const;
            static CMIOHardwarePlugInRef create();
            Object *findObject(CMIOObjectID objectID);

            HRESULT QueryInterface(REFIID uuid, LPVOID *interface);
            OSStatus Initialize();
            OSStatus InitializeWithObjectID(CMIOObjectID objectID);
            OSStatus Teardown();

        private:
            PluginInterfacePrivate *d;
            CMIOObjectID m_objectID;
            std::vector<DevicePtr> m_devices;

            static void serverStateChanged(void *userData,
                                           IpcBridge::ServerState state);
            static void devicesChanged(void *userData,
                                       const std::vector<std::string> &devices);
            static void frameReady(void *userData,
                                   const std::string &deviceId,
                                   const VideoFrame &frame);
            static void pictureChanged(void *userData,
                                       const std::string &picture);
            static void setBroadcasting(void *userData,
                                        const std::string &deviceId,
                                        const std::string &broadcaster);
            static void controlsChanged(void *userData,
                                        const std::string &deviceId,
                                        const std::map<std::string, int> &controls);
            static void addListener(void *userData,
                                    const std::string &deviceId);
            static void removeListener(void *userData,
                                       const std::string &deviceId);
            bool createDevice(const std::string &deviceId,
                              const std::wstring &description,
                              const std::vector<VideoFormat> &formats);
            void destroyDevice(const std::string &deviceId);

        friend struct PluginInterfacePrivate;
    };
}

#endif // PLUGININTERFACE_H
