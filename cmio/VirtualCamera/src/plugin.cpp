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

#include "plugin.h"
#include "utils.h"
#include "PlatformUtils/src/preferences.h"
#include "VCamUtils/src/ipcbridge.h"
#include "VCamUtils/src/logger/logger.h"

extern "C" void *akPluginMain(CFAllocatorRef allocator,
                              CFUUIDRef requestedTypeUUID)
{
    UNUSED(allocator);
    auto loglevel = AkVCam::Preferences::logLevel();
    AkVCam::Logger::setLogLevel(loglevel);

    if (AkVCam::Logger::logLevel() > AKVCAM_LOGLEVEL_DEFAULT) {
        // Turn on lights
        freopen("/dev/tty", "a", stdout);
        freopen("/dev/tty", "a", stderr);
    }

    auto logFile =
            AkVCam::Preferences::readString("logfile",
                                            "/tmp/" CMIO_PLUGIN_NAME ".log");
    AkVCam::Logger::setLogFile(logFile);

    if (!CFEqual(requestedTypeUUID, kCMIOHardwarePlugInTypeID))
        return nullptr;

    return AkVCam::PluginInterface::create();
}
