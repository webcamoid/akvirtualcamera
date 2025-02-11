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
#include <fstream>
#include <thread>
#include <unistd.h>

#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/logger.h"
#include "VCamUtils/src/message.h"
#include "VCamUtils/src/messageclient.h"
#include "VCamUtils/src/servicemsg.h"
#include "VCamUtils/src/utils.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1)

#define AKVCAM_BIND_HACK_FUNC(member) \
    std::bind(&member, this, std::placeholders::_1)

namespace AkVCam
{
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

    struct Slot
    {
        std::future<bool> messageFuture;
        bool run {false};

        Slot()
        {
        }

        Slot(std::future<bool> &messageFuture)
        {
        }

        Slot(const Slot &other)
        {

        }

        Slot &operator =(const Slot &other)
        {
            UNUSED(other);

            return *this;
        }
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
            std::mutex m_broadcastsMutex;
            std::map<int, Slot> m_messageSlots;

            explicit IpcBridgePrivate(IpcBridge *self);
            ~IpcBridgePrivate();

            bool launchService();
            void connectDeviceControlsMessages();
            inline const std::vector<DeviceControl> &controls() const;

            // Message handling methods
            bool devicesUpdated(const Message &message);
            bool pictureUpdated(const Message &message);
            bool controlsUpdated(const Message &message);
            bool frameRequired(const std::string &deviceId, Message &message);
            bool frameReady(const Message &message);

            // Utility methods
            bool isRoot() const;

            // Hacks
            const std::vector<Hack> &hacks();
            int disableLibraryValidation(const std::vector<std::string> &args);
            int codeResign(const std::vector<std::string> &args);
            int unsign(const std::vector<std::string> &args);
            int disableSIP(const std::vector<std::string> &args);
    };
}

AkVCam::IpcBridge::IpcBridge()
{
    AkLogFunction();
    this->d = new IpcBridgePrivate(this);
    auto loglevel = AkVCam::Preferences::logLevel();
    AkVCam::Logger::setLogLevel(loglevel);
}

AkVCam::IpcBridge::~IpcBridge()
{
    for (auto &device: this->devices())
        this->deviceStop(device);

    for (auto &slot: this->d->m_messageSlots) {
        slot.second.run = false;
        slot.second.messageFuture.wait();
    }

    delete this->d;
}

std::string AkVCam::IpcBridge::picture() const
{
    return Preferences::picture();
}

void AkVCam::IpcBridge::setPicture(const std::string &picture)
{
    AkLogFunction();

    if (picture == Preferences::picture())
        return;

    Preferences::setPicture(picture);
    this->d->m_messageClient.send(MsgUpdatePicture(picture).toMessage());
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

    auto defaultLogFile = "/tmp/" + logName + ".log";

    return AkVCam::Preferences::readString("logfile", defaultLogFile);
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
        PixelFormatUYVY,
        PixelFormatYUY2
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

    this->d->m_messageClient.send(MsgUpdateControls(deviceId).toMessage());
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

    this->d->m_messageClient.send(MsgUpdateDevices().toMessage());
 }

bool AkVCam::IpcBridge::deviceStart(StreamType type,
                                    const std::string &deviceId)
{
    AkLogFunction();

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
                this->d->m_messageClient.send([this, &deviceId] (Message &message) -> bool {
                                                  return this->d->frameRequired(deviceId, message);
                                              });
    } else  {
        slot.messageFuture =
                this->d->m_messageClient.send(MsgListen(deviceId, currentPid()).toMessage(),
                                              std::bind(&IpcBridgePrivate::frameReady,
                                                        this->d,
                                                        std::placeholders::_1));
    }

    this->d->m_broadcastsMutex.lock();

    return true;
}

void AkVCam::IpcBridge::deviceStop(const std::string &deviceId)
{
    AkLogFunction();

    this->d->m_broadcastsMutex.lock();

    if (this->d->m_broadcasts.count(deviceId) < 1) {
        this->d->m_broadcastsMutex.unlock();

        return;
    }

    auto &slot = this->d->m_broadcasts[deviceId];
    slot.run = false;
    slot.messageFuture.wait();
    this->d->m_broadcasts.erase(deviceId);

    this->d->m_broadcastsMutex.unlock();
}

bool AkVCam::IpcBridge::write(const std::string &deviceId,
                              const VideoFrame &frame)
{
    AkLogFunction();

    this->d->m_broadcastsMutex.lock();

    if (this->d->m_broadcasts.count(deviceId) < 1)
        this->d->m_broadcastsMutex.unlock();

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
    static const std::vector<std::string> operations;
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
    if (!this->launchService())
        AkLogWarning() << "There was not possible to communicate with the server consider increasing the timeout." << std::endl;

    this->m_messageClient.setPort(Preferences::servicePort());

    this->m_messageSlots[AKVCAM_SERVICE_MSG_DEVICES_UPDATED] = {};
    this->m_messageSlots[AKVCAM_SERVICE_MSG_DEVICES_UPDATED].run = true;
    this->m_messageSlots[AKVCAM_SERVICE_MSG_DEVICES_UPDATED].messageFuture =
            this->m_messageClient.send(MsgDevicesUpdated().toMessage(), AKVCAM_BIND_FUNC(IpcBridgePrivate::devicesUpdated));
    this->m_messageSlots[AKVCAM_SERVICE_MSG_PICTURE_UPDATED] = {};
    this->m_messageSlots[AKVCAM_SERVICE_MSG_PICTURE_UPDATED].run = true;
    this->m_messageSlots[AKVCAM_SERVICE_MSG_PICTURE_UPDATED].messageFuture =
            this->m_messageClient.send(MsgPictureUpdated().toMessage(), AKVCAM_BIND_FUNC(IpcBridgePrivate::pictureUpdated));

    this->connectDeviceControlsMessages();
}

AkVCam::IpcBridgePrivate::~IpcBridgePrivate()
{
}

bool AkVCam::IpcBridgePrivate::launchService()
{
    if (!isServiceRunning()) {
        AkLogDebug() << "Launching the service" << std::endl;
        char cmd[4096];
        snprintf(cmd, 4096, "'%s' &", locateServicePath().c_str());
        system(cmd);
    }

    bool ok = false;
    auto timeout = Preferences::serviceTimeout();

    for (int i = 0; i < timeout; ++i) {
        ok = isServicePortUp() && isServiceRunning();

        if (ok)
            break;

        std::this_thread::sleep_for(std::chrono::seconds(1));;
    }

    return ok;
}

void AkVCam::IpcBridgePrivate::connectDeviceControlsMessages()
{
    // Remove the old message listeners
    std::vector<int> keys;

    for (auto &slot: this->m_messageSlots)
        if ((slot.first & 0xffff) == AKVCAM_SERVICE_MSG_CONTROLS_UPDATED) {
            slot.second.run = false;
            slot.second.messageFuture.wait();
            keys.push_back(slot.first);
        }

    for (auto &key: keys)
        this->m_messageSlots.erase(key);

    // Add the new ones
    int i = 0;

    for (auto &device: self->devices()) {
        auto key = (i << 16) | AKVCAM_SERVICE_MSG_CONTROLS_UPDATED;
        this->m_messageSlots[key] = {};
        this->m_messageSlots[key].run = true;
        this->m_messageSlots[key].messageFuture =
                this->m_messageClient.send(MsgControlsUpdated(device).toMessage(), AKVCAM_BIND_FUNC(IpcBridgePrivate::controlsUpdated));
        ++i;
    }
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

bool AkVCam::IpcBridgePrivate::devicesUpdated(const Message &message)
{
    UNUSED(message);
    AkLogFunction();
    std::vector<std::string> devices;
    auto nCameras = Preferences::camerasCount();

    for (size_t i = 0; i < nCameras; i++)
        devices.push_back(Preferences::cameraId(i));

    AKVCAM_EMIT(this->self, DevicesChanged, devices)

    return this->m_messageSlots[AKVCAM_SERVICE_MSG_DEVICES_UPDATED].run;
}

bool AkVCam::IpcBridgePrivate::pictureUpdated(const Message &message)
{
    AkLogFunction();
    auto picture = MsgPictureUpdated(message).picture();
    AKVCAM_EMIT(this->self, PictureChanged, picture)

    return this->m_messageSlots[AKVCAM_SERVICE_MSG_PICTURE_UPDATED].run;
}

bool AkVCam::IpcBridgePrivate::controlsUpdated(const Message &message)
{
    AkLogFunction();
    auto deviceId = MsgControlsUpdated(message).device();
    auto cameraIndex = Preferences::cameraFromId(deviceId);

    if (cameraIndex < 0)
        return false;

    std::map<std::string, int> controls;

    for (auto &control: this->controls()) {
        controls[control.id] =
                Preferences::cameraControlValue(size_t(cameraIndex), control.id);
        AkLogDebug() << control.id << ": " << controls[control.id] << std::endl;
    }

    AKVCAM_EMIT(this->self, ControlsChanged, deviceId, controls)

    return true;
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

    slot.frameMutex.lock();

    if (!slot.available)
        slot.frameAvailable.wait_for(this->m_broadcastsMutex,
                                     std::chrono::seconds(1));

    auto frame = slot.frame;
    auto run = slot.run;
    slot.available = false;
    slot.frameMutex.unlock();

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

bool AkVCam::IpcBridgePrivate::isRoot() const
{
    AkLogFunction();

    return getuid() == 0;
}

const std::vector<AkVCam::Hack> &AkVCam::IpcBridgePrivate::hacks()
{
    static const std::vector<AkVCam::Hack> hacks {
        {"disable-library-validation",
         "Disable external plugins validation in app bundle",
         false,
         false,
         AKVCAM_BIND_HACK_FUNC(IpcBridgePrivate::disableLibraryValidation)},
        {"code-re-sign",
         "Remove app code signature and re-sign it with a developer signature",
         false,
         false,
         AKVCAM_BIND_HACK_FUNC(IpcBridgePrivate::codeResign)},
        {"unsign",
         "Remove app code signature",
         false,
         false,
         AKVCAM_BIND_HACK_FUNC(IpcBridgePrivate::unsign)},
        {"disable-sip",
         "Disable System Integrity Protection",
         false,
         false,
         AKVCAM_BIND_HACK_FUNC(IpcBridgePrivate::disableSIP)}
    };

    return hacks;
}

int AkVCam::IpcBridgePrivate::disableLibraryValidation(const std::vector<std::string> &args)
{
#ifndef FAKE_APPLE
    return AkVCam::disableLibraryValidation(args);
#else
    std::cerr << "Not implemented." << std::endl;

    return -1;
#endif
}

int AkVCam::IpcBridgePrivate::codeResign(const std::vector<std::string> &args)
{
    if (args.size() < 1) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    if (!fileExists(args[0])) {
        std::cerr << "No such file or directory." << std::endl;

        return -1;
    }

    static const std::string entitlementsXml = "/tmp/entitlements.xml";
    std::string cmd;

    if (readEntitlements(args[0], entitlementsXml))
        cmd = "codesign --entitlements \""
              + entitlementsXml
              + "\" -f -s - \""
              + args[0]
              + "\"";
    else
        cmd = "codesign -f -s - \"" + args[0] + "\"";

    int result = system(cmd.c_str());
    remove(entitlementsXml.c_str());

    return result;
}

int AkVCam::IpcBridgePrivate::unsign(const std::vector<std::string> &args)
{
    if (args.size() < 1) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    if (!fileExists(args[0])) {
        std::cerr << "No such file or directory." << std::endl;

        return -1;
    }

    auto cmd = "codesign --remove-signature \"" + args[0] + "\"";

    return system(cmd.c_str());
}

int AkVCam::IpcBridgePrivate::disableSIP(const std::vector<std::string> &args)
{
    std::cerr << "SIP (System Integrity Protection) can't be disbled from "
                 "inside the system, you must reboot your system and then "
                 "press and hold Command + R keys on boot to enter to the "
                 "recovery mode, then go to Utilities > Terminal and run:"
              << std::endl;
    std::cerr << std::endl;
    std::cerr << "csrutil enable --without fs" << std::endl;
    std::cerr << std::endl;
    std::cerr << "If that does not works, then run:" << std::endl;
    std::cerr << std::endl;
    std::cerr << "csrutil disable" << std::endl;
    std::cerr << std::endl;

    return -1;
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
