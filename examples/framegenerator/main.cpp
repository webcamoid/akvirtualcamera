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

#include <QCoreApplication>
#include <QByteArray>

#include <iostream>
#include <vector>
#include <string>

#include <VCamUtils/src/ipcbridge.h>
#include <VCamUtils/src/image/videoformat.h>
#include <VCamUtils/src/image/videoframe.h>

AkVCam::VideoData redFrame(void) {
    static AkVCam::VideoData frame;
    if ( frame.empty() ) {
        frame.reserve( 640*480*3 );
        for( int i = 0; i < 640; ++i )
            for ( int j = 0; j < 480; ++j ) {
                frame.push_back(0xFF);
                frame.push_back(0x00);
                frame.push_back(0x00);
            }
    }
    return frame;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    std::cout << "AkVirtualCamera frame generator example" << std::endl;

    AkVCam::IpcBridge ipcb;
    ipcb.connectService(false);
    auto devices = ipcb.listDevices();

    std::cout << "List of devices:" << std::endl;
    for( auto d : devices ) {
        std::cout << "\t" << d << std::endl;
    }

    if ( !ipcb.deviceStart(devices[0], AkVCam::VideoFormat(AkVCam::PixelFormat::PixelFormatRGB24, 640, 480)) ) {
        std::cout << "Error initializing " << devices[0];
        exit(-1);
    }
    
    AkVCam::VideoFrame videoFrame;
    videoFrame.data() = redFrame();
    ipcb.write( devices[0], videoFrame );

    return app.exec();
}
