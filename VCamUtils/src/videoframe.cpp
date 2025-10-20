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
#include <cmath>
#include <cstring>
#include <fstream>

#include "videoframe.h"
#include "algorithm.h"
#include "color.h"
#include "colorcomponent.h"
#include "colorconvert.h"
#include "videoformat.h"
#include "videoformatspec.h"
#include "utils.h"

#define MAX_PLANES 4

namespace AkVCam
{
    enum FillType
    {
        FillType_Vector,
        FillType_1,
        FillType_3,
    };

    enum FillDataTypes
    {
        FillDataTypes_8,
        FillDataTypes_16,
        FillDataTypes_32,
        FillDataTypes_64,
    };

    enum AlphaMode
    {
        AlphaMode_AO,
        AlphaMode_O,
    };

    class FillParameters
    {
        public:
            ColorConvert colorConvert;
            FillType fillType {FillType_3};
            FillDataTypes fillDataTypes {FillDataTypes_8};
            AlphaMode alphaMode {AlphaMode_AO};

            int endianess {ENDIANNESS_BO};

            int width {0};
            int height {0};

            int *dstWidthOffsetX {nullptr};
            int *dstWidthOffsetY {nullptr};
            int *dstWidthOffsetZ {nullptr};
            int *dstWidthOffsetA {nullptr};

            int planeXo {0};
            int planeYo {0};
            int planeZo {0};
            int planeAo {0};

            ColorComponent compXo;
            ColorComponent compYo;
            ColorComponent compZo;
            ColorComponent compAo;

            size_t xoOffset {0};
            size_t yoOffset {0};
            size_t zoOffset {0};
            size_t aoOffset {0};

            size_t xoShift {0};
            size_t yoShift {0};
            size_t zoShift {0};
            size_t aoShift {0};

            uint64_t maskXo {0};
            uint64_t maskYo {0};
            uint64_t maskZo {0};
            uint64_t maskAo {0};

            FillParameters();
            FillParameters(const FillParameters &other);
            ~FillParameters();
            FillParameters &operator =(const FillParameters &other);
            inline void clearBuffers();
            inline void allocateBuffers(const VideoFormat &caps);
            void configure(const VideoFormat &caps, ColorConvert &colorConvert);
            void configureFill(const VideoFormat &caps);
            void reset();
    };

    using FillParametersPtr = std::shared_ptr<FillParameters>;

    class VideoFramePrivate
    {
        public:
            VideoFormat m_format;
            uint8_t *m_data {nullptr};
            size_t m_dataSize {0};
            size_t m_nPlanes {0};
            uint8_t *m_planes[MAX_PLANES];
            size_t m_planeSize[MAX_PLANES];
            size_t m_planeOffset[MAX_PLANES];
            size_t m_pixelSize[MAX_PLANES];
            size_t m_lineSize[MAX_PLANES];
            size_t m_bytesUsed[MAX_PLANES];
            size_t m_widthDiv[MAX_PLANES];
            size_t m_heightDiv[MAX_PLANES];
            size_t m_align {32};
            FillParametersPtr m_fc;

            void updateParams(const VideoFormatSpec &specs);
            inline void updatePlanes();

            /* Fill functions */

            template <typename DataType>
            void fill3(const FillParameters &fc, Rgb color) const
            {
                auto xi = Color::red(color);
                auto yi = Color::green(color);
                auto zi = Color::blue(color);
                auto ai = Color::alpha(color);

                int64_t xo_ = 0;
                int64_t yo_ = 0;
                int64_t zo_ = 0;
                fc.colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);
                fc.colorConvert.applyAlpha(ai, &xo_, &yo_, &zo_);

                auto line_x = this->m_planes[fc.planeXo] + fc.xoOffset;
                auto line_y = this->m_planes[fc.planeYo] + fc.yoOffset;
                auto line_z = this->m_planes[fc.planeZo] + fc.zoOffset;

                auto width = std::max<size_t>(8 * this->m_pixelSize[0] / this->m_format.bpp(), 1);

                for (size_t x = 0; x < width; ++x) {
                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<DataType *>(line_x + xd_x);
                    auto yo = reinterpret_cast<DataType *>(line_y + xd_y);
                    auto zo = reinterpret_cast<DataType *>(line_z + xd_z);

                    *xo = (*xo & DataType(fc.maskXo)) | (DataType(xo_) << fc.xoShift);
                    *yo = (*yo & DataType(fc.maskYo)) | (DataType(yo_) << fc.yoShift);
                    *zo = (*zo & DataType(fc.maskZo)) | (DataType(zo_) << fc.zoShift);
                }
            }

            template <typename DataType>
            void fill3A(const FillParameters &fc, Rgb color) const
            {
                auto xi = Color::red(color);
                auto yi = Color::green(color);
                auto zi = Color::blue(color);
                auto ai = Color::alpha(color);

                int64_t xo_ = 0;
                int64_t yo_ = 0;
                int64_t zo_ = 0;
                fc.colorConvert.applyMatrix(xi, yi, zi, &xo_, &yo_, &zo_);

                auto line_x = this->m_planes[fc.planeXo] + fc.xoOffset;
                auto line_y = this->m_planes[fc.planeYo] + fc.yoOffset;
                auto line_z = this->m_planes[fc.planeZo] + fc.zoOffset;
                auto line_a = this->m_planes[fc.planeAo] + fc.aoOffset;

                auto width = std::max<size_t>(8 * this->m_pixelSize[0] / this->m_format.bpp(), 1);

                for (size_t x = 0; x < width; ++x) {
                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<DataType *>(line_x + xd_x);
                    auto yo = reinterpret_cast<DataType *>(line_y + xd_y);
                    auto zo = reinterpret_cast<DataType *>(line_z + xd_z);
                    auto ao = reinterpret_cast<DataType *>(line_a + xd_a);

                    *xo = (*xo & DataType(fc.maskXo)) | (DataType(xo_) << fc.xoShift);
                    *yo = (*yo & DataType(fc.maskYo)) | (DataType(yo_) << fc.yoShift);
                    *zo = (*zo & DataType(fc.maskZo)) | (DataType(zo_) << fc.zoShift);
                    *ao = (*ao & DataType(fc.maskAo)) | (DataType(ai) << fc.aoShift);
                }
            }

            template <typename DataType>
            void fill1(const FillParameters &fc, Rgb color) const
            {
                auto xi = Color::red(color);
                auto yi = Color::green(color);
                auto zi = Color::blue(color);
                auto ai = Color::alpha(color);

                int64_t xo_ = 0;
                fc.colorConvert.applyPoint(xi, yi, zi, &xo_);
                fc.colorConvert.applyAlpha(ai, &xo_);

                auto line_x = this->m_planes[fc.planeXo] + fc.xoOffset;
                auto width = std::max<size_t>(8 * this->m_pixelSize[0] / this->m_format.bpp(), 1);

                for (size_t x = 0; x < width; ++x) {
                    int &xd_x = fc.dstWidthOffsetX[x];
                    auto xo = reinterpret_cast<DataType *>(line_x + xd_x);
                    *xo = (*xo & DataType(fc.maskXo)) | (DataType(xo_) << fc.xoShift);
                }
            }

            template <typename DataType>
            void fill1A(const FillParameters &fc, Rgb color) const
            {
                auto xi = Color::red(color);
                auto yi = Color::green(color);
                auto zi = Color::blue(color);
                auto ai = Color::alpha(color);

                int64_t xo_ = 0;
                fc.colorConvert.applyPoint(xi, yi, zi, &xo_);

                auto line_x = this->m_planes[fc.planeXo] + fc.xoOffset;
                auto line_a = this->m_planes[fc.planeAo] + fc.aoOffset;

                auto width = std::max<size_t>(8 * this->m_pixelSize[0] / this->m_format.bpp(), 1);

                for (size_t x = 0; x < width; ++x) {
                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<DataType *>(line_x + xd_x);
                    auto ao = reinterpret_cast<DataType *>(line_a + xd_a);

                    *xo = (*xo & DataType(fc.maskXo)) | (DataType(xo_) << fc.xoShift);
                    *ao = (*ao & DataType(fc.maskAo)) | (DataType(ai) << fc.aoShift);
                }
            }

            // Vectorized fill functions

            template <typename DataType>
            void fillV3(const FillParameters &fc, Rgb color) const
            {
                auto xi = Color::red(color);
                auto yi = Color::green(color);
                auto zi = Color::blue(color);
                auto ai = Color::alpha(color);

                int64_t xo_ = 0;
                int64_t yo_ = 0;
                int64_t zo_ = 0;
                fc.colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);
                fc.colorConvert.applyAlpha(ai, &xo_, &yo_, &zo_);

                auto line_x = this->m_planes[fc.planeXo] + fc.xoOffset;
                auto line_y = this->m_planes[fc.planeYo] + fc.yoOffset;
                auto line_z = this->m_planes[fc.planeZo] + fc.zoOffset;

                auto width = std::max<size_t>(8 * this->m_pixelSize[0] / this->m_format.bpp(), 1);

                for (size_t x = 0; x < width; ++x) {
                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];

                    auto xo = reinterpret_cast<DataType *>(line_x + xd_x);
                    auto yo = reinterpret_cast<DataType *>(line_y + xd_y);
                    auto zo = reinterpret_cast<DataType *>(line_z + xd_z);

                    *xo = (*xo & DataType(fc.maskXo)) | (DataType(xo_) << fc.xoShift);
                    *yo = (*yo & DataType(fc.maskYo)) | (DataType(yo_) << fc.yoShift);
                    *zo = (*zo & DataType(fc.maskZo)) | (DataType(zo_) << fc.zoShift);
                }
            }

            template <typename DataType>
            void fillV3A(const FillParameters &fc, Rgb color) const
            {
                auto xi = Color::red(color);
                auto yi = Color::green(color);
                auto zi = Color::blue(color);
                auto ai = Color::alpha(color);

                int64_t xo_ = 0;
                int64_t yo_ = 0;
                int64_t zo_ = 0;
                fc.colorConvert.applyVector(xi, yi, zi, &xo_, &yo_, &zo_);

                auto line_x = this->m_planes[fc.planeXo] + fc.xoOffset;
                auto line_y = this->m_planes[fc.planeYo] + fc.yoOffset;
                auto line_z = this->m_planes[fc.planeZo] + fc.zoOffset;
                auto line_a = this->m_planes[fc.planeAo] + fc.aoOffset;

                auto width = std::max<size_t>(8 * this->m_pixelSize[0] / this->m_format.bpp(), 1);

                for (size_t x = 0; x < width; ++x) {
                    int &xd_x = fc.dstWidthOffsetX[x];
                    int &xd_y = fc.dstWidthOffsetY[x];
                    int &xd_z = fc.dstWidthOffsetZ[x];
                    int &xd_a = fc.dstWidthOffsetA[x];

                    auto xo = reinterpret_cast<DataType *>(line_x + xd_x);
                    auto yo = reinterpret_cast<DataType *>(line_y + xd_y);
                    auto zo = reinterpret_cast<DataType *>(line_z + xd_z);
                    auto ao = reinterpret_cast<DataType *>(line_a + xd_a);

                    *xo = (*xo & DataType(fc.maskXo)) | (DataType(xo_) << fc.xoShift);
                    *yo = (*yo & DataType(fc.maskYo)) | (DataType(yo_) << fc.yoShift);
                    *zo = (*zo & DataType(fc.maskZo)) | (DataType(zo_) << fc.zoShift);
                    *ao = (*ao & DataType(fc.maskAo)) | (DataType(ai) << fc.aoShift);
                }
            }

    #define FILL_FUNC(components) \
            template <typename DataType> \
                inline void fillFrame##components(const FillParameters &fc, \
                                                  Rgb color) const \
            { \
                    switch (fc.alphaMode) { \
                    case AlphaMode_AO: \
                        this->fill##components##A<DataType>(fc, color); \
                        break; \
                    case AlphaMode_O: \
                        this->fill##components<DataType>(fc, color); \
                        break; \
                }; \
            }

    #define FILLV_FUNC(components) \
            template <typename DataType> \
                inline void fillVFrame##components(const FillParameters &fc, \
                                                   Rgb color) const \
            { \
                    switch (fc.alphaMode) { \
                    case AlphaMode_AO: \
                        this->fillV##components##A<DataType>(fc, color); \
                        break; \
                    case AlphaMode_O: \
                        this->fillV##components<DataType>(fc, color); \
                        break; \
                }; \
            }

            FILL_FUNC(1)
            FILL_FUNC(3)
            FILLV_FUNC(3)

            template <typename DataType>
            inline void fill(const FillParameters &fc, Rgb color)
            {
                switch (fc.fillType) {
                case FillType_Vector:
                    this->fillVFrame3<DataType>(fc, color);
                    break;
                case FillType_3:
                    this->fillFrame3<DataType>(fc, color);
                    break;
                case FillType_1:
                    this->fillFrame1<DataType>(fc, color);
                    break;
                }
            }

            inline void fill(Rgb color);
    };

    struct BmpHeader
    {
        uint32_t size;
        uint16_t reserved1;
        uint16_t reserved2;
        uint32_t offBits;
    };

    struct BmpImageHeader
    {
        uint32_t size;
        uint32_t width;
        uint32_t height;
        uint16_t planes;
        uint16_t bitCount;
        uint32_t compression;
        uint32_t sizeImage;
        uint32_t xPelsPerMeter;
        uint32_t yPelsPerMeter;
        uint32_t clrUsed;
        uint32_t clrImportant;
    };
}

AkVCam::VideoFrame::VideoFrame()
{
    this->d = new VideoFramePrivate;
}

AkVCam::VideoFrame::VideoFrame(const std::string &fileName)
{
    this->d = new VideoFramePrivate;
    this->load(fileName);
}

AkVCam::VideoFrame::VideoFrame(const AkVCam::VideoFormat &format,
                               bool initialized)
{
    this->d = new VideoFramePrivate;
    this->d->m_format = format;
    auto specs = VideoFormat::formatSpecs(this->d->m_format.format());
    this->d->m_nPlanes = specs.planes();
    this->d->updateParams(specs);

    if (this->d->m_dataSize > 0) {
            this->d->m_data = new uint8_t [this->d->m_dataSize];

            if (initialized)
                memset(this->d->m_data, 0, this->d->m_dataSize);
    }

    this->d->updatePlanes();
}

AkVCam::VideoFrame::VideoFrame(const AkVCam::VideoFrame &other)
{
    this->d = new VideoFramePrivate;
    this->d->m_format = other.d->m_format;

    if (other.d->m_data && other.d->m_dataSize > 0) {
        this->d->m_data = new uint8_t [other.d->m_dataSize];
        memcpy(this->d->m_data, other.d->m_data, other.d->m_dataSize);
    }

    this->d->m_dataSize = other.d->m_dataSize;
    this->d->m_nPlanes = other.d->m_nPlanes;

    if (this->d->m_nPlanes > 0) {
        const size_t dataSize = MAX_PLANES * sizeof(size_t);
        memcpy(this->d->m_planeSize, other.d->m_planeSize, dataSize);
        memcpy(this->d->m_planeOffset, other.d->m_planeOffset, dataSize);
        memcpy(this->d->m_pixelSize, other.d->m_pixelSize, dataSize);
        memcpy(this->d->m_lineSize, other.d->m_lineSize, dataSize);
        memcpy(this->d->m_bytesUsed, other.d->m_bytesUsed, dataSize);
        memcpy(this->d->m_widthDiv, other.d->m_widthDiv, dataSize);
        memcpy(this->d->m_heightDiv, other.d->m_heightDiv, dataSize);
    }

    this->d->m_align = other.d->m_align;
    this->d->m_fc = other.d->m_fc;
    this->d->updatePlanes();
}

AkVCam::VideoFrame::~VideoFrame()
{
    if (this->d->m_data)
        delete [] this->d->m_data;

    delete this->d;
}

AkVCam::VideoFrame &AkVCam::VideoFrame::operator =(const AkVCam::VideoFrame &other)
{
    if (this != &other) {
        this->d->m_format = other.d->m_format;

        if (this->d->m_data) {
            delete [] this->d->m_data;
            this->d->m_data = nullptr;
        }

        if (other.d->m_data && other.d->m_dataSize > 0) {
            this->d->m_data = new uint8_t [other.d->m_dataSize];
            memcpy(this->d->m_data, other.d->m_data, other.d->m_dataSize);
        }

        this->d->m_dataSize = other.d->m_dataSize;
        this->d->m_nPlanes = other.d->m_nPlanes;

        if (this->d->m_nPlanes > 0) {
            memcpy(this->d->m_planeSize, other.d->m_planeSize, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_planeOffset, other.d->m_planeOffset, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_pixelSize, other.d->m_pixelSize, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_lineSize, other.d->m_lineSize, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_bytesUsed, other.d->m_bytesUsed, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_widthDiv, other.d->m_widthDiv, this->d->m_nPlanes * sizeof(size_t));
            memcpy(this->d->m_heightDiv, other.d->m_heightDiv, this->d->m_nPlanes * sizeof(size_t));
        }

        this->d->m_align = other.d->m_align;
        this->d->m_fc = other.d->m_fc;
        this->d->updatePlanes();
    }

    return *this;
}

AkVCam::VideoFrame::operator bool() const
{
    return this->d->m_format && this->d->m_data;
}

// http://www.dragonwins.com/domains/getteched/bmp/bmpfileformat.htm
bool AkVCam::VideoFrame::load(const std::string &fileName)
{
    AkLogFunction();

    this->d->m_format = {};

    if (this->d->m_data) {
        delete [] this->d->m_data;
        this->d->m_data = nullptr;
    }

    if (fileName.empty()) {
        AkLogError("The file name is empty");

        return false;
    }

    std::ifstream stream(fileName);

    if (!stream.is_open()) {
        AkLogError("Failed to open the file");

        return false;
    }

    char type[2];
    stream.read(type, 2);

    if (memcmp(type, "BM", 2) != 0) {
        AkLogError("The file does not have the BMP signature");

        return false;
    }

    BmpHeader header {};
    stream.read(reinterpret_cast<char *>(&header), sizeof(BmpHeader));

    BmpImageHeader imageHeader {};
    stream.read(reinterpret_cast<char *>(&imageHeader), sizeof(BmpImageHeader));

    if (imageHeader.width < 1 || imageHeader.height < 1) {
        AkLogError("The image size is empty: %dx%d", imageHeader.width, imageHeader.height);

        return false;
    }

    size_t srcLineSize =
            Algorithm::alignUp<size_t>(imageHeader.width
                                       * imageHeader.bitCount
                                       / 8,
                                       32);

    stream.seekg(header.offBits, std::ios_base::beg);
    std::vector<uint8_t> data;

    if (imageHeader.bitCount == 24 || imageHeader.bitCount == 32) {
        this->d->m_format = {PixelFormat_argbpack,
                             int(imageHeader.width),
                             int(imageHeader.height)};

        if (this->d->m_data) {
            delete [] this->d->m_data;
            this->d->m_data = nullptr;
        }

        auto specs = VideoFormat::formatSpecs(this->d->m_format.format());
        this->d->m_nPlanes = specs.planes();
        this->d->updateParams(specs);

        if (this->d->m_dataSize > 0)
            this->d->m_data = new uint8_t [this->d->m_dataSize];

        this->d->updatePlanes();
        data.resize(imageHeader.sizeImage);
        stream.read(reinterpret_cast<char *>(data.data()),
                    imageHeader.sizeImage);
    }

    switch (imageHeader.bitCount) {
    case 24: {
        for (uint32_t y = 0; y < imageHeader.height; y++) {
            auto srcLine = data.data() + y * srcLineSize;
            auto dstLine = reinterpret_cast<uint32_t *>
                           (this->line(0, size_t(imageHeader.height - y - 1)));

            for (uint32_t x = 0; x < imageHeader.width; x++)
                dstLine[x] = Color::rgb(srcLine[3 * x + 2],
                                        srcLine[3 * x + 1],
                                        srcLine[3 * x]);
        }

        break;
    }

    case 32: {
        for (uint32_t y = 0; y < imageHeader.height; y++) {
            auto srcLine = data.data() + y * srcLineSize;
            auto dstLine = reinterpret_cast<uint32_t *>
                           (this->line(0, size_t(imageHeader.height - y - 1)));

            for (uint32_t x = 0; x < imageHeader.width; x++)
                dstLine[x] = Color::rgb(srcLine[4 * x + 2],
                                        srcLine[4 * x + 1],
                                        srcLine[4 * x],
                                        srcLine[4 * x + 3]);
        }

        break;
    }

    default:
        AkLogError("Invalid bit cout: %d", imageHeader.bitCount);

        return false;
    }

    AkLogDebug("BMP info:");
    AkLogDebug("    Bits: %d", imageHeader.bitCount);
    AkLogDebug("    width: %d", imageHeader.width);
    AkLogDebug("    height: %d", imageHeader.height);
    AkLogDebug("    Data size: %d", imageHeader.sizeImage);
    AkLogDebug("    Allocated frame size: %zu", this->d->m_dataSize);

    return true;
}

AkVCam::VideoFormat AkVCam::VideoFrame::format() const
{
    return this->d->m_format;
}

size_t AkVCam::VideoFrame::size() const
{
    return this->d->m_dataSize;
}

size_t AkVCam::VideoFrame::planes() const
{
    return this->d->m_nPlanes;
}

size_t AkVCam::VideoFrame::planeSize(int plane) const
{
    return this->d->m_planeSize[plane];
}

size_t AkVCam::VideoFrame::pixelSize(int plane) const
{
    return this->d->m_pixelSize[plane];
}

size_t AkVCam::VideoFrame::lineSize(int plane) const
{
    return this->d->m_lineSize[plane];
}

size_t AkVCam::VideoFrame::bytesUsed(int plane) const
{
    return this->d->m_bytesUsed[plane];
}

size_t AkVCam::VideoFrame::widthDiv(int plane) const
{
    return this->d->m_widthDiv[plane];
}

size_t AkVCam::VideoFrame::heightDiv(int plane) const
{
    return this->d->m_heightDiv[plane];
}

const uint8_t *AkVCam::VideoFrame::constData() const
{
    return reinterpret_cast<uint8_t *>(this->d->m_data);
}

uint8_t *AkVCam::VideoFrame::data()
{
    return reinterpret_cast<uint8_t *>(this->d->m_data);
}

const uint8_t *AkVCam::VideoFrame::constPlane(int plane) const
{
    return this->d->m_planes[plane];
}

uint8_t *AkVCam::VideoFrame::plane(int plane)
{
    return this->d->m_planes[plane];
}

const uint8_t *AkVCam::VideoFrame::constLine(int plane, int y) const
{
    return this->d->m_planes[plane]
            + size_t(y >> this->d->m_heightDiv[plane])
            * this->d->m_lineSize[plane];
}

uint8_t *AkVCam::VideoFrame::line(int plane, int y)
{
    return this->d->m_planes[plane]
            + size_t(y >> this->d->m_heightDiv[plane])
            * this->d->m_lineSize[plane];
}

AkVCam::VideoFrame AkVCam::VideoFrame::copy(int x,
                                            int y,
                                            int width,
                                            int height) const
{
    auto ocaps = this->d->m_format;
    ocaps.setWidth(width);
    ocaps.setHeight(height);
    VideoFrame dst(ocaps, true);

    auto maxX = std::min(x + width, this->d->m_format.width());
    auto maxY = std::min(y + height, this->d->m_format.height());
    auto copyWidth = std::max(maxX - x, 0);

    if (copyWidth < 1)
        return dst;

    auto diffY = maxY - y;

    for (int plane = 0; plane < this->d->m_nPlanes; plane++) {
        size_t offset = x
                        * this->d->m_bytesUsed[plane]
                        / this->d->m_format.width();
        size_t copyBytes = copyWidth
                           * this->d->m_bytesUsed[plane]
                           / this->d->m_format.width();
        auto srcLineOffset = this->d->m_lineSize[plane];
        auto dstLineOffset = dst.d->m_lineSize[plane];
        auto srcLine = this->constLine(plane, y) + offset;
        auto dstLine = dst.d->m_planes[plane];
        auto maxY = diffY >> this->d->m_heightDiv[plane];

        for (int y = 0; y < maxY; y++) {
            memcpy(dstLine, srcLine, copyBytes);
            srcLine += srcLineOffset;
            dstLine += dstLineOffset;
        }
    }

    return dst;
}

void AkVCam::VideoFrame::fillRgb(Rgb color)
{
    return this->d->fill(color);
}

void AkVCam::VideoFramePrivate::updateParams(const VideoFormatSpec &specs)
{
    this->m_dataSize = 0;

    // Calculate parameters for each plane
    for (size_t i = 0; i < specs.planes(); ++i) {
        auto &plane = specs.plane(i);

        // Calculate bytes used per line (bits per pixel * width / 8)
        size_t bytesUsed = plane.bitsSize() * this->m_format.width() / 8;

        // Align line size for SIMD compatibility
        size_t lineSize =
                Algorithm::alignUp(bytesUsed, size_t(this->m_align));

        // Store pixel size, line size, and bytes used
        this->m_pixelSize[i] = plane.pixelSize();
        this->m_lineSize[i] = lineSize;
        this->m_bytesUsed[i] = bytesUsed;

        // Calculate plane size, considering sub-sampling
        size_t planeSize = (lineSize * this->m_format.height()) >> plane.heightDiv();

        // Align plane size to ensure next plane starts aligned
        planeSize =
                Algorithm::alignUp(planeSize, size_t(this->m_align));

        // Store plane size and offset
        this->m_planeSize[i] = planeSize;
        this->m_planeOffset[i] = this->m_dataSize;

        // Update total data size
        this->m_dataSize += planeSize;

        // Store width and height divisors for sub-sampling
        this->m_widthDiv[i] = plane.widthDiv();
        this->m_heightDiv[i] = plane.heightDiv();
    }

    // Align total data size for buffer allocation
    this->m_dataSize =
            Algorithm::alignUp(this->m_dataSize, size_t(this->m_align));
}

void AkVCam::VideoFramePrivate::updatePlanes()
{
    for (int i = 0; i < this->m_nPlanes; ++i)
        this->m_planes[i] = this->m_data + this->m_planeOffset[i];
}

#define DEFINE_FILL_FUNC(size) \
    case FillDataTypes_##size: \
        this->fill<uint##size##_t>(*this->m_fc, color); \
        \
        if (this->m_fc->endianess != ENDIANNESS_BO) \
            for (size_t plane = 0; plane < this->m_nPlanes; ++plane) \
                AkVCam::Algorithm::swapDataBytes(reinterpret_cast<uint##size##_t *>(this->m_planes[plane]), this->m_pixelSize[plane]); \
        \
        break;

void AkVCam::VideoFramePrivate::fill(Rgb color)
{
    if (!this->m_fc) {
        this->m_fc = FillParametersPtr(new FillParameters);
        this->m_fc->configure(this->m_format, this->m_fc->colorConvert);
        this->m_fc->configureFill(this->m_format);
    }

    switch (this->m_fc->fillDataTypes) {
    DEFINE_FILL_FUNC(8)
    DEFINE_FILL_FUNC(16)
    DEFINE_FILL_FUNC(32)
    DEFINE_FILL_FUNC(64)
    default:
        break;
    }

    for (size_t plane = 0; plane < this->m_nPlanes; plane++) {
        auto &lineSize = this->m_lineSize[plane];
        auto &pixelSize = this->m_pixelSize[plane];
        auto line0 = this->m_planes[plane];
        auto line = line0 + pixelSize;
        auto width = lineSize / pixelSize;
        auto height = this->m_fc->height >> this->m_heightDiv[plane];

        for (int x = 1; x < width; ++x) {
            memcpy(line, line0, pixelSize);
            line += pixelSize;
        }

        line = line0 + lineSize;

        for (int y = 1; y < height; ++y) {
            memcpy(line, line0, lineSize);
            line += lineSize;
        }
    }
}

AkVCam::FillParameters::FillParameters()
{
}

AkVCam::FillParameters::FillParameters(const FillParameters &other):
    colorConvert(other.colorConvert),
    fillType(other.fillType),
    fillDataTypes(other.fillDataTypes),
    alphaMode(other.alphaMode),
    endianess(other.endianess),
    width(other.width),
    height(other.height),
    planeXo(other.planeXo),
    planeYo(other.planeYo),
    planeZo(other.planeZo),
    planeAo(other.planeAo),
    compXo(other.compXo),
    compYo(other.compYo),
    compZo(other.compZo),
    compAo(other.compAo),
    xoOffset(other.xoOffset),
    yoOffset(other.yoOffset),
    zoOffset(other.zoOffset),
    aoOffset(other.aoOffset),
    xoShift(other.xoShift),
    yoShift(other.yoShift),
    zoShift(other.zoShift),
    aoShift(other.aoShift),
    maskXo(other.maskXo),
    maskYo(other.maskYo),
    maskZo(other.maskZo),
    maskAo(other.maskAo)
{
    if (this->width > 0) {
        size_t oWidthDataSize = sizeof(int) * this->width;

        if (other.dstWidthOffsetX) {
            this->dstWidthOffsetX = new int [this->width];
            memcpy(this->dstWidthOffsetX, other.dstWidthOffsetX, oWidthDataSize);
        }

        if (other.dstWidthOffsetY) {
            this->dstWidthOffsetY = new int [this->width];
            memcpy(this->dstWidthOffsetY, other.dstWidthOffsetY, oWidthDataSize);
        }

        if (other.dstWidthOffsetZ) {
            this->dstWidthOffsetZ = new int [this->width];
            memcpy(this->dstWidthOffsetZ, other.dstWidthOffsetZ, oWidthDataSize);
        }

        if (other.dstWidthOffsetA) {
            this->dstWidthOffsetA = new int [this->width];
            memcpy(this->dstWidthOffsetA, other.dstWidthOffsetA, oWidthDataSize);
        }
    }
}

AkVCam::FillParameters::~FillParameters()
{
    this->clearBuffers();
}

AkVCam::FillParameters &AkVCam::FillParameters::operator =(const FillParameters &other)
{
    if (this != &other) {
        this->colorConvert = other.colorConvert;
        this->fillType = other.fillType;
        this->fillDataTypes = other.fillDataTypes;
        this->alphaMode = other.alphaMode;
        this->endianess = other.endianess;
        this->width = other.width;
        this->height = other.height;
        this->planeXo = other.planeXo;
        this->planeYo = other.planeYo;
        this->planeZo = other.planeZo;
        this->planeAo = other.planeAo;
        this->compXo = other.compXo;
        this->compYo = other.compYo;
        this->compZo = other.compZo;
        this->compAo = other.compAo;
        this->xoOffset = other.xoOffset;
        this->yoOffset = other.yoOffset;
        this->zoOffset = other.zoOffset;
        this->aoOffset = other.aoOffset;
        this->xoShift = other.xoShift;
        this->yoShift = other.yoShift;
        this->zoShift = other.zoShift;
        this->aoShift = other.aoShift;
        this->maskXo = other.maskXo;
        this->maskYo = other.maskYo;
        this->maskZo = other.maskZo;
        this->maskAo = other.maskAo;

        if (this->width > 0) {
            size_t oWidthDataSize = sizeof(int) * this->width;

            if (other.dstWidthOffsetX) {
                this->dstWidthOffsetX = new int [this->width];
                memcpy(this->dstWidthOffsetX, other.dstWidthOffsetX, oWidthDataSize);
            }

            if (other.dstWidthOffsetY) {
                this->dstWidthOffsetY = new int [this->width];
                memcpy(this->dstWidthOffsetY, other.dstWidthOffsetY, oWidthDataSize);
            }

            if (other.dstWidthOffsetZ) {
                this->dstWidthOffsetZ = new int [this->width];
                memcpy(this->dstWidthOffsetZ, other.dstWidthOffsetZ, oWidthDataSize);
            }

            if (other.dstWidthOffsetA) {
                this->dstWidthOffsetA = new int [this->width];
                memcpy(this->dstWidthOffsetA, other.dstWidthOffsetA, oWidthDataSize);
            }
        }
    }

    return *this;
}

void AkVCam::FillParameters::clearBuffers()
{
    if (this->dstWidthOffsetX) {
        delete [] this->dstWidthOffsetX;
        this->dstWidthOffsetX = nullptr;
    }

    if (this->dstWidthOffsetY) {
        delete [] this->dstWidthOffsetY;
        this->dstWidthOffsetY = nullptr;
    }

    if (this->dstWidthOffsetZ) {
        delete [] this->dstWidthOffsetZ;
        this->dstWidthOffsetZ = nullptr;
    }

    if (this->dstWidthOffsetA) {
        delete [] this->dstWidthOffsetA;
        this->dstWidthOffsetA = nullptr;
    }
}

void AkVCam::FillParameters::allocateBuffers(const VideoFormat &caps)
{
    this->clearBuffers();

    if (caps.width() > 0) {
        this->dstWidthOffsetX = new int [caps.width()];
        this->dstWidthOffsetY = new int [caps.width()];
        this->dstWidthOffsetZ = new int [caps.width()];
        this->dstWidthOffsetA = new int [caps.width()];
    }
}

#define DEFINE_FILL_TYPES(size) \
    if (ospecs.depth() == size) \
        this->fillDataTypes = FillDataTypes_##size;

void AkVCam::FillParameters::configure(const VideoFormat &caps,
                                       ColorConvert &colorConvert)
{
    auto ispecs = VideoFormat::formatSpecs(PixelFormat_xrgbpack);
    auto ospecs = VideoFormat::formatSpecs(caps.format());

    DEFINE_FILL_TYPES(8);
    DEFINE_FILL_TYPES(16);
    DEFINE_FILL_TYPES(32);
    DEFINE_FILL_TYPES(64);

    auto components = ospecs.mainComponents();

    switch (components) {
    case 3:
        this->fillType =
            ospecs.type() == VideoFormatSpec::VFT_RGB?
                                FillType_Vector:
                                FillType_3;

        break;

    case 1:
        this->fillType = FillType_1;

        break;

    default:
        break;
    }

    this->endianess = ospecs.endianness();
    colorConvert.loadMatrix(ispecs, ospecs);

    switch (ospecs.type()) {
    case VideoFormatSpec::VFT_RGB:
        this->planeXo = ospecs.componentPlane(ColorComponent::CT_R);
        this->planeYo = ospecs.componentPlane(ColorComponent::CT_G);
        this->planeZo = ospecs.componentPlane(ColorComponent::CT_B);

        this->compXo = ospecs.component(ColorComponent::CT_R);
        this->compYo = ospecs.component(ColorComponent::CT_G);
        this->compZo = ospecs.component(ColorComponent::CT_B);

        break;

    case VideoFormatSpec::VFT_YUV:
        this->planeXo = ospecs.componentPlane(ColorComponent::CT_Y);
        this->planeYo = ospecs.componentPlane(ColorComponent::CT_U);
        this->planeZo = ospecs.componentPlane(ColorComponent::CT_V);

        this->compXo = ospecs.component(ColorComponent::CT_Y);
        this->compYo = ospecs.component(ColorComponent::CT_U);
        this->compZo = ospecs.component(ColorComponent::CT_V);

        break;

    case VideoFormatSpec::VFT_Gray:
        this->planeXo = ospecs.componentPlane(ColorComponent::CT_Y);
        this->compXo = ospecs.component(ColorComponent::CT_Y);

        break;

    default:
        break;
    }

    this->planeAo = ospecs.componentPlane(ColorComponent::CT_A);
    this->compAo = ospecs.component(ColorComponent::CT_A);

    this->xoOffset = this->compXo.offset();
    this->yoOffset = this->compYo.offset();
    this->zoOffset = this->compZo.offset();
    this->aoOffset = this->compAo.offset();

    this->xoShift = this->compXo.shift();
    this->yoShift = this->compYo.shift();
    this->zoShift = this->compZo.shift();
    this->aoShift = this->compAo.shift();

    this->maskXo = ~(this->compXo.max<uint64_t>() << this->compXo.shift());
    this->maskYo = ~(this->compYo.max<uint64_t>() << this->compYo.shift());
    this->maskZo = ~(this->compZo.max<uint64_t>() << this->compZo.shift());
    this->maskAo = ~(this->compAo.max<uint64_t>() << this->compAo.shift());

    this->alphaMode = ospecs.contains(ColorComponent::CT_A)?
                          AlphaMode_AO:
                          AlphaMode_O;
}

void AkVCam::FillParameters::configureFill(const VideoFormat &caps)
{
    this->allocateBuffers(caps);

    for (int x = 0; x < caps.width(); ++x) {
        this->dstWidthOffsetX[x] = (x >> this->compXo.widthDiv()) * this->compXo.step();
        this->dstWidthOffsetY[x] = (x >> this->compYo.widthDiv()) * this->compYo.step();
        this->dstWidthOffsetZ[x] = (x >> this->compZo.widthDiv()) * this->compZo.step();
        this->dstWidthOffsetA[x] = (x >> this->compAo.widthDiv()) * this->compAo.step();
    }

    this->width = caps.width();
    this->height = caps.height();
}

void AkVCam::FillParameters::reset()
{
    this->fillType = FillType_3;
    this->fillDataTypes = FillDataTypes_8;
    this->alphaMode = AlphaMode_AO;

    this->endianess = ENDIANNESS_BO;

    this->clearBuffers();

    this->width = 0;
    this->height = 0;

    this->planeXo = 0;
    this->planeYo = 0;
    this->planeZo = 0;
    this->planeAo = 0;

    this->compXo = {};
    this->compYo = {};
    this->compZo = {};
    this->compAo = {};

    this->xoOffset = 0;
    this->yoOffset = 0;
    this->zoOffset = 0;
    this->aoOffset = 0;

    this->xoShift = 0;
    this->yoShift = 0;
    this->zoShift = 0;
    this->aoShift = 0;

    this->maskXo = 0;
    this->maskYo = 0;
    this->maskZo = 0;
    this->maskAo = 0;
}
