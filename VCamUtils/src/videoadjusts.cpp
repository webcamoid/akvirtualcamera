/* akvirtualcamera, virtual camera for Mac and Windows.
 * Copyright (C) 2025  Gonzalo Exequiel Pedone
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

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include "videoadjusts.h"
#include "color.h"
#include "videoconverter.h"
#include "videoframe.h"

namespace AkVCam
{
    class VideoAdjustsPrivate
    {
        public:
            bool m_horizontalMirror {false};
            bool m_verticalMirror {false};
            bool m_swapRGB {false};
            int m_hue {0};
            int m_saturation {0};
            int m_luminance {0};
            int m_gamma {0};
            int m_contrast {0};
            bool m_grayScaled {false};
            VideoConverter m_inputVideoConverter {{PixelFormat_argbpack, 0, 0}};
            VideoConverter m_outputVideoConverter;

            void adjustMirror(VideoFrame &frame);
            void adjustSwapRGB(VideoFrame &frame);
            void adjustHsl(VideoFrame &frame);
            void adjustGamma(VideoFrame &frame);
            void adjustContrast(VideoFrame &frame);
            void adjustGrayScale(VideoFrame &frame);
            inline void rgbToHsl(int r, int g, int b, int *h, int *s, int *l);
            inline void hslToRgb(int h, int s, int l, int *r, int *g, int *b);

            inline const uint8_t *gammaTable() const
            {
                static bool gammaTableReady = false;
                static std::vector<uint8_t> gammaTable;

                if (gammaTableReady)
                    return gammaTable.data();

                for (int i = 0; i < 256; i++) {
                    auto ig = uint8_t(255. * pow(i / 255., 255));
                    gammaTable.push_back(ig);
                }

                for (int gamma = -254; gamma < 256; gamma++) {
                    double k = 255. / (gamma + 255);

                    for (int i = 0; i < 256; i++) {
                        auto ig = uint8_t(255. * pow(i / 255., k));
                        gammaTable.push_back(ig);
                    }
                }

                gammaTableReady = true;

                return gammaTable.data();
            }

            inline const uint8_t *contrastTable() const
            {
                static bool contrastTableReady = false;
                static std::vector<uint8_t> contrastTable;

                if (contrastTableReady)
                    return contrastTable.data();

                for (int contrast = -255; contrast < 256; contrast++) {
                    double f = 259. * (255 + contrast) / (255. * (259 - contrast));

                    for (int i = 0; i < 256; i++) {
                        int ic = int(f * (i - 128) + 128.);
                        contrastTable.push_back(uint8_t(bound(0, ic, 255)));
                    }
                }

                contrastTableReady = true;

                return contrastTable.data();
            }
    };
}

AkVCam::VideoAdjusts::VideoAdjusts()
{
    this->d = new VideoAdjustsPrivate;
}

AkVCam::VideoAdjusts::~VideoAdjusts()
{
    delete this->d;
}

bool AkVCam::VideoAdjusts::horizontalMirror() const
{
    return this->d->m_horizontalMirror;
}

bool AkVCam::VideoAdjusts::verticalMirror() const
{
    return this->d->m_verticalMirror;
}

bool AkVCam::VideoAdjusts::swapRGB() const
{
    return this->d->m_swapRGB;
}

int AkVCam::VideoAdjusts::hue() const
{
    return this->d->m_hue;
}

int AkVCam::VideoAdjusts::saturation() const
{
    return this->d->m_saturation;
}

int AkVCam::VideoAdjusts::luminance() const
{
    return this->d->m_luminance;
}

int AkVCam::VideoAdjusts::gamma() const
{
    return this->d->m_gamma;
}

int AkVCam::VideoAdjusts::contrast() const
{
    return this->d->m_contrast;
}

bool AkVCam::VideoAdjusts::grayScaled() const
{
    return this->d->m_grayScaled;
}

void AkVCam::VideoAdjusts::setHorizontalMirror(bool horizontalMirror)
{
    this->d->m_horizontalMirror = horizontalMirror;
}

void AkVCam::VideoAdjusts::setVerticalMirror(bool verticalMirror)
{
    this->d->m_verticalMirror = verticalMirror;
}

void AkVCam::VideoAdjusts::setSwapRGB(bool swapRGB)
{
    this->d->m_swapRGB = swapRGB;
}

void AkVCam::VideoAdjusts::setHue(int hue)
{
    this->d->m_hue = hue;
}

void AkVCam::VideoAdjusts::setSaturation(int saturation)
{
    this->d->m_saturation = saturation;
}

void AkVCam::VideoAdjusts::setLuminance(int  luminance)
{
    this->d->m_luminance = luminance;
}

void AkVCam::VideoAdjusts::setGamma(int gamma)
{
    this->d->m_gamma = gamma;
}

void AkVCam::VideoAdjusts::setContrast(int contrast)
{
    this->d->m_contrast = contrast;
}

void AkVCam::VideoAdjusts::setGrayScaled(bool grayScaled)
{
    this->d->m_grayScaled = grayScaled;
}

AkVCam::VideoFrame AkVCam::VideoAdjusts::adjust(const VideoFrame &frame)
{
    if (this->d->m_hue == 0
        && this->d->m_saturation == 0
        && this->d->m_luminance == 0
        && this->d->m_gamma == 0
        && this->d->m_contrast == 0
        && !this->d->m_grayScaled
        && !this->d->m_swapRGB
        && !this->d->m_horizontalMirror
        && !this->d->m_verticalMirror) {
        return frame;
    }

    this->d->m_inputVideoConverter.begin();
    auto src = this->d->m_inputVideoConverter.convert(frame);
    this->d->m_inputVideoConverter.end();

    if (!src)
        return frame;

    if (this->d->m_hue != 0
        || this->d->m_saturation != 0
        || this->d->m_luminance != 0) {
        this->d->adjustHsl(src);
    }

    if (this->d->m_contrast != 0)
        this->d->adjustContrast(src);

    if (this->d->m_gamma != 0)
        this->d->adjustGamma(src);

    if (this->d->m_grayScaled)
        this->d->adjustGrayScale(src);

    if (this->d->m_swapRGB)
        this->d->adjustSwapRGB(src);

    if (this->d->m_swapRGB)
        this->d->adjustSwapRGB(src);

    if (!this->d->m_horizontalMirror || this->d->m_verticalMirror)
        this->d->adjustMirror(src);

    this->d->m_outputVideoConverter.setOutputFormat(frame.format());

    this->d->m_outputVideoConverter.begin();
    auto dst = this->d->m_outputVideoConverter.convert(src);
    this->d->m_outputVideoConverter.end();

    return dst;
}

void AkVCam::VideoAdjustsPrivate::adjustMirror(VideoFrame &frame)
{
    int width = frame.format().width();
    int height = frame.format().height();

    if (this->m_horizontalMirror) {
        int width2 = frame.format().width() / 2;

        for (int y = 0; y < height; ++y) {
            auto srcLine = reinterpret_cast<uint32_t *>(frame.line(0, size_t(y)));

            for (int x = 0; x < width2; ++x) {
                auto pixel = srcLine[x];
                srcLine[x] = srcLine[width - x - 1];
                srcLine[width - x - 1] = pixel;
            }
        }
    }

    if (this->m_verticalMirror) {
        int height2= frame.format().height() / 2;
        auto lineSize = frame.lineSize(0);
        auto tmpLine = new uint8_t [lineSize];

        for (int y = 0; y < height2; ++y) {
            auto srcLine = reinterpret_cast<uint32_t *>(frame.line(0, size_t(height - y - 1)));
            auto dstLine = reinterpret_cast<uint32_t *>(frame.line(0, size_t(y)));
            memcpy(tmpLine, dstLine, lineSize);
            memcpy(dstLine, srcLine, lineSize);
            memcpy(srcLine, tmpLine, lineSize);
        }

        delete [] tmpLine;
    }
}

void AkVCam::VideoAdjustsPrivate::adjustSwapRGB(VideoFrame &frame)
{
    int width = frame.format().width();
    int height = frame.format().height();

    for (int y = 0; y < height; ++y) {
        auto srcLine = reinterpret_cast<uint32_t *>(frame.line(0, size_t(y)));

        for (int x = 0; x < width; ++x) {
            srcLine[x] = Color::rgb(Color::blue(srcLine[x]),
                                    Color::green(srcLine[x]),
                                    Color::red(srcLine[x]),
                                    Color::alpha(srcLine[x]));
        }
    }
}

void AkVCam::VideoAdjustsPrivate::adjustHsl(VideoFrame &frame)
{
    int width = frame.format().width();
    int height = frame.format().height();

    for (int y = 0; y < height; ++y) {
        auto srcLine = reinterpret_cast<uint32_t *>(frame.line(0, size_t(y)));

        for (int x = 0; x < width; ++x) {
            int r = Color::red(srcLine[x]);
            int g = Color::green(srcLine[x]);
            int b = Color::blue(srcLine[x]);

            int h;
            int s;
            int l;
            this->rgbToHsl(r, g, b, &h, &s, &l);

            h = mod(h + this->m_hue, 360);
            s = bound(0, s + this->m_saturation, 255);
            l = bound(0, l + this->m_luminance, 255);
            this->hslToRgb(h, s, l, &r, &g, &b);

            srcLine[x] = Color::rgb(uint8_t(r),
                                    uint8_t(g),
                                    uint8_t(b),
                                    Color::alpha(srcLine[x]));
        }
    }
}

void AkVCam::VideoAdjustsPrivate::adjustGamma(VideoFrame &frame)
{
    int width = frame.format().width();
    int height = frame.format().height();

    auto dataGt = this->gammaTable();

    auto gamma = bound(-255, this->m_gamma, 255);
    size_t gammaOffset = size_t(gamma + 255) << 8;

    for (int y = 0; y < height; ++y) {
        auto srcLine = reinterpret_cast<uint32_t *>(frame.line(0, size_t(y)));

        for (int x = 0; x < width; ++x) {
            int r = Color::red(srcLine[x]);
            int g = Color::green(srcLine[x]);
            int b = Color::blue(srcLine[x]);

            srcLine[x] = Color::rgb(uint8_t(dataGt[gammaOffset | size_t(r)]),
                                    uint8_t(dataGt[gammaOffset | size_t(g)]),
                                    uint8_t(dataGt[gammaOffset | size_t(b)]),
                                    Color::alpha(srcLine[x]));
        }
    }
}

void AkVCam::VideoAdjustsPrivate::adjustContrast(VideoFrame &frame)
{
    int width = frame.format().width();
    int height = frame.format().height();

    auto dataCt = this->contrastTable();

    auto contrast = bound(-255, this->m_contrast, 255);
    size_t contrastOffset = size_t(contrast + 255) << 8;

    for (int y = 0; y < height; ++y) {
        auto srcLine = reinterpret_cast<uint32_t *>(frame.line(0, size_t(y)));

        for (int x = 0; x < width; ++x) {
            int r = Color::red(srcLine[x]);
            int g = Color::green(srcLine[x]);
            int b = Color::blue(srcLine[x]);

            srcLine[x] = Color::rgb(uint8_t(dataCt[contrastOffset | size_t(r)]),
                                    uint8_t(dataCt[contrastOffset | size_t(g)]),
                                    uint8_t(dataCt[contrastOffset | size_t(b)]),
                                    Color::alpha(srcLine[x]));
        }
    }
}

void AkVCam::VideoAdjustsPrivate::adjustGrayScale(VideoFrame &frame)
{
    int width = frame.format().width();
    int height = frame.format().height();

    for (int y = 0; y < height; ++y) {
        auto srcLine = reinterpret_cast<uint32_t *>(frame.line(0, size_t(y)));

        for (int x = 0; x < width; ++x)
            srcLine[x] = Color::gray(srcLine[x]);
    }
}

// https://en.wikipedia.org/wiki/HSL_and_HSV

void AkVCam::VideoAdjustsPrivate::rgbToHsl(int r, int g, int b, int *h, int *s, int *l)
{
    int max = std::max(r, std::max(g, b));
    int min = std::min(r, std::min(g, b));
    int c = max - min;

    *l = (max + min) / 2;

    if (!c) {
        *h = 0;
        *s = 0;
    } else {
        if (max == r)
            *h = mod(g - b, 6 * c);
        else if (max == g)
            *h = b - r + 2 * c;
        else
            *h = r - g + 4 * c;

        *h = 60 * (*h) / c;
        *s = 255 * c / (255 - abs(max + min - 255));
    }
}

void AkVCam::VideoAdjustsPrivate::hslToRgb(int h, int s, int l, int *r, int *g, int *b)
{
    int c = s * (255 - abs(2 * l - 255)) / 255;
    int x = c * (60 - abs((h % 120) - 60)) / 60;

    if (h >= 0 && h < 60) {
        *r = c;
        *g = x;
        *b = 0;
    } else if (h >= 60 && h < 120) {
        *r = x;
        *g = c;
        *b = 0;
    } else if (h >= 120 && h < 180) {
        *r = 0;
        *g = c;
        *b = x;
    } else if (h >= 180 && h < 240) {
        *r = 0;
        *g = x;
        *b = c;
    } else if (h >= 240 && h < 300) {
        *r = x;
        *g = 0;
        *b = c;
    } else if (h >= 300 && h < 360) {
        *r = c;
        *g = 0;
        *b = x;
    } else {
        *r = 0;
        *g = 0;
        *b = 0;
    }

    int m = 2 * l - c;

    *r = (2 * (*r) + m) >> 1;
    *g = (2 * (*g) + m) >> 1;
    *b = (2 * (*b) + m) >> 1;
}
