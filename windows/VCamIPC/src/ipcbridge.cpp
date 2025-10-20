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
#include <fstream>
#include <thread>
#include <thread>

#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/logger.h"
#include "VCamUtils/src/message.h"
#include "VCamUtils/src/messageclient.h"
#include "VCamUtils/src/servicemsg.h"
#include "VCamUtils/src/sharedmemory.h"
#include "VCamUtils/src/timer.h"
#include "VCamUtils/src/utils.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1)

#define AKVCAM_BIND_HACK_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1)

namespace AkVCam
{
    using RegisterServerFunc = HRESULT (WINAPI *)();

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

    struct BroadcastSlot
    {
        IpcBridge::StreamType type;
        std::future<bool> messageFuture;
        VideoFrame frame;
        std::condition_variable_any frameAvailable;
        std::mutex frameMutex;
        SharedMemory sharedMemory;
        bool available {false};
        bool run {false};

        BroadcastSlot()
        {
        }

        BroadcastSlot(std::future<bool> &messageFuture)
        {
        }

        BroadcastSlot(const BroadcastSlot &other)
        {

        }

        BroadcastSlot &operator =(const BroadcastSlot &other)
        {
            UNUSED(other);

            return *this;
        }
    };

    struct SharedFrame
    {
        PixelFormat format;
        int width;
        int height;
        uint8_t data[1];
    };

    struct DirectModeStatus
    {
        bool directMode {false};
        VideoFormat format;

        DirectModeStatus()
        {
        }

        DirectModeStatus(const std::string &deviceId)
        {
            auto cameraId = Preferences::cameraFromId(deviceId);

            if (cameraId >= 0
                && Preferences::cameraDirectMode(size_t(cameraId))) {
                this->directMode = true;
                this->format = Preferences::cameraFormat(size_t(cameraId), 0);
            }
        }

        DirectModeStatus(const DirectModeStatus &other):
            directMode(other.directMode),
            format(other.format)
        {

        }

        DirectModeStatus &operator =(const DirectModeStatus &other)
        {
            if (this != &other) {
                this->directMode = other.directMode;
                this->format = other.format;
            }

            return *this;
        }

        inline bool isValid(const VideoFormat &format) const
        {
            return !this->directMode || format.isSameFormat(this->format);
        }
    };

    class IpcBridgePrivate
    {
        public:
            IpcBridge *self;
            MessageClient m_messageClient;
            std::map<std::string, BroadcastSlot> m_broadcasts;
            std::map<std::string, std::map<std::string, int>> m_controlValues;
            std::map<std::string, DirectModeStatus> m_directModeStatus;
            std::vector<std::string> m_devices;
            std::string m_picture;
            std::mutex m_broadcastsMutex;
            std::mutex m_statusMutex;
            Timer m_messagesTimer;
            int m_logLevel {-1};
            DataMode m_dataMode {DataMode_SharedMemory};
            size_t m_pageSize {0};

            explicit IpcBridgePrivate(IpcBridge *self);
            ~IpcBridgePrivate();

            void updateDevices();
            bool isServiceRunning();
            bool isMFServiceRunning();
            bool launchService();
            inline const std::vector<DeviceControl> &controls() const;

            // Message handling methods
            bool frameRequired(const std::string &deviceId, Message &message);
            bool frameReady(const Message &message);
            static void checkStatus(void *userData);

            // Utility methods
            bool isRoot() const;

            // Hacks
            const std::vector<Hack> &hacks();
    };
}

AkVCam::IpcBridge::IpcBridge()
{
    AkLogFunction();
    this->d = new IpcBridgePrivate(this);
}

AkVCam::IpcBridge::~IpcBridge()
{
    AkLogFunction();
    AkLogDebug("Stopping the devices:");

    for (auto &device: this->devices())
        this->deviceStop(device);

    delete this->d;
}

std::string AkVCam::IpcBridge::picture() const
{
    return this->d->m_picture;
}

void AkVCam::IpcBridge::setPicture(const std::string &picture)
{
    AkLogFunction();
    this->d->m_picture = picture;
    Preferences::setPicture(picture);
}

int AkVCam::IpcBridge::logLevel() const
{
    return this->d->m_logLevel;
}

void AkVCam::IpcBridge::setLogLevel(int logLevel)
{
    AkLogFunction();
    this->d->m_logLevel = logLevel;
    Preferences::setLogLevel(logLevel);
    Logger::setLogLevel(logLevel);
}

AkVCam::DataMode AkVCam::IpcBridge::dataMode()
{
    return this->d->m_dataMode;
}

void AkVCam::IpcBridge::setDataMode(DataMode dataMode)
{
    AkLogFunction();
    this->d->m_dataMode = dataMode;
    Preferences::setDataMode(dataMode);
}

size_t AkVCam::IpcBridge::pageSize()
{
    return this->d->m_pageSize;
}

void AkVCam::IpcBridge::setPageSize(size_t pageSize)
{
    AkLogFunction();
    this->d->m_pageSize = pageSize;
    Preferences::setPageSize(pageSize);
}

void AkVCam::IpcBridge::stopNotifications()
{
    AkLogFunction();
    this->d->m_messagesTimer.stop();
}

std::vector<std::string> AkVCam::IpcBridge::devices() const
{
    return this->d->m_devices;
}

std::string AkVCam::IpcBridge::description(const std::string &deviceId) const
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromId(deviceId);

    if (cameraIndex < 0)
        return {};

    return Preferences::cameraDescription(size_t(cameraIndex));
}

void AkVCam::IpcBridge::setDescription(const std::string &deviceId,
                                       const std::string &description)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromId(deviceId);

    if (cameraIndex >= 0)
        Preferences::cameraSetDescription(size_t(cameraIndex), description);
}

std::vector<AkVCam::PixelFormat> AkVCam::IpcBridge::supportedPixelFormats(StreamType type) const
{
    if (type == StreamType_Input)
        return VideoFormat::supportedPixelFormats();

    return {
        PixelFormat_bgrx,
        PixelFormat_rgb24,
        PixelFormat_uyvy422,
        PixelFormat_yuyv422,
        PixelFormat_nv12
    };
}

AkVCam::PixelFormat AkVCam::IpcBridge::defaultPixelFormat(StreamType type) const
{
    return type == StreamType_Input?
                PixelFormat_rgb24:
                PixelFormat_yuyv422;
}

std::vector<AkVCam::VideoFormat> AkVCam::IpcBridge::formats(const std::string &deviceId) const
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromId(deviceId);

    if (cameraIndex < 0)
        return {};

    return Preferences::cameraFormats(size_t(cameraIndex));
}

void AkVCam::IpcBridge::setFormats(const std::string &deviceId,
                                   const std::vector<VideoFormat> &formats)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromId(deviceId);

    if (cameraIndex >= 0)
        Preferences::cameraSetFormats(size_t(cameraIndex), formats);
}

std::vector<AkVCam::DeviceControl> AkVCam::IpcBridge::controls(const std::string &deviceId)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromId(deviceId);

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
    auto cameraIndex = Preferences::cameraFromId(deviceId);

    if (cameraIndex < 0)
        return;

    for (auto &control: this->d->controls()) {
        auto oldValue =
                Preferences::cameraControlValue(size_t(cameraIndex),
                                                control.id);

        if (controls.count(control.id)) {
            auto newValue = controls.at(control.id);

            if (newValue != oldValue)
                Preferences::cameraSetControlValue(size_t(cameraIndex),
                                                   control.id,
                                                   newValue);
        }
    }
}

std::vector<uint64_t> AkVCam::IpcBridge::clientsPids() const
{
    AkLogFunction();

    Message msgClients;

    if (!this->d->m_messageClient.send(MsgClients(MsgClients::ClientType_VCams).toMessage(), msgClients))
        return {};

    auto clients = MsgClients(msgClients).clients();
    auto it = std::find(clients.begin(), clients.end(), currentPid());

    if (it != clients.end())
        clients.erase(it);

    return clients;
}

std::string AkVCam::IpcBridge::clientExe(uint64_t pid) const
{
    return exePath(pid);
}

std::string AkVCam::IpcBridge::addDevice(const std::string &description,
                                         const std::string &deviceId)
{
    AkLogFunction();
    auto device = Preferences::addDevice(description, deviceId);
    this->updateDevices();

    return device;
}

void AkVCam::IpcBridge::removeDevice(const std::string &deviceId)
{
    AkLogFunction();
    Preferences::removeCamera(deviceId);
    this->updateDevices();
}

void AkVCam::IpcBridge::addFormat(const std::string &deviceId,
                                  const VideoFormat &format,
                                  int index)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromId(deviceId);

    if (cameraIndex >= 0)
        Preferences::cameraAddFormat(size_t(cameraIndex),
                                     format,
                                     index);
}

void AkVCam::IpcBridge::removeFormat(const std::string &deviceId, int index)
{
    AkLogFunction();
    auto cameraIndex = Preferences::cameraFromId(deviceId);

    if (cameraIndex >= 0)
        Preferences::cameraRemoveFormat(size_t(cameraIndex),
                                        index);
}

void AkVCam::IpcBridge::updateDevices()
{
    AkLogFunction();
    std::string pluginPath = supportsMediaFoundationVCam()?
                                 locateMFPluginPath():
                                 locatePluginPath();
    AkLogDebug("Plugin binary: %s", pluginPath.c_str());

    if (!fileExists(pluginPath)) {
        AkLogError("Plugin binary not found: %s", pluginPath.c_str());

        return;
    }

    if (auto hmodule = LoadLibraryA(pluginPath.c_str())) {
        auto registerServer =
                RegisterServerFunc(GetProcAddress(hmodule, "DllRegisterServer"));

        if (registerServer) {
            AkLogDebug("Registering server");
            auto result = (*registerServer)();
            AkLogDebug("Server registered with code %d", result);
            auto lockFileName = tempPath() + "\\akvcam_update.lck";

            if (!fileExists(lockFileName)) {
                std::ofstream lockFile;
                lockFile.open(lockFileName);
                lockFile << std::endl;
                lockFile.close();
                auto altManager = locateAltManagerPath();

                if (!altManager.empty())
                    exec({altManager, "update"});

                remove(lockFileName.c_str());
            }
        } else {
            AkLogError("Can't locate DllRegisterServer function.");
        }

        FreeLibrary(hmodule);
    } else {
        AkLogError("Error loading plugin binary: %s", pluginPath.c_str());
    }
}

bool AkVCam::IpcBridge::deviceStart(StreamType type,
                                    const std::string &deviceId)
{
    AkLogFunction();
    AkLogDebug("Starting device: %s with type: %s",
               deviceId.c_str(),
               type == StreamType_Input? "Input": "Output");

    this->d->m_broadcastsMutex.lock();

    if (this->d->m_broadcasts.count(deviceId) > 0) {
        this->d->m_broadcastsMutex.unlock();
        AkLogError("'%s' is busy.", deviceId.c_str());

        return false;
    }

    this->d->m_broadcasts[deviceId] = {};
    auto &slot = this->d->m_broadcasts[deviceId];
    slot.type = StreamType_Input;
    slot.run = true;

    /* NOTE: When the data mode is configured as SharedMemory, the socket
     * channel is not used to send/receive data, but to indicate that the
     * virtual camera is in use.
     */

    if (type == StreamType_Input) {
        slot.messageFuture =
            this->d->m_messageClient.send(MsgListen(deviceId, currentPid()).toMessage(),
                                          std::bind(&IpcBridgePrivate::frameReady,
                                                    this->d,
                                                    std::placeholders::_1));
        AkLogDebug("Started input stream for device: %s", deviceId.c_str());
    } else {
        slot.messageFuture =
            this->d->m_messageClient.send([this, deviceId] (Message &message) -> bool {
            return this->d->frameRequired(deviceId, message);
        });
        AkLogDebug("Started output stream for device: %s", deviceId.c_str());
    }

    if (this->d->m_dataMode == DataMode_SharedMemory) {
        slot.sharedMemory.setName(deviceId + "Shm");
        slot.sharedMemory.open(this->d->m_pageSize,
                               type == StreamType_Input?
                                   SharedMemory::OpenModeRead:
                                   SharedMemory::OpenModeWrite);
    }

    this->d->m_broadcastsMutex.unlock();

    return true;
}

void AkVCam::IpcBridge::deviceStop(const std::string &deviceId)
{
    AkLogFunction();
    AkLogDebug("Stopping device: %s", deviceId.c_str());

    std::future<bool> messageFuture;

    {
        std::lock_guard<std::mutex> lock(this->d->m_broadcastsMutex);

        if (this->d->m_broadcasts.count(deviceId) < 1) {
            AkLogDebug("Device %s not found in broadcasts", deviceId.c_str());

            return;
        }

        auto &slot = this->d->m_broadcasts[deviceId];
        slot.sharedMemory.close();
        slot.run = false;
        messageFuture = std::move(slot.messageFuture); // Move the future
        AkLogDebug("Set run = false for device: %s", deviceId.c_str());
    } // m_broadcastsMutex is released here

    // Wait for the connection loop to end
    if (messageFuture.valid()) {
        AkLogDebug("Waiting for messageFuture for device: %s", deviceId.c_str());
        auto status = messageFuture.wait_for(std::chrono::seconds(5));

        if (status == std::future_status::timeout)
            AkLogWarning("Timeout waiting for messageFuture in deviceStop for deviceId: %s", deviceId.c_str());
        else
            AkLogDebug("messageFuture completed for device: %s", deviceId.c_str());
    } else {
        AkLogWarning("Invalid messageFuture for device: %s", deviceId.c_str());
    }

    // Remove the device after the future is complete
    {
        std::lock_guard<std::mutex> lock(this->d->m_broadcastsMutex);
        this->d->m_broadcasts.erase(deviceId);
        AkLogDebug("Device %s removed from broadcasts", deviceId.c_str());
    }
}

bool AkVCam::IpcBridge::write(const std::string &deviceId,
                              const VideoFrame &frame)
{
    AkLogFunction();

    this->d->m_broadcastsMutex.lock();

    if (!this->d->m_directModeStatus.contains(deviceId))
        this->d->m_directModeStatus[deviceId] = {deviceId};

    if (!this->d->m_directModeStatus[deviceId].isValid(frame.format())) {
        this->d->m_broadcastsMutex.unlock();

        return false;
    }

    if (this->d->m_broadcasts.count(deviceId) < 1) {
        this->d->m_broadcastsMutex.unlock();

        return false;
    }

    auto &slot = this->d->m_broadcasts[deviceId];

    if (slot.type != StreamType_Input) {
        this->d->m_broadcastsMutex.unlock();

        return false;
    }

    slot.frameMutex.lock();

    if (slot.sharedMemory.isOpen()) {
        auto sharedFrame = reinterpret_cast<SharedFrame *>(slot.sharedMemory.lock());

        if (sharedFrame) {
            sharedFrame->format = frame.format().format();
            sharedFrame->width = frame.format().width();
            sharedFrame->height = frame.format().height();
            auto dataSize =
                    std::min(slot.sharedMemory.pageSize()
                             - sizeof(SharedFrame)
                             + sizeof(void *),
                             frame.size());

            if (dataSize > 0)
                memcpy(sharedFrame->data, frame.constData(), dataSize);

            slot.sharedMemory.unlock();
            slot.available = true;
            slot.frameAvailable.notify_all();
        }
    } else {
        slot.frame = frame;
        slot.available = true;
        slot.frameAvailable.notify_all();
    }

    slot.frameMutex.unlock();

    this->d->m_broadcastsMutex.unlock();

    return true;
}

bool AkVCam::IpcBridge::isBusyFor(const std::string &operation) const
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
        "update",
        "hack"
    };

    auto it = std::find(operations.begin(), operations.end(), operation);

    return it != operations.end() && !this->clientsPids().empty();
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
    AkLogFunction();

    this->m_logLevel = Preferences::logLevel();
    AkVCam::Logger::setLogLevel(this->m_logLevel);
    this->m_picture = Preferences::picture();
    this->m_dataMode = Preferences::dataMode();
    this->m_pageSize = Preferences::pageSize();
    this->updateDevices();

    if (!this->launchService())
        AkLogWarning("There was not possible to communicate with the server consider increasing the timeout.");

    this->m_messageClient.setPort(Preferences::servicePort());
    this->m_messagesTimer.connectTimeout(this, &IpcBridgePrivate::checkStatus);
    this->m_messagesTimer.setInterval(1000);
    this->m_messagesTimer.start();
}

AkVCam::IpcBridgePrivate::~IpcBridgePrivate()
{
    AkLogFunction();

    this->m_messagesTimer.stop();
    AkLogDebug("Bridge Destroyed");
}

void AkVCam::IpcBridgePrivate::updateDevices()
{
    AkLogFunction();

    this->m_devices.clear();
    auto nCameras = Preferences::camerasCount();
    AkLogInfo("Devices:");

    for (size_t i = 0; i < nCameras; i++) {
        auto deviceId = Preferences::cameraId(i);
        this->m_devices.push_back(deviceId);
        AkLogInfo("    %s", deviceId.c_str());
    }
}

bool AkVCam::IpcBridgePrivate::isServiceRunning()
{
    AkLogFunction();

    AkVCam::SharedMemory serviceLock;
    serviceLock.setName(AKVCAM_SERVICE_NAME "_Lock");
    bool result = false;

    if (serviceLock.open(1024)) {
        if (serviceLock.lock()) {
            result = true;
            serviceLock.unlock();
        }

        serviceLock.close();
    }

    AkLogDebug("Result: %d", result);

    return result;
}

bool AkVCam::IpcBridgePrivate::isMFServiceRunning()
{
    AkLogFunction();

    AkVCam::SharedMemory serviceLock;
    serviceLock.setName(AKVCAM_SERVICE_MF_NAME "_Lock");
    bool result = false;

    if (serviceLock.open(1024)) {
        if (serviceLock.lock()) {
            result = true;
            serviceLock.unlock();
        }

        serviceLock.close();
    }

    AkLogDebug("Result: %d", result);

    return result;
}

bool AkVCam::IpcBridgePrivate::launchService()
{
    AkLogFunction();

    if (!isServiceRunning()) {
        AkLogDebug("Launching the service");
        auto servicePath = locateServicePath();

        if (!servicePath.empty())
            execDetached({servicePath});
        else
            AkLogDebug("Service path not found");
    }

    if (supportsMediaFoundationVCam() && !isMFServiceRunning()) {
        AkLogDebug("Launching the Media Foundation service");
        auto mfServicePath = locateMFServicePath();

        if (!mfServicePath.empty())
            execDetached({mfServicePath});
        else
            AkLogDebug("Media Foundation Service path not found");
    }

    bool ok = false;
    auto timeout = Preferences::serviceTimeout();
    AkLogDebug("Service check Timeout: %d", timeout);

    for (int i = 0; i < timeout; ++i) {
        if (isServicePortUp()) {
            ok = true;

            break;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));;
    }

    return ok;
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

bool AkVCam::IpcBridgePrivate::frameRequired(const std::string &deviceId,
                                             Message &message)
{
    AkLogFunction();

    this->m_broadcastsMutex.lock();

    if (this->m_broadcasts.count(deviceId) < 1) {
        this->m_broadcastsMutex.unlock();

        return false;
    }

    auto &slot = this->m_broadcasts[deviceId];

    std::unique_lock<std::mutex> lock(slot.frameMutex);

    if (!slot.available)
        slot.frameAvailable.wait_for(lock,
                                     std::chrono::seconds(1));

    auto &frame = slot.frame;
    auto run = slot.run;
    slot.available = false;
    lock.unlock();

    message = MsgBroadcast(deviceId, currentPid(), frame).toMessage();
    this->m_broadcastsMutex.unlock();

    return run;
}

bool AkVCam::IpcBridgePrivate::frameReady(const Message &message)
{
    AkLogFunction();

    MsgFrameReady msgFrameReady(message);

    this->m_broadcastsMutex.lock();

    auto deviceId = msgFrameReady.device();

    if (this->m_broadcasts.count(deviceId) < 1) {
        this->m_broadcastsMutex.unlock();

        return false;
    }

    auto &slot = this->m_broadcasts[deviceId];
    auto run = slot.run;

    if (slot.sharedMemory.isOpen()) {
        auto sharedFrame =
                reinterpret_cast<SharedFrame *>(slot.sharedMemory.lock());

        if (sharedFrame) {
            VideoFormat format(sharedFrame->format,
                               sharedFrame->width,
                               sharedFrame->height);

            if (!format.isSameFormat(slot.frame.format()))
                slot.frame = VideoFrame(format);

            auto dataSize =
                    std::min(slot.sharedMemory.pageSize()
                             - sizeof(SharedFrame)
                             + sizeof(void *),
                             slot.frame.size());

            if (dataSize > 0)
                memcpy(slot.frame.data(), sharedFrame->data, dataSize);
            else
                slot.frame = {};

            slot.sharedMemory.unlock();
        }
    }

    this->m_broadcastsMutex.unlock();

    if (slot.sharedMemory.isOpen())
        AKVCAM_EMIT(this->self,
                    FrameReady,
                    deviceId,
                    slot.frame,
                    msgFrameReady.isActive())
    else
        AKVCAM_EMIT(this->self,
                    FrameReady,
                    deviceId,
                    msgFrameReady.frame(),
                    msgFrameReady.isActive())

    return run;
}

void AkVCam::IpcBridgePrivate::checkStatus(void *userData)
{
    AkLogFunction();
    auto self = reinterpret_cast<IpcBridgePrivate *>(userData);
    self->m_statusMutex.lock();
    auto devices = self->self->devices();

    if (devices != self->m_devices) {
        self->m_devices = devices;
        AKVCAM_EMIT(self->self, DevicesChanged, devices)
    }

    auto picture = self->self->picture();

    if (picture != self->m_picture) {
        self->m_picture = picture;
        AKVCAM_EMIT(self->self, PictureChanged, picture)
    }

    std::vector<std::string> removeDevices;

    for (auto &device: self->m_controlValues)
        if (std::count(devices.begin(), devices.begin(), device.first) < 1)
            removeDevices.push_back(device.first);

    for (auto &device: removeDevices)
        self->m_controlValues.erase(device);

    for (auto &device: devices) {
        std::map<std::string, int> controlValues;

        for (auto &control: self->self->controls(device))
            controlValues[control.id] = control.value;

        if (self->m_controlValues.count(device) < 1)
            self->m_controlValues[device] = {};

        if (controlValues != self->m_controlValues[device]) {
            self->m_controlValues[device] = controlValues;
            AKVCAM_EMIT(self->self, ControlsChanged, device, controlValues)
        }
    }

    self->m_statusMutex.unlock();
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

const std::vector<AkVCam::Hack> &AkVCam::IpcBridgePrivate::hacks()
{
    static const std::vector<AkVCam::Hack> hacks {
    };

    return hacks;
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
