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
#include <codecvt>
#include <iostream>
#include <functional>
#include <sstream>

#include "cmdparser.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/image/videoformat.h"

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this->d, std::placeholders::_1, std::placeholders::_2)

namespace AkVCam {
    struct CmdParserFlags
    {
        StringVector flags;
        std::string value;
        std::string helpString;
    };

    struct CmdParserCommand
    {
        std::string command;
        std::string arguments;
        std::string helpString;
        ProgramOptionsFunc func;
        std::vector<CmdParserFlags> flags;
    };

    class CmdParserPrivate
    {
        public:
            std::vector<CmdParserCommand> m_commands;
            IpcBridge m_ipcBridge;
            bool m_parseable {false};

            std::string basename(const std::string &path);
            void printFlags(const std::vector<CmdParserFlags> &cmdFlags,
                            size_t indent);
            size_t maxCommandLength();
            size_t maxArgumentsLength();
            size_t maxFlagsLength(const std::vector<CmdParserFlags> &flags);
            size_t maxFlagsValueLength(const std::vector<CmdParserFlags> &flags);
            size_t maxStringLength(const StringVector &strings);
            size_t maxStringLength(const WStringVector &strings);
            std::string fill(const std::string &str, size_t maxSize);
            std::wstring fill(const std::wstring &str, size_t maxSize);
            std::string join(const StringVector &strings,
                             const std::string &separator);
            CmdParserCommand *parserCommand(const std::string &command);
            const CmdParserFlags *parserFlag(const std::vector<CmdParserFlags> &cmdFlags,
                                             const std::string &flag);
            bool containsFlag(const StringMap &flags,
                              const std::string &command,
                              const std::string &flagAlias);
            std::string flagValue(const StringMap &flags,
                                  const std::string &command,
                                  const std::string &flagAlias);
            int defaultHandler(const StringMap &flags,
                               const StringVector &args);
            int showHelp(const StringMap &flags, const StringVector &args);
            int showDevices(const StringMap &flags, const StringVector &args);
            int addDevice(const StringMap &flags, const StringVector &args);
            int removeDevice(const StringMap &flags, const StringVector &args);
            int showDeviceType(const StringMap &flags,
                               const StringVector &args);
            int showDeviceDescription(const StringMap &flags,
                                      const StringVector &args);
            int showSupportedFormats(const StringMap &flags,
                                     const StringVector &args);
            int showFormats(const StringMap &flags, const StringVector &args);
            int addFormat(const StringMap &flags, const StringVector &args);
            int removeFormat(const StringMap &flags, const StringVector &args);
            int update(const StringMap &flags, const StringVector &args);
            int loadSettings(const StringMap &flags, const StringVector &args);
            int showConnections(const StringMap &flags,
                                const StringVector &args);
            int connectDevices(const StringMap &flags,
                               const StringVector &args);
            int disconnectDevices(const StringMap &flags,
                                  const StringVector &args);
            int showOptions(const StringMap &flags, const StringVector &args);
            int readOption(const StringMap &flags, const StringVector &args);
            int writeOption(const StringMap &flags, const StringVector &args);
            int showClients(const StringMap &flags, const StringVector &args);
    };

    std::string operator *(const std::string &str, size_t n);
}

AkVCam::CmdParser::CmdParser()
{
    this->d = new CmdParserPrivate();
    this->d->m_commands.push_back({});
    this->setDefaultFuntion(AKVCAM_BIND_FUNC(CmdParserPrivate::defaultHandler));
    this->addFlags("",
    {"-h", "--help"},
                   "Show help.");
    this->addFlags("",
    {"-p", "--parseable"},
                   "Show parseable output.");
    this->addCommand("devices",
                     "",
                     "List devices.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showDevices));
    this->addCommand("add-device",
                     "DESCRIPTION",
                     "Add a new device.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::addDevice));
    this->addFlags("add-device",
    {"-i", "--input"},
                   "Add an input device.");
    this->addFlags("add-device",
    {"-o", "--output"},
                   "Add an output device.");
    this->addCommand("remove-device",
                     "DEVICE",
                     "Remove a device.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::removeDevice));
    this->addCommand("device-type",
                     "DEVICE",
                     "Show device type.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showDeviceType));
    this->addCommand("device-description",
                     "DEVICE",
                     "Show device description.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showDeviceDescription));
    this->addCommand("supported-formats",
                     "",
                     "Show supported formats.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showSupportedFormats));
    this->addFlags("supported-formats",
    {"-i", "--input"},
                   "Show supported input formats.");
    this->addFlags("supported-formats",
    {"-o", "--output"},
                   "Show supported output formats.");
    this->addCommand("formats",
                     "DEVICE",
                     "Show device formats.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showFormats));
    this->addCommand("add-format",
                     "DEVICE FORMAT WIDTH HEIGHT FPS",
                     "Add a new device format.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::addFormat));
    this->addFlags("add-format",
    {"-i", "--index"},
                   "INDEX",
                   "Add format at INDEX.");
    this->addCommand("remove-format",
                     "DEVICE INDEX",
                     "Remove device format.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::removeFormat));
    this->addCommand("update",
                     "",
                     "Update devices.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::update));
    this->addCommand("load",
                     "SETTINGS.INI",
                     "Create devices from a setting file.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::loadSettings));
    this->addCommand("connections",
                     "[DEVICE]",
                     "Show device connections.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showConnections));
    this->addCommand("connect",
                     "OUTPUT_DEVICE INPUTDEVICE [INPUT_DEVICE ...]",
                     "Connect devices.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::connectDevices));
    this->addCommand("disconnect",
                     "OUTPUT_DEVICE INPUTDEVICE",
                     "Disconnect devices.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::disconnectDevices));
    this->addCommand("options",
                     "DEVICE",
                     "Show device options.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showOptions));
    this->addCommand("get-option",
                     "DEVICE OPTION",
                     "Read device option.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::readOption));
    this->addCommand("set-option",
                     "DEVICE OPTION VALUE",
                     "Write device option value.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::writeOption));
    this->addCommand("clients",
                     "",
                     "Show clients using the camera.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showClients));
    this->d->m_ipcBridge.connectService();
}

AkVCam::CmdParser::~CmdParser()
{
    this->d->m_ipcBridge.disconnectService();
    delete this->d;
}

int AkVCam::CmdParser::parse(int argc, char **argv)
{
    auto program = this->d->basename(argv[0]);
    auto command = &this->d->m_commands[0];
    StringMap flags;
    StringVector arguments {program};

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg[0] == '-') {
            auto flag = this->d->parserFlag(command->flags, arg);

            if (!flag) {
                if (command->command.empty())
                    std::cout << "Invalid option '"
                              << arg
                              << "'"
                              << std::endl;
                else
                    std::cout << "Invalid option '"
                              << arg << "' for '"
                              << command->command
                              << "'"
                              << std::endl;

                return -1;
            }

            std::string value;

            if (!flag->value.empty()) {
                auto next = i + 1;

                if (next < argc) {
                    value = argv[next];
                    i++;
                }
            }

            flags[arg] = value;
        } else {
            if (command->command.empty()) {
                if (!flags.empty()) {
                    auto result = command->func(flags, {program});

                    if (result < 0)
                        return result;

                    flags.clear();
                }

                auto cmd = this->d->parserCommand(arg);

                if (cmd) {
                    command = cmd;
                    flags.clear();
                } else {
                    std::cout << "Unknown command '" << arg << "'" << std::endl;

                    return -1;
                }
            } else {
                arguments.push_back(arg);
            }
        }
    }

    return command->func(flags, arguments);
}

void AkVCam::CmdParser::setDefaultFuntion(const ProgramOptionsFunc &func)
{
    this->d->m_commands[0].func = func;
}

void AkVCam::CmdParser::addCommand(const std::string &command,
                                   const std::string &arguments,
                                   const std::string &helpString,
                                   const ProgramOptionsFunc &func)
{
    auto it =
            std::find_if(this->d->m_commands.begin(),
                         this->d->m_commands.end(),
                         [&command] (const CmdParserCommand &cmd) -> bool {
        return cmd.command == command;
    });

    if (it == this->d->m_commands.end()) {
        this->d->m_commands.push_back({command,
                                       arguments,
                                       helpString,
                                       func,
                                       {}});
    } else {
        it->command = command;
        it->arguments = arguments;
        it->helpString = helpString;
        it->func = func;
    }
}

void AkVCam::CmdParser::addFlags(const std::string &command,
                                 const StringVector &flags,
                                 const std::string &value,
                                 const std::string &helpString)
{
    auto it =
            std::find_if(this->d->m_commands.begin(),
                         this->d->m_commands.end(),
                         [&command] (const CmdParserCommand &cmd) -> bool {
        return cmd.command == command;
    });

    if (it == this->d->m_commands.end())
        return;

    it->flags.push_back({flags, value, helpString});
}

void AkVCam::CmdParser::addFlags(const std::string &command,
                                 const StringVector &flags,
                                 const std::string &helpString)
{
    this->addFlags(command, flags, "", helpString);
}

std::string AkVCam::CmdParserPrivate::basename(const std::string &path)
{
    auto rit =
            std::find_if(path.rbegin(),
                         path.rend(),
                         [] (char c) -> bool {
        return c == '/' || c == '\\';
    });

    auto program =
            rit == path.rend()?
                path:
                path.substr(path.size() + (path.rbegin() - rit));

    auto it =
            std::find_if(program.begin(),
                         program.end(),
                         [] (char c) -> bool {
        return c == '.';
    });

    program =
            it == path.end()?
                program:
                program.substr(0, it - program.begin());

    return program;
}

void AkVCam::CmdParserPrivate::printFlags(const std::vector<CmdParserFlags> &cmdFlags,
                                          size_t indent)
{
    std::vector<char> spaces(indent, ' ');
    auto maxFlagsLen = this->maxFlagsLength(cmdFlags);
    auto maxFlagsValueLen = this->maxFlagsValueLength(cmdFlags);

    for (auto &flag: cmdFlags) {
        auto allFlags = this->join(flag.flags, ", ");
        std::cout << std::string(spaces.data(), indent)
                  << this->fill(allFlags, maxFlagsLen);

        if (maxFlagsValueLen > 0) {
            std::cout << " "
                      << this->fill(flag.value, maxFlagsValueLen);
        }

        std::cout << "    "
                  << flag.helpString
                  << std::endl;
    }
}

size_t AkVCam::CmdParserPrivate::maxCommandLength()
{
    size_t length = 0;

    for (auto &cmd: this->m_commands)
        length = std::max(cmd.command.size(), length);

    return length;
}

size_t AkVCam::CmdParserPrivate::maxArgumentsLength()
{
    size_t length = 0;

    for (auto &cmd: this->m_commands)
        length = std::max(cmd.arguments.size(), length);

    return length;
}

size_t AkVCam::CmdParserPrivate::maxFlagsLength(const std::vector<CmdParserFlags> &flags)
{
    size_t length = 0;

    for (auto &flag: flags)
        length = std::max(this->join(flag.flags, ", ").size(), length);

    return length;
}

size_t AkVCam::CmdParserPrivate::maxFlagsValueLength(const std::vector<CmdParserFlags> &flags)
{
    size_t length = 0;

    for (auto &flag: flags)
        length = std::max(flag.value.size(), length);

    return length;
}

size_t AkVCam::CmdParserPrivate::maxStringLength(const StringVector &strings)
{
    size_t length = 0;

    for (auto &str: strings)
        length = std::max(str.size(), length);

    return length;
}

size_t AkVCam::CmdParserPrivate::maxStringLength(const WStringVector &strings)
{
    size_t length = 0;

    for (auto &str: strings)
        length = std::max(str.size(), length);

    return length;
}

std::string AkVCam::CmdParserPrivate::fill(const std::string &str,
                                           size_t maxSize)
{
    std::stringstream ss;
    std::vector<char> spaces(maxSize, ' ');
    ss << str << std::string(spaces.data(), maxSize - str.size());

    return ss.str();
}

std::wstring AkVCam::CmdParserPrivate::fill(const std::wstring &str, size_t maxSize)
{
    std::wstringstream ss;
    std::vector<wchar_t> spaces(maxSize, ' ');
    ss << str << std::wstring(spaces.data(), maxSize - str.size());

    return ss.str();
}

std::string AkVCam::CmdParserPrivate::join(const StringVector &strings,
                                           const std::string &separator)
{
    std::stringstream ss;
    bool append = false;

    for (auto &str: strings) {
        if (append)
            ss << separator;
        else
            append = true;

        ss << str;
    }

    return ss.str();
}

AkVCam::CmdParserCommand *AkVCam::CmdParserPrivate::parserCommand(const std::string &command)
{
    for (auto &cmd: this->m_commands)
        if (cmd.command == command)
            return &cmd;

    return nullptr;
}

const AkVCam::CmdParserFlags *AkVCam::CmdParserPrivate::parserFlag(const std::vector<CmdParserFlags> &cmdFlags,
                                                                   const std::string &flag)
{
    for (auto &flags: cmdFlags)
        for (auto &f: flags.flags)
            if (f == flag)
                return &flags;

    return nullptr;
}

bool AkVCam::CmdParserPrivate::containsFlag(const StringMap &flags,
                                            const std::string &command,
                                            const std::string &flagAlias)
{
    for (auto &cmd: this->m_commands)
        if (cmd.command == command) {
            for (auto &flag: cmd.flags) {
                auto it = std::find(flag.flags.begin(),
                                    flag.flags.end(),
                                    flagAlias);

                if (it != flag.flags.end()) {
                    for (auto &f1: flags)
                        for (auto &f2: flag.flags)
                            if (f1.first == f2)
                                return true;

                    return false;
                }
            }

            return false;
        }

    return false;
}

std::string AkVCam::CmdParserPrivate::flagValue(const AkVCam::StringMap &flags,
                                                const std::string &command,
                                                const std::string &flagAlias)
{
    for (auto &cmd: this->m_commands)
        if (cmd.command == command) {
            for (auto &flag: cmd.flags) {
                auto it = std::find(flag.flags.begin(),
                                    flag.flags.end(),
                                    flagAlias);

                if (it != flag.flags.end()) {
                    for (auto &f1: flags)
                        for (auto &f2: flag.flags)
                            if (f1.first == f2)
                                return f1.second;

                    return {};
                }
            }

            return {};
        }

    return {};
}

int AkVCam::CmdParserPrivate::defaultHandler(const StringMap &flags,
                                             const StringVector &args)
{
    if (flags.empty() || this->containsFlag(flags, "", "-h"))
        return this->showHelp(flags, args);

    if (this->containsFlag(flags, "", "-p"))
        this->m_parseable = true;

    return 0;
}

int AkVCam::CmdParserPrivate::showHelp(const StringMap &flags,
                                       const StringVector &args)
{
    UNUSED(flags);

    std::cout << args[0]
            << " [OPTIONS...] COMMAND [COMMAND_OPTIONS...] ..."
            << std::endl;
    std::cout << std::endl;
    std::cout << "AkVirtualCamera virtual device manager." << std::endl;
    std::cout << std::endl;
    std::cout << "General Options:" << std::endl;
    std::cout << std::endl;
    this->printFlags(this->m_commands[0].flags, 4);
    std::cout << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << std::endl;

    auto maxCmdLen = this->maxCommandLength();
    auto maxArgsLen = this->maxArgumentsLength();

    for (auto &cmd: this->m_commands) {
        if (cmd.command.empty())
            continue;

        std::cout << "    "
                  << this->fill(cmd.command, maxCmdLen)
                  << " "
                  << this->fill(cmd.arguments, maxArgsLen)
                  << "    "
                  << cmd.helpString << std::endl;

        if (!cmd.flags.empty())
            std::cout << std::endl;

        this->printFlags(cmd.flags, 8);

        if (!cmd.flags.empty())
            std::cout << std::endl;
    }

    return 0;
}

int AkVCam::CmdParserPrivate::showDevices(const StringMap &flags,
                                          const StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);

    if (this->m_parseable) {
        for (auto &device: this->m_ipcBridge.listDevices())
            std::cout << device << std::endl;
    } else {
        StringVector devicesColumn;
        StringVector typesColumn;
        WStringVector descriptionsColumn;

        devicesColumn.push_back("Device");
        typesColumn.push_back("Type");
        descriptionsColumn.push_back(L"Description");

        for (auto &device: this->m_ipcBridge.listDevices()) {
            devicesColumn.push_back(device);
            typesColumn.push_back(this->m_ipcBridge.deviceType(device) == IpcBridge::DeviceTypeInput?
                                      "Input":
                                      "Output");
            descriptionsColumn.push_back(this->m_ipcBridge.description(device));
        }

        auto devicesColumnSize = this->maxStringLength(devicesColumn);
        auto typesColumnSize = this->maxStringLength(typesColumn);
        auto descriptionsColumnSize = this->maxStringLength(descriptionsColumn);

        std::cout << this->fill("Device", devicesColumnSize)
                  << " | "
                  << this->fill("Type", typesColumnSize)
                  << " | "
                  << this->fill("Description", descriptionsColumnSize)
                  << std::endl;
        std::cout << std::string("-")
                     * (devicesColumnSize
                        + typesColumnSize
                        + descriptionsColumnSize
                        + 6) << std::endl;

        for (auto &device: this->m_ipcBridge.listDevices()) {
            std::string type =
                    this->m_ipcBridge.deviceType(device) == IpcBridge::DeviceTypeInput?
                        "Input":
                        "Output";
            std::cout << this->fill(device, devicesColumnSize)
                      << " | "
                      << this->fill(type, typesColumnSize)
                      << " | ";
            std::wcout << this->fill(this->m_ipcBridge.description(device),
                                     descriptionsColumnSize)
                       << std::endl;
        }
    }

    return 0;
}

int AkVCam::CmdParserPrivate::addDevice(const StringMap &flags,
                                        const StringVector &args)
{
    if (args.size() < 2) {
        std::cerr << "Device description not provided." << std::endl;

        return -1;
    }

    IpcBridge::DeviceType type =
            this->containsFlag(flags, "add-device", "-i")?
                IpcBridge::DeviceTypeInput:
                IpcBridge::DeviceTypeOutput;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cv;
    auto deviceId = this->m_ipcBridge.addDevice(cv.from_bytes(args[1]), type);

    if (deviceId.empty()) {
        std::cerr << "Failed to create device." << std::endl;

        return -1;
    }

    if (this->m_parseable)
        std::cout << deviceId << std::endl;
    else
        std::cout << "Device created as " << deviceId << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::removeDevice(const StringMap &flags,
                                           const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Device not provided." << std::endl;

        return -1;
    }

    auto devices = this->m_ipcBridge.listDevices();
    auto it = std::find(devices.begin(), devices.end(), args[1]);

    if (it == devices.end()) {
        std::cerr << "Device not doesn't exists." << std::endl;

        return -1;
    }

    this->m_ipcBridge.removeDevice(args[1]);

    return 0;
}

int AkVCam::CmdParserPrivate::showDeviceType(const StringMap &flags,
                                             const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Device not provided." << std::endl;

        return -1;
    }

    auto devices = this->m_ipcBridge.listDevices();
    auto it = std::find(devices.begin(), devices.end(), args[1]);

    if (it == devices.end()) {
        std::cerr << "Device not doesn't exists." << std::endl;

        return -1;
    }

    if (this->m_ipcBridge.deviceType(args[1]) == IpcBridge::DeviceTypeInput)
        std::cout << "Input" << std::endl;
    else
        std::cout << "Output" << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::showDeviceDescription(const StringMap &flags,
                                                    const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Device not provided." << std::endl;

        return -1;
    }

    auto devices = this->m_ipcBridge.listDevices();
    auto it = std::find(devices.begin(), devices.end(), args[1]);

    if (it == devices.end()) {
        std::cerr << "Device not doesn't exists." << std::endl;

        return -1;
    }

    std::wcout << this->m_ipcBridge.description(args[1]) << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::showSupportedFormats(const StringMap &flags,
                                                   const StringVector &args)
{
    UNUSED(args);

    auto type =
            this->containsFlag(flags, "supported-formats", "-i")?
                IpcBridge::DeviceTypeInput:
                IpcBridge::DeviceTypeOutput;
    auto formats = this->m_ipcBridge.supportedPixelFormats(type);

    if (!this->m_parseable) {
        if (type == IpcBridge::DeviceTypeInput)
            std::cout << "Input formats:" << std::endl;
        else
            std::cout << "Output formats:" << std::endl;

        std::cout << std::endl;
    }

    for (auto &format: formats)
        std::cout << VideoFormat::stringFromFourcc(format) << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::showFormats(const StringMap &flags,
                                          const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Device not provided." << std::endl;

        return -1;
    }

    auto devices = this->m_ipcBridge.listDevices();
    auto it = std::find(devices.begin(), devices.end(), args[1]);

    if (it == devices.end()) {
        std::cerr << "Device not doesn't exists." << std::endl;

        return -1;
    }

    if (this->m_parseable) {
        for  (auto &format: this->m_ipcBridge.formats(args[1]))
            std::cout << VideoFormat::stringFromFourcc(format.fourcc())
                      << ' '
                      << format.width()
                      << ' '
                      << format.height()
                      << ' '
                      << format.minimumFrameRate().num()
                      << ' '
                      << format.minimumFrameRate().den()
                      << std::endl;
    } else {
        int i = 0;

        for  (auto &format: this->m_ipcBridge.formats(args[1])) {
            std::cout << i
                      << ": "
                      << VideoFormat::stringFromFourcc(format.fourcc())
                      << ' '
                      << format.width()
                      << 'x'
                      << format.height()
                      << ' '
                      << format.minimumFrameRate().num()
                      << '/'
                      << format.minimumFrameRate().den()
                      << " FPS"
                      << std::endl;
            i++;
        }
    }

    return 0;
}

int AkVCam::CmdParserPrivate::addFormat(const StringMap &flags,
                                        const StringVector &args)
{
    if (args.size() < 6) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.listDevices();
    auto dit = std::find(devices.begin(), devices.end(), args[1]);

    if (dit == devices.end()) {
        std::cerr << "Device not doesn't exists." << std::endl;

        return -1;
    }

    auto format = VideoFormat::fourccFromString(args[2]);

    if (!format) {
        std::cerr << "Invalid pixel format." << std::endl;

        return -1;
    }

    auto type = this->m_ipcBridge.deviceType(deviceId);
    auto formats = this->m_ipcBridge.supportedPixelFormats(type);
    auto fit = std::find(formats.begin(), formats.end(), format);

    if (fit == formats.end()) {
        if (type == IpcBridge::DeviceTypeInput)
            std::cerr << "Input format not supported." << std::endl;
        else
            std::cerr << "Output format not supported." << std::endl;

        return -1;
    }

    char *p = nullptr;
    auto width = strtoul(args[3].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Width must be an unsigned integer." << std::endl;

        return -1;
    }

    p = nullptr;
    auto height = strtoul(args[4].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Height must be an unsigned integer." << std::endl;

        return -1;
    }

    Fraction fps(args[5]);

    if (fps.num() < 1 || fps.den() < 1) {
        std::cerr << "Invalid frame rate." << std::endl;

        return -1;
    }

    auto indexStr = this->flagValue(flags, "add-format", "-i");
    int index = -1;

    if (!indexStr.empty()) {
        p = nullptr;
        index = strtoul(indexStr.c_str(), &p, 10);

        if (*p) {
            std::cerr << "Index must be an unsigned integer." << std::endl;

            return -1;
        }
    }

    VideoFormat fmt(format, width, height, {fps});
    this->m_ipcBridge.addFormat(deviceId, fmt, index);

    return 0;
}

int AkVCam::CmdParserPrivate::removeFormat(const StringMap &flags,
                                           const StringVector &args)
{
    if (args.size() < 3) {
        std::cerr << "Not enough arguments." << std::endl;

        return -1;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.listDevices();
    auto dit = std::find(devices.begin(), devices.end(), args[1]);

    if (dit == devices.end()) {
        std::cerr << "Device not doesn't exists." << std::endl;

        return -1;
    }

    char *p = nullptr;
    auto index = strtoul(args[2].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Index must be an unsigned integer." << std::endl;

        return -1;
    }

    auto formats = this->m_ipcBridge.formats(deviceId);

    if (index >= formats.size()) {
        std::cerr << "Index is out of range." << std::endl;

        return -1;
    }

    this->m_ipcBridge.removeFormat(deviceId, index);

    return 0;
}

int AkVCam::CmdParserPrivate::update(const StringMap &flags,
                                     const StringVector &args)
{
    return 0;
}

int AkVCam::CmdParserPrivate::loadSettings(const AkVCam::StringMap &flags,
                                           const AkVCam::StringVector &args)
{
    return 0;
}

int AkVCam::CmdParserPrivate::showConnections(const StringMap &flags,
                                              const StringVector &args)
{
    return 0;
}

int AkVCam::CmdParserPrivate::connectDevices(const StringMap &flags,
                                             const StringVector &args)
{
    return 0;
}

int AkVCam::CmdParserPrivate::disconnectDevices(const StringMap &flags,
                                                const StringVector &args)
{
    return 0;
}

int AkVCam::CmdParserPrivate::showOptions(const StringMap &flags,
                                          const StringVector &args)
{
    return 0;
}

int AkVCam::CmdParserPrivate::readOption(const StringMap &flags,
                                         const StringVector &args)
{
    return 0;
}

int AkVCam::CmdParserPrivate::writeOption(const StringMap &flags,
                                          const StringVector &args)
{
    return 0;
}

int AkVCam::CmdParserPrivate::showClients(const StringMap &flags,
                                          const StringVector &args)
{
    return 0;
}

std::string AkVCam::operator *(const std::string &str, size_t n)
{
    std::stringstream ss;

    for (size_t i = 0; i < n; i++)
        ss << str;

    return ss.str();
}
