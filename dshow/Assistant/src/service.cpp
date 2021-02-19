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
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>
#include <shellapi.h>
#include <sddl.h>

#include "service.h"
#include "PlatformUtils/src/messageserver.h"
#include "PlatformUtils/src/preferences.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"
#include "VCamUtils/src/timer.h"
#include "VCamUtils/src/logger.h"

namespace AkVCam
{
    struct AssistantDevice
    {
        std::string broadcaster;
        std::vector<std::string> listeners;
    };

    typedef std::map<std::string, std::string> AssistantPeers;
    typedef std::map<std::string, AssistantDevice> DeviceConfigs;

    class ServicePrivate
    {
        public:
            SERVICE_STATUS m_status;
            SERVICE_STATUS_HANDLE m_statusHandler;
            MessageServer m_messageServer;
            AssistantPeers m_clients;
            DeviceConfigs m_deviceConfigs;
            Timer m_timer;
            std::mutex m_peerMutex;

            ServicePrivate();
            inline static uint64_t id();
            static void stateChanged(void *userData,
                                     MessageServer::State state);
            void sendStatus(DWORD currentState, DWORD exitCode, DWORD wait);
            static void checkPeers(void *userData);
            void removePortByName(const std::string &portName);
            void releaseDevicesFromPeer(const std::string &portName);
            void requestPort(Message *message);
            void addPort(Message *message);
            void removePort(Message *message);
            void devicesUpdate(Message *message);
            void setBroadCasting(Message *message);
            void frameReady(Message *message);
            void pictureUpdated(Message *message);
            void listeners(Message *message);
            void listener(Message *message);
            void broadcasting(Message *message);
            void listenerAdd(Message *message);
            void listenerRemove(Message *message);
            void controlsUpdated(Message *message);
    };

    GLOBAL_STATIC(ServicePrivate, servicePrivate)
}

DWORD WINAPI controlHandler(DWORD control,
                            DWORD eventType,
                            LPVOID eventData,
                            LPVOID context);
BOOL WINAPI controlDebugHandler(DWORD control);

AkVCam::Service::Service()
{
}

AkVCam::Service::~Service()
{
}

BOOL AkVCam::Service::install()
{
    AkLogFunction();
    TCHAR fileName[MAX_PATH];

    if (!GetModuleFileName(nullptr, fileName, MAX_PATH)) {
        AkLogError() << "Can't read module file name" << std::endl;

       return false;
    }

    auto scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);

    if (!scManager) {
        AkLogError() << "Can't open SCManager" << std::endl;

        return false;
    }

    auto service =
            CreateService(scManager,
                          TEXT(DSHOW_PLUGIN_ASSISTANT_NAME),
                          TEXT(DSHOW_PLUGIN_ASSISTANT_DESCRIPTION),
                          SERVICE_ALL_ACCESS,
                          SERVICE_WIN32_OWN_PROCESS,
                          SERVICE_AUTO_START,
                          SERVICE_ERROR_NORMAL,
                          fileName,
                          nullptr,
                          nullptr,
                          nullptr,
                          nullptr,
                          nullptr);

    if (!service) {
        AkLogError() << "Can't create service" << std::endl;
        CloseServiceHandle(scManager);

        return false;
    }

    // Add detailed description to the service.
    SERVICE_DESCRIPTION serviceDescription;
    TCHAR description[] = TEXT(DSHOW_PLUGIN_DESCRIPTION_EXT);
    serviceDescription.lpDescription = description;
    auto result =
            ChangeServiceConfig2(service,
                                 SERVICE_CONFIG_DESCRIPTION,
                                 &serviceDescription);

    // Configure the service so it will restart if fail.
    TCHAR rebootMsg[] = TEXT("Service failed restarting...");

    std::vector<SC_ACTION> actions {
        {SC_ACTION_RESTART, 5000}
    };

    SERVICE_FAILURE_ACTIONS failureActions {
        INFINITE,
        rebootMsg,
        nullptr,
        DWORD(actions.size()),
        actions.data()
    };

    result =
        ChangeServiceConfig2(service,
                             SERVICE_CONFIG_FAILURE_ACTIONS,
                             &failureActions);

    // Run the service
    StartService(service, 0, nullptr);
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);

    return result;
}

void AkVCam::Service::uninstall()
{
    AkLogFunction();
    auto scManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);

    if (!scManager) {
        AkLogError() << "Can't open SCManager" << std::endl;

        return;
    }

    auto sevice = OpenService(scManager,
                              TEXT(DSHOW_PLUGIN_ASSISTANT_NAME),
                              SERVICE_ALL_ACCESS);

    if (sevice) {
        if (ControlService(sevice,
                           SERVICE_CONTROL_STOP,
                           &servicePrivate()->m_status)) {
            AkLogInfo() << "Stopping service" << std::endl;

            do {
                Sleep(1000);
                QueryServiceStatus(sevice, &servicePrivate()->m_status);
            } while(servicePrivate()->m_status.dwCurrentState != SERVICE_STOPPED);
        }

        if (!DeleteService(sevice)) {
            AkLogError() << "Delete service failed" << std::endl;
        }

        CloseServiceHandle(sevice);
    } else {
        AkLogError() << "Can't open service" << std::endl;
    }

    CloseServiceHandle(scManager);
}

void AkVCam::Service::debug()
{
    AkLogFunction();
    SetConsoleCtrlHandler(controlDebugHandler, TRUE);
    servicePrivate()->m_messageServer.start(true);
}

void AkVCam::Service::showHelp(int argc, char **argv)
{
    AkLogFunction();
    UNUSED(argc);

    auto programName = strrchr(argv[0], '\\');

    if (!programName)
        programName = strrchr(argv[0], '/');

    if (!programName)
        programName = argv[0];
    else
        programName++;

    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Webcamoid virtual camera server." << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << std::endl;
    std::cout << "\t-i, --install\tInstall the service." << std::endl;
    std::cout << "\t-u, --uninstall\tUnistall the service." << std::endl;
    std::cout << "\t-d, --debug\tDebug the service." << std::endl;
    std::cout << "\t-h, --help\tShow this help." << std::endl;
}

AkVCam::ServicePrivate::ServicePrivate()
{
    AkLogFunction();

    this->m_status = {
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_STOPPED,
        0,
        NO_ERROR,
        NO_ERROR,
        0,
        0
    };
    this->m_statusHandler = nullptr;
    this->m_messageServer.setPipeName("\\\\.\\pipe\\" DSHOW_PLUGIN_ASSISTANT_NAME);
    this->m_messageServer.setHandlers({
        {AKVCAM_ASSISTANT_MSG_FRAME_READY            , AKVCAM_BIND_FUNC(ServicePrivate::frameReady)     },
        {AKVCAM_ASSISTANT_MSG_PICTURE_UPDATED        , AKVCAM_BIND_FUNC(ServicePrivate::pictureUpdated) },
        {AKVCAM_ASSISTANT_MSG_REQUEST_PORT           , AKVCAM_BIND_FUNC(ServicePrivate::requestPort)    },
        {AKVCAM_ASSISTANT_MSG_ADD_PORT               , AKVCAM_BIND_FUNC(ServicePrivate::addPort)        },
        {AKVCAM_ASSISTANT_MSG_REMOVE_PORT            , AKVCAM_BIND_FUNC(ServicePrivate::removePort)     },
        {AKVCAM_ASSISTANT_MSG_DEVICE_UPDATE          , AKVCAM_BIND_FUNC(ServicePrivate::devicesUpdate)  },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_ADD    , AKVCAM_BIND_FUNC(ServicePrivate::listenerAdd)    },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER_REMOVE , AKVCAM_BIND_FUNC(ServicePrivate::listenerRemove) },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENERS       , AKVCAM_BIND_FUNC(ServicePrivate::listeners)      },
        {AKVCAM_ASSISTANT_MSG_DEVICE_LISTENER        , AKVCAM_BIND_FUNC(ServicePrivate::listener)       },
        {AKVCAM_ASSISTANT_MSG_DEVICE_BROADCASTING    , AKVCAM_BIND_FUNC(ServicePrivate::broadcasting)   },
        {AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING , AKVCAM_BIND_FUNC(ServicePrivate::setBroadCasting)},
        {AKVCAM_ASSISTANT_MSG_DEVICE_CONTROLS_UPDATED, AKVCAM_BIND_FUNC(ServicePrivate::controlsUpdated)},
    });
    this->m_timer.setInterval(60000);
    this->m_timer.connectTimeout(this, &ServicePrivate::checkPeers);
    this->m_timer.start();
}

uint64_t AkVCam::ServicePrivate::id()
{
    static uint64_t id = 0;

    return id++;
}

void AkVCam::ServicePrivate::stateChanged(void *userData,
                                          MessageServer::State state)
{
    UNUSED(userData);

    switch (state) {
    case MessageServer::StateAboutToStart:
        AkVCam::servicePrivate()->sendStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
        break;

    case MessageServer::StateStarted:
        AkVCam::servicePrivate()->sendStatus(SERVICE_RUNNING, NO_ERROR, 0);
        break;

    case MessageServer::StateAboutToStop:
        AkVCam::servicePrivate()->sendStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        break;

    case MessageServer::StateStopped:
        AkVCam::servicePrivate()->sendStatus(SERVICE_STOPPED, NO_ERROR, 0);
        break;
    }
}

void AkVCam::ServicePrivate::sendStatus(DWORD currentState,
                                        DWORD exitCode,
                                        DWORD wait)
{
    AkLogFunction();
    this->m_status.dwControlsAccepted =
            currentState == SERVICE_START_PENDING? 0: SERVICE_ACCEPT_STOP;
    this->m_status.dwCurrentState = currentState;
    this->m_status.dwWin32ExitCode = exitCode;
    this->m_status.dwWaitHint = wait;

    if (currentState == SERVICE_RUNNING || currentState == SERVICE_STOPPED)
        this->m_status.dwCheckPoint = 0;
    else
        this->m_status.dwCheckPoint++;

    SetServiceStatus(this->m_statusHandler, &this->m_status);
}

void AkVCam::ServicePrivate::checkPeers(void *userData)
{
    AkLogFunction();
    auto self = reinterpret_cast<ServicePrivate *>(userData);
    std::vector<std::string> deadPeers;

    self->m_peerMutex.lock();

    for (auto &client: self->m_clients) {
        Message message;
        message.messageId = AKVCAM_ASSISTANT_MSG_ISALIVE;
        message.dataSize = sizeof(MsgIsAlive);
        MessageServer::sendMessage(client.second, &message);
        auto requestData = messageData<MsgIsAlive>(&message);

        if (!requestData->alive)
            deadPeers.push_back(client.first);
    }

    self->m_peerMutex.unlock();

    for (auto &port: deadPeers) {
        AkLogWarning() << port << " died, removing..." << std::endl;
        self->removePortByName(port);
    }
}

void AkVCam::ServicePrivate::removePortByName(const std::string &portName)
{
    AkLogFunction();
    AkLogDebug() << "Port: " << portName << std::endl;

    this->m_peerMutex.lock();

    for (auto &peer: this->m_clients)
        if (peer.first == portName) {
            this->m_clients.erase(portName);

            break;
        }

    this->m_peerMutex.unlock();
    this->releaseDevicesFromPeer(portName);
}

void AkVCam::ServicePrivate::releaseDevicesFromPeer(const std::string &portName)
{
    AkLogFunction();

    for (auto &config: this->m_deviceConfigs)
        if (config.second.broadcaster == portName) {
            config.second.broadcaster.clear();

            Message message;
            message.messageId = AKVCAM_ASSISTANT_MSG_DEVICE_SETBROADCASTING;
            message.dataSize = sizeof(MsgBroadcasting);
            auto data = messageData<MsgBroadcasting>(&message);
            memcpy(data->device,
                   config.first.c_str(),
                   (std::min<size_t>)(config.first.size(), MAX_STRING));
            this->m_peerMutex.lock();

            for (auto &client: this->m_clients)
                MessageServer::sendMessage(client.second, &message);

            this->m_peerMutex.unlock();
        } else {
            auto it = std::find(config.second.listeners.begin(),
                                config.second.listeners.end(),
                                portName);

            if (it != config.second.listeners.end())
                config.second.listeners.erase(it);
        }

    AkLogInfo() << portName << " released." << std::endl;
}

void AkVCam::ServicePrivate::requestPort(AkVCam::Message *message)
{
    AkLogFunction();

    auto data = messageData<MsgRequestPort>(message);
    std::string portName = AKVCAM_ASSISTANT_CLIENT_NAME;
    portName += std::to_string(this->id());
    AkLogInfo() << "Returning Port: " << portName << std::endl;
    memcpy(data->port,
           portName.c_str(),
           (std::min<size_t>)(portName.size(), MAX_STRING));
}

void AkVCam::ServicePrivate::addPort(AkVCam::Message *message)
{
    AkLogFunction();

    auto data = messageData<MsgAddPort>(message);
    std::string portName(data->port);
    std::string pipeName(data->pipeName);
    bool ok = true;

    this->m_peerMutex.lock();

    for (auto &client: this->m_clients)
        if (client.first == portName) {
            ok = false;

            break;
        }

    if (ok) {
        AkLogInfo() << "Adding Peer: " << portName << std::endl;
        this->m_clients[portName] = pipeName;
    }

    this->m_peerMutex.unlock();
    data->status = ok;
}

void AkVCam::ServicePrivate::removePort(AkVCam::Message *message)
{
    AkLogFunction();

    auto data = messageData<MsgRemovePort>(message);
    this->removePortByName(data->port);
}

void AkVCam::ServicePrivate::devicesUpdate(AkVCam::Message *message)
{
    AkLogFunction();
    auto data = messageData<MsgDevicesUpdated>(message);
    DeviceConfigs configs;

    for (size_t i = 0; i < Preferences::camerasCount(); i++) {
        auto device = Preferences::cameraPath(i);

        if (this->m_deviceConfigs.count(device) > 0)
            configs[device] = this->m_deviceConfigs[device];
        else
            configs[device] = {};
    }

    this->m_peerMutex.lock();
    this->m_deviceConfigs = configs;

    if (data->propagate)
        for (auto &client: this->m_clients)
            MessageServer::sendMessage(client.second, message);

    this->m_peerMutex.unlock();
}

void AkVCam::ServicePrivate::setBroadCasting(AkVCam::Message *message)
{
    AkLogFunction();
    auto data = messageData<MsgBroadcasting>(message);
    std::string deviceId(data->device);
    std::string broadcaster(data->broadcaster);
    data->status = false;

    if (this->m_deviceConfigs.count(deviceId) > 0)
        if (this->m_deviceConfigs[deviceId].broadcaster != broadcaster) {
            AkLogInfo() << "Device: " << deviceId << std::endl;
            AkLogInfo() << "Broadcaster: " << broadcaster << std::endl;
            this->m_deviceConfigs[deviceId].broadcaster = broadcaster;
            data->status = true;

            this->m_peerMutex.lock();

            for (auto &client: this->m_clients) {
                Message msg(message);
                MessageServer::sendMessage(client.second, &msg);
            }

            this->m_peerMutex.unlock();
        }
}

void AkVCam::ServicePrivate::frameReady(AkVCam::Message *message)
{
    AkLogFunction();
    this->m_peerMutex.lock();

    for (auto &client: this->m_clients)
        MessageServer::sendMessage(client.second, message);

    this->m_peerMutex.unlock();
}

void AkVCam::ServicePrivate::pictureUpdated(AkVCam::Message *message)
{
    AkLogFunction();
    this->m_peerMutex.lock();

    for (auto &client: this->m_clients)
        MessageServer::sendMessage(client.second, message);

    this->m_peerMutex.unlock();
}

void AkVCam::ServicePrivate::listeners(AkVCam::Message *message)
{
    AkLogFunction();
    auto data = messageData<MsgListeners>(message);
    std::string deviceId(data->device);

    if (this->m_deviceConfigs.count(deviceId) < 1)
        this->m_deviceConfigs[deviceId] = {};

    data->nlistener = this->m_deviceConfigs[deviceId].listeners.size();

    if (data->nlistener > 0) {
        memcpy(data->listener,
               this->m_deviceConfigs[deviceId].listeners[0].c_str(),
               std::min<size_t>(this->m_deviceConfigs[deviceId].listeners[0].size(),
                                MAX_STRING));
    }

    data->status = true;
}

void AkVCam::ServicePrivate::listener(AkVCam::Message *message)
{
    AkLogFunction();
    auto data = messageData<MsgListeners>(message);
    std::string deviceId(data->device);

    if (this->m_deviceConfigs.count(deviceId) < 1)
        this->m_deviceConfigs[deviceId] = {};

    auto nlistener = this->m_deviceConfigs[deviceId].listeners.size();

    if (data->nlistener >= nlistener) {
        data->status = false;

        return;
    }

    memcpy(data->listener,
           this->m_deviceConfigs[deviceId].listeners[data->nlistener].c_str(),
           std::min<size_t>(this->m_deviceConfigs[deviceId].listeners[data->nlistener].size(),
                            MAX_STRING));

    data->status = true;
}

void AkVCam::ServicePrivate::broadcasting(AkVCam::Message *message)
{
    AkLogFunction();
    auto data = messageData<MsgBroadcasting>(message);
    std::string deviceId(data->device);

    if (this->m_deviceConfigs.count(deviceId) < 1)
        this->m_deviceConfigs[deviceId] = {};

    memcpy(data->broadcaster,
           this->m_deviceConfigs[deviceId].broadcaster.c_str(),
           std::min<size_t>(this->m_deviceConfigs[deviceId].broadcaster.size(),
                            MAX_STRING));
    data->status = true;
}

void AkVCam::ServicePrivate::listenerAdd(AkVCam::Message *message)
{
    AkLogFunction();
    auto data = messageData<MsgListeners>(message);
    std::string deviceId(data->device);

    if (this->m_deviceConfigs.count(deviceId) < 1)
        this->m_deviceConfigs[deviceId] = {};

    auto &listeners = this->m_deviceConfigs[deviceId].listeners;
    std::string listener(data->listener);
    auto it = std::find(listeners.begin(), listeners.end(), listener);

    if (it == listeners.end()) {
        listeners.push_back(listener);
        data->nlistener = listeners.size();
        data->status = true;

        this->m_peerMutex.lock();

        for (auto &client: this->m_clients) {
            Message msg(message);
            MessageServer::sendMessage(client.second, &msg);
        }

        this->m_peerMutex.unlock();
    } else {
        data->nlistener = listeners.size();
        data->status = false;
    }
}

void AkVCam::ServicePrivate::listenerRemove(AkVCam::Message *message)
{
    AkLogFunction();
    auto data = messageData<MsgListeners>(message);
    std::string deviceId(data->device);

    if (this->m_deviceConfigs.count(deviceId) < 1)
        this->m_deviceConfigs[deviceId] = {};

    auto &listeners = this->m_deviceConfigs[deviceId].listeners;
    std::string listener(data->listener);
    auto it = std::find(listeners.begin(), listeners.end(), listener);

    if (it != listeners.end()) {
        listeners.erase(it);
        data->nlistener = listeners.size();
        data->status = true;

        this->m_peerMutex.lock();

        for (auto &client: this->m_clients) {
            Message msg(message);
            MessageServer::sendMessage(client.second, &msg);
        }

        this->m_peerMutex.unlock();
    } else {
        data->nlistener = listeners.size();
        data->status = false;
    }
}

void AkVCam::ServicePrivate::controlsUpdated(AkVCam::Message *message)
{
    AkLogFunction();
    this->m_peerMutex.lock();

    for (auto &client: this->m_clients)
        MessageServer::sendMessage(client.second, message);

    this->m_peerMutex.unlock();
}

DWORD WINAPI controlHandler(DWORD control,
                            DWORD  eventType,
                            LPVOID eventData,
                            LPVOID context)
{
    UNUSED(eventType);
    UNUSED(eventData);
    UNUSED(context);
    AkLogFunction();

    DWORD result = ERROR_CALL_NOT_IMPLEMENTED;

    switch (control) {
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
            AkVCam::servicePrivate()->sendStatus(SERVICE_STOP_PENDING,
                                                 NO_ERROR,
                                                 0);
            AkVCam::servicePrivate()->m_messageServer.stop(true);
            result = NO_ERROR;

            break;

        case SERVICE_CONTROL_INTERROGATE:
            result = NO_ERROR;

            break;

        default:
            break;
    }

    auto state = AkVCam::servicePrivate()->m_status.dwCurrentState;
    AkVCam::servicePrivate()->sendStatus(state, NO_ERROR, 0);

    return result;
}

BOOL WINAPI controlDebugHandler(DWORD control)
{
    AkLogFunction();

    if (control == CTRL_BREAK_EVENT || control == CTRL_C_EVENT) {
        AkVCam::servicePrivate()->m_messageServer.stop(true);

        return TRUE;
    }

    return FALSE;
}

void WINAPI serviceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
    UNUSED(dwArgc);
    UNUSED(lpszArgv);
    AkLogFunction();
    AkLogInfo() << "Setting service control handler" << std::endl;

    AkVCam::servicePrivate()->m_statusHandler =
            RegisterServiceCtrlHandlerEx(TEXT(DSHOW_PLUGIN_ASSISTANT_NAME),
                                         controlHandler,
                                         nullptr);

    if (!AkVCam::servicePrivate()->m_statusHandler)
        return;

    AkVCam::servicePrivate()->sendStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    AkLogInfo() << "Setting up service" << std::endl;
    AkVCam::servicePrivate()->m_messageServer
            .connectStateChanged(AkVCam::servicePrivate(),
                                 &AkVCam::ServicePrivate::stateChanged);
    AkVCam::servicePrivate()->m_messageServer.start(true);
}
