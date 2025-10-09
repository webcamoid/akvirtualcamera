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
#include <csignal>

#include "service.h"
#include "PlatformUtils/src/preferences.h"
#include "PlatformUtils/src/utils.h"
#include "VCamUtils/src/logger.h"

AkVCam::Service *servicePtr = nullptr;

int main(int argc, char **argv)
{
    AkVCam::logSetup();
    AkVCam::Service service;
    servicePtr = &service;

    signal(SIGTERM, [] (int) {
        servicePtr->stop();
    });

#ifndef _WIN32
    signal(SIGPIPE, [] (int) {
    });
#endif

    return service.run();
}
