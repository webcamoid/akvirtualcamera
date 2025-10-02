/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2021  Gonzalo Exequiel Pedone
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
 * Web-Site: http://github.com/webcamoid/akvirtualcamera
 */

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <thread>

#include "capi.h"

/* For the sake of simplicity, the program won't print anything to the terminal,
 * or do any validation check, you are adviced the check every single value
 * returned by each function.
 * This program shows the very minimum code required to write frames to the
 * plugin.
 */

// We'll assume this is a valid akvcam output device.
#define VIDEO_OUTPUT "AkVCamVideoDevice0"

// Send frames for about 30 seconds in a 30 FPS stream.
#define FPS 30
#define DURATION_SECONDS 30
#define N_FRAMES (FPS * DURATION_SECONDS)

int main()
{
    // Set the parameters of the stream.
    const char format[] = "RGB24";
    int width = 640;
    int height = 480;
    size_t line_size = 3 * width;

    // Open a virtual camera instance.
    auto vcam = vcam_open();

    if (vcam) {
        // Start streaming to the virtual camera.
        if (vcam_stream_start(vcam, VIDEO_OUTPUT) == 0) {
            // Allocate the frame buffer. The frame must be 32 bits aligned.
            auto frame_buffer = new char [line_size * height];

            // Generate some random noise frames.
            srand(time(0));

            for (int i = 0; i < N_FRAMES; i++) {
                // Write the frame line by line.
                for (int y = 0; y < height; y++) {
                    auto line = frame_buffer + y * line_size;

                    for (int x = 0; x < width; x++)
                        line[x] = rand() & 0xff;
                }

                // Write the frame data to the buffer.
                vcam_stream_send(vcam,
                                 VIDEO_OUTPUT,
                                 format,
                                 width,
                                 height,
                                 const_cast<const char **>(&frame_buffer),
                                 &line_size);

                std::this_thread::sleep_for(std::chrono::milliseconds(1000 / FPS));
            }

            // Release the frame buffer.
            delete [] frame_buffer;

            // Stop streaming
            vcam_stream_stop(vcam, VIDEO_OUTPUT);
        }

        // Close the virtual camera instance.
        vcam_close(vcam);
    }

    return 0;
}
