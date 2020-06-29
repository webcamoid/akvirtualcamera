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

#include <string>
#include <windows.h>

#include "service.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/logger/logger.h"

int main(int argc, char **argv)
{
    auto loglevel = AkVCam::regReadInt("loglevel", AKVCAM_LOGLEVEL_DEFAULT);
    AkVCam::Logger::setLogLevel(loglevel);
    auto temp = AkVCam::tempPath();
    auto logFile =
            AkVCam::regReadString("logfile",
                                  std::string(temp.begin(), temp.end())
                                  + "\\" DSHOW_PLUGIN_ASSISTANT_NAME ".log");
    AkVCam::Logger::setLogFile(logFile);
    AkVCam::Service service;

    if (argc > 1) {
        if (!strcmp(argv[1], "-i") || !strcmp(argv[1], "--install")) {
            return service.install()? EXIT_SUCCESS: EXIT_FAILURE;
        } else if (!strcmp(argv[1], "-u") || !strcmp(argv[1], "--uninstall")) {
            service.uninstall();

            return EXIT_SUCCESS;
        } else if (!strcmp(argv[1], "-d") || !strcmp(argv[1], "--debug")) {
            service.debug();

            return EXIT_SUCCESS;
        } else if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
            service.showHelp(argc, argv);

            return EXIT_SUCCESS;
        }
    }

    AkLogInfo() << "Setting service dispatcher" << std::endl;

    WCHAR serviceName[] = TEXT(DSHOW_PLUGIN_ASSISTANT_NAME);
    SERVICE_TABLE_ENTRY serviceTable[] = {
        {serviceName, serviceMain},
        {nullptr    , nullptr    }
    };

    if (!StartServiceCtrlDispatcher(serviceTable)) {
        AkLogError() << "Service dispatcher failed" << std::endl;

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
