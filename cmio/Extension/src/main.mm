/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2026  Gonzalo Exequiel Pedone
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

#ifndef FAKE_APPLE
#import <Foundation/Foundation.h>
#import <CoreMediaIO/CMIOExtensionProvider.h>

#import "extensionprovidersource.h"
#endif

#include "VCamUtils/src/logger.h"

int main(int argc, char *argv[])
{
    AkVCam::logSetup();

#ifndef FAKE_APPLE
    @autoreleasepool {
        /* Create the provider source. The virtual device and its stream are
         * constructed internally, but not yet registered with any provider.
         */
        ExtensionProviderSource *providerSource =
            [[ExtensionProviderSource alloc] init];

        /* Create the CMIOExtensionProvider backed by our source. The framework
         * uses the CMIOExtensionMachServiceName from Info.plist to register the
         * Mach service with the operating system.
         */
        CMIOExtensionProvider *provider =
            [CMIOExtensionProvider
             providerWithSource: providerSource
             clientQueue: dispatch_get_main_queue()];

        // Store the cross-reference needed by addDevice/removeDevice.
        providerSource.provider = provider;

        // Start serving clients. From this point the extension is visible to
        // AVCaptureDevice and other CoreMediaIO consumers.
        [CMIOExtensionProvider startServiceWithProvider: provider];

        // Run the main run loop to keep the extension process alive.
        [[NSRunLoop mainRunLoop] run];
    }
#endif

    return 0;
}
