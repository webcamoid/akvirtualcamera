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

    class IpcBridgePrivate
    {
        public:
            IpcBridge *self;
            MessageClient m_messageClient;
            std::map<std::string, BroadcastSlot> m_broadcasts;
            std::map<std::string, std::map<std::string, int>> m_controlValues;
            std::vector<std::string> m_devices;
            std::string m_picture;
            std::mutex m_broadcastsMutex;
            std::mutex m_statusMutex;
            Timer m_messagesTimer;

            explicit IpcBridgePrivate(IpcBridge *self);
            ~IpcBridgePrivate();

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
    AkLogDebug() << "Stopping the devices:" << std::endl;

    for (auto &device: this->devices())
        this->deviceStop(device);

    this->d->m_messagesTimer.stop();

    AkLogDebug() << "Bridge Destroyed" << std::endl;

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

std::string AkVCam::IpcBridge::logPath(const std::string &logName) const
{
    if (logName.empty())
        return {};

    auto defaultLogFile = AkVCam::tempPath() + "\\" + logName + ".log";

    return AkVCam::Preferences::readString("logfile", defaultLogFile);
}

void AkVCam::IpcBridge::stopNotifications()
{
    AkLogFunction();
    this->d->m_messagesTimer.stop();
}

std::vector<std::string> AkVCam::IpcBridge::devices() const
{
    AkLogFunction();
    std::vector<std::string> devices;
    auto nCameras = Preferences::camerasCount();
    AkLogInfo() << "Devices:" << std::endl;

    for (size_t i = 0; i < nCameras; i++) {
        auto deviceId = Preferences::cameraId(i);
        devices.push_back(deviceId);
        AkLogInfo() << "    " << deviceId << std::endl;
    }

    return devices;
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
    return type == StreamType_Input?
                PixelFormatRGB24:
                PixelFormatYUY2;
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

    return Preferences::addDevice(description, deviceId);
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
    auto pluginDir = dirname(locatePluginPath());
    AkLogDebug() << "Plugin path: " << pluginDir << std::endl;

    if (pluginDir.empty())
        return;

    auto path = pluginDir + "\\" AKVCAM_PLUGIN_NAME ".dll";
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
            AkLogError() << "Can't locate DllRegisterServer function." << std::endl;
        }

        FreeLibrary(hmodule);
    } else {
        AkLogError() << "Error loading plugin binary: " << path << std::endl;
    }
}

bool AkVCam::IpcBridge::deviceStart(StreamType type,
                                    const std::string &deviceId)
{
    AkLogFunction();
    AkLogDebug() << "Starting device: "
                 << deviceId
                 << " with type: "
                 << (type == StreamType_Input? "Input": "Output")
                 << std::endl;

    this->d->m_broadcastsMutex.lock();

    if (this->d->m_broadcasts.count(deviceId) > 0) {
        this->d->m_broadcastsMutex.unlock();
        AkLogError() << '\'' << deviceId << "' is busy." << std::endl;

        return false;
    }

    this->d->m_broadcasts[deviceId] = {};
    auto &slot = this->d->m_broadcasts[deviceId];
    slot.type = StreamType_Input;
    slot.run = true;

    if (type == StreamType_Input) {
        slot.messageFuture =
            this->d->m_messageClient.send(MsgListen(deviceId, currentPid()).toMessage(),
                                          std::bind(&IpcBridgePrivate::frameReady,
                                                    this->d,
                                                    std::placeholders::_1));
        AkLogDebug() << "Started input stream for device: " << deviceId << std::endl;
    } else {
        slot.messageFuture =
            this->d->m_messageClient.send([this, deviceId] (Message &message) -> bool {
            return this->d->frameRequired(deviceId, message);
        });
        AkLogDebug() << "Started output stream for device: " << deviceId << std::endl;
    }

    this->d->m_broadcastsMutex.unlock();

    return true;
}

void AkVCam::IpcBridge::deviceStop(const std::string &deviceId)
{
    AkLogFunction();
    AkLogDebug() << "Stopping device: " << deviceId << std::endl;

    std::future<bool> messageFuture;

    {
        std::lock_guard<std::mutex> lock(this->d->m_broadcastsMutex);

        if (this->d->m_broadcasts.count(deviceId) < 1) {
            AkLogDebug() << "Device " << deviceId << " not found in broadcasts" << std::endl;
            return;
        }

        auto &slot = this->d->m_broadcasts[deviceId];
        slot.run = false;
        messageFuture = std::move(slot.messageFuture); // Move the future
        AkLogDebug() << "Set run = false for device: " << deviceId << std::endl;
    } // m_broadcastsMutex is released here

    // Wait for the connection loop to end
    if (messageFuture.valid()) {
        AkLogDebug() << "Waiting for messageFuture for device: " << deviceId << std::endl;
        auto status = messageFuture.wait_for(std::chrono::seconds(5));

        if (status == std::future_status::timeout)
            AkLogWarning() << "Timeout waiting for messageFuture in deviceStop for deviceId: " << deviceId << std::endl;
        else
            AkLogDebug() << "messageFuture completed for device: " << deviceId << std::endl;
    } else {
        AkLogWarning() << "Invalid messageFuture for device: " << deviceId << std::endl;
    }

    // Remove the device after the future is complete
    {
        std::lock_guard<std::mutex> lock(this->d->m_broadcastsMutex);
        this->d->m_broadcasts.erase(deviceId);
        AkLogDebug() << "Device " << deviceId << " removed from broadcasts" << std::endl;
    }
}

bool AkVCam::IpcBridge::write(const std::string &deviceId,
                              const VideoFrame &frame)
{
    AkLogFunction();

    this->d->m_broadcastsMutex.lock();

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
    slot.frame = frame;
    slot.available = true;
    slot.frameAvailable.notify_all();
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

    auto loglevel = AkVCam::Preferences::logLevel();
    AkVCam::Logger::setLogLevel(loglevel);

    if (!this->launchService())
        AkLogWarning() << "There was not possible to communicate with the server consider increasing the timeout." << std::endl;

    this->m_messageClient.setPort(Preferences::servicePort());
    this->m_messagesTimer.connectTimeout(this, &IpcBridgePrivate::checkStatus);
    this->m_messagesTimer.setInterval(1000);
    this->m_messagesTimer.start();
}

AkVCam::IpcBridgePrivate::~IpcBridgePrivate()
{
    AkLogFunction();
}

bool AkVCam::IpcBridgePrivate::launchService()
{
    AkLogFunction();

    if (!isServicePortUp()) {
        AkLogDebug() << "Launching the service" << std::endl;
        auto servicePath = locateServicePath();

        if (!servicePath.empty())
            execDetached({servicePath});
        else
            AkLogDebug() << "Service path not found" << std::endl;
    }

    bool ok = false;
    auto timeout = Preferences::serviceTimeout();
    AkLogDebug() << "Service check Timeout:" << timeout << std::endl;

    for (int i = 0; i < timeout; ++i) {
        if (isServicePortUp())
            break;

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

    auto frame = slot.frame;
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
    this->m_broadcastsMutex.unlock();

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
