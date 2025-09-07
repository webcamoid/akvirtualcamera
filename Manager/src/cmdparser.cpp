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
#include <cerrno>
#include <chrono>
#include <cmath>
#include <codecvt>
#include <csignal>
#include <cstring>
#include <iostream>
#include <functional>
#include <locale>
#include <random>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "cmdparser.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/fraction.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/settings.h"
#include "VCamUtils/src/videoformat.h"
#include "VCamUtils/src/videoframe.h"
#include "VCamUtils/src/logger.h"

#define COMMONS_PROJECT_COMMIT_URL "https://github.com/webcamoid/akvirtualcamera/commit"

#define AKVCAM_BIND_FUNC(member) \
    std::bind(&member, this->d, std::placeholders::_1, std::placeholders::_2)

namespace AkVCam {
    using StringMatrix = std::vector<StringVector>;
    using VideoFormatMatrix = std::vector<std::vector<VideoFormat>>;

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
        bool advanced {false};

        CmdParserCommand();
        CmdParserCommand(const std::string &command,
                         const std::string &arguments,
                         const std::string &helpString,
                         const ProgramOptionsFunc &func,
                         const std::vector<CmdParserFlags> &flags,
                         bool advanced);
    };

    class CmdParserPrivate
    {
        public:
            std::vector<CmdParserCommand> m_commands;
            IpcBridge m_ipcBridge;
            bool m_parseable {false};
            bool m_force {false};

            static const std::map<ControlType, std::string> &typeStrMap();
            void printFlags(const std::vector<CmdParserFlags> &cmdFlags,
                            size_t indent);
            size_t maxCommandLength(bool showAdvancedHelp);
            size_t maxArgumentsLength(bool showAdvancedHelp);
            size_t maxFlagsLength(const std::vector<CmdParserFlags> &flags);
            size_t maxFlagsValueLength(const std::vector<CmdParserFlags> &flags);
            size_t maxColumnLength(const StringVector &table,
                                   size_t width,
                                   size_t column);
            std::vector<size_t> maxColumnsLength(const StringVector &table,
                                                 size_t width);
            void drawTableHLine(const std::vector<size_t> &columnsLength,
                                bool toStdErr=false);
            void drawTable(const StringVector &table,
                           size_t width,
                           bool toStdErr=false);
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
            int removeDevices(const StringMap &flags, const StringVector &args);
            int showDeviceDescription(const StringMap &flags,
                                      const StringVector &args);
            int setDeviceDescription(const StringMap &flags,
                                     const StringVector &args);
            int showSupportedFormats(const StringMap &flags,
                                     const StringVector &args);
            int showDefaultFormat(const StringMap &flags,
                                  const StringVector &args);
            int showFormats(const StringMap &flags, const StringVector &args);
            int addFormat(const StringMap &flags, const StringVector &args);
            int removeFormat(const StringMap &flags, const StringVector &args);
            int removeFormats(const StringMap &flags, const StringVector &args);
            int update(const StringMap &flags, const StringVector &args);
            int loadSettings(const StringMap &flags, const StringVector &args);
            int stream(const StringMap &flags, const StringVector &args);
            int streamPattern(const StringMap &flags, const StringVector &args);
            int listenEvents(const StringMap &flags, const StringVector &args);
            int showControls(const StringMap &flags, const StringVector &args);
            int readControl(const StringMap &flags, const StringVector &args);
            int writeControls(const StringMap &flags, const StringVector &args);
            int picture(const StringMap &flags, const StringVector &args);
            int setPicture(const StringMap &flags, const StringVector &args);
            int logLevel(const StringMap &flags, const StringVector &args);
            int setLogLevel(const StringMap &flags, const StringVector &args);
            int showClients(const StringMap &flags, const StringVector &args);
            int dumpInfo(const StringMap &flags, const StringVector &args);
            int hacks(const StringMap &flags, const StringVector &args);
            int hackInfo(const StringMap &flags, const StringVector &args);
            int hack(const StringMap &flags, const StringVector &args);
            void loadGenerals(Settings &settings);
            VideoFormatMatrix readFormats(Settings &settings);
            std::vector<VideoFormat> readFormat(Settings &settings);
            StringMatrix matrixCombine(const StringMatrix &matrix);
            void matrixCombineP(const StringMatrix &matrix,
                                size_t index,
                                StringVector combined,
                                StringMatrix &combinations);
            void createDevices(Settings &settings,
                               const VideoFormatMatrix &availableFormats);
            void createDevice(Settings &settings,
                              const VideoFormatMatrix &availableFormats);
            std::vector<VideoFormat> readDeviceFormats(Settings &settings,
                                                       const VideoFormatMatrix &availableFormats);
    };

    std::string operator *(const std::string &str, size_t n);
    std::string operator *(size_t n, const std::string &str);
}

AkVCam::CmdParser::CmdParser()
{
    this->d = new CmdParserPrivate();
    auto logFile = this->d->m_ipcBridge.logPath("AkVCamManager");
    AkLogInfo() << "Sending debug output to " << logFile << std::endl;
    AkVCam::Logger::setLogFile(logFile);

    this->d->m_commands.push_back({});
    this->setDefaultFuntion(AKVCAM_BIND_FUNC(CmdParserPrivate::defaultHandler));
    this->addFlags("",
                   {"-h", "--help"},
                   "Show help.");
    this->addFlags("",
                   {"--help-all"},
                   "Show advanced help.");
    this->addFlags("",
                   {"-v", "--version"},
                   "Show program version.");
    this->addFlags("",
                   {"-p", "--parseable"},
                   "Show parseable output.");
    this->addFlags("",
                   {"-f", "--force"},
                   "Force command.");
    this->addFlags("",
                   {"--build-info"},
                   "Show build information.");
    this->addCommand("devices",
                     "",
                     "List devices.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showDevices));
    this->addCommand("add-device",
                     "DESCRIPTION",
                     "Add a new device.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::addDevice));
    this->addFlags("add-device",
                   {"-i", "--id"},
                   "DEVICEID",
                   "Create device as DEVICEID.");
    this->addCommand("remove-device",
                     "DEVICE",
                     "Remove a device.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::removeDevice));
    this->addCommand("remove-devices",
                     "",
                     "Remove all devices.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::removeDevices));
    this->addCommand("description",
                     "DEVICE",
                     "Show device description.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showDeviceDescription));
    this->addCommand("set-description",
                     "DEVICE DESCRIPTION",
                     "Set device description.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::setDeviceDescription));
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
    this->addCommand("default-format",
                     "",
                     "Default device format.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showDefaultFormat));
    this->addFlags("default-format",
                   {"-i", "--input"},
                   "Default input format.");
    this->addFlags("default-format",
                   {"-o", "--output"},
                   "Default output format.");
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
    this->addCommand("remove-formats",
                     "DEVICE",
                     "Remove all device formats.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::removeFormats));
    this->addCommand("update",
                     "",
                     "Update devices.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::update));
    this->addCommand("load",
                     "SETTINGS.INI",
                     "Create devices from a setting file.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::loadSettings));
    this->addCommand("stream",
                     "DEVICE FORMAT WIDTH HEIGHT",
                     "Read frames from stdin and send them to the device.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::stream));
    this->addFlags("stream",
                   {"-f", "--fps"},
                   "FPS",
                   "Read stream input at a constant frame rate.");
    this->addCommand("stream-pattern",
                     "DEVICE WIDTH HEIGHT",
                     "Send a test video pattern to the device.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::streamPattern));
    this->addFlags("stream-pattern",
                   {"-f", "--fps"},
                   "FPS",
                   "Send test pattern at a constant frame rate.");
    this->addCommand("listen-events",
                     "",
                     "Keep the manager running and listening to global events.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::listenEvents));
    this->addCommand("controls",
                     "DEVICE",
                     "Show device controls.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showControls));
    this->addCommand("get-control",
                     "DEVICE CONTROL",
                     "Read device control.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::readControl));
    this->addFlags("get-control",
                   {"-c", "--description"},
                   "Show control description.");
    this->addFlags("get-control",
                   {"-t", "--type"},
                   "Show control type.");
    this->addFlags("get-control",
                   {"-m", "--min"},
                   "Show minimum value for the control.");
    this->addFlags("get-control",
                   {"-M", "--max"},
                   "Show maximum value for the control.");
    this->addFlags("get-control",
                   {"-s", "--step"},
                   "Show increment/decrement step for the control.");
    this->addFlags("get-control",
                   {"-d", "--default"},
                   "Show default value for the control.");
    this->addFlags("get-control",
                   {"-l", "--menu"},
                   "Show options of a memu type control.");
    this->addCommand("set-controls",
                     "DEVICE CONTROL_1=VALUE CONTROL_2=VALUE...",
                     "Write device controls values.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::writeControls));
    this->addCommand("picture",
                     "",
                     "Placeholder picture to show when no streaming.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::picture));
    this->addCommand("set-picture",
                     "FILE",
                     "Set placeholder picture.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::setPicture));
    this->addCommand("loglevel",
                     "",
                     "Show current debugging level.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::logLevel));
    this->addCommand("set-loglevel",
                     "LEVEL",
                     "Set debugging level.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::setLogLevel));
    this->addCommand("clients",
                     "",
                     "Show clients using the camera.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::showClients));
    this->addCommand("dump",
                     "",
                     "Show all information in a parseable XML format.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::dumpInfo));
    this->addCommand("hacks",
                     "",
                     "List system hacks to make the virtual camera work.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::hacks),
                     true);
    this->addCommand("hack-info",
                     "HACK",
                     "Show hack information.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::hackInfo),
                     true);
    this->addFlags("hack-info",
                   {"-s", "--issafe"},
                   "Is hack safe?");
    this->addFlags("hack-info",
                   {"-c", "--description"},
                   "Show hack description.");
    this->addCommand("hack",
                     "HACK PARAMS...",
                     "Apply system hack.",
                     AKVCAM_BIND_FUNC(CmdParserPrivate::hack),
                     true);
    this->addFlags("hack",
                   {"-y", "--yes"},
                   "Accept all risks and continue anyway.");
}

AkVCam::CmdParser::~CmdParser()
{
    delete this->d;
}

int AkVCam::CmdParser::parse(int argc, char **argv)
{
    auto program = basename(argv[0]);
    auto command = &this->d->m_commands[0];
    StringMap flags;
    StringVector arguments {program};

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        char *p = nullptr;
        strtod(arg.c_str(), &p);

        if (arg[0] == '-' && p && strnlen(p, 1024) != 0) {
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

                return -EINVAL;
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

                    return -EINVAL;
                }
            } else {
                arguments.push_back(arg);
            }
        }
    }

    if (!this->d->m_force && this->d->m_ipcBridge.isBusyFor(command->command)) {
        std::cerr << "This operation is not permitted." << std::endl;
        std::cerr << "The virtual camera is in use. Stop or close the virtual "
                  << "camera clients and try again." << std::endl;
        std::cerr << std::endl;
        auto clients = this->d->m_ipcBridge.clientsPids();

        if (!clients.empty()) {
            std::vector<std::string> table {
                "Pid",
                "Executable"
            };
            auto columns = table.size();

            for (auto &pid: clients) {
                table.push_back(std::to_string(pid));
                table.push_back(this->d->m_ipcBridge.clientExe(pid));
            }

            this->d->drawTable(table, columns, true);
        }

        return -EBUSY;
    }

    if (this->d->m_ipcBridge.needsRoot(command->command)
        || (command->command == "hack"
            && arguments.size() >= 2
            && this->d->m_ipcBridge.hackNeedsRoot(arguments[1]))) {
        std::cerr << "You must run this command with administrator privileges." << std::endl;

        return -EPERM;
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
                                   const ProgramOptionsFunc &func,
                                   bool advanced)
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
                                       {},
                                       advanced});
    } else {
        it->command = command;
        it->arguments = arguments;
        it->helpString = helpString;
        it->func = func;
        it->advanced = advanced;
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

const std::map<AkVCam::ControlType, std::string> &AkVCam::CmdParserPrivate::typeStrMap()
{
    static const std::map<ControlType, std::string> typeStr {
        {ControlTypeInteger, "Integer"},
        {ControlTypeBoolean, "Boolean"},
        {ControlTypeMenu   , "Menu"   },
    };

    return typeStr;
}

void AkVCam::CmdParserPrivate::printFlags(const std::vector<CmdParserFlags> &cmdFlags,
                                          size_t indent)
{
    std::vector<char> spaces(indent, ' ');
    auto maxFlagsLen = this->maxFlagsLength(cmdFlags);
    auto maxFlagsValueLen = this->maxFlagsValueLength(cmdFlags);

    for (auto &flag: cmdFlags) {
        auto allFlags = join(flag.flags, ", ");
        std::cout << std::string(spaces.data(), indent)
                  << fill(allFlags, maxFlagsLen);

        if (maxFlagsValueLen > 0)
            std::cout << " " << fill(flag.value, maxFlagsValueLen);

        std::cout << "    "
                  << flag.helpString
                  << std::endl;
    }
}

size_t AkVCam::CmdParserPrivate::maxCommandLength(bool showAdvancedHelp)
{
    size_t length = 0;

    for (auto &cmd: this->m_commands)
        if (!cmd.advanced || showAdvancedHelp)
            length = std::max(cmd.command.size(), length);

    return length;
}

size_t AkVCam::CmdParserPrivate::maxArgumentsLength(bool showAdvancedHelp)
{
    size_t length = 0;

    for (auto &cmd: this->m_commands)
        if (!cmd.advanced || showAdvancedHelp)
            length = std::max(cmd.arguments.size(), length);

    return length;
}

size_t AkVCam::CmdParserPrivate::maxFlagsLength(const std::vector<CmdParserFlags> &flags)
{
    size_t length = 0;

    for (auto &flag: flags)
        length = std::max(join(flag.flags, ", ").size(), length);

    return length;
}

size_t AkVCam::CmdParserPrivate::maxFlagsValueLength(const std::vector<CmdParserFlags> &flags)
{
    size_t length = 0;

    for (auto &flag: flags)
        length = std::max(flag.value.size(), length);

    return length;
}

size_t AkVCam::CmdParserPrivate::maxColumnLength(const AkVCam::StringVector &table,
                                                 size_t width,
                                                 size_t column)
{
    size_t length = 0;
    size_t height = table.size() / width;

    for (size_t y = 0; y < height; y++) {
        auto &str = table[y * width + column];
        length = std::max(str.size(), length);
    }

    return length;
}

std::vector<size_t> AkVCam::CmdParserPrivate::maxColumnsLength(const AkVCam::StringVector &table,
                                                               size_t width)
{
    std::vector<size_t> lengths;

    for (size_t x = 0; x < width; x++)
        lengths.push_back(this->maxColumnLength(table, width, x));

    return lengths;
}

void AkVCam::CmdParserPrivate::drawTableHLine(const std::vector<size_t> &columnsLength,
                                              bool toStdErr)
{
    std::ostream *out = &std::cout;

    if (toStdErr)
        out = &std::cerr;

    *out << '+';

    for (auto &len: columnsLength)
        *out << std::string("-") * (len + 2) << '+';

    *out << std::endl;
}

void AkVCam::CmdParserPrivate::drawTable(const AkVCam::StringVector &table,
                                         size_t width,
                                         bool toStdErr)
{
    size_t height = table.size() / width;
    auto columnsLength = this->maxColumnsLength(table, width);
    this->drawTableHLine(columnsLength, toStdErr);
    std::ostream *out = &std::cout;

    if (toStdErr)
        out = &std::cerr;

    for (size_t y = 0; y < height; y++) {
        *out << "|";

        for (size_t x = 0; x < width; x++) {
            auto &element = table[x + y * width];
            *out << " " << fill(element, columnsLength[x]) << " |";
        }

        *out << std::endl;

        if (y == 0 && height > 1)
            this->drawTableHLine(columnsLength, toStdErr);
    }

    this->drawTableHLine(columnsLength, toStdErr);
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
    if (flags.empty()
        || this->containsFlag(flags, "", "-h")
        || this->containsFlag(flags, "", "--help-all")) {
        return this->showHelp(flags, args);
    }

    if (this->containsFlag(flags, "", "-v")) {
        std::cout << COMMONS_VERSION << std::endl;

        return 0;
    }

    if (this->containsFlag(flags, "", "--build-info")) {
#ifdef GIT_COMMIT_HASH
        std::string commitHash = GIT_COMMIT_HASH;
        std::string commitUrl = COMMONS_PROJECT_COMMIT_URL "/" GIT_COMMIT_HASH;

        if (commitHash.empty())
            commitHash = "Unknown";

        if (commitUrl.empty())
            commitUrl = "Unknown";
#else
        std::string commitHash;
        std::string commitUrl;
#endif

        std::cout << "Commit hash: " << commitHash << std::endl;
        std::cout << "Commit URL: " << commitUrl << std::endl;

        return 0;
    }

    if (this->containsFlag(flags, "", "-p"))
        this->m_parseable = true;

    if (this->containsFlag(flags, "", "-f"))
        this->m_force = true;

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

    bool showAdvancedHelp = this->containsFlag(flags, "", "--help-all");
    auto maxCmdLen = this->maxCommandLength(showAdvancedHelp);
    auto maxArgsLen = this->maxArgumentsLength(showAdvancedHelp);

    for (auto &cmd: this->m_commands) {
        if (cmd.command.empty()
            || (cmd.advanced && !showAdvancedHelp))
            continue;

        std::cout << "    "
                  << fill(cmd.command, maxCmdLen)
                  << " "
                  << fill(cmd.arguments, maxArgsLen)
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

    auto devices = this->m_ipcBridge.devices();

    if (devices.empty())
        return 0;

    std::sort(devices.begin(), devices.end());

    if (this->m_parseable) {
        for (auto &device: devices)
            std::cout << device << std::endl;
    } else {
        std::vector<std::string> table {
            "Device",
            "Description"
        };
        auto columns = table.size();

        for (auto &device: devices) {
            table.push_back(device);
            table.push_back(this->m_ipcBridge.description(device));
        }

        this->drawTable(table, columns);
    }

    return 0;
}

int AkVCam::CmdParserPrivate::addDevice(const StringMap &flags,
                                        const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Device description not provided." << std::endl;

        return -EINVAL;
    }

    auto deviceId = this->flagValue(flags, "add-device", "-i");
    deviceId = this->m_ipcBridge.addDevice(args[1], deviceId);

    if (deviceId.empty()) {
        std::cerr << "Failed to create device." << std::endl;

        return -EIO;
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

        return -EINVAL;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto it = std::find(devices.begin(), devices.end(), deviceId);

    if (it == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -ENODEV;
    }

    this->m_ipcBridge.removeDevice(args[1]);

    return 0;
}

int AkVCam::CmdParserPrivate::removeDevices(const AkVCam::StringMap &flags,
                                            const AkVCam::StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);

    for (auto &device: this->m_ipcBridge.devices())
        this->m_ipcBridge.removeDevice(device);

    return 0;
}

int AkVCam::CmdParserPrivate::showDeviceDescription(const StringMap &flags,
                                                    const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Device not provided." << std::endl;

        return -EINVAL;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto it = std::find(devices.begin(), devices.end(), deviceId);

    if (it == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -ENODEV;
    }

    std::cout << this->m_ipcBridge.description(args[1]) << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::setDeviceDescription(const AkVCam::StringMap &flags,
                                                   const AkVCam::StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 3) {
        std::cerr << "Not enough arguments." << std::endl;

        return -EINVAL;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -ENODEV;
    }

    this->m_ipcBridge.setDescription(deviceId, args[2]);

    return 0;
}

int AkVCam::CmdParserPrivate::showSupportedFormats(const StringMap &flags,
                                                   const StringVector &args)
{
    UNUSED(args);

    auto type =
            this->containsFlag(flags, "supported-formats", "-i")?
                IpcBridge::StreamType_Input:
                IpcBridge::StreamType_Output;
    auto formats = this->m_ipcBridge.supportedPixelFormats(type);

    if (!this->m_parseable) {
        if (type == IpcBridge::StreamType_Input)
            std::cout << "Input formats:" << std::endl;
        else
            std::cout << "Output formats:" << std::endl;

        std::cout << std::endl;
    }

    std::vector<std::string> supportedFormats;

    for (auto &format: formats)
        supportedFormats.push_back(pixelFormatToCommonString(format));

    std::sort(supportedFormats.begin(), supportedFormats.end());

    for (auto &format: supportedFormats)
        std::cout << format << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::showDefaultFormat(const AkVCam::StringMap &flags,
                                                const AkVCam::StringVector &args)
{
    UNUSED(args);

    auto type =
            this->containsFlag(flags, "default-format", "-i")?
                IpcBridge::StreamType_Input:
                IpcBridge::StreamType_Output;
    auto format = this->m_ipcBridge.defaultPixelFormat(type);
    std::cout << pixelFormatToCommonString(format) << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::showFormats(const StringMap &flags,
                                          const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Device not provided." << std::endl;

        return -EINVAL;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto it = std::find(devices.begin(), devices.end(), deviceId);

    if (it == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -ENODEV;
    }

    if (this->m_parseable) {
        for  (auto &format: this->m_ipcBridge.formats(args[1]))
            std::cout << pixelFormatToCommonString(format.format())
                      << ' '
                      << format.width()
                      << ' '
                      << format.height()
                      << ' '
                      << format.fps().num()
                      << ' '
                      << format.fps().den()
                      << std::endl;
    } else {
        int i = 0;

        for  (auto &format: this->m_ipcBridge.formats(args[1])) {
            std::cout << i
                      << ": "
                      << pixelFormatToCommonString(format.format())
                      << ' '
                      << format.width()
                      << 'x'
                      << format.height()
                      << ' '
                      << format.fps().num()
                      << '/'
                      << format.fps().den()
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

        return -EINVAL;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -ENODEV;
    }

    auto format = pixelFormatFromCommonString(args[2]);

    if (!format) {
        std::cerr << "Invalid pixel format." << std::endl;

        return -EINVAL;
    }

    auto formats =
            this->m_ipcBridge.supportedPixelFormats(IpcBridge::StreamType_Output);
    auto fit = std::find(formats.begin(), formats.end(), format);

    if (fit == formats.end()) {
        std::cerr << "Format not supported." << std::endl;

        return -EINVAL;
    }

    char *p = nullptr;
    auto width = strtoul(args[3].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Width must be an unsigned integer." << std::endl;

        return -EINVAL;
    }

    p = nullptr;
    auto height = strtoul(args[4].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Height must be an unsigned integer." << std::endl;

        return -EINVAL;
    }

    Fraction fps(args[5]);

    if (fps.num() < 1 || fps.den() < 1) {
        std::cerr << "Invalid frame rate." << std::endl;

        return -EINVAL;
    }

    auto indexStr = this->flagValue(flags, "add-format", "-i");
    int index = -1;

    if (!indexStr.empty()) {
        p = nullptr;
        index = int(strtoul(indexStr.c_str(), &p, 10));

        if (*p) {
            std::cerr << "Index must be an unsigned integer." << std::endl;

            return -EINVAL;
        }
    }

    VideoFormat fmt(format, int(width), int(height), {fps});
    this->m_ipcBridge.addFormat(deviceId, fmt, index);

    return 0;
}

int AkVCam::CmdParserPrivate::removeFormat(const StringMap &flags,
                                           const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 3) {
        std::cerr << "Not enough arguments." << std::endl;

        return -EINVAL;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -ENODEV;
    }

    char *p = nullptr;
    auto index = strtoul(args[2].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Index must be an unsigned integer." << std::endl;

        return -EINVAL;
    }

    auto formats = this->m_ipcBridge.formats(deviceId);

    if (index >= formats.size()) {
        std::cerr << "Index is out of range." << std::endl;

        return -ERANGE;
    }

    this->m_ipcBridge.removeFormat(deviceId, int(index));

    return 0;
}

int AkVCam::CmdParserPrivate::removeFormats(const AkVCam::StringMap &flags,
                                           const AkVCam::StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Not enough arguments." << std::endl;

        return -EINVAL;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -ENODEV;
    }

    this->m_ipcBridge.setFormats(deviceId, {});

    return 0;
}

int AkVCam::CmdParserPrivate::update(const StringMap &flags,
                                     const StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);
    this->m_ipcBridge.updateDevices();

    return 0;
}

int AkVCam::CmdParserPrivate::loadSettings(const AkVCam::StringMap &flags,
                                           const AkVCam::StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Settings file not provided." << std::endl;

        return -EINVAL;
    }

    Settings settings;

    if (!settings.load(args[1])) {
        std::cerr << "Settings file not valid." << std::endl;

        return -EIO;
    }

    this->loadGenerals(settings);
    auto devices = this->m_ipcBridge.devices();

    for (auto &device: devices)
        this->m_ipcBridge.removeDevice(device);

    this->createDevices(settings, this->readFormats(settings));

    return 0;
}

int AkVCam::CmdParserPrivate::stream(const AkVCam::StringMap &flags,
                                     const AkVCam::StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 5) {
        std::cerr << "Not enough arguments." << std::endl;

        return -EINVAL;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -ENODEV;
    }

    auto format = pixelFormatFromCommonString(args[2]);

    if (!format) {
        std::cerr << "Invalid pixel format." << std::endl;

        return -EINVAL;
    }

    auto formats =
            this->m_ipcBridge.supportedPixelFormats(IpcBridge::StreamType_Output);
    auto fit = std::find(formats.begin(), formats.end(), format);

    if (fit == formats.end()) {
        std::cerr << "Format not supported." << std::endl;

        return -EINVAL;
    }

    char *p = nullptr;
    auto width = strtoul(args[3].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Width must be an unsigned integer." << std::endl;

        return -EINVAL;
    }

    p = nullptr;
    auto height = strtoul(args[4].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Height must be an unsigned integer." << std::endl;

        return -EINVAL;
    }

    auto fpsStr = this->flagValue(flags, "stream", "-f");
    double fps = std::numeric_limits<double>::quiet_NaN();

    if (!fpsStr.empty()) {
        p = nullptr;
        fps = int(strtod(fpsStr.c_str(), &p));

        if (*p) {
            if (!Fraction::isFraction(fpsStr)) {
                std::cerr << "The framerate must be a number or a fraction." << std::endl;

                return -EINVAL;
            }

            fps = Fraction(fpsStr).value();
        }

        if (fps <= 0 || std::isinf(fps)) {
            std::cerr << "The framerate is out of range." << std::endl;

            return -ERANGE;
        }
    }

    VideoFormat fmt(format,
                    int(width),
                    int(height),
                    Fraction(int(std::round(fps)), 1));

    if (!this->m_ipcBridge.deviceStart(IpcBridge::StreamType_Output, deviceId)) {
        std::cerr << "Can't start stream." << std::endl;

        return -EIO;
    }

    static bool exit = false;
    auto signalHandler = [] (int) {
        exit = true;
    };
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

#ifndef _WIN32
    signal(SIGPIPE, [] (int) {
    });
#endif

    AkVCam::VideoFrame frame(fmt);
    size_t bufferSize = 0;

#ifdef _WIN32
    // Set std::cin in binary mode.
    _setmode(_fileno(stdin), _O_BINARY);
#endif

    auto clock = [] (const std::chrono::time_point<std::chrono::high_resolution_clock> &since) -> double {
        return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - since).count();
    };

    const double minThreshold = 0.04;
    const double maxThreshold = 0.1;
    const double framedupThreshold = 0.1;
    const double nosyncThreshold = 10.0;

    double lastPts = 0.0;
    auto t0 = std::chrono::high_resolution_clock::now();
    double drift = 0;
    uint64_t i = 0;

    do {
        std::cin.read(reinterpret_cast<char *>(frame.data()
                                               + bufferSize),
                      std::streamsize(frame.size() - bufferSize));
        bufferSize += size_t(std::cin.gcount());

        if (bufferSize == frame.size()) {
            if (fpsStr.empty()) {
                this->m_ipcBridge.write(deviceId, frame);
            } else {
                double pts = double(i) / fps;

                for (;;) {
                    double clock_pts = clock(t0) + drift;
                    double diff = pts - clock_pts;
                    double delay = pts - lastPts;
                    double syncThreshold =
                            std::max(minThreshold,
                                     std::min(delay, maxThreshold));

                    if (!std::isnan(diff)
                        && std::abs(diff) < nosyncThreshold
                        && delay < framedupThreshold) {
                        if (diff <= -syncThreshold) {
                            lastPts = pts;

                            break;
                        }

                        if (diff > syncThreshold) {
                            std::this_thread::sleep_for(std::chrono::duration<double>(diff - syncThreshold));

                            continue;
                        }
                    } else {
                        drift = clock(t0) - pts;
                    }

                    this->m_ipcBridge.write(deviceId, frame);
                    lastPts = pts;

                    break;
                }

                i++;
            }

            bufferSize = 0;
        }
    } while (!std::cin.eof() && !exit);

    this->m_ipcBridge.deviceStop(deviceId);

    return 0;
}

int AkVCam::CmdParserPrivate::streamPattern(const StringMap &flags,
                                            const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 4) {
        std::cerr << "Not enough arguments." << std::endl;

        return -EINVAL;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -ENODEV;
    }

    static const AkVCam::PixelFormat format = AkVCam::PixelFormat_rgb24;
    auto formats =
            this->m_ipcBridge.supportedPixelFormats(IpcBridge::StreamType_Output);
    auto fit = std::find(formats.begin(), formats.end(), format);

    if (fit == formats.end()) {
        std::cerr << "RGB24 format not supported." << std::endl;

        return -EINVAL;
    }

    char *p = nullptr;
    auto width = strtoul(args[2].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Width must be an unsigned integer." << std::endl;

        return -EINVAL;
    }

    p = nullptr;
    auto height = strtoul(args[3].c_str(), &p, 10);

    if (*p) {
        std::cerr << "Height must be an unsigned integer." << std::endl;

        return -EINVAL;
    }

    auto fpsStr = this->flagValue(flags, "stream", "-f");
    double fps = 30.0;

    if (!fpsStr.empty()) {
        p = nullptr;
        fps = int(strtod(fpsStr.c_str(), &p));

        if (*p) {
            if (!Fraction::isFraction(fpsStr)) {
                std::cerr << "The framerate must be a number or a fraction." << std::endl;

                return -EINVAL;
            }

            fps = Fraction(fpsStr).value();
        }

        if (fps <= 0 || std::isinf(fps)) {
            std::cerr << "The framerate is out of range." << std::endl;

            return -ERANGE;
        }
    }

    VideoFormat fmt(format,
                    int(width),
                    int(height),
                    Fraction(int(std::round(fps)), 1));

    if (!this->m_ipcBridge.deviceStart(IpcBridge::StreamType_Output, deviceId)) {
        std::cerr << "Can't start stream." << std::endl;

        return -EIO;
    }

    static bool exit = false;
    auto signalHandler = [] (int) {
        exit = true;
    };
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

#ifndef _WIN32
    signal(SIGPIPE, [] (int) {
    });
#endif

    AkVCam::VideoFrame frame(fmt);

#ifdef _WIN32
    // Set std::cin in binary mode.
    _setmode(_fileno(stdin), _O_BINARY);
#endif

    // Define colors

    struct RGB
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    };

    std::vector<RGB> colors = {
        {0  , 0  , 0  }, // Black
        {255, 0  , 0  }, // Red
        {255, 255, 0  }, // Yellow
        {0  , 255, 0  }, // Green
        {0  , 255, 255}, // Cyan
        {0  , 0  , 255}, // Blue
        {255, 0  , 255}, // Magenta
        {255, 255, 255}  // White
    };

    const int numBars = colors.size();
    const int barWidth = width / numBars;

    // Square properties

    const int squareSize = (std::min)(width, height) / 8; // Square side length
    double squareX = width / 2.0;     // Initial position (center)
    double squareY = height / 2.0;
    double speedX = 0.1 * width; // Speed: 10% of width per second
    double speedY = 0.1 * height; // Speed: 10% of height per second
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> colorDist(0, numBars - 1);
    RGB squareColor = colors[colorDist(gen)]; // Initial random color

    auto clock = [] (const std::chrono::time_point<std::chrono::high_resolution_clock> &since) -> double {
        return std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - since).count();
    };

    const double nosyncThreshold = 10.0;

    double lastPts = 0.0;
    auto t0 = std::chrono::high_resolution_clock::now();
    double drift = 0;
    uint64_t i = 0;

    do {
        // Update square position based on elapsed time
        double currentTime = clock(t0);
        double deltaTime = currentTime - lastPts; // Time since last frame
        deltaTime = (std::min)(deltaTime, 0.1); // Cap at 100ms to avoid large jumps
        lastPts = currentTime;

        // Move square based on elapsed time for smooth motion
        squareX += speedX * deltaTime;
        squareY += speedY * deltaTime;

        // Check for collisions and update direction/color
        bool collision = false;

        if (squareX <= 0 || squareX + squareSize >= width) {
            speedX = -speedX;
            squareX = std::max(0.0, (std::min)(double(width - squareSize), squareX));
            collision = true;
        }

        if (squareY <= 0 || squareY + squareSize >= height) {
            speedY = -speedY;
            squareY = std::max(0.0, (std::min)(double(height - squareSize), squareY));
            collision = true;
        }

        if (collision)
            squareColor = colors[colorDist(gen)]; // Change color on collision

        // Fill frame with color bars

        auto firstLine = frame.line(0, 0);

        // Fill first line with color bars
        for (int x = 0; x < width; ++x) {
            int barIndex = x / barWidth;

            if (barIndex >= numBars)
                barIndex = numBars - 1;

            const auto &color = colors[barIndex];
            auto pixel = firstLine + 3 * x;
            pixel[0] = color.r;
            pixel[1] = color.g;
            pixel[2] = color.b;
        }

        auto lineSize = frame.lineSize(0);

        // Copy first line to all rows
        for (int y = 1; y < height; ++y)
            memcpy(frame.line(0, y), firstLine, lineSize);

        // Draw square
        int xStart = (std::max)(0, int(squareX));
        int xEnd = (std::min)(int(width), int(squareX + squareSize));
        int yStart = (std::max)(0, int(squareY));
        int yEnd = (std::min)(int(height), int(squareY + squareSize));

        auto line = frame.line(0, yStart);

        for (int x = xStart; x < xEnd; ++x) {
            auto pixel = line + 3 * x;
            pixel[0] = squareColor.r;
            pixel[1] = squareColor.g;
            pixel[2] = squareColor.b;
        }

        auto bytesToFill = 3 * (xEnd - xStart);

        for (int y = yStart + 1; y < yEnd; ++y) {
            memcpy(frame.line(0, y) + 3 * xStart,
                   line + 3 * xStart,
                   bytesToFill);
        }

        double targetPts = double(i) / fps;
        double clockPts = clock(t0) + drift;
        double diff = targetPts - clockPts;

        if (diff > 0.001) {
            // Sleep only for significant delays
            std::this_thread::sleep_for(std::chrono::duration<double>(diff));
        } else if (diff < -nosyncThreshold) {
            // Reset drift if too far behind
            drift = clock(t0) - targetPts;
        }

        this->m_ipcBridge.write(deviceId, frame);
        i++;
    } while (!exit);

    this->m_ipcBridge.deviceStop(deviceId);

    return 0;
}

int AkVCam::CmdParserPrivate::listenEvents(const AkVCam::StringMap &flags,
                                           const AkVCam::StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);

    auto devicesChanged = [] (void *, const std::vector<std::string> &) {
        std::cout << "DevicesUpdated" << std::endl;
    };
    auto pictureChanged = [] (void *, const std::string &) {
        std::cout << "PictureUpdated" << std::endl;
    };

    this->m_ipcBridge.connectDevicesChanged(this, devicesChanged);
    this->m_ipcBridge.connectPictureChanged(this, pictureChanged);

    static bool exit = false;
    auto signalHandler = [] (int) {
        exit = true;
    };
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    while (!exit)
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    return 0;
}

int AkVCam::CmdParserPrivate::showControls(const StringMap &flags,
                                           const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Device not provided." << std::endl;

        return -EINVAL;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -ENODEV;
    }

    if (this->m_parseable) {
        for (auto &control: this->m_ipcBridge.controls(deviceId))
            std::cout << control.id << std::endl;
    } else {
        auto typeStr = typeStrMap();

        std::vector<std::string> table {
            "Control",
            "Description",
            "Type",
            "Minimum",
            "Maximum",
            "Step",
            "Default",
            "Value"
        };
        auto columns = table.size();

        for (auto &control: this->m_ipcBridge.controls(deviceId)) {
            table.push_back(control.id);
            table.push_back(control.description);
            table.push_back(typeStr[control.type]);
            table.push_back(std::to_string(control.minimum));
            table.push_back(std::to_string(control.maximum));
            table.push_back(std::to_string(control.step));
            table.push_back(std::to_string(control.defaultValue));
            table.push_back(std::to_string(control.value));
        }

        this->drawTable(table, columns);
    }

    return 0;
}

int AkVCam::CmdParserPrivate::readControl(const StringMap &flags,
                                          const StringVector &args)
{
    if (args.size() < 3) {
        std::cerr << "Not enough arguments." << std::endl;

        return -EINVAL;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -ENODEV;
    }

    for (auto &control: this->m_ipcBridge.controls(deviceId))
        if (control.id == args[2]) {
            if (flags.empty()) {
                std::cout << control.value << std::endl;
            } else {
                if (this->containsFlag(flags, "get-control", "-c")) {
                    std::cout << control.description << std::endl;
                }

                if (this->containsFlag(flags, "get-control", "-t")) {
                    auto typeStr = typeStrMap();
                    std::cout << typeStr[control.type] << std::endl;
                }

                if (this->containsFlag(flags, "get-control", "-m")) {
                    std::cout << control.minimum << std::endl;
                }

                if (this->containsFlag(flags, "get-control", "-M")) {
                    std::cout << control.maximum << std::endl;
                }

                if (this->containsFlag(flags, "get-control", "-s")) {
                    std::cout << control.step << std::endl;
                }

                if (this->containsFlag(flags, "get-control", "-d")) {
                    std::cout << control.defaultValue << std::endl;
                }

                if (this->containsFlag(flags, "get-control", "-l")) {
                    for (size_t i = 0; i < control.menu.size(); i++)
                        if (this->m_parseable)
                            std::cout << control.menu[i] << std::endl;
                        else
                            std::cout << i
                                      << ": "
                                      << control.menu[i]
                                      << std::endl;
                }
            }

            return 0;
        }

    std::cerr << "'" << args[2] << "' control not available." << std::endl;

    return -ENOSYS;
}

int AkVCam::CmdParserPrivate::writeControls(const StringMap &flags,
                                            const StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 3) {
        std::cerr << "Not enough arguments." << std::endl;

        return -EINVAL;
    }

    auto deviceId = args[1];
    auto devices = this->m_ipcBridge.devices();
    auto dit = std::find(devices.begin(), devices.end(), deviceId);

    if (dit == devices.end()) {
        std::cerr << "'" << deviceId << "' doesn't exists." << std::endl;

        return -ENODEV;
    }

    std::map<std::string, int> controls;

    for (size_t i = 2; i < args.size(); i++) {
        if (args[i].find('=') == std::string::npos) {
            std::cerr << "Argumment "
                      << i
                      << " is not in the form KEY=VALUE."
                      << std::endl;

            return -EINVAL;
        }

        auto pair = splitOnce(args[i], "=");

        if (pair.first.empty()) {
            std::cerr << "Key for argumment "
                      << i
                      << " is emty."
                      << std::endl;

            return -EINVAL;
        }

        auto key = trimmed(pair.first);
        auto value = trimmed(pair.second);
        bool found = false;

        for (auto &control: this->m_ipcBridge.controls(deviceId))
                if (control.id == key) {
                    switch (control.type) {
                    case ControlTypeInteger: {
                        char *p = nullptr;
                        auto val = strtol(value.c_str(), &p, 10);

                        if (*p) {
                            std::cerr << "Value at argument "
                                      << i
                                      << " must be an integer."
                                      << std::endl;

                            return -EINVAL;
                        }

                        controls[key] = val;

                        break;
                    }

                    case ControlTypeBoolean: {
                        std::locale loc;
                        std::transform(value.begin(),
                                       value.end(),
                                       value.begin(),
                                       [&loc](char c) {
                            return std::tolower(c, loc);
                        });

                        if (value == "0" || value == "false") {
                            controls[key] = 0;
                        } else if (value == "1" || value == "true") {
                            controls[key] = 1;
                        } else {
                            std::cerr << "Value at argument "
                                      << i
                                      << " must be a boolean."
                                      << std::endl;

                            return -EINVAL;
                        }

                        break;
                    }

                    case ControlTypeMenu: {
                        char *p = nullptr;
                        auto val = strtoul(value.c_str(), &p, 10);

                        if (*p) {
                            auto it = std::find(control.menu.begin(),
                                                control.menu.end(),
                                                value);

                            if (it == control.menu.end()) {
                                std::cerr << "Value at argument "
                                          << i
                                          << " is not valid."
                                          << std::endl;

                                return -EINVAL;
                            }

                            controls[key] = int(it - control.menu.begin());
                        } else {
                            if (val >= control.menu.size()) {
                                std::cerr << "Value at argument "
                                          << i
                                          << " is out of range."
                                          << std::endl;

                                return -ERANGE;
                            }

                            controls[key] = int(val);
                        }

                        break;
                    }

                    default:
                        break;
                    }

                    found = true;

                    break;
                }

        if (!found) {
            std::cerr << "No such '"
                      << key
                      << "' control in argument "
                      << i
                      << "."
                      << std::endl;

            return -ENOSYS;
        }
    }

    this->m_ipcBridge.setControls(deviceId, controls);

    return 0;
}

int AkVCam::CmdParserPrivate::picture(const AkVCam::StringMap &flags,
                                      const AkVCam::StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);

    std::cout << this->m_ipcBridge.picture() << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::setPicture(const AkVCam::StringMap &flags,
                                         const AkVCam::StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Not enough arguments." << std::endl;

        return -EINVAL;
    }

    this->m_ipcBridge.setPicture(args[1]);

    return 0;
}

int AkVCam::CmdParserPrivate::logLevel(const AkVCam::StringMap &flags,
                                       const AkVCam::StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);

    auto level = this->m_ipcBridge.logLevel();

    if (this->m_parseable)
        std::cout << level << std::endl;
    else
        std::cout << AkVCam::Logger::levelToString(level) << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::setLogLevel(const AkVCam::StringMap &flags,
                                          const AkVCam::StringVector &args)
{
    UNUSED(flags);

    if (args.size() < 2) {
        std::cerr << "Not enough arguments." << std::endl;

        return -EINVAL;
    }

    auto levelStr = args[1];
    char *p = nullptr;
    auto level = strtol(levelStr.c_str(), &p, 10);

    if (*p)
        level = AkVCam::Logger::levelFromString(levelStr);

    this->m_ipcBridge.setLogLevel(level);

    return 0;
}

int AkVCam::CmdParserPrivate::showClients(const StringMap &flags,
                                          const StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);
    auto clients = this->m_ipcBridge.clientsPids();

    if (clients.empty())
        return 0;

    if (this->m_parseable) {
        for (auto &pid: clients)
            std::cout << pid
                      << " "
                      << this->m_ipcBridge.clientExe(pid)
                      << std::endl;
    } else {
        std::vector<std::string> table {
            "Pid",
            "Executable"
        };
        auto columns = table.size();

        for (auto &pid: clients) {
            table.push_back(std::to_string(pid));
            table.push_back(this->m_ipcBridge.clientExe(pid));
        }

        this->drawTable(table, columns);
    }

    return 0;
}

int AkVCam::CmdParserPrivate::dumpInfo(const AkVCam::StringMap &flags,
                                       const AkVCam::StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);
    static const auto indent = 4 * std::string(" ");

    std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl;
    std::cout << "<info>" << std::endl;
    std::cout << indent << "<devices>" << std::endl;

    auto devices = this->m_ipcBridge.devices();

    for (auto &device: devices) {
        std::cout << 2 * indent << "<device>" << std::endl;
        std::cout << 3 * indent << "<id>" << device << "</id>" << std::endl;
        std::cout << 3 * indent
                  << "<description>"
                  << this->m_ipcBridge.description(device)
                  << "</description>"
                  << std::endl;
        std::cout << 3 * indent << "<formats>" << std::endl;

        for  (auto &format: this->m_ipcBridge.formats(device)) {
            std::cout << 4 * indent << "<format>" << std::endl;
            std::cout << 5 * indent
                      << "<pixel-format>"
                      << pixelFormatToCommonString(format.format())
                      << "</pixel-format>"
                      << std::endl;
            std::cout << 5 * indent
                      << "<width>"
                      << format.width()
                      << "</width>"
                      << std::endl;
            std::cout << 5 * indent
                      << "<height>"
                      << format.height()
                      << "</height>"
                      << std::endl;
            std::cout << 5 * indent
                      << "<fps>"
                      << format.fps().toString()
                      << "</fps>"
                      << std::endl;
            std::cout << 4 * indent << "</format>" << std::endl;
        }

        std::cout << 3 * indent << "</formats>" << std::endl;
        std::cout << 3 * indent << "<controls>" << std::endl;

        for (auto &control: this->m_ipcBridge.controls(device)) {
            std::cout << 4 * indent << "<control>" << std::endl;
            std::cout << 5 * indent
                      << "<id>"
                      << control.id
                      << "</id>"
                      << std::endl;
            std::cout << 5 * indent
                      << "<description>"
                      << control.description
                      << "</description>"
                      << std::endl;
            auto typeStr = typeStrMap();
            std::cout << 5 * indent
                      << "<type>"
                      << typeStr[control.type]
                      << "</type>"
                      << std::endl;
            std::cout << 5 * indent
                      << "<minimum>"
                      << control.minimum
                      << "</minimum>"
                      << std::endl;
            std::cout << 5 * indent
                      << "<maximum>"
                      << control.maximum
                      << "</maximum>"
                      << std::endl;
            std::cout << 5 * indent
                      << "<step>"
                      << control.step
                      << "</step>"
                      << std::endl;
            std::cout << 5 * indent
                      << "<default-value>"
                      << control.defaultValue
                      << "</default-value>"
                      << std::endl;
            std::cout << 5 * indent
                      << "<value>"
                      << control.value
                      << "</value>"
                      << std::endl;

            if (!control.menu.empty() && control.type == ControlTypeMenu) {
                std::cout << 5 * indent << "<menu>" << std::endl;

                for (auto &item: control.menu)
                    std::cout << 6 * indent
                              << "<item>"
                              << item
                              << "</item>"
                              << std::endl;

                std::cout << 5 * indent << "</menu>" << std::endl;
            }

            std::cout << 4 * indent << "</control>" << std::endl;
        }

        std::cout << 3 * indent << "</controls>" << std::endl;
        std::cout << 2 * indent << "</device>" << std::endl;
    }

    std::cout << indent << "</devices>" << std::endl;
    std::cout << indent << "<input-formats>" << std::endl;

    for (auto &format: this->m_ipcBridge.supportedPixelFormats(IpcBridge::StreamType_Input))
        std::cout << 2 * indent
                  << "<pixel-format>"
                  << pixelFormatToCommonString(format)
                  << "</pixel-format>"
                  << std::endl;

    std::cout << indent << "</input-formats>" << std::endl;

    auto defInputFormat =
            this->m_ipcBridge.defaultPixelFormat(IpcBridge::StreamType_Input);
    std::cout << indent
              << "<default-input-format>"
              << pixelFormatToCommonString(defInputFormat)
              << "</default-input-format>"
              << std::endl;

    std::cout << indent << "<output-formats>" << std::endl;

    for (auto &format: this->m_ipcBridge.supportedPixelFormats(IpcBridge::StreamType_Output))
        std::cout << 2 * indent
                  << "<pixel-format>"
                  << pixelFormatToCommonString(format)
                  << "</pixel-format>"
                  << std::endl;

    std::cout << indent << "</output-formats>" << std::endl;

    auto defOutputFormat =
            this->m_ipcBridge.defaultPixelFormat(IpcBridge::StreamType_Output);
    std::cout << indent
              << "<default-output-format>"
              << pixelFormatToCommonString(defOutputFormat)
              << "</default-output-format>"
              << std::endl;

    std::cout << indent << "<clients>" << std::endl;

   for (auto &pid: this->m_ipcBridge.clientsPids()) {
       std::cout << 2 * indent << "<client>" << std::endl;
       std::cout << 3 * indent << "<pid>" << pid << "</pid>" << std::endl;
       std::cout << 3 * indent
                 << "<exe>"
                 << this->m_ipcBridge.clientExe(pid)
                 << "</exe>"
                 << std::endl;
       std::cout << 2 * indent << "</client>" << std::endl;
   }

    std::cout << indent << "</clients>" << std::endl;
    std::cout << indent
              << "<picture>"
              << this->m_ipcBridge.picture()
              << "</picture>"
              << std::endl;
    std::cout << indent
              << "<loglevel>"
              << this->m_ipcBridge.logLevel()
              << "</loglevel>"
              << std::endl;
    std::cout << "</info>" << std::endl;

    return 0;
}

int AkVCam::CmdParserPrivate::hacks(const AkVCam::StringMap &flags,
                                    const AkVCam::StringVector &args)
{
    UNUSED(flags);
    UNUSED(args);

    auto hacks = this->m_ipcBridge.hacks();

    if (hacks.empty())
        return 0;

    if (this->m_parseable) {
        for (auto &hack: hacks)
            std::cout << hack << std::endl;
    } else {
        std::cout << "Hacks are intended to fix common problems with the "
                     "virtual camera, and are intended to be used by developers "
                     "and advanced users only." << std::endl;
        std::cout << std::endl;
        std::cout << "WARNING: Unsafe hacks can brick your system, make it "
                     "unstable, or expose it to a serious security risk, "
                     "remember to make a backup of your files and system. You "
                     "are solely responsible of whatever happens for using "
                     "them. You been warned, don't come and cry later."
                  << std::endl;
        std::cout << std::endl;
        std::vector<std::string> table {
            "Hack",
            "Is safe?",
            "Description"
        };
        auto columns = table.size();

        for (auto &hack: hacks) {
            table.push_back(hack);
            table.push_back(this->m_ipcBridge.hackIsSafe(hack)? "Yes": "No");
            table.push_back(this->m_ipcBridge.hackDescription(hack));
        }

        this->drawTable(table, columns);
    }

    return 0;
}

int AkVCam::CmdParserPrivate::hackInfo(const AkVCam::StringMap &flags,
                                       const AkVCam::StringVector &args)
{
    if (args.size() < 2) {
        std::cerr << "Not enough arguments." << std::endl;

        return -EINVAL;
    }

    auto hack = args[1];
    auto hacks = this->m_ipcBridge.hacks();
    auto dit = std::find(hacks.begin(), hacks.end(), hack);

    if (dit == hacks.end()) {
        std::cerr << "Unknown hack: " << hack << "." << std::endl;

        return -ENOSYS;
    }

    if (this->containsFlag(flags, "hack-info", "-c"))
        std::cout << this->m_ipcBridge.hackDescription(hack) << std::endl;

    if (this->containsFlag(flags, "hack-info", "-s")) {
        if (this->m_ipcBridge.hackIsSafe(hack))
            std::cout << "Yes" << std::endl;
        else
            std::cout << "No" << std::endl;
    }

    return 0;
}

int AkVCam::CmdParserPrivate::hack(const AkVCam::StringMap &flags,
                                   const AkVCam::StringVector &args)
{
    if (args.size() < 2) {
        std::cerr << "Not enough arguments." << std::endl;

        return -EINVAL;
    }

    auto hack = args[1];
    auto hacks = this->m_ipcBridge.hacks();
    auto dit = std::find(hacks.begin(), hacks.end(), hack);

    if (dit == hacks.end()) {
        std::cerr << "Unknown hack: " << hack << "." << std::endl;

        return -ENOSYS;
    }

    bool accepted = this->m_parseable | this->m_ipcBridge.hackIsSafe(hack);

    if (!accepted && !this->m_parseable) {
        std::cout << "WARNING: Applying this hack can brick your system, make "
                     "it unstable, or expose it to a serious security risk, "
                     "remember to make a backup of your files and system. "
                     "Agreeing to continue, you accept the full responsability "
                     "of whatever happens from now on."
                  << std::endl;
        std::cout << std::endl;

        if (this->containsFlag(flags, "hack", "-y")) {
            std::cout << "You agreed to continue from command line."
                      << std::endl;
            std::cout << std::endl;
            accepted = true;
        } else {
            std::cout << "If you agree to continue write YES: ";
            std::string answer;
            std::cin >> answer;
            std::cout << std::endl;
            accepted = answer == "YES";
        }
    }

    if (!accepted) {
        std::cerr << "Hack not applied." << std::endl;

        return -EIO;
    }

    StringVector hargs;

    for (size_t i = 2; i < args.size(); i++)
        hargs.push_back(args[i]);

    auto result = this->m_ipcBridge.execHack(hack, hargs);

    if (result == 0)
        std::cout << "Success" << std::endl;
    else
        std::cout << "Failed" << std::endl;

    return result;
}

void AkVCam::CmdParserPrivate::loadGenerals(Settings &settings)
{
    settings.beginGroup("General");

    if (settings.contains("default_frame"))
        this->m_ipcBridge.setPicture(settings.value("default_frame"));

    if (settings.contains("loglevel")) {
        auto logLevel= settings.value("loglevel");
        char *p = nullptr;
        auto level = strtol(logLevel.c_str(), &p, 10);

        if (*p)
            level = AkVCam::Logger::levelFromString(logLevel);

        this->m_ipcBridge.setLogLevel(level);
    }

    settings.endGroup();
}

AkVCam::VideoFormatMatrix AkVCam::CmdParserPrivate::readFormats(Settings &settings)
{
    VideoFormatMatrix formatsMatrix;
    settings.beginGroup("Formats");
    auto nFormats = settings.beginArray("formats");

    for (size_t i = 0; i < nFormats; i++) {
        settings.setArrayIndex(i);
        formatsMatrix.push_back(this->readFormat(settings));
    }

    settings.endArray();
    settings.endGroup();

    return formatsMatrix;
}

std::vector<AkVCam::VideoFormat> AkVCam::CmdParserPrivate::readFormat(Settings &settings)
{
    std::vector<AkVCam::VideoFormat> formats;

    auto pixFormats = settings.valueList("format", ",");
    auto widths = settings.valueList("width", ",");
    auto heights = settings.valueList("height", ",");
    auto frameRates = settings.valueList("fps", ",");

    if (pixFormats.empty()
        || widths.empty()
        || heights.empty()
        || frameRates.empty()) {
        std::cerr << "Error reading formats." << std::endl;

        return {};
    }

    StringMatrix formatMatrix;
    formatMatrix.push_back(pixFormats);
    formatMatrix.push_back(widths);
    formatMatrix.push_back(heights);
    formatMatrix.push_back(frameRates);

    for (auto &formatList: this->matrixCombine(formatMatrix)) {
        auto pixFormat = pixelFormatFromCommonString(formatList[0]);
        char *p = nullptr;
        auto width = strtol(formatList[1].c_str(), &p, 10);
        p = nullptr;
        auto height = strtol(formatList[2].c_str(), &p, 10);
        Fraction frameRate(formatList[3]);
        VideoFormat format(pixFormat,
                           width,
                           height,
                           {frameRate});

        if (format.isValid())
            formats.push_back(format);
    }

    return formats;
}

AkVCam::StringMatrix AkVCam::CmdParserPrivate::matrixCombine(const StringMatrix &matrix)
{
    StringVector combined;
    StringMatrix combinations;
    this->matrixCombineP(matrix, 0, combined, combinations);

    return combinations;
}

/* A matrix is a list of lists where each element in the main list is a row,
 * and each element in a row is a column. We combine each element in a row with
 * each element in the next rows.
 */
void AkVCam::CmdParserPrivate::matrixCombineP(const StringMatrix &matrix,
                                              size_t index,
                                              StringVector combined,
                                              StringMatrix &combinations)
{
    if (index >= matrix.size()) {
        combinations.push_back(combined);

        return;
    }

    for (auto &data: matrix[index]) {
        auto combinedP1 = combined;
        combinedP1.push_back(data);
        this->matrixCombineP(matrix, index + 1, combinedP1, combinations);
    }
}

void AkVCam::CmdParserPrivate::createDevices(Settings &settings,
                                             const VideoFormatMatrix &availableFormats)
{
    for (auto &device: this->m_ipcBridge.devices())
        this->m_ipcBridge.removeDevice(device);

    settings.beginGroup("Cameras");
    size_t nCameras = settings.beginArray("cameras");

    for (size_t i = 0; i < nCameras; i++) {
        settings.setArrayIndex(i);
        this->createDevice(settings, availableFormats);
    }

    settings.endArray();
    settings.endGroup();
    this->m_ipcBridge.updateDevices();
}

void AkVCam::CmdParserPrivate::createDevice(Settings &settings,
                                            const VideoFormatMatrix &availableFormats)
{
    auto description = settings.value("description");

    if (description.empty()) {
        std::cerr << "Device description is empty" << std::endl;

        return;
    }

    auto formats = this->readDeviceFormats(settings, availableFormats);

    if (formats.empty()) {
        std::cerr << "Can't read device formats" << std::endl;

        return;
    }

    auto deviceId = settings.value("id");
    deviceId = this->m_ipcBridge.addDevice(description, deviceId);
    auto supportedFormats =
            this->m_ipcBridge.supportedPixelFormats(IpcBridge::StreamType_Output);

    for (auto &format: formats) {
        auto it = std::find(supportedFormats.begin(),
                            supportedFormats.end(),
                            format.format());

        if (it != supportedFormats.end())
            this->m_ipcBridge.addFormat(deviceId, format, -1);
    }
}

std::vector<AkVCam::VideoFormat> AkVCam::CmdParserPrivate::readDeviceFormats(Settings &settings,
                                                                             const VideoFormatMatrix &availableFormats)
{
    std::vector<AkVCam::VideoFormat> formats;
    auto formatsIndex = settings.valueList("formats", ",");

    for (auto &indexStr: formatsIndex) {
        char *p = nullptr;
        auto index = strtoul(indexStr.c_str(), &p, 10);

        if (*p)
            continue;

        index--;

        if (index >= availableFormats.size())
            continue;

        for (auto &format: availableFormats[index])
            formats.push_back(format);
    }

    return formats;
}

std::string AkVCam::operator *(const std::string &str, size_t n)
{
    std::stringstream ss;

    for (size_t i = 0; i < n; i++)
        ss << str;

    return ss.str();
}

std::string AkVCam::operator *(size_t n, const std::string &str)
{
    std::stringstream ss;

    for (size_t i = 0; i < n; i++)
        ss << str;

    return ss.str();
}

AkVCam::CmdParserCommand::CmdParserCommand()
{
}

AkVCam::CmdParserCommand::CmdParserCommand(const std::string &command,
                                           const std::string &arguments,
                                           const std::string &helpString,
                                           const AkVCam::ProgramOptionsFunc &func,
                                           const std::vector<AkVCam::CmdParserFlags> &flags,
                                           bool advanced):
    command(command),
    arguments(arguments),
    helpString(helpString),
    func(func),
    flags(flags),
    advanced(advanced)
{
}
