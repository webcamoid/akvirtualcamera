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
#include <cmath>
#include <codecvt>
#include <fstream>
#include <locale>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <windows.h>
#include <psapi.h>

#include "PlatformUtils/src/messageserver.h"
#include "PlatformUtils/src/mutex.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/sharedmemory.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/logger.h"

#ifndef SERVICE_NOTIFY_STOPPED
#define SERVICE_NOTIFY_STOPPED 0x1
#endif

#ifndef SERVICE_NOTIFY_RUNNING
#define SERVICE_NOTIFY_RUNNING 0x8
#endif

#ifndef SERVICE_NOTIFY_STATUS_CHANGE
#define SERVICE_NOTIFY_STATUS_CHANGE 2
#endif

namespace AkVCam
{
    using RegisterServerFunc = HRESULT (WINAPI *)();

    struct DeviceSharedProperties
    {
        SharedMemory sharedMemory;
        Mutex mutex;
    };

    class Hack
    {
        public:
            using HackFunc = std::function<int (const std::vector<std::string> &args)>;

            std::string name;
            std::string description;
            bool isSafe {false};
            bool needsRoot {false};
            HackFunc func;

            Hack();
            Hack(const std::string &name,
                 const std::string &description,
                 bool isSafe,
                 bool needsRoot,
                 const HackFunc &func);
            Hack(const Hack &other);
            Hack &operator =(const Hack &other);
    };

    class IpcBridgePrivate
    {
        public:
            IpcBridge *self;
            std::string m_portName;
            std::map<std::string, DeviceSharedProperties> m_devices;
            std::map<uint32_t, MessageHandler> m_messageHandlers;
            std::vector<std::string> m_broadcasting;
            MessageServer m_messageServer;
            MessageServer m_mainServer;
            SharedMemory m_sharedMemory;
            Mutex m_globalMutex;
            SC_HANDLE m_scManager {nullptr};
            SC_HANDLE m_assistantService {nullptr};
            SERVICE_NOTIFY m_notifyBuffer;

            explicit IpcBridgePrivate(IpcBridge *self);
            ~IpcBridgePrivate();

            inline const std::vector<DeviceControl> &controls() const;
            void updateDevices(bool propagate);
            void updateDeviceSharedProperties();
            void updateDeviceSharedProperties(const std::string &deviceId,
                                              const std::string &owner);
            bool isServiceRunning() const;
            void startServiceStatusCheck();
            void stopServiceStatusCheck();
            static void CALLBACK notifyCallback(void *parameter);

            // Message handling methods
            void isAlive(Message *message);
            void deviceUpdate(Message *message);
            void frameReady(Message *message);
            void pictureUpdated(Message *message);
            void setBroadcasting(Message *message);
            void controlsUpdated(Message *message);
            void listenerAdd(Message *message);
            void listenerRemove (Message *message);

            bool isRoot() const;
            int sudo(const std::vector<std::string> &parameters,
                     const std::string &directory={},
                     bool show=false);
            std::string assistant() const;
            std::string manager() const;
            std::string alternativeManager() const;
            std::string alternativePlugin() const;
            std::string service() const;

            // Hacks
            const std::vector<Hack> &hacks();
            int setServiceUp(const std::vector<std::string> &args);
            int setServiceDown(const std::vector<std::string> &args);
    };

    static const int maxFrameWidth = 1920;
    static const int maxFrameHeight = 1080;
    static const size_t maxFrameSize = maxFrameWidth * maxFrameHeight;
    static const size_t maxBufferSize = sizeof(Frame) + 3 * maxFrameSize;
}

AkVCam::IpcBridge::IpcBridge()
{
    AkLogFunction();
    this->d = new IpcBridgePrivate(this);
    auto loglevel = AkVCam::Preferences::logLevel();
    AkVCam::Logger::setLogLevel(loglevel);
    this->d->m_mainServer.start();
    this->registerPeer();
    this->d->updateDeviceSharedProperties();
    this->d->startServiceStatusCheck();
}

AkVCam::IpcBridge::~IpcBridge()
{
    this->d->stopServiceStatusCheck();
    this->unregisterPeer();
    this->d->m_mainServer.stop();
    delete this->d;
}

std::string AkVCam::IpcBridge::picture() const
{
    return Preferences::picture();
}

void AkVCam::IpcBridge::setPicture(const std::string &picture)
{
    AkLogFunction();
    Preferences::setPicture(picture);
    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_PICTURE_UPDATED;
    message.dataSize = sizeof(MsgPictureUpdated);
    auto data = messageData<MsgPictureUpdated>(&message);
    memcpy(data->picture,
           picture.c_str(),
           (std::min<size_t>)(picture.size(), MAX_STRING));
    this->d->m_mainServer.sendMessage(&message);
}

int AkVCam::IpcBridge::logLevel() const
{
    return Preferences::logLevel();
}

void AkVCam::IpcBridge::setLogLevel(int logLevel)
{
    AkLogFunction();
    Preferences::setLogLevel(logLevel);
    Logger::setLogLevel(logLevel);
}

bool AkVCam::IpcBridge::registerPeer()
{
    AkLogFunction();

    if (!this->d->m_portName.empty())
        return true;

    AkLogDebug() << "Requesting port." << std::endl;

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_REQUEST_PORT;
    message.dataSize = sizeof(MsgRequestPort);
    auto requestData = messageData<MsgRequestPort>(&message);

    if (!MessageServer::sendMessage("\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME,
                                    &message))
        return false;

    std::string portName(requestData->port);
    AkLogInfo() << "Recommended port name: " << portName << std::endl;

    if (portName.empty()) {
        AkLogError() << "The returned por name is empty." << std::endl;

        return false;
    }

    AkLogDebug() << "Starting message server." << std::endl;
    auto pipeName = "\\\\.\\pipe\\" + portName;
    this->d->m_messageServer.setPipeName(pipeName);
    this->d->m_messageServer.setHandlers(this->d->m_messageHandlers);

    if (!this->d->m_messageServer.start()) {
        AkLogError() << "Can't start message server" << std::endl;

        return false;
    }

    AkLogInfo() << "Registering port: " << portName << std::endl;

    message.clear();
    message.messageId = AKVCAM_ASSISTANT_MSG_ADD_PORT;
    message.dataSize = sizeof(MsgAddPort);
    auto addData = messageData<MsgAddPort>(&message);
    memcpy(addData->port,
           portName.c_str(),
           (std::min<size_t>)(portName.size(), MAX_STRING));
    memcpy(addData->pipeName,
           pipeName.c_str(),
           (std::min<size_t>)(pipeName.size(), MAX_STRING));

    if (!MessageServer::sendMessage("\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME,
                                    &message)) {
        AkLogError() << "Failed registering port." << std::endl;
        this->d->m_messageServer.stop();

        return false;
    }

    if (!addData->status) {
        AkLogError() << "Failed registering port." << std::endl;
        this->d->m_messageServer.stop();

        return false;
    }

    this->d->m_sharedMemory.setName("Local\\" + portName + ".data");
    this->d->m_globalMutex = Mutex(portName + ".mutex");
    this->d->m_portName = portName;
    AkLogInfo() << "Peer registered as " << portName << std::endl;

    AkLogInfo() << "SUCCESSFUL" << std::endl;
    this->d->updateDevices(false);

    return true;
}

void AkVCam::IpcBridge::unregisterPeer()
{
    AkLogFunction();

    if (this->d->m_portName.empty())
        return;

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_REMOVE_PORT;
    message.dataSize = sizeof(MsgRemovePort);
    auto data = messageData<MsgRemovePort>(&message);
    memcpy(data->port,
           this->d->m_portName.c_str(),
           (std::min<size_t>)(this->d->m_portName.size(), MAX_STRING));
    MessageServer::sendMessage("\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME,
                               &message);
    this->d->m_messageServer.stop();
    this->d->m_sharedMemory.setName({});
    this->d->m_globalMutex = {};
    this->d->m_portName.clear();
}

std::vector<std::string> AkVCam::IpcBridge::devices() const
{
    AkLogFunction();
    std::vector<std::string> devices;
    auto nCameras = Preferences::camerasCount();
    AkLogInfo() << "Devices:" << std::endl;

    for (size_t i = 0; i < nCameras; i++) {
        auto deviceId = Preferences::cameraPath(i);
        devices.push_back(deviceId);
        AkLogInfo() << "    " << deviceId << std::endl;
    }

    return devices;
}

std::string AkVCam::IpcBridge::description(const std::string &deviceId) const
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex < 0)
        return {};

    return Preferences::cameraDescription(size_t(cameraIndex));
}

void AkVCam::IpcBridge::setDescription(const std::string &deviceId,
                                       const std::string &description)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex >= 0)
        Preferences::cameraSetDescription(size_t(cameraIndex), description);
}

std::vector<AkVCam::PixelFormat> AkVCam::IpcBridge::supportedPixelFormats(StreamType type) const
{
    if (type == StreamTypeInput)
        return {PixelFormatRGB24};

    return {
        PixelFormatRGB32,
        PixelFormatRGB24,
        PixelFormatRGB16,
        PixelFormatRGB15,
        PixelFormatUYVY,
        PixelFormatYUY2,
        PixelFormatNV12
    };
}

AkVCam::PixelFormat AkVCam::IpcBridge::defaultPixelFormat(StreamType type) const
{
    return type == StreamTypeInput?
                PixelFormatRGB24:
                PixelFormatYUY2;
}

std::vector<AkVCam::VideoFormat> AkVCam::IpcBridge::formats(const std::string &deviceId) const
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex < 0)
        return {};

    return Preferences::cameraFormats(size_t(cameraIndex));
}

void AkVCam::IpcBridge::setFormats(const std::string &deviceId,
                                   const std::vector<VideoFormat> &formats)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex >= 0)
        Preferences::cameraSetFormats(size_t(cameraIndex), formats);
}

std::string AkVCam::IpcBridge::broadcaster(const std::string &deviceId) const
{
    AkLogFunction();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING;
    message.dataSize = sizeof(MsgBroadcasting);
    auto data = messageData<MsgBroadcasting>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));

    if (!this->d->m_mainServer.sendMessage(&message))
        return {};

    if (!data->status)
        return {};

    std::string broadcaster(data->broadcaster);

    AkLogInfo() << "Device: " << deviceId << std::endl;
    AkLogInfo() << "Broadcaster: " << broadcaster << std::endl;

    return broadcaster;
}

std::vector<AkVCam::DeviceControl> AkVCam::IpcBridge::controls(const std::string &deviceId)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex < 0)
        return {};

    std::vector<DeviceControl> controls;

    for (auto &control: this->d->controls()) {
        controls.push_back(control);
        controls.back().value =
                Preferences::cameraControlValue(size_t(cameraIndex), control.id);
    }

    return controls;
}

void AkVCam::IpcBridge::setControls(const std::string &deviceId,
                                    const std::map<std::string, int> &controls)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex < 0)
        return;

    bool updated = false;

    for (auto &control: this->d->controls()) {
        auto oldValue =
                Preferences::cameraControlValue(size_t(cameraIndex),
                                                control.id);

        if (controls.count(control.id)) {
            auto newValue = controls.at(control.id);

            if (newValue != oldValue) {
                Preferences::cameraSetControlValue(size_t(cameraIndex),
                                                   control.id,
                                                   newValue);
                updated = true;
            }
        }
    }

    if (!updated)
        return;

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_CONTROLS_UPDATED;
    message.dataSize = sizeof(MsgControlsUpdated);
    auto data = messageData<MsgControlsUpdated>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    this->d->m_mainServer.sendMessage(&message);
}

std::vector<std::string> AkVCam::IpcBridge::listeners(const std::string &deviceId)
{
    AkLogFunction();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_LISTENERS;
    message.dataSize = sizeof(MsgListeners);
    auto data = messageData<MsgListeners>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));

    if (!this->d->m_mainServer.sendMessage(&message))
        return {};

    if (!data->status)
        return {};

    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER;
    std::vector<std::string> listeners;

    for (size_t i = 0; i < data->nlistener; i++) {
        data->nlistener = i;

        if (!this->d->m_mainServer.sendMessage(&message))
            continue;

        if (!data->status)
            continue;

        listeners.push_back(std::string(data->listener));
    }

    return listeners;
}

std::vector<uint64_t> AkVCam::IpcBridge::clientsPids() const
{
    AkLogFunction();
    auto pluginPath = locatePluginPath();
    AkLogDebug() << "Plugin path: " << pluginPath << std::endl;

    if (pluginPath.empty())
        return {};

    std::vector<std::string> plugins;

    // First check for the existence of the main plugin binary.
    auto path = pluginPath + "\\" DSHOW_PLUGIN_NAME ".dll";
    AkLogDebug() << "Plugin binary: " << path << std::endl;

    if (fileExists(path))
        plugins.push_back(path);

    // Check if the alternative architecture plugin exists.
    auto altPlugin = this->d->alternativePlugin();

    if (!altPlugin.empty())
        plugins.push_back(path);

    if (plugins.empty())
        return {};

    std::vector<uint64_t> pids;

    const DWORD nElements = 4096;
    DWORD process[nElements];
    memset(process, 0, nElements * sizeof(DWORD));
    DWORD needed = 0;

    if (!EnumProcesses(process, nElements * sizeof(DWORD), &needed))
        return {};

    size_t nProcess = needed / sizeof(DWORD);
    auto currentPid = GetCurrentProcessId();

    for (size_t i = 0; i < nProcess; i++) {
        auto processHnd = OpenProcess(PROCESS_QUERY_INFORMATION
                                      | PROCESS_VM_READ,
                                      FALSE,
                                      process[i]);

        if (!processHnd)
            continue;

        char processName[MAX_PATH];
        memset(&processName, 0, sizeof(MAX_PATH));

        if (GetModuleBaseNameA(processHnd,
                               nullptr,
                               processName,
                               MAX_PATH))
            AkLogDebug() << "Enumerating modules for '" << processName << "'" << std::endl;

        HMODULE modules[nElements];
        memset(modules, 0, nElements * sizeof(HMODULE));

        if (EnumProcessModules(processHnd,
                               modules,
                               nElements * sizeof(HMODULE),
                               &needed)) {
            size_t nModules =
                    std::min<DWORD>(needed / sizeof(HMODULE), nElements);

            for (size_t j = 0; j < nModules; j++) {
                CHAR moduleName[MAX_PATH];
                memset(moduleName, 0, MAX_PATH * sizeof(CHAR));

                if (GetModuleFileNameExA(processHnd,
                                         modules[j],
                                         moduleName,
                                         MAX_PATH)) {
                    auto it = std::find(plugins.begin(),
                                        plugins.end(),
                                        moduleName);

                    if (it != plugins.end()) {
                        auto pidsIt = std::find(pids.begin(),
                                                pids.end(),
                                                process[i]);

                        if (process[i] > 0
                            && pidsIt == pids.end()
                            && process[i] != currentPid)
                            pids.push_back(process[i]);
                    }
                }
            }
        }

        CloseHandle(processHnd);
    }

    return pids;
}

std::string AkVCam::IpcBridge::clientExe(uint64_t pid) const
{
    std::string exe;
    auto processHnd = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,
                                  FALSE,
                                  DWORD(pid));
    if (processHnd) {
        CHAR exeName[MAX_PATH];
        memset(exeName, 0, MAX_PATH * sizeof(CHAR));
        auto size =
                GetModuleFileNameExA(processHnd, nullptr, exeName, MAX_PATH);

        if (size > 0)
            exe = std::string(exeName, size);

        CloseHandle(processHnd);
    }

    return exe;
}

std::string AkVCam::IpcBridge::addDevice(const std::string &description)
{
    AkLogFunction();

    return Preferences::addDevice(description);
}

void AkVCam::IpcBridge::removeDevice(const std::string &deviceId)
{
    AkLogFunction();

    Preferences::removeCamera(deviceId);
}

void AkVCam::IpcBridge::addFormat(const std::string &deviceId,
                                  const VideoFormat &format,
                                  int index)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex >= 0)
        Preferences::cameraAddFormat(size_t(cameraIndex),
                                     format,
                                     index);
}

void AkVCam::IpcBridge::removeFormat(const std::string &deviceId, int index)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex >= 0)
        Preferences::cameraRemoveFormat(size_t(cameraIndex),
                                        index);
}

void AkVCam::IpcBridge::updateDevices()
{
    AkLogFunction();
    auto pluginPath = locatePluginPath();
    AkLogDebug() << "Plugin path: " << pluginPath << std::endl;

    if (pluginPath.empty())
        return;

    auto path = pluginPath + "\\" DSHOW_PLUGIN_NAME ".dll";
    AkLogDebug() << "Plugin binary: " << path << std::endl;

    if (!fileExists(path)) {
        AkLogError() << "Plugin binary not found: " << path << std::endl;

        return;
    }

    if (auto hmodule = LoadLibraryA(path.c_str())) {
        auto registerServer =
                RegisterServerFunc(GetProcAddress(hmodule, "DllRegisterServer"));

        if (registerServer) {
            AkLogDebug() << "Registering server" << std::endl;
            auto result = (*registerServer)();
            AkLogDebug() << "Server registered with code " << result << std::endl;
            this->d->updateDevices(true);
            auto lockFileName = tempPath() + "\\akvcam_update.lck";

            if (!fileExists(lockFileName)) {
                std::ofstream lockFile;
                lockFile.open(lockFileName);
                lockFile << std::endl;
                lockFile.close();
                auto altManager = this->d->alternativeManager();

                if (!altManager.empty())
                    this->d->sudo({altManager, "update"});

                DeleteFileA(lockFileName.c_str());
            }
        } else {
            AkLogError() << "Can't locate DllRegisterServer function." << std::endl;
        }

        FreeLibrary(hmodule);
    } else {
        AkLogError() << "Error loading plugin binary: " << path << std::endl;
    }
}

bool AkVCam::IpcBridge::deviceStart(const std::string &deviceId,
                                    const VideoFormat &format)
{
    UNUSED(format);
    AkLogFunction();
    auto it = std::find(this->d->m_broadcasting.begin(),
                        this->d->m_broadcasting.end(),
                        deviceId);

    if (it != this->d->m_broadcasting.end()) {
        AkLogError() << '\'' << deviceId << "' is busy." << std::endl;

        return false;
    }

    this->d->m_sharedMemory.setName("Local\\" + this->d->m_portName + ".data");
    this->d->m_globalMutex = Mutex(this->d->m_portName + ".mutex");

    if (!this->d->m_sharedMemory.open(maxBufferSize,
                                      SharedMemory::OpenModeWrite)) {
        AkLogError() << "Can't open shared memory for writing." << std::endl;

        return false;
    }

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING;
    message.dataSize = sizeof(MsgBroadcasting);
    auto data = messageData<MsgBroadcasting>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    memcpy(data->broadcaster,
           this->d->m_portName.c_str(),
           (std::min<size_t>)(this->d->m_portName.size(), MAX_STRING));

    if (!this->d->m_mainServer.sendMessage(&message)) {
        AkLogError() << "Error sending message." << std::endl;
        this->d->m_sharedMemory.close();

        return false;
    }

    this->d->m_broadcasting.push_back(deviceId);

    return true;
}

void AkVCam::IpcBridge::deviceStop(const std::string &deviceId)
{
    AkLogFunction();
    auto it = std::find(this->d->m_broadcasting.begin(),
                        this->d->m_broadcasting.end(),
                        deviceId);

    if (it == this->d->m_broadcasting.end())
        return;

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING;
    message.dataSize = sizeof(MsgBroadcasting);
    auto data = messageData<MsgBroadcasting>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));

    this->d->m_mainServer.sendMessage(&message);
    this->d->m_sharedMemory.close();
    this->d->m_broadcasting.erase(it);
}

bool AkVCam::IpcBridge::write(const std::string &deviceId,
                              const VideoFrame &frame)
{
    AkLogFunction();

    if (frame.format().size() < 1)
        return false;

    auto buffer =
            reinterpret_cast<Frame *>(this->d->m_sharedMemory.lock(&this->d->m_globalMutex));

    if (!buffer)
        return false;

    if (size_t(frame.format().width() * frame.format().height()) > maxFrameSize) {
        auto scaledFrame = frame.scaled(maxFrameSize);
        buffer->format = scaledFrame.format().fourcc();
        buffer->width = scaledFrame.format().width();
        buffer->height = scaledFrame.format().height();
        buffer->size = uint32_t(frame.data().size());
        memcpy(buffer->data,
               scaledFrame.data().data(),
               scaledFrame.data().size());
    } else {
        buffer->format = frame.format().fourcc();
        buffer->width = frame.format().width();
        buffer->height = frame.format().height();
        buffer->size = uint32_t(frame.data().size());
        memcpy(buffer->data,
               frame.data().data(),
               frame.data().size());
    }

    this->d->m_sharedMemory.unlock(&this->d->m_globalMutex);

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_FRAME_READY;
    message.dataSize = sizeof(MsgFrameReady);
    auto data = messageData<MsgFrameReady>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    memcpy(data->port,
           this->d->m_portName.c_str(),
           (std::min<size_t>)(this->d->m_portName.size(), MAX_STRING));

    return this->d->m_mainServer.sendMessage(&message) == TRUE;
}

bool AkVCam::IpcBridge::addListener(const std::string &deviceId)
{
    AkLogFunction();
    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD;
    message.dataSize = sizeof(MsgListeners);
    auto data = messageData<MsgListeners>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    memcpy(data->listener,
           this->d->m_portName.c_str(),
           (std::min<size_t>)(this->d->m_portName.size(), MAX_STRING));

    if (!this->d->m_mainServer.sendMessage(&message))
        return false;

    return data->status;
}

bool AkVCam::IpcBridge::removeListener(const std::string &deviceId)
{
    AkLogFunction();
    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE;
    message.dataSize = sizeof(MsgListeners);
    auto data = messageData<MsgListeners>(&message);
    memcpy(data->device,
           deviceId.c_str(),
           (std::min<size_t>)(deviceId.size(), MAX_STRING));
    memcpy(data->listener,
           this->d->m_portName.c_str(),
           (std::min<size_t>)(this->d->m_portName.size(), MAX_STRING));

    if (!this->d->m_mainServer.sendMessage(&message))
        return false;

    return data->status;
}

bool AkVCam::IpcBridge::needsRoot(const std::string &operation) const
{
    static const std::vector<std::string> operations {
        "add-device",
        "add-format",
        "load",
        "remove-device",
        "remove-devices",
        "remove-format",
        "remove-formats",
        "set-description",
        "set-loglevel",
        "update"
    };

    auto it = std::find(operations.begin(), operations.end(), operation);

    return it != operations.end() && !this->d->isRoot();
}

int AkVCam::IpcBridge::sudo(int argc, char **argv) const
{
    AkLogFunction();
    std::vector<std::string> arguments;

    for (int i = 0; i < argc; i++)
        arguments.push_back(argv[i]);

    return this->d->sudo(arguments);
}

std::vector<std::string> AkVCam::IpcBridge::hacks() const
{
    std::vector<std::string> hacks;

    for (auto &hack: this->d->hacks())
        hacks.push_back(hack.name);

    return hacks;
}

std::string AkVCam::IpcBridge::hackDescription(const std::string &hack) const
{
    for (auto &hck: this->d->hacks())
        if (hck.name == hack)
            return hck.description;

    return {};
}

bool AkVCam::IpcBridge::hackIsSafe(const std::string &hack) const
{
    for (auto &hck: this->d->hacks())
        if (hck.name == hack)
            return hck.isSafe;

    return true;
}

bool AkVCam::IpcBridge::hackNeedsRoot(const std::string &hack) const
{
    for (auto &hck: this->d->hacks())
        if (hck.name == hack)
            return hck.needsRoot && !this->d->isRoot();

    return false;
}

int AkVCam::IpcBridge::execHack(const std::string &hack,
                                const std::vector<std::string> &args)
{
    for (auto &hck: this->d->hacks())
        if (hck.name == hack)
            return hck.func(args);

    return 0;
}

AkVCam::IpcBridgePrivate::IpcBridgePrivate(IpcBridge *self):
    self(self)
{
    this->m_mainServer.setPipeName("\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME);
    this->m_mainServer.setMode(MessageServer::ServerModeSend);
    this->m_messageHandlers = std::map<uint32_t, MessageHandler> {
        {AKVCAM_ASSISTANT_MSG_ISALIVE                , AKVCAM_BIND_FUNC(IpcBridgePrivate::isAlive)        },
        {AKVCAM_ASSISTANT_MSG_FRAME_READY            , AKVCAM_BIND_FUNC(IpcBridgePrivate::frameReady)     },
        {AKVCAM_ASSISTANT_MSG_PICTURE_UPDATED        , AKVCAM_BIND_FUNC(IpcBridgePrivate::pictureUpdated) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_UPDATE          , AKVCAM_BIND_FUNC(IpcBridgePrivate::deviceUpdate)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD    , AKVCAM_BIND_FUNC(IpcBridgePrivate::listenerAdd)    },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE , AKVCAM_BIND_FUNC(IpcBridgePrivate::listenerRemove) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING , AKVCAM_BIND_FUNC(IpcBridgePrivate::setBroadcasting)},
        {AKVCAM_ASSISTANT_MSG_DEVICE_CONTROLS_UPDATED, AKVCAM_BIND_FUNC(IpcBridgePrivate::controlsUpdated)},
    };
}

AkVCam::IpcBridgePrivate::~IpcBridgePrivate()
{
}

const std::vector<AkVCam::DeviceControl> &AkVCam::IpcBridgePrivate::controls() const
{
    static const std::vector<std::string> scalingMenu {
        "Fast",
        "Linear"
    };
    static const std::vector<std::string> aspectRatioMenu {
        "Ignore",
        "Keep",
        "Expanding"
    };
    static const auto scalingMax = int(scalingMenu.size()) - 1;
    static const auto aspectRatioMax = int(aspectRatioMenu.size()) - 1;

    static const std::vector<DeviceControl> controls {
        {"hflip"       , "Horizontal Mirror", ControlTypeBoolean, 0, 1             , 1, 0, 0, {}             },
        {"vflip"       , "Vertical Mirror"  , ControlTypeBoolean, 0, 1             , 1, 0, 0, {}             },
        {"scaling"     , "Scaling"          , ControlTypeMenu   , 0, scalingMax    , 1, 0, 0, scalingMenu    },
        {"aspect_ratio", "Aspect Ratio"     , ControlTypeMenu   , 0, aspectRatioMax, 1, 0, 0, aspectRatioMenu},
        {"swap_rgb"    , "Swap RGB"         , ControlTypeBoolean, 0, 1             , 1, 0, 0, {}             },
    };

    return controls;
}

void AkVCam::IpcBridgePrivate::updateDevices(bool propagate)
{
    AkLogFunction();

    Message message;
    message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_UPDATE;
    message.dataSize = sizeof(MsgDevicesUpdated);
    auto data = messageData<MsgDevicesUpdated>(&message);
    data->propagate = propagate;
    this->m_mainServer.sendMessage(&message);
}

void AkVCam::IpcBridgePrivate::updateDeviceSharedProperties()
{
    AkLogFunction();

    for (size_t i = 0; i < Preferences::camerasCount(); i++) {
        auto path = Preferences::cameraPath(i);
        Message message;
        message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING;
        message.dataSize = sizeof(MsgBroadcasting);
        auto data = messageData<MsgBroadcasting>(&message);
        memcpy(data->device,
               path.c_str(),
               (std::min<size_t>)(path.size(), MAX_STRING));
        this->m_mainServer.sendMessage(&message);
        this->updateDeviceSharedProperties(path,
                                           std::string(data->broadcaster));
    }
}

void AkVCam::IpcBridgePrivate::updateDeviceSharedProperties(const std::string &deviceId,
                                                            const std::string &owner)
{
    AkLogFunction();

    if (owner.empty()) {
        this->m_devices[deviceId] = {SharedMemory(), Mutex()};
    } else {
        Mutex mutex(owner + ".mutex");
        SharedMemory sharedMemory;
        sharedMemory.setName("Local\\" + owner + ".data");

        if (sharedMemory.open())
            this->m_devices[deviceId] = {sharedMemory, mutex};
    }
}

bool AkVCam::IpcBridgePrivate::isServiceRunning() const
{
    AkLogFunction();
    bool isRunning = false;
    auto manager = OpenSCManager(nullptr, nullptr, GENERIC_READ);

    if (manager) {
        auto service = OpenService(manager,
                                   TEXT(DSHOW_PLUGIN_ASSISTANT_NAME),
                                   SERVICE_QUERY_STATUS);

        if (service) {
            SERVICE_STATUS status;
            memset(&status, 0, sizeof(SERVICE_STATUS));
            QueryServiceStatus(service, &status);
            isRunning = status.dwCurrentState == SERVICE_RUNNING;
            CloseServiceHandle(service);
        }

        CloseServiceHandle(manager);
    }

    return isRunning;
}

void AkVCam::IpcBridgePrivate::startServiceStatusCheck()
{
    AkLogFunction();
    this->m_scManager = OpenSCManager(nullptr,
                                      nullptr,
                                      SC_MANAGER_CONNECT);

    if (!this->m_scManager)
        return;

    this->m_assistantService =
            OpenService(this->m_scManager,
                        TEXT(DSHOW_PLUGIN_ASSISTANT_NAME),
                        SERVICE_QUERY_STATUS);

    if (!this->m_assistantService) {
        CloseServiceHandle(this->m_scManager);
        this->m_scManager = nullptr;

        return;
    }

    memset(&this->m_notifyBuffer, 0, sizeof(SERVICE_NOTIFY));
    this->m_notifyBuffer.dwVersion = SERVICE_NOTIFY_STATUS_CHANGE;
    this->m_notifyBuffer.pfnNotifyCallback =
            PFN_SC_NOTIFY_CALLBACK(notifyCallback);
    this->m_notifyBuffer.pContext = this;
    NotifyServiceStatusChange(this->m_assistantService,
                              SERVICE_NOTIFY_RUNNING | SERVICE_NOTIFY_STOPPED,
                              &this->m_notifyBuffer);
}

void AkVCam::IpcBridgePrivate::stopServiceStatusCheck()
{
    if (this->m_assistantService) {
        CloseServiceHandle(this->m_assistantService);
        this->m_assistantService = nullptr;
    }

    if (this->m_scManager) {
        CloseServiceHandle(this->m_scManager);
        this->m_scManager = nullptr;
    }
}

void AkVCam::IpcBridgePrivate::notifyCallback(void *parameter)
{
    AkLogFunction();
    auto serviceNotify = PSERVICE_NOTIFY(parameter);
    auto self = reinterpret_cast<IpcBridgePrivate *>(serviceNotify->pContext);

    if (!self)
        return;

    if (serviceNotify->ServiceStatus.dwCurrentState == SERVICE_RUNNING) {
        AkLogInfo() << "Server Available" << std::endl;

        if (self->self->registerPeer()) {
            AKVCAM_EMIT(self->self,
                        ServerStateChanged,
                        IpcBridge::ServerStateAvailable)
        }
    } else {
        AkLogWarning() << "Server Gone" << std::endl;
        AKVCAM_EMIT(self->self,
                    ServerStateChanged,
                    IpcBridge::ServerStateGone)
        self->self->unregisterPeer();
    }
}

void AkVCam::IpcBridgePrivate::isAlive(Message *message)
{
    AkLogFunction();
    auto data = messageData<MsgIsAlive>(message);
    data->alive = true;
}

void AkVCam::IpcBridgePrivate::deviceUpdate(Message *message)
{
    UNUSED(message);
    AkLogFunction();
    std::vector<std::string> devices;
    auto nCameras = Preferences::camerasCount();

    for (size_t i = 0; i < nCameras; i++)
        devices.push_back(Preferences::cameraPath(i));

    AKVCAM_EMIT(this->self, DevicesChanged, devices)
}

void AkVCam::IpcBridgePrivate::frameReady(Message *message)
{
    AkLogFunction();
    auto data = messageData<MsgFrameReady>(message);
    std::string deviceId(data->device);

    if (this->m_devices.count(deviceId) < 1) {
        this->updateDeviceSharedProperties(deviceId, std::string(data->port));

        return;
    }

    auto frame =
            reinterpret_cast<Frame *>(this->m_devices[deviceId]
                                      .sharedMemory
                                      .lock(&this->m_devices[deviceId].mutex));

    if (!frame)
        return;

    VideoFormat videoFormat(frame->format, frame->width, frame->height);
    VideoFrame videoFrame(videoFormat);
    memcpy(videoFrame.data().data(), frame->data, frame->size);
    this->m_devices[deviceId].sharedMemory.unlock(&this->m_devices[deviceId].mutex);
    AKVCAM_EMIT(this->self, FrameReady, deviceId, videoFrame)
}

void AkVCam::IpcBridgePrivate::pictureUpdated(Message *message)
{
    AkLogFunction();
    auto data = messageData<MsgPictureUpdated>(message);
    AKVCAM_EMIT(this->self, PictureChanged, std::string(data->picture))
}

void AkVCam::IpcBridgePrivate::setBroadcasting(Message *message)
{
    AkLogFunction();
    auto data = messageData<MsgBroadcasting>(message);
    std::string deviceId(data->device);
    std::string broadcaster(data->broadcaster);
    this->updateDeviceSharedProperties(deviceId, broadcaster);
    AKVCAM_EMIT(this->self, BroadcastingChanged, deviceId, broadcaster)
}

void AkVCam::IpcBridgePrivate::controlsUpdated(Message *message)
{
    AkLogFunction();
    auto data = messageData<MsgControlsUpdated>(message);
    std::string deviceId(data->device);
    auto cameraIndex = Preferences::cameraFromPath(deviceId);

    if (cameraIndex < 0)
        return;

    std::map<std::string, int> controls;

    for (auto &control: this->controls()) {
        controls[control.id] =
                Preferences::cameraControlValue(size_t(cameraIndex), control.id);
        AkLogDebug() << control.id << ": " << controls[control.id] << std::endl;
    }

    AKVCAM_EMIT(this->self, ControlsChanged, deviceId, controls)
}

void AkVCam::IpcBridgePrivate::listenerAdd(Message *message)
{
    AkLogFunction();
    auto data = messageData<MsgListeners>(message);
    AKVCAM_EMIT(this->self,
                ListenerAdded,
                std::string(data->device),
                std::string(data->listener))
}

void AkVCam::IpcBridgePrivate::listenerRemove(Message *message)
{
    AkLogFunction();
    auto data = messageData<MsgListeners>(message);
    AKVCAM_EMIT(this->self,
                ListenerRemoved,
                std::string(data->device),
                std::string(data->listener))
}

bool AkVCam::IpcBridgePrivate::isRoot() const
{
    AkLogFunction();
    HANDLE token = nullptr;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return false;

    TOKEN_ELEVATION elevationInfo;
    memset(&elevationInfo, 0, sizeof(TOKEN_ELEVATION));
    DWORD len = 0;

    if (!GetTokenInformation(token,
                             TokenElevation,
                             &elevationInfo,
                             sizeof(TOKEN_ELEVATION),
                             &len)) {
        CloseHandle(token);

        return false;
    }

    CloseHandle(token);

    return elevationInfo.TokenIsElevated;
}

int AkVCam::IpcBridgePrivate::sudo(const std::vector<std::string> &parameters,
                                   const std::string &directory,
                                   bool show)
{
    AkLogFunction();

    if (parameters.size() < 1)
        return E_FAIL;

    auto command = parameters[0];
    std::string params;
    size_t i = 0;

    for (auto &param: parameters) {
        if (i < 1) {
            i++;

            continue;
        }

        if (!params.empty())
            params += ' ';

        auto param_ = replace(param, "\"", "\"\"\"");

        if (param_.find(" ") == std::string::npos)
            params += param_;
        else
            params += "\"" + param_ + "\"";
    }

    AkLogDebug() << "Command: " << command << std::endl;
    AkLogDebug() << "Arguments: " << params << std::endl;

    SHELLEXECUTEINFOA execInfo;
    memset(&execInfo, 0, sizeof(SHELLEXECUTEINFO));
    execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    execInfo.hwnd = nullptr;
    execInfo.lpVerb = "runas";
    execInfo.lpFile = command.data();
    execInfo.lpParameters = params.data();
    execInfo.lpDirectory = directory.data();
    execInfo.nShow = show? SW_SHOWNORMAL: SW_HIDE;
    execInfo.hInstApp = nullptr;
    ShellExecuteExA(&execInfo);

    if (!execInfo.hProcess) {
        AkLogError() << "Failed executing command" << std::endl;

        return E_FAIL;
    }

    WaitForSingleObject(execInfo.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(execInfo.hProcess, &exitCode);
    CloseHandle(execInfo.hProcess);

    if (FAILED(exitCode))
        AkLogError() << "Command failed with code "
                     << exitCode
                     << " ("
                     << stringFromError(exitCode)
                     << ")"
                     << std::endl;

    AkLogError() << "Command exited with code " << exitCode << std::endl;

    return int(exitCode);
}

std::string AkVCam::IpcBridgePrivate::assistant() const
{
    AkLogFunction();
    auto pluginPath = locatePluginPath();
    auto path = realPath(pluginPath + "\\" DSHOW_PLUGIN_ASSISTANT_NAME ".exe");

    if (fileExists(path))
        return path;

#ifdef _WIN64
    path = realPath(pluginPath + "\\..\\x86\\" DSHOW_PLUGIN_ASSISTANT_NAME ".exe");
#else
    SYSTEM_INFO info;
    memset(&info, 0, sizeof(SYSTEM_INFO));
    GetNativeSystemInfo(&info);

    if  (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        return {};

    path = realPath(pluginPath + "\\..\\x64\\" DSHOW_PLUGIN_ASSISTANT_NAME ".exe");
#endif

    return fileExists(path)? path: std::string();
}

std::string AkVCam::IpcBridgePrivate::manager() const
{
    AkLogFunction();
    auto pluginPath = locatePluginPath();
    auto path = realPath(pluginPath + "\\AkVCamManager.dll");

    return fileExists(path)? path: std::string();
}

std::string AkVCam::IpcBridgePrivate::alternativeManager() const
{
    AkLogFunction();
    auto pluginPath = locatePluginPath();

#ifdef _WIN64
    auto path = realPath(pluginPath + "\\..\\x86\\AkVCamManager.dll");
#else
    SYSTEM_INFO info;
    memset(&info, 0, sizeof(SYSTEM_INFO));
    GetNativeSystemInfo(&info);

    if  (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        return {};

    auto path = realPath(pluginPath + "\\..\\x64\\AkVCamManager.dll");
#endif

    return fileExists(path)? path: std::string();
}

std::string AkVCam::IpcBridgePrivate::alternativePlugin() const
{
    AkLogFunction();
    auto pluginPath = locatePluginPath();

#ifdef _WIN64
    auto path = realPath(pluginPath + "\\..\\x86\\" DSHOW_PLUGIN_NAME ".dll");
#else
    SYSTEM_INFO info;
    memset(&info, 0, sizeof(SYSTEM_INFO));
    GetNativeSystemInfo(&info);

    if  (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        return {};

    auto path = realPath(pluginPath + "\\..\\x64\\" DSHOW_PLUGIN_NAME ".dll");
#endif

    return fileExists(path)? path: std::string();
}

std::string AkVCam::IpcBridgePrivate::service() const
{
    std::string path;
    auto manager = OpenSCManager(nullptr, nullptr, GENERIC_READ);

    if (manager) {
        auto service = OpenServiceA(manager,
                                    DSHOW_PLUGIN_ASSISTANT_NAME,
                                    SERVICE_QUERY_CONFIG);

        if (service) {
            DWORD bytesNeeded = 0;
            QueryServiceConfig(service, nullptr, 0, &bytesNeeded);
            auto bufSize = bytesNeeded;
            auto serviceConfig =
                    reinterpret_cast<LPQUERY_SERVICE_CONFIGA>(LocalAlloc(LMEM_FIXED,
                                                                         bufSize));
            if (serviceConfig) {
                if (QueryServiceConfigA(service,
                                        serviceConfig,
                                        bufSize,
                                        &bytesNeeded)) {
                    path = std::string(serviceConfig->lpBinaryPathName);
                }

                LocalFree(serviceConfig);
            }

            CloseServiceHandle(service);
        }

        CloseServiceHandle(manager);
    }

    return path;
}

const std::vector<AkVCam::Hack> &AkVCam::IpcBridgePrivate::hacks()
{
    static const std::vector<AkVCam::Hack> hacks {
        {"set-service-up",
         "Setup and start virtual camera service if isn't working",
         true,
         true,
         AKVCAM_BIND_FUNC(IpcBridgePrivate::setServiceUp)},
        {"set-service-down",
         "Stop and unregister virtual camera service",
         true,
         true,
         AKVCAM_BIND_FUNC(IpcBridgePrivate::setServiceDown)}
    };

    return hacks;
}

int AkVCam::IpcBridgePrivate::setServiceUp(const std::vector<std::string> &args)
{
    UNUSED(args);
    AkLogFunction();

    // If the service is not installed, install it.
    auto servicePath = this->service();

    if (servicePath.empty()) {
        auto assistant = this->assistant();

        if (assistant.empty())
            return -1;

        auto result = this->sudo({assistant, "--install"});

        if (result < 0)
            return result;
    }

    // Start the service.
    bool result = false;
    auto manager = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);

    if (manager) {
        auto service = OpenService(manager,
                                   TEXT(DSHOW_PLUGIN_ASSISTANT_NAME),
                                   SERVICE_START);

        if (service) {
            result = StartService(service, 0, nullptr);
            CloseServiceHandle(service);
        }

        CloseServiceHandle(manager);
    }

    return result? 0: -1;
}

int AkVCam::IpcBridgePrivate::setServiceDown(const std::vector<std::string> &args)
{
    UNUSED(args);
    AkLogFunction();
    auto servicePath = this->service();

    if (servicePath.empty())
        return 0;

    // Stop the service.
    bool stopped = false;
    auto manager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);

    if (manager) {
        auto service = OpenService(manager,
                                   TEXT(DSHOW_PLUGIN_ASSISTANT_NAME),
                                   SERVICE_STOP | SERVICE_QUERY_STATUS);

        if (service) {
            SERVICE_STATUS status;
            memset(&status, 0, sizeof(SERVICE_STATUS));

            if (ControlService(service, SERVICE_CONTROL_STOP, &status)) {
                memset(&status, 0, sizeof(SERVICE_STATUS));

                while(QueryServiceStatus(service, &status)) {
                    if (status.dwCurrentState != SERVICE_STOP_PENDING)
                        break;

                    Sleep(1000);
                }

                stopped = status.dwCurrentState == SERVICE_STOPPED;
            }

            CloseServiceHandle(service);
        }

        CloseServiceHandle(manager);
    }

    if (!stopped)
        return -1;

    // Unistall the service.

    return this->sudo({servicePath, "--uninstall"});
}

AkVCam::Hack::Hack()
{

}

AkVCam::Hack::Hack(const std::string &name,
                   const std::string &description,
                   bool isSafe,
                   bool needsRoot,
                   const Hack::HackFunc &func):
    name(name),
    description(description),
    isSafe(isSafe),
    needsRoot(needsRoot),
    func(func)
{

}

AkVCam::Hack::Hack(const Hack &other):
    name(other.name),
    description(other.description),
    isSafe(other.isSafe),
    needsRoot(other.needsRoot),
    func(other.func)
{
}

AkVCam::Hack &AkVCam::Hack::operator =(const Hack &other)
{
    if (this != &other) {
        this->name = other.name;
        this->description = other.description;
        this->isSafe = other.isSafe;
        this->needsRoot = other.needsRoot;
        this->func = other.func;
    }

    return *this;
}
