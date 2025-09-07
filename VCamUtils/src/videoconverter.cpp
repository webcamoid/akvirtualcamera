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

#include <cstring>
#include <sstream>
#include <mutex>

#include "videoconverter.h"
#include "color.h"
#include "algorithm.h"
#include "fraction.h"
#include "rect.h"
#include "videoformat.h"
#include "videoformatspec.h"
#include "videoframe.h"

#define SCALE_EMULT 8

/*
 * NOTE: Using integer numbers is much faster but can overflow with high
 * resolution and depth frames.
 */

#if 0
using DlSumType = uint64_t;
#else
using DlSumType = double;
#endif

enum ConvertType
{
    ConvertType_Vector,
    ConvertType_1to1,
    ConvertType_1to3,
    ConvertType_3to1,
    ConvertType_3to3,
};

enum ConvertDataTypes
{
    ConvertDataTypes_8_8,
    ConvertDataTypes_8_16,
    ConvertDataTypes_8_32,
    ConvertDataTypes_16_8,
    ConvertDataTypes_16_16,
    ConvertDataTypes_16_32,
    ConvertDataTypes_32_8,
    ConvertDataTypes_32_16,
    ConvertDataTypes_32_32,
};

enum ConvertAlphaMode
{
    ConvertAlphaMode_AI_AO,
    ConvertAlphaMode_AI_O,
    ConvertAlphaMode_I_AO,
    ConvertAlphaMode_I_O,
};

enum ResizeMode
{
    ResizeMode_Keep,
    ResizeMode_Up,
    ResizeMode_Down,
};

namespace AkVCam
{
    class FrameConvertParameters
    {
        public:
            ColorConvert colorConvert;

            VideoFormat inputFormat;
            VideoFormat outputFormat;
            VideoFormat outputConvertFormat;
            VideoFrame outputFrame;
            Rect inputRect;
            AkVCam::ColorConvert::YuvColorSpace yuvColorSpace {AkVCam::ColorConvert::YuvColorSpace_ITUR_BT601};
            AkVCam::ColorConvert::YuvColorSpaceType yuvColorSpaceType {AkVCam::ColorConvert::YuvColorSpaceType_StudioSwing};
            AkVCam::VideoConverter::ScalingMode scalingMode {AkVCam::VideoConverter::ScalingMode_Fast};
            AkVCam::VideoConverter::AspectRatioMode aspectRatioMode {AkVCam::VideoConverter::AspectRatioMode_Ignore};
            ConvertType convertType {ConvertType_Vector};
            ConvertDataTypes convertDataTypes {ConvertDataTypes_8_8};
            ConvertAlphaMode alphaMode {ConvertAlphaMode_AI_AO};
            ResizeMode resizeMode {ResizeMode_Keep};
            bool fastConvertion {false};

            int fromEndian {ENDIANNESS_BO};
            int toEndian {ENDIANNESS_BO};

            int xmin {0};
            int ymin {0};
            int xmax {0};
            int ymax {0};

            int inputWidth {0};
            int inputWidth_1 {0};
            int inputHeight {0};

            int *srcWidth {nullptr};
            int *srcWidth_1 {nullptr};
            int *srcWidthOffsetX {nullptr};
            int *srcWidthOffsetY {nullptr};
            int *srcWidthOffsetZ {nullptr};
            int *srcWidthOffsetA {nullptr};
            int *srcHeight {nullptr};

            int *dlSrcWidthOffsetX {nullptr};
            int *dlSrcWidthOffsetY {nullptr};
            int *dlSrcWidthOffsetZ {nullptr};
            int *dlSrcWidthOffsetA {nullptr};

            int *srcWidthOffsetX_1 {nullptr};
            int *srcWidthOffsetY_1 {nullptr};
            int *srcWidthOffsetZ_1 {nullptr};
            int *srcWidthOffsetA_1 {nullptr};
            int *srcHeight_1 {nullptr};

            int *dstWidthOffsetX {nullptr};
            int *dstWidthOffsetY {nullptr};
            int *dstWidthOffsetZ {nullptr};
            int *dstWidthOffsetA {nullptr};

            size_t *srcHeightDlOffset {nullptr};
            size_t *srcHeightDlOffset_1 {nullptr};

            DlSumType *integralImageDataX {nullptr};
            DlSumType *integralImageDataY {nullptr};
            DlSumType *integralImageDataZ {nullptr};
            DlSumType *integralImageDataA {nullptr};

            int64_t *kx {nullptr};
            int64_t *ky {nullptr};
            DlSumType *kdl {nullptr};

            int planeXi {0};
            int planeYi {0};
            int planeZi {0};
            int planeAi {0};

            ColorComponent compXi;
            ColorComponent compYi;
            ColorComponent compZi;
            ColorComponent compAi;

            int planeXo {0};
            int planeYo {0};
            int planeZo {0};
            int planeAo {0};

            ColorComponent compXo;
            ColorComponent compYo;
            ColorComponent compZo;
            ColorComponent compAo;

            size_t xiOffset {0};
            size_t yiOffset {0};
            size_t ziOffset {0};
            size_t aiOffset {0};

            size_t xoOffset {0};
            size_t yoOffset {0};
            size_t zoOffset {0};
            size_t aoOffset {0};

            size_t xiShift {0};
            size_t yiShift {0};
            size_t ziShift {0};
            size_t aiShift {0};

            size_t xoShift {0};
            size_t yoShift {0};
            size_t zoShift {0};
            size_t aoShift {0};

            uint64_t maxXi {0};
            uint64_t maxYi {0};
            uint64_t maxZi {0};
            uint64_t maxAi {0};

            uint64_t maskXo {0};
            uint64_t maskYo {0};
            uint64_t maskZo {0};
            uint64_t maskAo {0};

            uint64_t alphaMask {0};

            FrameConvertParameters();
            FrameConvertParameters(const FrameConvertParameters &other);
            ~FrameConvertParameters();
            FrameConvertParameters &operator =(const FrameConvertParameters &other);
            inline void clearBuffers();
            inline void clearDlBuffers();
            inline void allocateBuffers(const VideoFormat &oformat);
            inline void allocateDlBuffers(const VideoFormat &iformat,
                                          const VideoFormat &oformat);
            void configure(const VideoFormat &iformat,
                           const VideoFormat &oformat,
                           ColorConvert &colorConvert,
                           AkVCam::ColorConvert::YuvColorSpace yuvColorSpace,
                           AkVCam::ColorConvert::YuvColorSpaceType yuvColorSpaceType);
            void configureScaling(const VideoFormat &iformat,
                                  const VideoFormat &oformat,
                                  const Rect &inputRect,
                                  AkVCam::VideoConverter::AspectRatioMode aspectRatioMode);
            void reset();
    };

    class VideoConverterPrivate
    {
        public:
            std::mutex m_mutex;
            VideoFormat m_outputFormat;
            FrameConvertParameters *m_fc {nullptr};
            size_t m_fcSize {0};
            int m_cacheIndex {0};
            AkVCam::ColorConvert::YuvColorSpace m_yuvColorSpace {AkVCam::ColorConvert::YuvColorSpace_ITUR_BT601};
            AkVCam::ColorConvert::YuvColorSpaceType m_yuvColorSpaceType {AkVCam::ColorConvert::YuvColorSpaceType_StudioSwing};
            AkVCam::VideoConverter::ScalingMode m_scalingMode {AkVCam::VideoConverter::ScalingMode_Fast};
            AkVCam::VideoConverter::AspectRatioMode m_aspectRatioMode {AkVCam::VideoConverter::AspectRatioMode_Ignore};
            Rect m_inputRect;

            /* Color blendig functions
             *
             * kx and ky must be in the range of [0, 2^N]
             */

            template <int N>
            inline void blend(int64_t a,
                              int64_t bx, int64_t by,
                              int64_t kx, int64_t ky,
                              int64_t *c) const
            {
                *c = (kx * (bx - a) + ky * (by - a) + (a << (N + 1))) >> (N + 1);
            }

            template <int N>
            inline void blend2(const int64_t *ax,
                               const int64_t *bx, const int64_t *by,
                               int64_t kx, int64_t ky,
                               int64_t *c) const
            {
                this->blend<N>(ax[0],
                               bx[0], by[0],
                               kx, ky,
                               c);
                this->blend<N>(ax[1],
                               bx[1], by[1],
                               kx, ky,
                               c + 1);
            }

            template <int N>
            inline void blend3(const int64_t *ax,
                               const int64_t *bx, const int64_t *by,
                               int64_t kx, int64_t ky,
                               int64_t *c) const
            {
                this->blend<N>(ax[0],
                               bx[0], by[0],
                               kx, ky,
                               c);
                this->blend<N>(ax[1],
                               bx[1], by[1],
                               kx, ky,
                               c + 1);
                this->blend<N>(ax[2],
                               bx[2], by[2],
                               kx, ky,
                               c + 2);
            }

            template <int N>
            inline void blend4(const int64_t *ax,
                               const int64_t *bx, const int64_t *by,
                               int64_t kx, int64_t ky,
                               int64_t *c) const
            {
                this->blend<N>(ax[0],
                               bx[0], by[0],
                               kx, ky,
                               c);
                this->blend<N>(ax[1],
                               bx[1], by[1],
                               kx, ky,
                               c + 1);
                this->blend<N>(ax[2],
                               bx[2], by[2],
                               kx, ky,
                               c + 2);
                this->blend<N>(ax[3],
                               bx[3], by[3],
                               kx, ky,
                               c + 3);
            }

            /* Component reading functions */

            template <typename InputType>
            inline void read1(const FrameConvertParameters &fc,
                              const uint8_t *src_line_x,
                              int x,
                              InputType *xi) const
            {
                int &xs_x = fc.srcWidthOffsetX[x];
                *xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);

                if (fc.fromEndian != ENDIANNESS_BO)
                    *xi = Algorithm::swapBytes(InputType(*xi));

                *xi = (*xi >> fc.xiShift) & fc.maxXi;
            }

            template <typename InputType>
            inline void read1A(const FrameConvertParameters &fc,
                               const uint8_t *src_line_x,
                               const uint8_t *src_line_a,
                               int x,
                               InputType *xi,
                               InputType *ai) const
            {
                int &xs_x = fc.srcWidthOffsetX[x];
                int &xs_a = fc.srcWidthOffsetA[x];

                auto xit = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                auto ait = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                if (fc.fromEndian != ENDIANNESS_BO) {
                    xit = Algorithm::swapBytes(InputType(xit));
                    ait = Algorithm::swapBytes(InputType(ait));
                }

                *xi = (xit >> fc.xiShift) & fc.maxXi;
                *ai = (ait >> fc.aiShift) & fc.maxAi;
            }

            template <typename InputType>
            inline void readDL1(const FrameConvertParameters &fc,
                                const DlSumType *src_line_x,
                                const DlSumType *src_line_x_1,
                                int x,
                                const DlSumType *kdl,
                                InputType *xi) const
            {
                auto &xs = fc.srcWidth[x];
                auto &xs_1 = fc.srcWidth_1[x];
                auto &k = kdl[x];

                *xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
            }

            template <typename InputType>
            inline void readDL1A(const FrameConvertParameters &fc,
                                 const DlSumType *src_line_x,
                                 const DlSumType *src_line_a,
                                 const DlSumType *src_line_x_1,
                                 const DlSumType *src_line_a_1,
                                 int x,
                                 const DlSumType *kdl,
                                 InputType *xi,
                                 InputType *ai) const
            {
                auto &xs = fc.srcWidth[x];
                auto &xs_1 = fc.srcWidth_1[x];
                auto &k = kdl[x];

                *xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                *ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;
            }

            template <typename InputType>
            inline void readUL1(const FrameConvertParameters &fc,
                                const uint8_t *src_line_x,
                                const uint8_t *src_line_x_1,
                                int x,
                                int64_t ky,
                                InputType *xi) const
            {
                int &xs_x = fc.srcWidthOffsetX[x];
                int &xs_x_1 = fc.srcWidthOffsetX_1[x];

                auto xi_ = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                auto xi_x = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                auto xi_y = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);

                if (fc.fromEndian != ENDIANNESS_BO) {
                    xi_ = Algorithm::swapBytes(InputType(xi_));
                    xi_x = Algorithm::swapBytes(InputType(xi_x));
                    xi_y = Algorithm::swapBytes(InputType(xi_y));
                }

                xi_ = (xi_ >> fc.xiShift) & fc.maxXi;
                xi_x = (xi_x >> fc.xiShift) & fc.maxXi;
                xi_y = (xi_y >> fc.xiShift) & fc.maxXi;

                int64_t xib = 0;
                this->blend<SCALE_EMULT>(xi_,
                                         xi_x, xi_y,
                                         fc.kx[x], ky,
                                         &xib);
                *xi = xib;
            }

            inline void readF8UL1(const FrameConvertParameters &fc,
                                  const uint8_t *src_line_x,
                                  const uint8_t *src_line_x_1,
                                  int x,
                                  int64_t ky,
                                  uint8_t *xi) const
            {
                int &xs_x = fc.srcWidthOffsetX[x];
                int &xs_x_1 = fc.srcWidthOffsetX_1[x];

                auto xi_ = src_line_x[xs_x];
                auto xi_x = src_line_x[xs_x_1];
                auto xi_y = src_line_x_1[xs_x];

                int64_t xib = 0;
                this->blend<SCALE_EMULT>(xi_,
                                         xi_x, xi_y,
                                         fc.kx[x], ky,
                                         &xib);
                *xi = uint8_t(xib);
            }

            template <typename InputType>
            inline void readUL1A(const FrameConvertParameters &fc,
                                 const uint8_t *src_line_x,
                                 const uint8_t *src_line_a,
                                 const uint8_t *src_line_x_1,
                                 const uint8_t *src_line_a_1,
                                 int x,
                                 int64_t ky,
                                 InputType *xi,
                                 InputType *ai) const
            {
                int &xs_x = fc.srcWidthOffsetX[x];
                int &xs_a = fc.srcWidthOffsetA[x];

                int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                int &xs_a_1 = fc.srcWidthOffsetA_1[x];

                int64_t xai[2];
                int64_t xai_x[2];
                int64_t xai_y[2];

                auto xai0 = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                auto xai1 = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                auto xai_x0 = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                auto xai_x1 = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                auto xai_y0 = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                auto xai_y1 = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                if (fc.fromEndian != ENDIANNESS_BO) {
                    xai0 = Algorithm::swapBytes(InputType(xai0));
                    xai1 = Algorithm::swapBytes(InputType(xai1));
                    xai_x0 = Algorithm::swapBytes(InputType(xai_x0));
                    xai_x1 = Algorithm::swapBytes(InputType(xai_x1));
                    xai_y0 = Algorithm::swapBytes(InputType(xai_y0));
                    xai_y1 = Algorithm::swapBytes(InputType(xai_y1));
                }

                xai[0] = (xai0 >> fc.xiShift) & fc.maxXi;
                xai[1] = (xai1 >> fc.aiShift) & fc.maxAi;
                xai_x[0] = (xai_x0 >> fc.xiShift) & fc.maxXi;
                xai_x[1] = (xai_x1 >> fc.aiShift) & fc.maxAi;
                xai_y[0] = (xai_y0 >> fc.xiShift) & fc.maxXi;
                xai_y[1] = (xai_y1 >> fc.aiShift) & fc.maxAi;

                int64_t xaib[2];
                this->blend2<SCALE_EMULT>(xai,
                                          xai_x, xai_y,
                                          fc.kx[x], ky,
                                          xaib);

                *xi = xaib[0];
                *ai = xaib[1];
            }

            inline void readF8UL1A(const FrameConvertParameters &fc,
                                   const uint8_t *src_line_x,
                                   const uint8_t *src_line_a,
                                   const uint8_t *src_line_x_1,
                                   const uint8_t *src_line_a_1,
                                   int x,
                                   int64_t ky,
                                   uint8_t *xi,
                                   uint8_t *ai) const
            {
                int &xs_x = fc.srcWidthOffsetX[x];
                int &xs_a = fc.srcWidthOffsetA[x];

                int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                int &xs_a_1 = fc.srcWidthOffsetA_1[x];

                int64_t xai[] = {
                    src_line_x[xs_x],
                    src_line_a[xs_a]
                };
                int64_t xai_x[] = {
                    src_line_x[xs_x_1],
                    src_line_a[xs_a_1]
                };
                int64_t xai_y[] = {
                    src_line_x_1[xs_x],
                    src_line_a_1[xs_a]
                };

                int64_t xaib[2];
                this->blend2<SCALE_EMULT>(xai,
                                          xai_x, xai_y,
                                          fc.kx[x], ky,
                                          xaib);

                *xi = uint8_t(xaib[0]);
                *ai = uint8_t(xaib[1]);
            }

            template <typename InputType>
            inline void read3(const FrameConvertParameters &fc,
                              const uint8_t *src_line_x,
                              const uint8_t *src_line_y,
                              const uint8_t *src_line_z,
                              int x,
                              InputType *xi,
                              InputType *yi,
                              InputType *zi) const
            {
                int &xs_x = fc.srcWidthOffsetX[x];
                int &xs_y = fc.srcWidthOffsetY[x];
                int &xs_z = fc.srcWidthOffsetZ[x];

                auto xit = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                auto yit = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                auto zit = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                if (fc.fromEndian != ENDIANNESS_BO) {
                    xit = Algorithm::swapBytes(InputType(xit));
                    yit = Algorithm::swapBytes(InputType(yit));
                    zit = Algorithm::swapBytes(InputType(zit));
                }

                *xi = (xit >> fc.xiShift) & fc.maxXi;
                *yi = (yit >> fc.yiShift) & fc.maxYi;
                *zi = (zit >> fc.ziShift) & fc.maxZi;
            }

            template <typename InputType>
            inline void read3A(const FrameConvertParameters &fc,
                               const uint8_t *src_line_x,
                               const uint8_t *src_line_y,
                               const uint8_t *src_line_z,
                               const uint8_t *src_line_a,
                               int x,
                               InputType *xi,
                               InputType *yi,
                               InputType *zi,
                               InputType *ai) const
            {
                int &xs_x = fc.srcWidthOffsetX[x];
                int &xs_y = fc.srcWidthOffsetY[x];
                int &xs_z = fc.srcWidthOffsetZ[x];
                int &xs_a = fc.srcWidthOffsetA[x];

                auto xit = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                auto yit = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                auto zit = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                auto ait = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                if (fc.fromEndian != ENDIANNESS_BO) {
                    xit = Algorithm::swapBytes(InputType(xit));
                    yit = Algorithm::swapBytes(InputType(yit));
                    zit = Algorithm::swapBytes(InputType(zit));
                    ait = Algorithm::swapBytes(InputType(ait));
                }

                *xi = (xit >> fc.xiShift) & fc.maxXi;
                *yi = (yit >> fc.yiShift) & fc.maxYi;
                *zi = (zit >> fc.ziShift) & fc.maxZi;
                *ai = (ait >> fc.aiShift) & fc.maxAi;
            }

            template <typename InputType>
            inline void readDL3(const FrameConvertParameters &fc,
                                const DlSumType *src_line_x,
                                const DlSumType *src_line_y,
                                const DlSumType *src_line_z,
                                const DlSumType *src_line_x_1,
                                const DlSumType *src_line_y_1,
                                const DlSumType *src_line_z_1,
                                int x,
                                const DlSumType *kdl,
                                InputType *xi,
                                InputType *yi,
                                InputType *zi) const
            {
                auto &xs = fc.srcWidth[x];
                auto &xs_1 = fc.srcWidth_1[x];
                auto &k = kdl[x];

                *xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                *yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                *zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
            }

            template <typename InputType>
            inline void readDL3A(const FrameConvertParameters &fc,
                                 const DlSumType *src_line_x,
                                 const DlSumType *src_line_y,
                                 const DlSumType *src_line_z,
                                 const DlSumType *src_line_a,
                                 const DlSumType *src_line_x_1,
                                 const DlSumType *src_line_y_1,
                                 const DlSumType *src_line_z_1,
                                 const DlSumType *src_line_a_1,
                                 int x,
                                 const DlSumType *kdl,
                                 InputType *xi,
                                 InputType *yi,
                                 InputType *zi,
                                 InputType *ai) const
            {
                auto &xs = fc.srcWidth[x];
                auto &xs_1 = fc.srcWidth_1[x];
                auto &k = kdl[x];

                *xi = (src_line_x[xs] + src_line_x_1[xs_1] - src_line_x[xs_1] - src_line_x_1[xs]) / k;
                *yi = (src_line_y[xs] + src_line_y_1[xs_1] - src_line_y[xs_1] - src_line_y_1[xs]) / k;
                *zi = (src_line_z[xs] + src_line_z_1[xs_1] - src_line_z[xs_1] - src_line_z_1[xs]) / k;
                *ai = (src_line_a[xs] + src_line_a_1[xs_1] - src_line_a[xs_1] - src_line_a_1[xs]) / k;
            }

            template <typename InputType>
            inline void readUL3(const FrameConvertParameters &fc,
                                const uint8_t *src_line_x,
                                const uint8_t *src_line_y,
                                const uint8_t *src_line_z,
                                const uint8_t *src_line_x_1,
                                const uint8_t *src_line_y_1,
                                const uint8_t *src_line_z_1,
                                int x,
                                int64_t ky,
                                InputType *xi,
                                InputType *yi,
                                InputType *zi) const
            {
                int &xs_x = fc.srcWidthOffsetX[x];
                int &xs_y = fc.srcWidthOffsetY[x];
                int &xs_z = fc.srcWidthOffsetZ[x];

                int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                int &xs_z_1 = fc.srcWidthOffsetZ_1[x];

                int64_t xyzi[3];
                int64_t xyzi_x[3];
                int64_t xyzi_y[3];

                auto xyzi0 = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                auto xyzi1 = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                auto xyzi2 = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                auto xyzi_x0 = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                auto xyzi_x1 = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                auto xyzi_x2 = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                auto xyzi_y0 = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                auto xyzi_y1 = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                auto xyzi_y2 = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);

                if (fc.fromEndian != ENDIANNESS_BO) {
                    xyzi0 = Algorithm::swapBytes(InputType(xyzi0));
                    xyzi1 = Algorithm::swapBytes(InputType(xyzi1));
                    xyzi2 = Algorithm::swapBytes(InputType(xyzi2));
                    xyzi_x0 = Algorithm::swapBytes(InputType(xyzi_x0));
                    xyzi_x1 = Algorithm::swapBytes(InputType(xyzi_x1));
                    xyzi_x2 = Algorithm::swapBytes(InputType(xyzi_x2));
                    xyzi_y0 = Algorithm::swapBytes(InputType(xyzi_y0));
                    xyzi_y1 = Algorithm::swapBytes(InputType(xyzi_y1));
                    xyzi_y2 = Algorithm::swapBytes(InputType(xyzi_y2));
                }

                xyzi[0] = (xyzi0 >> fc.xiShift) & fc.maxXi;
                xyzi[1] = (xyzi1 >> fc.yiShift) & fc.maxYi;
                xyzi[2] = (xyzi2 >> fc.ziShift) & fc.maxZi;
                xyzi_x[0] = (xyzi_x0 >> fc.xiShift) & fc.maxXi;
                xyzi_x[1] = (xyzi_x1 >> fc.yiShift) & fc.maxYi;
                xyzi_x[2] = (xyzi_x2 >> fc.ziShift) & fc.maxZi;
                xyzi_y[0] = (xyzi_y0 >> fc.xiShift) & fc.maxXi;
                xyzi_y[1] = (xyzi_y1 >> fc.yiShift) & fc.maxYi;
                xyzi_y[2] = (xyzi_y2 >> fc.ziShift) & fc.maxZi;

                int64_t xyzib[3];
                this->blend3<SCALE_EMULT>(xyzi,
                                          xyzi_x, xyzi_y,
                                          fc.kx[x], ky,
                                          xyzib);

                *xi = xyzib[0];
                *yi = xyzib[1];
                *zi = xyzib[2];
            }

            inline void readF8UL3(const FrameConvertParameters &fc,
                                  const uint8_t *src_line_x,
                                  const uint8_t *src_line_y,
                                  const uint8_t *src_line_z,
                                  const uint8_t *src_line_x_1,
                                  const uint8_t *src_line_y_1,
                                  const uint8_t *src_line_z_1,
                                  int x,
                                  int64_t ky,
                                  uint8_t *xi,
                                  uint8_t *yi,
                                  uint8_t *zi) const
            {
                int &xs_x = fc.srcWidthOffsetX[x];
                int &xs_y = fc.srcWidthOffsetY[x];
                int &xs_z = fc.srcWidthOffsetZ[x];

                int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                int &xs_z_1 = fc.srcWidthOffsetZ_1[x];

                int64_t xyzi[] = {
                    src_line_x[xs_x],
                    src_line_y[xs_y],
                    src_line_z[xs_z]
                };
                int64_t xyzi_x[] = {
                    src_line_x[xs_x_1],
                    src_line_y[xs_y_1],
                    src_line_z[xs_z_1]
                };
                int64_t xyzi_y[] = {
                    src_line_x_1[xs_x],
                    src_line_y_1[xs_y],
                    src_line_z_1[xs_z]
                };

                int64_t xyzib[3];
                this->blend3<SCALE_EMULT>(xyzi,
                                          xyzi_x, xyzi_y,
                                          fc.kx[x], ky,
                                          xyzib);

                *xi = uint8_t(xyzib[0]);
                *yi = uint8_t(xyzib[1]);
                *zi = uint8_t(xyzib[2]);
            }

            template <typename InputType>
            inline void readUL3A(const FrameConvertParameters &fc,
                                 const uint8_t *src_line_x,
                                 const uint8_t *src_line_y,
                                 const uint8_t *src_line_z,
                                 const uint8_t *src_line_a,
                                 const uint8_t *src_line_x_1,
                                 const uint8_t *src_line_y_1,
                                 const uint8_t *src_line_z_1,
                                 const uint8_t *src_line_a_1,
                                 int x,
                                 int64_t ky,
                                 InputType *xi,
                                 InputType *yi,
                                 InputType *zi,
                                 InputType *ai) const
            {
                int &xs_x = fc.srcWidthOffsetX[x];
                int &xs_y = fc.srcWidthOffsetY[x];
                int &xs_z = fc.srcWidthOffsetZ[x];
                int &xs_a = fc.srcWidthOffsetA[x];

                int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                int &xs_z_1 = fc.srcWidthOffsetZ_1[x];
                int &xs_a_1 = fc.srcWidthOffsetA_1[x];

                int64_t xyzai[4];
                int64_t xyzai_x[4];
                int64_t xyzai_y[4];

                auto xyzai0 = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                auto xyzai1 = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                auto xyzai2 = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                auto xyzai3 = *reinterpret_cast<const InputType *>(src_line_a + xs_a);
                auto xyzai_x0 = *reinterpret_cast<const InputType *>(src_line_x + xs_x_1);
                auto xyzai_x1 = *reinterpret_cast<const InputType *>(src_line_y + xs_y_1);
                auto xyzai_x2 = *reinterpret_cast<const InputType *>(src_line_z + xs_z_1);
                auto xyzai_x3 = *reinterpret_cast<const InputType *>(src_line_a + xs_a_1);
                auto xyzai_y0 = *reinterpret_cast<const InputType *>(src_line_x_1 + xs_x);
                auto xyzai_y1 = *reinterpret_cast<const InputType *>(src_line_y_1 + xs_y);
                auto xyzai_y2 = *reinterpret_cast<const InputType *>(src_line_z_1 + xs_z);
                auto xyzai_y3 = *reinterpret_cast<const InputType *>(src_line_a_1 + xs_a);

                if (fc.fromEndian != ENDIANNESS_BO) {
                    xyzai0 = Algorithm::swapBytes(InputType(xyzai0));
                    xyzai1 = Algorithm::swapBytes(InputType(xyzai1));
                    xyzai2 = Algorithm::swapBytes(InputType(xyzai2));
                    xyzai3 = Algorithm::swapBytes(InputType(xyzai3));
                    xyzai_x0 = Algorithm::swapBytes(InputType(xyzai_x0));
                    xyzai_x1 = Algorithm::swapBytes(InputType(xyzai_x1));
                    xyzai_x2 = Algorithm::swapBytes(InputType(xyzai_x2));
                    xyzai_x3 = Algorithm::swapBytes(InputType(xyzai_x3));
                    xyzai_y0 = Algorithm::swapBytes(InputType(xyzai_y0));
                    xyzai_y1 = Algorithm::swapBytes(InputType(xyzai_y1));
                    xyzai_y2 = Algorithm::swapBytes(InputType(xyzai_y2));
                    xyzai_y3 = Algorithm::swapBytes(InputType(xyzai_y3));
                }

                xyzai[0] = (xyzai0 >> fc.xiShift) & fc.maxXi;
                xyzai[1] = (xyzai1 >> fc.yiShift) & fc.maxYi;
                xyzai[2] = (xyzai2 >> fc.ziShift) & fc.maxZi;
                xyzai[3] = (xyzai3 >> fc.aiShift) & fc.maxAi;
                xyzai_x[0] = (xyzai_x0 >> fc.xiShift) & fc.maxXi;
                xyzai_x[1] = (xyzai_x1 >> fc.yiShift) & fc.maxYi;
                xyzai_x[2] = (xyzai_x2 >> fc.ziShift) & fc.maxZi;
                xyzai_x[3] = (xyzai_x3 >> fc.aiShift) & fc.maxAi;
                xyzai_y[0] = (xyzai_y0 >> fc.xiShift) & fc.maxXi;
                xyzai_y[1] = (xyzai_y1 >> fc.yiShift) & fc.maxYi;
                xyzai_y[2] = (xyzai_y2 >> fc.ziShift) & fc.maxZi;
                xyzai_y[3] = (xyzai_y3 >> fc.aiShift) & fc.maxAi;

                int64_t xyzaib[4];
                this->blend4<SCALE_EMULT>(xyzai,
                                          xyzai_x, xyzai_y,
                                          fc.kx[x], ky,
                                          xyzaib);

                *xi = xyzaib[0];
                *yi = xyzaib[1];
                *zi = xyzaib[2];
                *ai = xyzaib[3];
            }

            inline void readF8UL3A(const FrameConvertParameters &fc,
                                   const uint8_t *src_line_x,
                                   const uint8_t *src_line_y,
                                   const uint8_t *src_line_z,
                                   const uint8_t *src_line_a,
                                   const uint8_t *src_line_x_1,
                                   const uint8_t *src_line_y_1,
                                   const uint8_t *src_line_z_1,
                                   const uint8_t *src_line_a_1,
                                   int x,
                                   int64_t ky,
                                   uint8_t *xi,
                                   uint8_t *yi,
                                   uint8_t *zi,
                                   uint8_t *ai) const
            {
                int &xs_x = fc.srcWidthOffsetX[x];
                int &xs_y = fc.srcWidthOffsetY[x];
                int &xs_z = fc.srcWidthOffsetZ[x];
                int &xs_a = fc.srcWidthOffsetA[x];

                int &xs_x_1 = fc.srcWidthOffsetX_1[x];
                int &xs_y_1 = fc.srcWidthOffsetY_1[x];
                int &xs_z_1 = fc.srcWidthOffsetZ_1[x];
                int &xs_a_1 = fc.srcWidthOffsetA_1[x];

                int64_t xyzai[] = {
                    src_line_x[xs_x],
                    src_line_y[xs_y],
                    src_line_z[xs_z],
                    src_line_a[xs_a]
                };
                int64_t xyzai_x[] = {
                    src_line_x[xs_x_1],
                    src_line_y[xs_y_1],
                    src_line_z[xs_z_1],
                    src_line_a[xs_a_1]
                };
                int64_t xyzai_y[] = {
                    src_line_x_1[xs_x],
                    src_line_y_1[xs_y],
                    src_line_z_1[xs_z],
                    src_line_a_1[xs_a]
                };

                int64_t xyzaib[4];
                this->blend4<SCALE_EMULT>(xyzai,
                                          xyzai_x, xyzai_y,
                                          fc.kx[x], ky,
                                          xyzaib);

                *xi = uint8_t(xyzaib[0]);
                *yi = uint8_t(xyzaib[1]);
                *zi = uint8_t(xyzaib[2]);
                *ai = uint8_t(xyzaib[3]);
            }

            /* Component writing functions */

            template <typename OutputType>
            inline void write1(const FrameConvertParameters &fc,
                               uint8_t *dst_line_x,
                               int x,
                               OutputType xo) const
            {
                int &xd_x = fc.dstWidthOffsetX[x];
                auto xo_ = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                *xo_ = (*xo_ & OutputType(fc.maskXo)) | (OutputType(xo) << fc.xoShift);
            }

            template <typename OutputType>
            inline void write1A(const FrameConvertParameters &fc,
                                uint8_t *dst_line_x,
                                uint8_t *dst_line_a,
                                int x,
                                OutputType xo,
                                OutputType ao) const
            {
                int &xd_x = fc.dstWidthOffsetX[x];
                int &xd_a = fc.dstWidthOffsetA[x];

                auto xo_ = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                auto ao_ = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                *xo_ = (*xo_ & OutputType(fc.maskXo)) | (OutputType(xo) << fc.xoShift);
                *ao_ = (*ao_ & OutputType(fc.maskAo)) | (OutputType(ao) << fc.aoShift);
            }

            template <typename OutputType>
            inline void write1A(const FrameConvertParameters &fc,
                                uint8_t *dst_line_x,
                                uint8_t *dst_line_a,
                                int x,
                                OutputType xo) const
            {
                int &xd_x = fc.dstWidthOffsetX[x];
                int &xd_a = fc.dstWidthOffsetA[x];

                auto xo_ = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                auto ao_ = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                *xo_ = (*xo_ & OutputType(fc.maskXo)) | (OutputType(xo) << fc.xoShift);
                *ao_ = *ao_ | OutputType(fc.alphaMask);
            }

            template <typename OutputType>
            inline void write3(const FrameConvertParameters &fc,
                               uint8_t *dst_line_x,
                               uint8_t *dst_line_y,
                               uint8_t *dst_line_z,
                               int x,
                               OutputType xo,
                               OutputType yo,
                               OutputType zo) const
            {
                int &xd_x = fc.dstWidthOffsetX[x];
                int &xd_y = fc.dstWidthOffsetY[x];
                int &xd_z = fc.dstWidthOffsetZ[x];

                auto xo_ = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                auto yo_ = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                auto zo_ = reinterpret_cast<OutputType *>(dst_line_z + xd_z);

                *xo_ = (*xo_ & OutputType(fc.maskXo)) | (OutputType(xo) << fc.xoShift);
                *yo_ = (*yo_ & OutputType(fc.maskYo)) | (OutputType(yo) << fc.yoShift);
                *zo_ = (*zo_ & OutputType(fc.maskZo)) | (OutputType(zo) << fc.zoShift);
            }

            template <typename OutputType>
            inline void write3A(const FrameConvertParameters &fc,
                                uint8_t *dst_line_x,
                                uint8_t *dst_line_y,
                                uint8_t *dst_line_z,
                                uint8_t *dst_line_a,
                                int x,
                                OutputType xo,
                                OutputType yo,
                                OutputType zo,
                                OutputType ao) const
            {
                int &xd_x = fc.dstWidthOffsetX[x];
                int &xd_y = fc.dstWidthOffsetY[x];
                int &xd_z = fc.dstWidthOffsetZ[x];
                int &xd_a = fc.dstWidthOffsetA[x];

                auto xo_ = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                auto yo_ = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                auto zo_ = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                auto ao_ = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                *xo_ = (*xo_ & OutputType(fc.maskXo)) | (OutputType(xo) << fc.xoShift);
                *yo_ = (*yo_ & OutputType(fc.maskYo)) | (OutputType(yo) << fc.yoShift);
                *zo_ = (*zo_ & OutputType(fc.maskZo)) | (OutputType(zo) << fc.zoShift);
                *ao_ = (*ao_ & OutputType(fc.maskAo)) | (OutputType(ao) << fc.aoShift);
            }

            template <typename OutputType>
            inline void write3A(const FrameConvertParameters &fc,
                                uint8_t *dst_line_x,
                                uint8_t *dst_line_y,
                                uint8_t *dst_line_z,
                                uint8_t *dst_line_a,
                                int x,
                                OutputType xo,
                                OutputType yo,
                                OutputType zo) const
            {
                int &xd_x = fc.dstWidthOffsetX[x];
                int &xd_y = fc.dstWidthOffsetY[x];
                int &xd_z = fc.dstWidthOffsetZ[x];
                int &xd_a = fc.dstWidthOffsetA[x];

                auto xo_ = reinterpret_cast<OutputType *>(dst_line_x + xd_x);
                auto yo_ = reinterpret_cast<OutputType *>(dst_line_y + xd_y);
                auto zo_ = reinterpret_cast<OutputType *>(dst_line_z + xd_z);
                auto ao_ = reinterpret_cast<OutputType *>(dst_line_a + xd_a);

                *xo_ = (*xo_ & OutputType(fc.maskXo)) | (OutputType(xo) << fc.xoShift);
                *yo_ = (*yo_ & OutputType(fc.maskYo)) | (OutputType(yo) << fc.yoShift);
                *zo_ = (*zo_ & OutputType(fc.maskZo)) | (OutputType(zo) << fc.zoShift);
                *ao_ = *ao_ | OutputType(fc.alphaMask);
            }

            /* Integral image functions */

            template <typename InputType>
            inline void integralImage1(const FrameConvertParameters &fc,
                                       const VideoFrame &src) const
            {
                auto dst_line_x = fc.integralImageDataX;
                auto dst_line_x_1 = dst_line_x + fc.inputWidth_1;

                for (int y = 0; y < fc.inputHeight; ++y) {
                    auto src_line_x = src.constLine(fc.planeXi, y) + fc.xiOffset;

                    // Reset current line summation.

                    DlSumType sumX = 0;

                    for (int x = 0; x < fc.inputWidth; ++x) {
                        int &xs_x = fc.dlSrcWidthOffsetX[x];
                        auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);

                        if (fc.fromEndian != ENDIANNESS_BO)
                            xi = Algorithm::swapBytes(InputType(xi));

                        // Accumulate pixels in current line.

                        sumX += (xi >> fc.xiShift) & fc.maxXi;

                        // Accumulate current line and previous line.

                        int x_1 = x + 1;
                        dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                    }

                    dst_line_x += fc.inputWidth_1;
                    dst_line_x_1 += fc.inputWidth_1;
                }
            }

            template <typename InputType>
            inline void integralImage1A(const FrameConvertParameters &fc,
                                        const VideoFrame &src) const
            {
                auto dst_line_x = fc.integralImageDataX;
                auto dst_line_a = fc.integralImageDataA;
                auto dst_line_x_1 = dst_line_x + fc.inputWidth_1;
                auto dst_line_a_1 = dst_line_a + fc.inputWidth_1;

                for (int y = 0; y < fc.inputHeight; ++y) {
                    auto src_line_x = src.constLine(fc.planeXi, y) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, y) + fc.aiOffset;

                    // Reset current line summation.

                    DlSumType sumX = 0;
                    DlSumType sumA = 0;

                    for (int x = 0; x < fc.inputWidth; ++x) {
                        int &xs_x = fc.dlSrcWidthOffsetX[x];
                        int &xs_a = fc.dlSrcWidthOffsetA[x];

                        auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                        auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                        if (fc.fromEndian != ENDIANNESS_BO) {
                            xi = Algorithm::swapBytes(InputType(xi));
                            ai = Algorithm::swapBytes(InputType(ai));
                        }

                        // Accumulate pixels in current line.

                        sumX += (xi >> fc.xiShift) & fc.maxXi;
                        sumA += (ai >> fc.aiShift) & fc.maxAi;

                        // Accumulate current line and previous line.

                        int x_1 = x + 1;
                        dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                        dst_line_a_1[x_1] = sumA + dst_line_a[x_1];
                    }

                    dst_line_x += fc.inputWidth_1;
                    dst_line_a += fc.inputWidth_1;
                    dst_line_x_1 += fc.inputWidth_1;
                    dst_line_a_1 += fc.inputWidth_1;
                }
            }

            template <typename InputType>
            inline void integralImage3(const FrameConvertParameters &fc,
                                       const VideoFrame &src) const
            {
                auto dst_line_x = fc.integralImageDataX;
                auto dst_line_y = fc.integralImageDataY;
                auto dst_line_z = fc.integralImageDataZ;
                auto dst_line_x_1 = dst_line_x + fc.inputWidth_1;
                auto dst_line_y_1 = dst_line_y + fc.inputWidth_1;
                auto dst_line_z_1 = dst_line_z + fc.inputWidth_1;

                for (int y = 0; y < fc.inputHeight; ++y) {
                    auto src_line_x = src.constLine(fc.planeXi, y) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, y) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, y) + fc.ziOffset;

                    // Reset current line summation.

                    DlSumType sumX = 0;
                    DlSumType sumY = 0;
                    DlSumType sumZ = 0;

                    for (int x = 0; x < fc.inputWidth; ++x) {
                        int &xs_x = fc.dlSrcWidthOffsetX[x];
                        int &xs_y = fc.dlSrcWidthOffsetY[x];
                        int &xs_z = fc.dlSrcWidthOffsetZ[x];

                        auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                        auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                        auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);

                        if (fc.fromEndian != ENDIANNESS_BO) {
                            xi = Algorithm::swapBytes(InputType(xi));
                            yi = Algorithm::swapBytes(InputType(yi));
                            zi = Algorithm::swapBytes(InputType(zi));
                        }

                        // Accumulate pixels in current line.

                        sumX += (xi >> fc.xiShift) & fc.maxXi;
                        sumY += (yi >> fc.yiShift) & fc.maxYi;
                        sumZ += (zi >> fc.ziShift) & fc.maxZi;

                        // Accumulate current line and previous line.

                        int x_1 = x + 1;
                        dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                        dst_line_y_1[x_1] = sumY + dst_line_y[x_1];
                        dst_line_z_1[x_1] = sumZ + dst_line_z[x_1];
                    }

                    dst_line_x += fc.inputWidth_1;
                    dst_line_y += fc.inputWidth_1;
                    dst_line_z += fc.inputWidth_1;
                    dst_line_x_1 += fc.inputWidth_1;
                    dst_line_y_1 += fc.inputWidth_1;
                    dst_line_z_1 += fc.inputWidth_1;
                }
            }

            template <typename InputType>
            inline void integralImage3A(const FrameConvertParameters &fc,
                                        const VideoFrame &src) const
            {
                auto dst_line_x = fc.integralImageDataX;
                auto dst_line_y = fc.integralImageDataY;
                auto dst_line_z = fc.integralImageDataZ;
                auto dst_line_a = fc.integralImageDataA;
                auto dst_line_x_1 = dst_line_x + fc.inputWidth_1;
                auto dst_line_y_1 = dst_line_y + fc.inputWidth_1;
                auto dst_line_z_1 = dst_line_z + fc.inputWidth_1;
                auto dst_line_a_1 = dst_line_a + fc.inputWidth_1;

                for (int y = 0; y < fc.inputHeight; ++y) {
                    auto src_line_x = src.constLine(fc.planeXi, y) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, y) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, y) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, y) + fc.aiOffset;

                    // Reset current line summation.

                    DlSumType sumX = 0;
                    DlSumType sumY = 0;
                    DlSumType sumZ = 0;
                    DlSumType sumA = 0;

                    for (int x = 0; x < fc.inputWidth; ++x) {
                        int &xs_x = fc.dlSrcWidthOffsetX[x];
                        int &xs_y = fc.dlSrcWidthOffsetY[x];
                        int &xs_z = fc.dlSrcWidthOffsetZ[x];
                        int &xs_a = fc.dlSrcWidthOffsetA[x];

                        auto xi = *reinterpret_cast<const InputType *>(src_line_x + xs_x);
                        auto yi = *reinterpret_cast<const InputType *>(src_line_y + xs_y);
                        auto zi = *reinterpret_cast<const InputType *>(src_line_z + xs_z);
                        auto ai = *reinterpret_cast<const InputType *>(src_line_a + xs_a);

                        if (fc.fromEndian != ENDIANNESS_BO) {
                            xi = Algorithm::swapBytes(InputType(xi));
                            yi = Algorithm::swapBytes(InputType(yi));
                            zi = Algorithm::swapBytes(InputType(zi));
                            ai = Algorithm::swapBytes(InputType(ai));
                        }

                        // Accumulate pixels in current line.

                        sumX += (xi >> fc.xiShift) & fc.maxXi;
                        sumY += (yi >> fc.yiShift) & fc.maxYi;
                        sumZ += (zi >> fc.ziShift) & fc.maxZi;
                        sumA += (ai >> fc.aiShift) & fc.maxAi;

                        // Accumulate current line and previous line.

                        int x_1 = x + 1;
                        dst_line_x_1[x_1] = sumX + dst_line_x[x_1];
                        dst_line_y_1[x_1] = sumY + dst_line_y[x_1];
                        dst_line_z_1[x_1] = sumZ + dst_line_z[x_1];
                        dst_line_a_1[x_1] = sumA + dst_line_a[x_1];
                    }

                    dst_line_x += fc.inputWidth_1;
                    dst_line_y += fc.inputWidth_1;
                    dst_line_z += fc.inputWidth_1;
                    dst_line_a += fc.inputWidth_1;
                    dst_line_x_1 += fc.inputWidth_1;
                    dst_line_y_1 += fc.inputWidth_1;
                    dst_line_z_1 += fc.inputWidth_1;
                    dst_line_a_1 += fc.inputWidth_1;
                }
            }

            /* Fast conversion functions */

            // Conversion functions for 3 components to 3 components formats

            template <typename InputType, typename OutputType>
            void convert3to3(const FrameConvertParameters &fc,
                             const VideoFrame &src,
                             VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->read3(fc,
                                    src_line_x,
                                    src_line_y,
                                    src_line_z,
                                    x,
                                    &xi,
                                    &yi,
                                    &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }
                }
            }

            void convertFast8bits3to3(const FrameConvertParameters &fc,
                                      const VideoFrame &src,
                                      VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    int x = fc.xmin;

                    for (int i = x; i < fc.xmax; ++i) {
                        auto xi = src_line_x[fc.srcWidthOffsetX[i]];
                        auto yi = src_line_y[fc.srcWidthOffsetY[i]];
                        auto zi = src_line_z[fc.srcWidthOffsetZ[i]];

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[i]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[i]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[i]] = uint8_t(zo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convert3to3A(const FrameConvertParameters &fc,
                              const VideoFrame &src,
                              VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->read3(fc,
                                    src_line_x,
                                    src_line_y,
                                    src_line_z,
                                    x,
                                    &xi,
                                    &yi,
                                    &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo));
                    }
                }
            }

            void convertFast8bits3to3A(const FrameConvertParameters &fc,
                                       const VideoFrame &src,
                                       VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    int x = fc.xmin;

                    for (int i = x; i < fc.xmax; ++i) {
                        auto xi = src_line_x[fc.srcWidthOffsetX[i]];
                        auto yi = src_line_y[fc.srcWidthOffsetY[i]];
                        auto zi = src_line_z[fc.srcWidthOffsetZ[i]];

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[i]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[i]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[i]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[i]] = 0xff;
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convert3Ato3(const FrameConvertParameters &fc,
                              const VideoFrame &src,
                              VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->read3A(fc,
                                     src_line_x,
                                     src_line_y,
                                     src_line_z,
                                     src_line_a,
                                     x,
                                     &xi,
                                     &yi,
                                     &zi,
                                     &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);
                        fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }
                }
            }

            void convertFast8bits3Ato3(const FrameConvertParameters &fc,
                                       const VideoFrame &src,
                                       VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    int x = fc.xmin;

                    for (int i = x; i < fc.xmax; ++i) {
                        auto xi = src_line_x[fc.srcWidthOffsetX[i]];
                        auto yi = src_line_y[fc.srcWidthOffsetY[i]];
                        auto zi = src_line_z[fc.srcWidthOffsetZ[i]];
                        auto ai = src_line_a[fc.srcWidthOffsetA[i]];

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);
                        fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[i]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[i]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[i]] = uint8_t(zo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convert3Ato3A(const FrameConvertParameters &fc,
                               const VideoFrame &src,
                               VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->read3A(fc,
                                     src_line_x,
                                     src_line_y,
                                     src_line_z,
                                     src_line_a,
                                     x,
                                     &xi,
                                     &yi,
                                     &zi,
                                     &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo),
                                      OutputType(ai));
                    }
                }
            }

            void convertFast8bits3Ato3A(const FrameConvertParameters &fc,
                                        const VideoFrame &src,
                                        VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    int x = fc.xmin;

                    for (int i = x; i < fc.xmax; ++i) {
                        auto xi = src_line_x[fc.srcWidthOffsetX[i]];
                        auto yi = src_line_y[fc.srcWidthOffsetY[i]];
                        auto zi = src_line_z[fc.srcWidthOffsetZ[i]];
                        auto ai = src_line_a[fc.srcWidthOffsetA[i]];

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi, yi, zi, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[i]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[i]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[i]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[i]] = ai;
                    }
                }
            }

            // Conversion functions for 3 components to 3 components formats
            // (same color space)

            template <typename InputType, typename OutputType>
            void convertV3to3(const FrameConvertParameters &fc,
                              const VideoFrame &src,
                              VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->read3(fc,
                                    src_line_x,
                                    src_line_y,
                                    src_line_z,
                                    x,
                                    &xi,
                                    &yi,
                                    &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi, yi, zi, &xo, &yo, &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }
                }
            }

            void convertFast8bitsV3to3(const FrameConvertParameters &fc,
                                       const VideoFrame &src,
                                       VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        dst_line_x[fc.dstWidthOffsetX[x]] = src_line_x[fc.srcWidthOffsetX[x]];
                        dst_line_y[fc.dstWidthOffsetY[x]] = src_line_y[fc.srcWidthOffsetY[x]];
                        dst_line_z[fc.dstWidthOffsetZ[x]] = src_line_z[fc.srcWidthOffsetZ[x]];
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertV3to3A(const FrameConvertParameters &fc,
                               const VideoFrame &src,
                               VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->read3(fc,
                                    src_line_x,
                                    src_line_y,
                                    src_line_z,
                                    x,
                                    &xi,
                                    &yi,
                                    &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi, yi, zi, &xo, &yo, &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo));
                    }
                }
            }

            void convertFast8bitsV3to3A(const FrameConvertParameters &fc,
                                        const VideoFrame &src,
                                        VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        dst_line_x[fc.dstWidthOffsetX[x]] = src_line_x[fc.srcWidthOffsetX[x]];
                        dst_line_y[fc.dstWidthOffsetY[x]] = src_line_y[fc.srcWidthOffsetY[x]];
                        dst_line_z[fc.dstWidthOffsetZ[x]] = src_line_z[fc.srcWidthOffsetZ[x]];
                        dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertV3Ato3(const FrameConvertParameters &fc,
                               const VideoFrame &src,
                               VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->read3A(fc,
                                     src_line_x,
                                     src_line_y,
                                     src_line_z,
                                     src_line_a,
                                     x,
                                     &xi,
                                     &yi,
                                     &zi,
                                     &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi, yi, zi, &xo, &yo, &zo);
                        fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }
                }
            }

            void convertFast8bitsV3Ato3(const FrameConvertParameters &fc,
                                        const VideoFrame &src,
                                        VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    int x = fc.xmin;

                    for (int i = x; i < fc.xmax; ++i) {
                        int64_t xi = src_line_x[fc.srcWidthOffsetX[i]];
                        int64_t yi = src_line_y[fc.srcWidthOffsetY[i]];
                        int64_t zi = src_line_z[fc.srcWidthOffsetZ[i]];
                        auto &ai = src_line_a[fc.srcWidthOffsetA[i]];

                        fc.colorConvert.applyAlpha(ai, &xi, &yi, &zi);

                        dst_line_x[fc.dstWidthOffsetX[i]] = uint8_t(xi);
                        dst_line_y[fc.dstWidthOffsetY[i]] = uint8_t(yi);
                        dst_line_z[fc.dstWidthOffsetZ[i]] = uint8_t(zi);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertV3Ato3A(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->read3A(fc,
                                     src_line_x,
                                     src_line_y,
                                     src_line_z,
                                     src_line_a,
                                     x,
                                     &xi,
                                     &yi,
                                     &zi,
                                     &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi, yi, zi, &xo, &yo, &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo),
                                      OutputType(ai));
                    }
                }
            }

            void convertFast8bitsV3Ato3A(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        dst_line_x[fc.dstWidthOffsetX[x]] = src_line_x[fc.srcWidthOffsetX[x]];
                        dst_line_y[fc.dstWidthOffsetY[x]] = src_line_y[fc.srcWidthOffsetY[x]];
                        dst_line_z[fc.dstWidthOffsetZ[x]] = src_line_z[fc.srcWidthOffsetZ[x]];
                        dst_line_a[fc.dstWidthOffsetA[x]] = src_line_a[fc.srcWidthOffsetA[x]];
                    }
                }
            }

            // Conversion functions for 3 components to 1 components formats

            template <typename InputType, typename OutputType>
            void convert3to1(const FrameConvertParameters &fc,
                             const VideoFrame &src,
                             VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->read3(fc,
                                    src_line_x,
                                    src_line_y,
                                    src_line_z,
                                    x,
                                    &xi,
                                    &yi,
                                    &zi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                        this->write1(fc,
                                     dst_line_x,
                                     x,
                                     OutputType(xo));
                    }
                }
            }

            void convertFast8bits3to1(const FrameConvertParameters &fc,
                                      const VideoFrame &src,
                                      VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    int x = fc.xmin;

                    for (int i = x; i < fc.xmax; ++i) {
                        auto xi = src_line_x[fc.srcWidthOffsetX[i]];
                        auto yi = src_line_y[fc.srcWidthOffsetY[i]];
                        auto zi = src_line_z[fc.srcWidthOffsetZ[i]];

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                        dst_line_x[fc.dstWidthOffsetX[i]] = uint8_t(xo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convert3to1A(const FrameConvertParameters &fc,
                              const VideoFrame &src,
                              VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->read3(fc,
                                    src_line_x,
                                    src_line_y,
                                    src_line_z,
                                    x,
                                    &xi,
                                    &yi,
                                    &zi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                        this->write1A(fc,
                                      dst_line_x,
                                      dst_line_a,
                                      x,
                                      OutputType(xo));
                    }
                }
            }

            void convertFast8bits3to1A(const FrameConvertParameters &fc,
                                       const VideoFrame &src,
                                       VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    int x = fc.xmin;

                    for (int i = x; i < fc.xmax; ++i) {
                        auto xi = src_line_x[fc.srcWidthOffsetX[i]];
                        auto yi = src_line_y[fc.srcWidthOffsetY[i]];
                        auto zi = src_line_z[fc.srcWidthOffsetZ[i]];

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                        dst_line_x[fc.dstWidthOffsetX[i]] = uint8_t(xo);
                        dst_line_a[fc.dstWidthOffsetA[i]] = 0xff;
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convert3Ato1(const FrameConvertParameters &fc,
                              const VideoFrame &src,
                              VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->read3A(fc,
                                     src_line_x,
                                     src_line_y,
                                     src_line_z,
                                     src_line_a,
                                     x,
                                     &xi,
                                     &yi,
                                     &zi,
                                     &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);
                        fc.colorConvert.applyAlpha(ai, &xo);

                        this->write1(fc,
                                     dst_line_x,
                                     x,
                                     OutputType(xo));
                    }
                }
            }

            void convertFast8bits3Ato1(const FrameConvertParameters &fc,
                                       const VideoFrame &src,
                                       VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    int x = fc.xmin;

                    for (int i = x; i < fc.xmax; ++i) {
                        auto xi = src_line_x[fc.srcWidthOffsetX[i]];
                        auto yi = src_line_y[fc.srcWidthOffsetY[i]];
                        auto zi = src_line_z[fc.srcWidthOffsetZ[i]];
                        auto ai = src_line_a[fc.srcWidthOffsetA[i]];

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);
                        fc.colorConvert.applyAlpha(ai, &xo);

                        dst_line_x[fc.dstWidthOffsetX[i]] = uint8_t(xo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convert3Ato1A(const FrameConvertParameters &fc,
                               const VideoFrame &src,
                               VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->read3A(fc,
                                     src_line_x,
                                     src_line_y,
                                     src_line_z,
                                     src_line_a,
                                     x,
                                     &xi,
                                     &yi,
                                     &zi,
                                     &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                        this->write1A(fc,
                                      dst_line_x,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(ai));
                    }
                }
            }

            void convertFast8bits3Ato1A(const FrameConvertParameters &fc,
                                        const VideoFrame &src,
                                        VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    int x = fc.xmin;

                    for (int i = x; i < fc.xmax; ++i) {
                        auto xi = src_line_x[fc.srcWidthOffsetX[i]];
                        auto yi = src_line_y[fc.srcWidthOffsetY[i]];
                        auto zi = src_line_z[fc.srcWidthOffsetZ[i]];
                        auto ai = src_line_a[fc.srcWidthOffsetA[i]];

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                        dst_line_x[fc.dstWidthOffsetX[i]] = uint8_t(xo);
                        dst_line_a[fc.dstWidthOffsetA[i]] = ai;
                    }
                }
            }

            // Conversion functions for 1 components to 3 components formats

            template <typename InputType, typename OutputType>
            void convert1to3(const FrameConvertParameters &fc,
                             const VideoFrame &src, VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        this->read1(fc,
                                    src_line_x,
                                    x,
                                    &xi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }
                }
            }

            void convertFast8bits1to3(const FrameConvertParameters &fc,
                                      const VideoFrame &src,
                                      VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    int x = fc.xmin;

                    for (int i = x; i < fc.xmax; ++i) {
                        auto xi = src_line_x[fc.srcWidthOffsetX[i]];

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[i]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[i]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[i]] = uint8_t(zo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convert1to3A(const FrameConvertParameters &fc,
                              const VideoFrame &src,
                              VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        this->read1(fc,
                                    src_line_x,
                                    x,
                                    &xi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo));
                    }
                }
            }

            void convertFast8bits1to3A(const FrameConvertParameters &fc,
                                       const VideoFrame &src,
                                       VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    int x = fc.xmin;

                    for (int i = x; i < fc.xmax; ++i) {
                        auto xi = src_line_x[fc.srcWidthOffsetX[i]];

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[i]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[i]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[i]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[i]] = 0xff;
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convert1Ato3(const FrameConvertParameters &fc,
                              const VideoFrame &src,
                              VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType ai;
                        this->read1A(fc,
                                     src_line_x,
                                     src_line_a,
                                     x,
                                     &xi,
                                     &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);
                        fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }
                }
            }

            void convertFast8bits1Ato3(const FrameConvertParameters &fc,
                                       const VideoFrame &src,
                                       VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    int x = fc.xmin;

                    for (int i = x; i < fc.xmax; ++i) {
                        auto xi = src_line_x[fc.srcWidthOffsetX[i]];
                        auto ai = src_line_a[fc.srcWidthOffsetA[i]];

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);
                        fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[i]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[i]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[i]] = uint8_t(zo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convert1Ato3A(const FrameConvertParameters &fc,
                               const VideoFrame &src,
                               VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType ai;
                        this->read1A(fc,
                                     src_line_x,
                                     src_line_a,
                                     x,
                                     &xi,
                                     &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo),
                                      OutputType(ai));
                    }
                }
            }

            void convertFast8bits1Ato3A(const FrameConvertParameters &fc,
                                        const VideoFrame &src,
                                        VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    int x = fc.xmin;

                    for (int i = x; i < fc.xmax; ++i) {
                        auto xi = src_line_x[fc.srcWidthOffsetX[i]];
                        auto ai = src_line_a[fc.srcWidthOffsetA[i]];

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[i]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[i]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[i]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[i]] = ai;
                    }
                }
            }

            // Conversion functions for 1 components to 1 components formats

            template <typename InputType, typename OutputType>
            void convert1to1(const FrameConvertParameters &fc,
                             const VideoFrame &src,
                             VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        this->read1(fc,
                                    src_line_x,
                                    x,
                                    &xi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        this->write1(fc,
                                      dst_line_x,
                                      x,
                                      OutputType(xo));
                    }
                }
            }

            void convertFast8bits1to1(const FrameConvertParameters &fc,
                                      const VideoFrame &src,
                                      VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x)
                        dst_line_x[fc.dstWidthOffsetX[x]] =
                                src_line_x[fc.srcWidthOffsetX[x]];
                }
            }

            template <typename InputType, typename OutputType>
            void convert1to1A(const FrameConvertParameters &fc,
                              const VideoFrame &src,
                              VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        this->read1(fc,
                                    src_line_x,
                                    x,
                                    &xi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        this->write1A(fc,
                                      dst_line_x,
                                      dst_line_a,
                                      x,
                                      OutputType(xo));
                    }
                }
            }

            void convertFast8bits1to1A(const FrameConvertParameters &fc,
                                       const VideoFrame &src,
                                       VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        dst_line_x[fc.dstWidthOffsetX[x]] = src_line_x[fc.srcWidthOffsetX[x]];
                        dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convert1Ato1(const FrameConvertParameters &fc,
                              const VideoFrame &src,
                              VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType ai;
                        this->read1A(fc,
                                     src_line_x,
                                     src_line_a,
                                     x,
                                     &xi,
                                     &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);
                        fc.colorConvert.applyAlpha(ai, &xo);

                        this->write1(fc,
                                     dst_line_x,
                                     x,
                                     OutputType(xo));
                    }
                }
            }

            void convertFast8bits1Ato1(const FrameConvertParameters &fc,
                                       const VideoFrame &src,
                                       VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    int x = fc.xmin;

                    for (int i = x; i < fc.xmax; ++i) {
                        dst_line_x[fc.dstWidthOffsetX[i]] =
                                uint8_t(uint16_t(src_line_x[fc.srcWidthOffsetX[i]])
                                       * uint16_t(src_line_a[fc.srcWidthOffsetA[i]])
                                       / 255);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convert1Ato1A(const FrameConvertParameters &fc,
                               const VideoFrame &src,
                               VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType ai;
                        this->read1A(fc,
                                     src_line_x,
                                     src_line_a,
                                     x,
                                     &xi,
                                     &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        this->write1A(fc,
                                      dst_line_x,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(ai));
                    }
                }
            }

            void convertFast8bits1Ato1A(const FrameConvertParameters &fc,
                                        const VideoFrame &src,
                                        VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        dst_line_x[fc.dstWidthOffsetX[x]] = src_line_x[fc.srcWidthOffsetX[x]];
                        dst_line_a[fc.dstWidthOffsetA[x]] = src_line_a[fc.srcWidthOffsetA[x]];
                    }
                }
            }

            /* Linear downscaling conversion funtions */

            // Conversion functions for 3 components to 3 components formats

            template <typename InputType, typename OutputType>
            void convertDL3to3(const FrameConvertParameters &fc,
                               const VideoFrame &src,
                               VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->readDL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      kdl,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL3to3(const FrameConvertParameters &fc,
                                        const VideoFrame &src,
                                        VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        this->readDL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      kdl,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDL3to3A(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->readDL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      kdl,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL3to3A(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        this->readDL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      kdl,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDL3Ato3(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->readDL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);
                        fc.colorConvert.applyAlpha(ai,
                                                   &xo,
                                                   &yo,
                                                   &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL3Ato3(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        uint8_t ai;
                        this->readDL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);
                        fc.colorConvert.applyAlpha(ai,
                                                   &xo,
                                                   &yo,
                                                   &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDL3Ato3A(const FrameConvertParameters &fc,
                                 const VideoFrame &src,
                                 VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->readDL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo),
                                      OutputType(ai));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL3Ato3A(const FrameConvertParameters &fc,
                                          const VideoFrame &src,
                                          VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        uint8_t ai;
                        this->readDL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                    }

                    kdl += fc.inputWidth;
                }
            }

            // Conversion functions for 3 components to 3 components formats
            // (same color space)

            template <typename InputType, typename OutputType>
            void convertDLV3to3(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->readDL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      kdl,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDLV3to3(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        this->readDL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      kdl,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDLV3to3A(const FrameConvertParameters &fc,
                                 const VideoFrame &src,
                                 VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->readDL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      kdl,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDLV3to3A(const FrameConvertParameters &fc,
                                          const VideoFrame &src,
                                          VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        this->readDL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      kdl,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDLV3Ato3(const FrameConvertParameters &fc,
                                 const VideoFrame &src,
                                 VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->readDL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);
                        fc.colorConvert.applyAlpha(ai,
                                                   &xo,
                                                   &yo,
                                                   &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDLV3Ato3(const FrameConvertParameters &fc,
                                          const VideoFrame &src,
                                          VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        uint8_t ai;
                        this->readDL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);
                        fc.colorConvert.applyAlpha(ai,
                                                   &xo,
                                                   &yo,
                                                   &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDLV3Ato3A(const FrameConvertParameters &fc,
                                  const VideoFrame &src,
                                  VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->readDL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo),
                                      OutputType(ai));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDLV3Ato3A(const FrameConvertParameters &fc,
                                           const VideoFrame &src,
                                           VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        uint8_t ai;
                        this->readDL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                    }

                    kdl += fc.inputWidth;
                }
            }

            // Conversion functions for 3 components to 1 components formats

            template <typename InputType, typename OutputType>
            void convertDL3to1(const FrameConvertParameters &fc,
                               const VideoFrame &src,
                               VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y);

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->readDL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      kdl,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                        this->write1(fc,
                                     dst_line_x,
                                     x,
                                     OutputType(xo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL3to1(const FrameConvertParameters &fc,
                                        const VideoFrame &src,
                                        VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        this->readDL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      kdl,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDL3to1A(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->readDL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      kdl,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                        this->write1A(fc,
                                      dst_line_x,
                                      dst_line_a,
                                      x,
                                      OutputType(xo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL3to1A(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        this->readDL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      kdl,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDL3Ato1(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y);

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->readDL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);
                        fc.colorConvert.applyAlpha(ai, &xo);

                        this->write1(fc,
                                     dst_line_x,
                                     x,
                                     OutputType(xo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL3Ato1(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        uint8_t ai;
                        this->readDL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);
                        fc.colorConvert.applyAlpha(ai, &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDL3Ato1A(const FrameConvertParameters &fc,
                                 const VideoFrame &src,
                                 VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->readDL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                        this->write1A(fc,
                                      dst_line_x,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(ai));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL3Ato1A(const FrameConvertParameters &fc,
                                          const VideoFrame &src,
                                          VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_y = fc.integralImageDataY + yOffset;
                    auto src_line_z = fc.integralImageDataZ + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_y_1 = fc.integralImageDataY + y1Offset;
                    auto src_line_z_1 = fc.integralImageDataZ + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        uint8_t ai;
                        this->readDL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, yi, zi, &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                    }

                    kdl += fc.inputWidth;
                }
            }

            // Conversion functions for 1 components to 3 components formats

            template <typename InputType, typename OutputType>
            void convertDL1to3(const FrameConvertParameters &fc,
                               const VideoFrame &src,
                               VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        this->readDL1(fc,
                                      src_line_x,
                                      src_line_x_1,
                                      x,
                                      kdl,
                                      &xi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL1to3(const FrameConvertParameters &fc,
                                        const VideoFrame &src,
                                        VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        this->readDL1(fc,
                                      src_line_x,
                                      src_line_x_1,
                                      x,
                                      kdl,
                                      &xi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDL1to3A(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        this->readDL1(fc,
                                      src_line_x,
                                      src_line_x_1,
                                      x,
                                      kdl,
                                      &xi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL1to3A(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        this->readDL1(fc,
                                      src_line_x,
                                      src_line_x_1,
                                      x,
                                      kdl,
                                      &xi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDL1Ato3(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType ai;
                        this->readDL1A(fc,
                                       src_line_x,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);
                        fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL1Ato3(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t ai;
                        this->readDL1A(fc,
                                       src_line_x,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);
                        fc.colorConvert.applyAlpha(ai, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDL1Ato3A(const FrameConvertParameters &fc,
                                 const VideoFrame &src,
                                 VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType ai;
                        this->readDL1A(fc,
                                       src_line_x,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo),
                                      OutputType(ai));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL1Ato3A(const FrameConvertParameters &fc,
                                          const VideoFrame &src,
                                          VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t ai;
                        this->readDL1A(fc,
                                       src_line_x,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                    }

                    kdl += fc.inputWidth;
                }
            }

            // Conversion functions for 1 components to 1 components formats

            template <typename InputType, typename OutputType>
            void convertDL1to1(const FrameConvertParameters &fc,
                               const VideoFrame &src,
                               VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        this->readDL1(fc,
                                      src_line_x,
                                      src_line_x_1,
                                      x,
                                      kdl,
                                      &xi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        this->write1(fc,
                                     dst_line_x,
                                     x,
                                     OutputType(xo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL1to1(const FrameConvertParameters &fc,
                                        const VideoFrame &src,
                                        VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        this->readDL1(fc,
                                      src_line_x,
                                      src_line_x_1,
                                      x,
                                      kdl,
                                      &xi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDL1to1A(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        this->readDL1(fc,
                                      src_line_x,
                                      src_line_x_1,
                                      x,
                                      kdl,
                                      &xi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        this->write1A(fc,
                                      dst_line_x,
                                      dst_line_a,
                                      x,
                                      OutputType(xo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL1to1A(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        this->readDL1(fc,
                                      src_line_x,
                                      src_line_x_1,
                                      x,
                                      kdl,
                                      &xi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDL1Ato1(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType ai;
                        this->readDL1A(fc,
                                       src_line_x,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);
                        fc.colorConvert.applyAlpha(ai, &xo);

                        this->write1(fc,
                                     dst_line_x,
                                     x,
                                     OutputType(xo));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL1Ato1(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t ai;
                        this->readDL1A(fc,
                                       src_line_x,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);
                        fc.colorConvert.applyAlpha(ai, &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                    }

                    kdl += fc.inputWidth;
                }
            }

            template <typename InputType, typename OutputType>
            void convertDL1Ato1A(const FrameConvertParameters &fc,
                                 const VideoFrame &src,
                                 VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType ai;
                        this->readDL1A(fc,
                                       src_line_x,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        this->write1A(fc,
                                      dst_line_x,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(ai));
                    }

                    kdl += fc.inputWidth;
                }
            }

            void convertFast8bitsDL1Ato1A(const FrameConvertParameters &fc,
                                          const VideoFrame &src,
                                          VideoFrame &dst) const
            {
                UNUSED(src);
                auto kdl = fc.kdl;

                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &yOffset = fc.srcHeightDlOffset[y];
                    auto &y1Offset = fc.srcHeightDlOffset_1[y];

                    auto src_line_x = fc.integralImageDataX + yOffset;
                    auto src_line_a = fc.integralImageDataA + yOffset;

                    auto src_line_x_1 = fc.integralImageDataX + y1Offset;
                    auto src_line_a_1 = fc.integralImageDataA + y1Offset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t ai;
                        this->readDL1A(fc,
                                       src_line_x,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_a_1,
                                       x,
                                       kdl,
                                       &xi,
                                       &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                    }

                    kdl += fc.inputWidth;
                }
            }

            /* Linear upscaling conversion funtions */

            // Conversion functions for 3 components to 3 components formats

            template <typename InputType, typename OutputType>
            void convertUL3to3(const FrameConvertParameters &fc,
                               const VideoFrame &src,
                               VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->readUL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      ky,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }
                }
            }

            void convertFast8bitsUL3to3(const FrameConvertParameters &fc,
                                        const VideoFrame &src,
                                        VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        this->readF8UL3(fc,
                                        src_line_x,
                                        src_line_y,
                                        src_line_z,
                                        src_line_x_1,
                                        src_line_y_1,
                                        src_line_z_1,
                                        x,
                                        ky,
                                        &xi,
                                        &yi,
                                        &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertUL3to3A(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->readUL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      ky,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo));
                    }
                }
            }

            void convertFast8bitsUL3to3A(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        this->readF8UL3(fc,
                                        src_line_x,
                                        src_line_y,
                                        src_line_z,
                                        src_line_x_1,
                                        src_line_y_1,
                                        src_line_z_1,
                                        x,
                                        ky,
                                        &xi,
                                        &yi,
                                        &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertUL3Ato3(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->readUL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       ky,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);
                        fc.colorConvert.applyAlpha(ai,
                                                   &xo,
                                                   &yo,
                                                   &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }
                }
            }

            void convertFast8bitsUL3Ato3(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        uint8_t ai;
                        this->readF8UL3A(fc,
                                         src_line_x,
                                         src_line_y,
                                         src_line_z,
                                         src_line_a,
                                         src_line_x_1,
                                         src_line_y_1,
                                         src_line_z_1,
                                         src_line_a_1,
                                         x,
                                         ky,
                                         &xi,
                                         &yi,
                                         &zi,
                                         &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);
                        fc.colorConvert.applyAlpha(ai,
                                                   &xo,
                                                   &yo,
                                                   &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertUL3Ato3A(const FrameConvertParameters &fc,
                                 const VideoFrame &src,
                                 VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->readUL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       ky,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo),
                                      OutputType(ai));
                    }
                }
            }

            void convertFast8bitsUL3Ato3A(const FrameConvertParameters &fc,
                                          const VideoFrame &src,
                                          VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        uint8_t ai;
                        this->readF8UL3A(fc,
                                         src_line_x,
                                         src_line_y,
                                         src_line_z,
                                         src_line_a,
                                         src_line_x_1,
                                         src_line_y_1,
                                         src_line_z_1,
                                         src_line_a_1,
                                         x,
                                         ky,
                                         &xi,
                                         &yi,
                                         &zi,
                                         &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyMatrix(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                    }
                }
            }

            // Conversion functions for 3 components to 3 components formats
            // (same color space)

            template <typename InputType, typename OutputType>
            void convertULV3to3(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->readUL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      ky,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }
                }
            }

            void convertFast8bitsULV3to3(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        this->readF8UL3(fc,
                                        src_line_x,
                                        src_line_y,
                                        src_line_z,
                                        src_line_x_1,
                                        src_line_y_1,
                                        src_line_z_1,
                                        x,
                                        ky,
                                        &xi,
                                        &yi,
                                        &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertULV3to3A(const FrameConvertParameters &fc,
                                 const VideoFrame &src,
                                 VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->readUL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      ky,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo));
                    }
                }
            }

            void convertFast8bitsULV3to3A(const FrameConvertParameters &fc,
                                          const VideoFrame &src,
                                          VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        this->readF8UL3(fc,
                                        src_line_x,
                                        src_line_y,
                                        src_line_z,
                                        src_line_x_1,
                                        src_line_y_1,
                                        src_line_z_1,
                                        x,
                                        ky,
                                        &xi,
                                        &yi,
                                        &zi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertULV3Ato3(const FrameConvertParameters &fc,
                                 const VideoFrame &src,
                                 VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->readUL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       ky,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);
                        fc.colorConvert.applyAlpha(ai,
                                                   &xo,
                                                   &yo,
                                                   &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }
                }
            }

            void convertFast8bitsULV3Ato3(const FrameConvertParameters &fc,
                                          const VideoFrame &src,
                                          VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        uint8_t ai;
                        this->readF8UL3A(fc,
                                         src_line_x,
                                         src_line_y,
                                         src_line_z,
                                         src_line_a,
                                         src_line_x_1,
                                         src_line_y_1,
                                         src_line_z_1,
                                         src_line_a_1,
                                         x,
                                         ky,
                                         &xi,
                                         &yi,
                                         &zi,
                                         &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);
                        fc.colorConvert.applyAlpha(ai,
                                                   &xo,
                                                   &yo,
                                                   &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertULV3Ato3A(const FrameConvertParameters &fc,
                                  const VideoFrame &src,
                                  VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->readUL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       ky,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo),
                                      OutputType(ai));
                    }
                }
            }

            void convertFast8bitsULV3Ato3A(const FrameConvertParameters &fc,
                                           const VideoFrame &src,
                                           VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        uint8_t ai;
                        this->readF8UL3A(fc,
                                         src_line_x,
                                         src_line_y,
                                         src_line_z,
                                         src_line_a,
                                         src_line_x_1,
                                         src_line_y_1,
                                         src_line_z_1,
                                         src_line_a_1,
                                         x,
                                         ky,
                                         &xi,
                                         &yi,
                                         &zi,
                                         &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyVector(xi,
                                                    yi,
                                                    zi,
                                                    &xo,
                                                    &yo,
                                                    &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                    }
                }
            }

            // Conversion functions for 3 components to 1 components formats

            template <typename InputType, typename OutputType>
            void convertUL3to1(const FrameConvertParameters &fc,
                               const VideoFrame &src,
                               VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y);

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->readUL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      ky,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi,
                                                   yi,
                                                   zi,
                                                   &xo);

                        this->write1(fc,
                                     dst_line_x,
                                     x,
                                     OutputType(xo));
                    }
                }
            }

            void convertFast8bitsUL3to1(const FrameConvertParameters &fc,
                                        const VideoFrame &src,
                                        VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        this->readF8UL3(fc,
                                        src_line_x,
                                        src_line_y,
                                        src_line_z,
                                        src_line_x_1,
                                        src_line_y_1,
                                        src_line_z_1,
                                        x,
                                        ky,
                                        &xi,
                                        &yi,
                                        &zi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi,
                                                   yi,
                                                   zi,
                                                   &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertUL3to1A(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        this->readUL3(fc,
                                      src_line_x,
                                      src_line_y,
                                      src_line_z,
                                      src_line_x_1,
                                      src_line_y_1,
                                      src_line_z_1,
                                      x,
                                      ky,
                                      &xi,
                                      &yi,
                                      &zi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi,
                                                   yi,
                                                   zi,
                                                   &xo);

                        this->write1A(fc,
                                      dst_line_x,
                                      dst_line_a,
                                      x,
                                      OutputType(xo));
                    }
                }
            }

            void convertFast8bitsUL3to1A(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        this->readF8UL3(fc,
                                        src_line_x,
                                        src_line_y,
                                        src_line_z,
                                        src_line_x_1,
                                        src_line_y_1,
                                        src_line_z_1,
                                        x,
                                        ky,
                                        &xi,
                                        &yi,
                                        &zi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi,
                                                   yi,
                                                   zi,
                                                   &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertUL3Ato1(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y);

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->readUL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       ky,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi,
                                                   yi,
                                                   zi,
                                                   &xo);
                        fc.colorConvert.applyAlpha(ai, &xo);

                        this->write1(fc,
                                     dst_line_x,
                                     x,
                                     OutputType(xo));
                    }
                }
            }

            void convertFast8bitsUL3Ato1(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        uint8_t ai;
                        this->readF8UL3A(fc,
                                         src_line_x,
                                         src_line_y,
                                         src_line_z,
                                         src_line_a,
                                         src_line_x_1,
                                         src_line_y_1,
                                         src_line_z_1,
                                         src_line_a_1,
                                         x,
                                         ky,
                                         &xi,
                                         &yi,
                                         &zi,
                                         &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi,
                                                   yi,
                                                   zi,
                                                   &xo);
                        fc.colorConvert.applyAlpha(ai, &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertUL3Ato1A(const FrameConvertParameters &fc,
                                 const VideoFrame &src,
                                 VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType yi;
                        InputType zi;
                        InputType ai;
                        this->readUL3A(fc,
                                       src_line_x,
                                       src_line_y,
                                       src_line_z,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_y_1,
                                       src_line_z_1,
                                       src_line_a_1,
                                       x,
                                       ky,
                                       &xi,
                                       &yi,
                                       &zi,
                                       &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi,
                                                   yi,
                                                   zi,
                                                   &xo);

                        this->write1A(fc,
                                      dst_line_x,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(ai));
                    }
                }
            }

            void convertFast8bitsUL3Ato1A(const FrameConvertParameters &fc,
                                          const VideoFrame &src,
                                          VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_y = src.constLine(fc.planeYi, ys) + fc.yiOffset;
                    auto src_line_z = src.constLine(fc.planeZi, ys) + fc.ziOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;

                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_y_1 = src.constLine(fc.planeYi, ys_1) + fc.yiOffset;
                    auto src_line_z_1 = src.constLine(fc.planeZi, ys_1) + fc.ziOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t yi;
                        uint8_t zi;
                        uint8_t ai;
                        this->readF8UL3A(fc,
                                         src_line_x,
                                         src_line_y,
                                         src_line_z,
                                         src_line_a,
                                         src_line_x_1,
                                         src_line_y_1,
                                         src_line_z_1,
                                         src_line_a_1,
                                         x,
                                         ky,
                                         &xi,
                                         &yi,
                                         &zi,
                                         &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi,
                                                   yi,
                                                   zi,
                                                   &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                    }
                }
            }

            // Conversion functions for 1 components to 3 components formats

            template <typename InputType, typename OutputType>
            void convertUL1to3(const FrameConvertParameters &fc,
                               const VideoFrame &src,
                               VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        this->readUL1(fc,
                                      src_line_x,
                                      src_line_x_1,
                                      x,
                                      ky,
                                      &xi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }
                }
            }

            void convertFast8bitsUL1to3(const FrameConvertParameters &fc,
                                        const VideoFrame &src,
                                        VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        this->readF8UL1(fc,
                                        src_line_x,
                                        src_line_x_1,
                                        x,
                                        ky,
                                        &xi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertUL1to3A(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        this->readUL1(fc,
                                      src_line_x,
                                      src_line_x_1,
                                      x,
                                      ky,
                                      &xi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo));
                    }
                }
            }

            void convertFast8bitsUL1to3A(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        this->readF8UL1(fc,
                                        src_line_x,
                                        src_line_x_1,
                                        x,
                                        ky,
                                        &xi);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertUL1Ato3(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType ai;
                        this->readUL1A(fc,
                                       src_line_x,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_a_1,
                                       x,
                                       ky,
                                       &xi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);
                        fc.colorConvert.applyAlpha(xi, &xo, &yo, &zo);

                        this->write3(fc,
                                     dst_line_x,
                                     dst_line_y,
                                     dst_line_z,
                                     x,
                                     OutputType(xo),
                                     OutputType(yo),
                                     OutputType(zo));
                    }
                }
            }

            void convertFast8bitsUL1Ato3(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t ai;
                        this->readF8UL1A(fc,
                                         src_line_x,
                                         src_line_a,
                                         src_line_x_1,
                                         src_line_a_1,
                                         x,
                                         ky,
                                         &xi,
                                         &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);
                        fc.colorConvert.applyAlpha(xi, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertUL1Ato3A(const FrameConvertParameters &fc,
                                 const VideoFrame &src,
                                 VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType ai;
                        this->readUL1A(fc,
                                       src_line_x,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_a_1,
                                       x,
                                       ky,
                                       &xi,
                                       &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        this->write3A(fc,
                                      dst_line_x,
                                      dst_line_y,
                                      dst_line_z,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(yo),
                                      OutputType(zo),
                                      OutputType(ai));
                    }
                }
            }

            void convertFast8bitsUL1Ato3A(const FrameConvertParameters &fc,
                                          const VideoFrame &src,
                                          VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_y = dst.line(fc.planeYo, y) + fc.yoOffset;
                    auto dst_line_z = dst.line(fc.planeZo, y) + fc.zoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t ai;
                        this->readF8UL1A(fc,
                                         src_line_x,
                                         src_line_a,
                                         src_line_x_1,
                                         src_line_a_1,
                                         x,
                                         ky,
                                         &xi,
                                         &ai);

                        int64_t xo = 0;
                        int64_t yo = 0;
                        int64_t zo = 0;
                        fc.colorConvert.applyPoint(xi, &xo, &yo, &zo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_y[fc.dstWidthOffsetY[x]] = uint8_t(yo);
                        dst_line_z[fc.dstWidthOffsetZ[x]] = uint8_t(zo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                    }
                }
            }

            // Conversion functions for 1 components to 1 components formats

            template <typename InputType, typename OutputType>
            void convertUL1to1(const FrameConvertParameters &fc,
                               const VideoFrame &src,
                               VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        this->readUL1(fc,
                                      src_line_x,
                                      src_line_x_1,
                                      x,
                                      ky,
                                      &xi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        this->write1(fc,
                                     dst_line_x,
                                     x,
                                     OutputType(xo));
                    }
                }
            }

            void convertFast8bitsUL1to1(const FrameConvertParameters &fc,
                                        const VideoFrame &src,
                                        VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        this->readF8UL1(fc,
                                        src_line_x,
                                        src_line_x_1,
                                        x,
                                        ky,
                                        &xi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertUL1to1A(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        this->readUL1(fc,
                                      src_line_x,
                                      src_line_x_1,
                                      x,
                                      ky,
                                      &xi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        this->write1A(fc,
                                      dst_line_x,
                                      dst_line_a,
                                      x,
                                      OutputType(xo));
                    }
                }
            }

            void convertFast8bitsUL1to1A(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        this->readF8UL1(fc,
                                        src_line_x,
                                        src_line_x_1,
                                        x,
                                        ky,
                                        &xi);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = 0xff;
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertUL1Ato1(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType ai;
                        this->readUL1A(fc,
                                       src_line_x,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_a_1,
                                       x,
                                       ky,
                                       &xi,
                                       &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);
                        fc.colorConvert.applyAlpha(ai, &xo);

                        this->write1(fc,
                                     dst_line_x,
                                     x,
                                     OutputType(xo));
                    }
                }
            }

            void convertFast8bitsUL1Ato1(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t ai;
                        this->readF8UL1A(fc,
                                         src_line_x,
                                         src_line_a,
                                         src_line_x_1,
                                         src_line_a_1,
                                         x,
                                         ky,
                                         &xi,
                                         &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);
                        fc.colorConvert.applyAlpha(ai, &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                    }
                }
            }

            template <typename InputType, typename OutputType>
            void convertUL1Ato1A(const FrameConvertParameters &fc,
                                 const VideoFrame &src,
                                 VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        InputType xi;
                        InputType ai;
                        this->readUL1A(fc,
                                       src_line_x,
                                       src_line_a,
                                       src_line_x_1,
                                       src_line_a_1,
                                       x,
                                       ky,
                                       &xi,
                                       &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        this->write1A(fc,
                                      dst_line_x,
                                      dst_line_a,
                                      x,
                                      OutputType(xo),
                                      OutputType(ai));
                    }
                }
            }

            void convertFast8bitsUL1Ato1A(const FrameConvertParameters &fc,
                                          const VideoFrame &src,
                                          VideoFrame &dst) const
            {
                for (int y = fc.ymin; y < fc.ymax; ++y) {
                    auto &ys = fc.srcHeight[y];
                    auto &ys_1 = fc.srcHeight_1[y];

                    auto src_line_x = src.constLine(fc.planeXi, ys) + fc.xiOffset;
                    auto src_line_a = src.constLine(fc.planeAi, ys) + fc.aiOffset;
                    auto src_line_x_1 = src.constLine(fc.planeXi, ys_1) + fc.xiOffset;
                    auto src_line_a_1 = src.constLine(fc.planeAi, ys_1) + fc.aiOffset;

                    auto dst_line_x = dst.line(fc.planeXo, y) + fc.xoOffset;
                    auto dst_line_a = dst.line(fc.planeAo, y) + fc.aoOffset;

                    auto &ky = fc.ky[y];

                    for (int x = fc.xmin; x < fc.xmax; ++x) {
                        uint8_t xi;
                        uint8_t ai;
                        this->readF8UL1A(fc,
                                         src_line_x,
                                         src_line_a,
                                         src_line_x_1,
                                         src_line_a_1,
                                         x,
                                         ky,
                                         &xi,
                                         &ai);

                        int64_t xo = 0;
                        fc.colorConvert.applyPoint(xi, &xo);

                        dst_line_x[fc.dstWidthOffsetX[x]] = uint8_t(xo);
                        dst_line_a[fc.dstWidthOffsetA[x]] = ai;
                    }
                }
            }

    #define CONVERT_FUNC(icomponents, ocomponents) \
            template <typename InputType, typename OutputType> \
            inline void convertFormat##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                    const VideoFrame &src, \
                                                                    VideoFrame &dst) const \
            { \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                    this->convert##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_AI_O: \
                    this->convert##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_AO: \
                    this->convert##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_O: \
                    this->convert##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                    break; \
                }; \
            }

    #define CONVERT_FAST_FUNC(icomponents, ocomponents) \
            inline void convertFormatFast8bits##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                             const VideoFrame &src, \
                                                                             VideoFrame &dst) const \
            { \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                    this->convertFast8bits##icomponents##Ato##ocomponents##A(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_AI_O: \
                    this->convertFast8bits##icomponents##Ato##ocomponents(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_AO: \
                    this->convertFast8bits##icomponents##to##ocomponents##A(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_O: \
                    this->convertFast8bits##icomponents##to##ocomponents(fc, src, dst); \
                    break; \
                }; \
            }

    #define CONVERTV_FUNC(icomponents, ocomponents) \
            template <typename InputType, typename OutputType> \
            inline void convertVFormat##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                     const VideoFrame &src, \
                                                                     VideoFrame &dst) const \
            { \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                    this->convertV##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_AI_O: \
                    this->convertV##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_AO: \
                    this->convertV##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_O: \
                    this->convertV##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                    break; \
                }; \
            }

    #define CONVERT_FASTV_FUNC(icomponents, ocomponents) \
            inline void convertFormatFast8bitsV##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                              const VideoFrame &src, \
                                                                              VideoFrame &dst) const \
            { \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                    this->convertFast8bitsV##icomponents##Ato##ocomponents##A(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_AI_O: \
                    this->convertFast8bitsV##icomponents##Ato##ocomponents(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_AO: \
                    this->convertFast8bitsV##icomponents##to##ocomponents##A(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_O: \
                    this->convertFast8bitsV##icomponents##to##ocomponents(fc, src, dst); \
                    break; \
                }; \
            }

    #define CONVERTDL_FUNC(icomponents, ocomponents) \
            template <typename InputType, typename OutputType> \
            inline void convertFormatDL##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                      const VideoFrame &src, \
                                                                      VideoFrame &dst) const \
            { \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                case ConvertAlphaMode_AI_O: \
                    this->integralImage##icomponents##A<InputType>(fc, src); \
                    break; \
                default: \
                    this->integralImage##icomponents<InputType>(fc, src); \
                    break; \
                } \
                \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                    this->convertDL##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_AI_O: \
                    this->convertDL##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_AO: \
                    this->convertDL##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_O: \
                    this->convertDL##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                    break; \
                }; \
            }

    #define CONVERT_FASTDL_FUNC(icomponents, ocomponents) \
            inline void convertFormatFast8bitsDL##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                               const VideoFrame &src, \
                                                                               VideoFrame &dst) const \
            { \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                case ConvertAlphaMode_AI_O: \
                    this->integralImage##icomponents##A<uint8_t>(fc, src); \
                    break; \
                default: \
                    this->integralImage##icomponents<uint8_t>(fc, src); \
                    break; \
                } \
                \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                    this->convertFast8bitsDL##icomponents##Ato##ocomponents##A(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_AI_O: \
                    this->convertFast8bitsDL##icomponents##Ato##ocomponents(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_AO: \
                    this->convertFast8bitsDL##icomponents##to##ocomponents##A(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_O: \
                    this->convertFast8bitsDL##icomponents##to##ocomponents(fc, src, dst); \
                    break; \
                }; \
            }

    #define CONVERTDLV_FUNC(icomponents, ocomponents) \
            template <typename InputType, typename OutputType> \
            inline void convertFormatDLV##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                       const VideoFrame &src, \
                                                                       VideoFrame &dst) const \
            { \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                case ConvertAlphaMode_AI_O: \
                    this->integralImage##icomponents##A<InputType>(fc, src); \
                    break; \
                default: \
                    this->integralImage##icomponents<InputType>(fc, src); \
                    break; \
                } \
                \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                    this->convertDLV##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_AI_O: \
                    this->convertDLV##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_AO: \
                    this->convertDLV##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_O: \
                    this->convertDLV##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                    break; \
                }; \
            }

    #define CONVERT_FASTDLV_FUNC(icomponents, ocomponents) \
            inline void convertFormatFast8bitsDLV##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                                const VideoFrame &src, \
                                                                                VideoFrame &dst) const \
            { \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                case ConvertAlphaMode_AI_O: \
                    this->integralImage##icomponents##A<uint8_t>(fc, src); \
                    break; \
                default: \
                    this->integralImage##icomponents<uint8_t>(fc, src); \
                    break; \
                } \
                \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                    this->convertFast8bitsDLV##icomponents##Ato##ocomponents##A(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_AI_O: \
                    this->convertFast8bitsDLV##icomponents##Ato##ocomponents(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_AO: \
                    this->convertFast8bitsDLV##icomponents##to##ocomponents##A(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_O: \
                    this->convertFast8bitsDLV##icomponents##to##ocomponents(fc, src, dst); \
                    break; \
                }; \
            }

    #define CONVERTUL_FUNC(icomponents, ocomponents) \
            template <typename InputType, typename OutputType> \
            inline void convertFormatUL##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                      const VideoFrame &src, \
                                                                      VideoFrame &dst) const \
            { \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                    this->convertUL##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_AI_O: \
                    this->convertUL##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_AO: \
                    this->convertUL##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_O: \
                    this->convertUL##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                    break; \
                }; \
            }

    #define CONVERT_FASTUL_FUNC(icomponents, ocomponents) \
            inline void convertFormatFast8bitsUL##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                               const VideoFrame &src, \
                                                                               VideoFrame &dst) const \
            { \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                    this->convertFast8bitsUL##icomponents##Ato##ocomponents##A(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_AI_O: \
                    this->convertFast8bitsUL##icomponents##Ato##ocomponents(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_AO: \
                    this->convertFast8bitsUL##icomponents##to##ocomponents##A(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_O: \
                    this->convertFast8bitsUL##icomponents##to##ocomponents(fc, src, dst); \
                    break; \
                }; \
            }

    #define CONVERTULV_FUNC(icomponents, ocomponents) \
            template <typename InputType, typename OutputType> \
            inline void convertFormatULV##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                       const VideoFrame &src, \
                                                                       VideoFrame &dst) const \
            { \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                    this->convertULV##icomponents##Ato##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_AI_O: \
                    this->convertULV##icomponents##Ato##ocomponents<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_AO: \
                    this->convertULV##icomponents##to##ocomponents##A<InputType, OutputType>(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_O: \
                    this->convertULV##icomponents##to##ocomponents<InputType, OutputType>(fc, src, dst); \
                    break; \
                }; \
            }

    #define CONVERT_FASTULV_FUNC(icomponents, ocomponents) \
            inline void convertFormatFast8bitsULV##icomponents##to##ocomponents(const FrameConvertParameters &fc, \
                                                                                const VideoFrame &src, \
                                                                                VideoFrame &dst) const \
            { \
                switch (fc.alphaMode) { \
                case ConvertAlphaMode_AI_AO: \
                    this->convertFast8bitsULV##icomponents##Ato##ocomponents##A(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_AI_O: \
                    this->convertFast8bitsULV##icomponents##Ato##ocomponents(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_AO: \
                    this->convertFast8bitsULV##icomponents##to##ocomponents##A(fc, src, dst); \
                    break; \
                case ConvertAlphaMode_I_O: \
                    this->convertFast8bitsULV##icomponents##to##ocomponents(fc, src, dst); \
                    break; \
                }; \
            }

            CONVERT_FUNC(3, 3)
            CONVERT_FUNC(3, 1)
            CONVERT_FUNC(1, 3)
            CONVERT_FUNC(1, 1)
            CONVERTV_FUNC(3, 3)
            CONVERTDL_FUNC(3, 3)
            CONVERTDL_FUNC(3, 1)
            CONVERTDL_FUNC(1, 3)
            CONVERTDL_FUNC(1, 1)
            CONVERTDLV_FUNC(3, 3)
            CONVERTUL_FUNC(3, 3)
            CONVERTUL_FUNC(3, 1)
            CONVERTUL_FUNC(1, 3)
            CONVERTUL_FUNC(1, 1)
            CONVERTULV_FUNC(3, 3)

            CONVERT_FAST_FUNC(3, 3)
            CONVERT_FAST_FUNC(3, 1)
            CONVERT_FAST_FUNC(1, 3)
            CONVERT_FAST_FUNC(1, 1)
            CONVERT_FASTV_FUNC(3, 3)
            CONVERT_FASTDL_FUNC(3, 3)
            CONVERT_FASTDL_FUNC(3, 1)
            CONVERT_FASTDL_FUNC(1, 3)
            CONVERT_FASTDL_FUNC(1, 1)
            CONVERT_FASTDLV_FUNC(3, 3)
            CONVERT_FASTUL_FUNC(3, 3)
            CONVERT_FASTUL_FUNC(3, 1)
            CONVERT_FASTUL_FUNC(1, 3)
            CONVERT_FASTUL_FUNC(1, 1)
            CONVERT_FASTULV_FUNC(3, 3)

            template <typename InputType, typename OutputType>
            inline void convert(const FrameConvertParameters &fc,
                                const VideoFrame &src,
                                VideoFrame &dst)
            {
                if (this->m_scalingMode == AkVCam::VideoConverter::ScalingMode_Linear
                    && fc.resizeMode == ResizeMode_Up) {
                    switch (fc.convertType) {
                    case ConvertType_Vector:
                        this->convertFormatULV3to3<InputType, OutputType>(fc, src, dst);
                        break;
                    case ConvertType_3to3:
                        this->convertFormatUL3to3<InputType, OutputType>(fc, src, dst);
                        break;
                    case ConvertType_3to1:
                        this->convertFormatUL3to1<InputType, OutputType>(fc, src, dst);
                        break;
                    case ConvertType_1to3:
                        this->convertFormatUL1to3<InputType, OutputType>(fc, src, dst);
                        break;
                    case ConvertType_1to1:
                        this->convertFormatUL1to1<InputType, OutputType>(fc, src, dst);
                        break;
                    }
                } else if (this->m_scalingMode == AkVCam::VideoConverter::ScalingMode_Linear
                           && fc.resizeMode == ResizeMode_Down) {
                    switch (fc.convertType) {
                    case ConvertType_Vector:
                        this->convertFormatDLV3to3<InputType, OutputType>(fc, src, dst);
                        break;
                    case ConvertType_3to3:
                        this->convertFormatDL3to3<InputType, OutputType>(fc, src, dst);
                        break;
                    case ConvertType_3to1:
                        this->convertFormatDL3to1<InputType, OutputType>(fc, src, dst);
                        break;
                    case ConvertType_1to3:
                        this->convertFormatDL1to3<InputType, OutputType>(fc, src, dst);
                        break;
                    case ConvertType_1to1:
                        this->convertFormatDL1to1<InputType, OutputType>(fc, src, dst);
                        break;
                    }
                } else {
                    switch (fc.convertType) {
                    case ConvertType_Vector:
                        this->convertVFormat3to3<InputType, OutputType>(fc, src, dst);
                        break;
                    case ConvertType_3to3:
                        this->convertFormat3to3<InputType, OutputType>(fc, src, dst);
                        break;
                    case ConvertType_3to1:
                        this->convertFormat3to1<InputType, OutputType>(fc, src, dst);
                        break;
                    case ConvertType_1to3:
                        this->convertFormat1to3<InputType, OutputType>(fc, src, dst);
                        break;
                    case ConvertType_1to1:
                        this->convertFormat1to1<InputType, OutputType>(fc, src, dst);
                        break;
                    }
                }
            }

            inline void convertFast8bits(const FrameConvertParameters &fc,
                                         const VideoFrame &src,
                                         VideoFrame &dst)
            {
                if (this->m_scalingMode == AkVCam::VideoConverter::ScalingMode_Linear
                    && fc.resizeMode == ResizeMode_Up) {
                    switch (fc.convertType) {
                    case ConvertType_Vector:
                        this->convertFormatFast8bitsULV3to3(fc, src, dst);
                        break;
                    case ConvertType_3to3:
                        this->convertFormatFast8bitsUL3to3(fc, src, dst);
                        break;
                    case ConvertType_3to1:
                        this->convertFormatFast8bitsUL3to1(fc, src, dst);
                        break;
                    case ConvertType_1to3:
                        this->convertFormatFast8bitsUL1to3(fc, src, dst);
                        break;
                    case ConvertType_1to1:
                        this->convertFormatFast8bitsUL1to1(fc, src, dst);
                        break;
                    }
                } else if (this->m_scalingMode == AkVCam::VideoConverter::ScalingMode_Linear
                           && fc.resizeMode == ResizeMode_Down) {
                    switch (fc.convertType) {
                    case ConvertType_Vector:
                        this->convertFormatFast8bitsDLV3to3(fc, src, dst);
                        break;
                    case ConvertType_3to3:
                        this->convertFormatFast8bitsDL3to3(fc, src, dst);
                        break;
                    case ConvertType_3to1:
                        this->convertFormatFast8bitsDL3to1(fc, src, dst);
                        break;
                    case ConvertType_1to3:
                        this->convertFormatFast8bitsDL1to3(fc, src, dst);
                        break;
                    case ConvertType_1to1:
                        this->convertFormatFast8bitsDL1to1(fc, src, dst);
                        break;
                    }
                } else {
                    switch (fc.convertType) {
                    case ConvertType_Vector:
                        this->convertFormatFast8bitsV3to3(fc, src, dst);
                        break;
                    case ConvertType_3to3:
                        this->convertFormatFast8bits3to3(fc, src, dst);
                        break;
                    case ConvertType_3to1:
                        this->convertFormatFast8bits3to1(fc, src, dst);
                        break;
                    case ConvertType_1to3:
                        this->convertFormatFast8bits1to3(fc, src, dst);
                        break;
                    case ConvertType_1to1:
                        this->convertFormatFast8bits1to1(fc, src, dst);
                        break;
                    }
                }
            }

            inline VideoFrame convert(const VideoFrame &frame,
                                         const VideoFormat &oformat);
    };
}

AkVCam::VideoConverter::VideoConverter()
{
    this->d = new VideoConverterPrivate();
}

AkVCam::VideoConverter::VideoConverter(const VideoFormat &outputFormat)
{
    this->d = new VideoConverterPrivate();
    this->d->m_outputFormat = outputFormat;
}

AkVCam::VideoConverter::VideoConverter(const VideoConverter &other)
{
    this->d = new VideoConverterPrivate();
    this->d->m_outputFormat = other.d->m_outputFormat;
    this->d->m_yuvColorSpace = other.d->m_yuvColorSpace;
    this->d->m_yuvColorSpaceType = other.d->m_yuvColorSpaceType;
    this->d->m_scalingMode = other.d->m_scalingMode;
    this->d->m_aspectRatioMode = other.d->m_aspectRatioMode;
    this->d->m_inputRect = other.d->m_inputRect;
}

AkVCam::VideoConverter::~VideoConverter()
{
    if (this->d->m_fc) {
        delete [] this->d->m_fc;
        this->d->m_fc = nullptr;
    }

    delete this->d;
}

AkVCam::VideoConverter &AkVCam::VideoConverter::operator =(const VideoConverter &other)
{
    if (this != &other) {
        this->d->m_yuvColorSpace = other.d->m_yuvColorSpace;
        this->d->m_yuvColorSpaceType = other.d->m_yuvColorSpaceType;
        this->d->m_outputFormat = other.d->m_outputFormat;
        this->d->m_scalingMode = other.d->m_scalingMode;
        this->d->m_aspectRatioMode = other.d->m_aspectRatioMode;
        this->d->m_inputRect = other.d->m_inputRect;
    }

    return *this;
}

AkVCam::VideoFormat AkVCam::VideoConverter::outputFormat() const
{
    return this->d->m_outputFormat;
}

AkVCam::ColorConvert::YuvColorSpace AkVCam::VideoConverter::yuvColorSpace() const
{
    return this->d->m_yuvColorSpace;
}

AkVCam::ColorConvert::YuvColorSpaceType AkVCam::VideoConverter::yuvColorSpaceType() const
{
    return this->d->m_yuvColorSpaceType;
}

AkVCam::VideoConverter::ScalingMode AkVCam::VideoConverter::scalingMode() const
{
    return this->d->m_scalingMode;
}

AkVCam::VideoConverter::AspectRatioMode AkVCam::VideoConverter::aspectRatioMode() const
{
    return this->d->m_aspectRatioMode;
}

AkVCam::Rect AkVCam::VideoConverter::inputRect() const
{
    return this->d->m_inputRect;
}

bool AkVCam::VideoConverter::begin()
{
    this->d->m_cacheIndex = 0;

    return true;
}

void AkVCam::VideoConverter::end()
{
    this->d->m_cacheIndex = 0;
}

AkVCam::VideoFrame AkVCam::VideoConverter::convert(const VideoFrame &frame)
{
    if (!frame)
        return {};

    auto format = frame.format();

    if (format.format() == this->d->m_outputFormat.format()
        && format.width() == this->d->m_outputFormat.width()
        && format.height() == this->d->m_outputFormat.height()
        && this->d->m_inputRect.isEmpty())
        return frame;

    return this->d->convert(frame, this->d->m_outputFormat);
}

void AkVCam::VideoConverter::setCacheIndex(int index)
{
    this->d->m_cacheIndex = index;
}

void AkVCam::VideoConverter::setOutputFormat(const VideoFormat &outputFormat)
{
    this->d->m_mutex.lock();
    this->d->m_outputFormat = outputFormat;
    this->d->m_mutex.unlock();
}

void AkVCam::VideoConverter::setYuvColorSpace(AkVCam::ColorConvert::YuvColorSpace yuvColorSpace)
{
    this->d->m_yuvColorSpace = yuvColorSpace;
}

void AkVCam::VideoConverter::setYuvColorSpaceType(AkVCam::ColorConvert::YuvColorSpaceType yuvColorSpaceType)
{
    this->d->m_yuvColorSpaceType = yuvColorSpaceType;
}

void AkVCam::VideoConverter::setScalingMode(AkVCam::VideoConverter::ScalingMode scalingMode)
{
    this->d->m_scalingMode = scalingMode;
}

void AkVCam::VideoConverter::setAspectRatioMode(AkVCam::VideoConverter::AspectRatioMode aspectRatioMode)
{
    this->d->m_aspectRatioMode = aspectRatioMode;
}

void AkVCam::VideoConverter::setInputRect(const Rect &inputRect)
{
    this->d->m_inputRect = inputRect;
}

void AkVCam::VideoConverter::reset()
{
    if (this->d->m_fc) {
        delete [] this->d->m_fc;
        this->d->m_fc = nullptr;
    }

    this->d->m_fcSize = 0;
}

#define DEFINE_CASE_SM(sm) \
    case AkVCam::VideoConverter::sm: \
        os << #sm; \
        \
        break;

std::ostream &operator <<(std::ostream &os, AkVCam::VideoConverter::ScalingMode mode)
{
    switch (mode) {
    DEFINE_CASE_SM(ScalingMode_Fast)
    DEFINE_CASE_SM(ScalingMode_Linear)

    default:
        os << "ScalingMode_Unknown";

        break;
    }

    return os;
}

#define DEFINE_CASE_ARM(arm) \
    case AkVCam::VideoConverter::arm: \
        os << #arm; \
        \
        break;

std::ostream &operator <<(std::ostream &os, AkVCam::VideoConverter::AspectRatioMode mode)
{
    switch (mode) {
    DEFINE_CASE_ARM(AspectRatioMode_Ignore)
    DEFINE_CASE_ARM(AspectRatioMode_Keep)
    DEFINE_CASE_ARM(AspectRatioMode_Expanding)
    DEFINE_CASE_ARM(AspectRatioMode_Fit)

    default:
        os << "AspectRatioMode_Unknown";

        break;
    }

    return os;
}

#define DEFINE_CONVERT_FUNC(isize, osize) \
    case ConvertDataTypes_##isize##_##osize: \
        this->convert<uint##isize##_t, uint##osize##_t>(fc, \
                                                        frame, \
                                                        fc.outputFrame); \
        \
        if (fc.toEndian != ENDIANNESS_BO) \
            Algorithm::swapDataBytes(reinterpret_cast<uint##osize##_t *>(fc.outputFrame.data()), fc.outputFrame.size()); \
        \
        break;

AkVCam::VideoFrame AkVCam::VideoConverterPrivate::convert(const VideoFrame &frame,
                                                          const VideoFormat &oformat)
{
    static const int maxCacheAlloc = 1 << 16;

    if (this->m_cacheIndex >= this->m_fcSize) {
        static const int cacheBlockSize = 8;
        auto newSize = bound(cacheBlockSize, this->m_cacheIndex + cacheBlockSize, maxCacheAlloc);
        auto fc = new FrameConvertParameters[newSize];

        if (this->m_fc) {
            for (int i = 0; i < this->m_fcSize; ++i)
                fc[i] = this->m_fc[i];

            delete [] this->m_fc;
        }

        this->m_fc = fc;
        this->m_fcSize = newSize;
    }

    if (this->m_cacheIndex >= maxCacheAlloc)
        return {};

    auto &fc = this->m_fc[this->m_cacheIndex];

    if (frame.format() != fc.inputFormat
        || oformat != fc.outputFormat
        || this->m_yuvColorSpace != fc.yuvColorSpace
        || this->m_yuvColorSpaceType != fc.yuvColorSpaceType
        || this->m_scalingMode != fc.scalingMode
        || this->m_aspectRatioMode != fc.aspectRatioMode
        || this->m_inputRect != fc.inputRect) {
        fc.configure(frame.format(),
                     oformat,
                     fc.colorConvert,
                     this->m_yuvColorSpace,
                     this->m_yuvColorSpaceType);
        fc.configureScaling(frame.format(),
                            oformat,
                            this->m_inputRect,
                            this->m_aspectRatioMode);
        fc.inputFormat = frame.format();
        fc.outputFormat = oformat;
        fc.yuvColorSpace = this->m_yuvColorSpace;
        fc.yuvColorSpaceType = this->m_yuvColorSpaceType;
        fc.scalingMode = this->m_scalingMode;
        fc.aspectRatioMode = this->m_aspectRatioMode;
        fc.inputRect = this->m_inputRect;
    }

    if (fc.outputConvertFormat.isSameFormat(frame.format())) {
        this->m_cacheIndex++;

        return frame;
    }

    if (fc.fastConvertion) {
        this->convertFast8bits(fc, frame, fc.outputFrame);
    } else {
        switch (fc.convertDataTypes) {
        DEFINE_CONVERT_FUNC(8 , 8 )
        DEFINE_CONVERT_FUNC(8 , 16)
        DEFINE_CONVERT_FUNC(8 , 32)
        DEFINE_CONVERT_FUNC(16, 8 )
        DEFINE_CONVERT_FUNC(16, 16)
        DEFINE_CONVERT_FUNC(16, 32)
        DEFINE_CONVERT_FUNC(32, 8 )
        DEFINE_CONVERT_FUNC(32, 16)
        DEFINE_CONVERT_FUNC(32, 32)
        }
    }

    this->m_cacheIndex++;

    return fc.outputFrame;
}

AkVCam::FrameConvertParameters::FrameConvertParameters()
{
}

AkVCam::FrameConvertParameters::FrameConvertParameters(const FrameConvertParameters &other):
    inputFormat(other.inputFormat),
    outputFormat(other.outputFormat),
    outputConvertFormat(other.outputConvertFormat),
    outputFrame(other.outputFrame),
    scalingMode(other.scalingMode),
    aspectRatioMode(other.aspectRatioMode),
    convertType(other.convertType),
    convertDataTypes(other.convertDataTypes),
    alphaMode(other.alphaMode),
    resizeMode(other.resizeMode),
    fastConvertion(other.fastConvertion),
    fromEndian(other.fromEndian),
    toEndian(other.toEndian),
    xmin(other.xmin),
    ymin(other.ymin),
    xmax(other.xmax),
    ymax(other.ymax),
    inputWidth(other.inputWidth),
    inputWidth_1(other.inputWidth_1),
    inputHeight(other.inputHeight),
    planeXi(other.planeXi),
    planeYi(other.planeYi),
    planeZi(other.planeZi),
    planeAi(other.planeAi),
    compXi(other.compXi),
    compYi(other.compYi),
    compZi(other.compZi),
    compAi(other.compAi),
    planeXo(other.planeXo),
    planeYo(other.planeYo),
    planeZo(other.planeZo),
    planeAo(other.planeAo),
    compXo(other.compXo),
    compYo(other.compYo),
    compZo(other.compZo),
    compAo(other.compAo),
    xiOffset(other.xiOffset),
    yiOffset(other.yiOffset),
    ziOffset(other.ziOffset),
    aiOffset(other.aiOffset),
    xoOffset(other.xoOffset),
    yoOffset(other.yoOffset),
    zoOffset(other.zoOffset),
    aoOffset(other.aoOffset),
    xiShift(other.xiShift),
    yiShift(other.yiShift),
    ziShift(other.ziShift),
    aiShift(other.aiShift),
    xoShift(other.xoShift),
    yoShift(other.yoShift),
    zoShift(other.zoShift),
    aoShift(other.aoShift),
    maxXi(other.maxXi),
    maxYi(other.maxYi),
    maxZi(other.maxZi),
    maxAi(other.maxAi),
    maskXo(other.maskXo),
    maskYo(other.maskYo),
    maskZo(other.maskZo),
    maskAo(other.maskAo),
    alphaMask(other.alphaMask)
{
    auto oWidth = this->outputFormat.width();
    auto oHeight = this->outputFormat.height();

    size_t oWidthDataSize = sizeof(int) * oWidth;
    size_t oHeightDataSize = sizeof(int) * oHeight;

    if (other.srcWidth) {
        this->srcWidth = new int [oWidth];
        memcpy(this->srcWidth, other.srcWidth, oWidthDataSize);
    }

    if (other.srcWidth_1) {
        this->srcWidth_1 = new int [oWidth];
        memcpy(this->srcWidth_1, other.srcWidth_1, oWidthDataSize);
    }

    if (other.srcWidthOffsetX) {
        this->srcWidthOffsetX = new int [oWidth];
        memcpy(this->srcWidthOffsetX, other.srcWidthOffsetX, oWidthDataSize);
    }

    if (other.srcWidthOffsetY) {
        this->srcWidthOffsetY = new int [oWidth];
        memcpy(this->srcWidthOffsetY, other.srcWidthOffsetY, oWidthDataSize);
    }

    if (other.srcWidthOffsetZ) {
        this->srcWidthOffsetZ = new int [oWidth];
        memcpy(this->srcWidthOffsetZ, other.srcWidthOffsetZ, oWidthDataSize);
    }

    if (other.srcWidthOffsetA) {
        this->srcWidthOffsetA = new int [oWidth];
        memcpy(this->srcWidthOffsetA, other.srcWidthOffsetA, oWidthDataSize);
    }

    if (other.srcHeight) {
        this->srcHeight = new int [oHeight];
        memcpy(this->srcHeight, other.srcHeight, oHeightDataSize);
    }

    auto iWidth = this->inputFormat.width();
    size_t iWidthDataSize = sizeof(int) * iWidth;

    if (other.dlSrcWidthOffsetX) {
        this->dlSrcWidthOffsetX = new int [iWidth];
        memcpy(this->dlSrcWidthOffsetX, other.dlSrcWidthOffsetX, iWidthDataSize);
    }

    if (other.dlSrcWidthOffsetY) {
        this->dlSrcWidthOffsetY = new int [iWidth];
        memcpy(this->dlSrcWidthOffsetY, other.dlSrcWidthOffsetY, iWidthDataSize);
    }

    if (other.dlSrcWidthOffsetZ) {
        this->dlSrcWidthOffsetZ = new int [iWidth];
        memcpy(this->dlSrcWidthOffsetZ, other.dlSrcWidthOffsetZ, iWidthDataSize);
    }

    if (other.dlSrcWidthOffsetA) {
        this->dlSrcWidthOffsetA = new int [iWidth];
        memcpy(this->dlSrcWidthOffsetA, other.dlSrcWidthOffsetA, iWidthDataSize);
    }

    if (other.srcWidthOffsetX_1) {
        this->srcWidthOffsetX_1 = new int [oWidth];
        memcpy(this->srcWidthOffsetX_1, other.srcWidthOffsetX_1, oWidthDataSize);
    }

    if (other.srcWidthOffsetY_1) {
        this->srcWidthOffsetY_1 = new int [oWidth];
        memcpy(this->srcWidthOffsetY_1, other.srcWidthOffsetY_1, oWidthDataSize);
    }

    if (other.srcWidthOffsetZ_1) {
        this->srcWidthOffsetZ_1 = new int [oWidth];
        memcpy(this->srcWidthOffsetZ_1, other.srcWidthOffsetZ_1, oWidthDataSize);
    }

    if (other.srcWidthOffsetA_1) {
        this->srcWidthOffsetA_1 = new int [oWidth];
        memcpy(this->srcWidthOffsetA_1, other.srcWidthOffsetA_1, oWidthDataSize);
    }

    if (other.srcHeight_1) {
        this->srcHeight_1 = new int [oHeight];
        memcpy(this->srcHeight_1, other.srcHeight_1, oHeightDataSize);
    }

    if (other.dstWidthOffsetX) {
        this->dstWidthOffsetX = new int [oWidth];
        memcpy(this->dstWidthOffsetX, other.dstWidthOffsetX, oWidthDataSize);
    }

    if (other.dstWidthOffsetY) {
        this->dstWidthOffsetY = new int [oWidth];
        memcpy(this->dstWidthOffsetY, other.dstWidthOffsetY, oWidthDataSize);
    }

    if (other.dstWidthOffsetZ) {
        this->dstWidthOffsetZ = new int [oWidth];
        memcpy(this->dstWidthOffsetZ, other.dstWidthOffsetZ, oWidthDataSize);
    }

    if (other.dstWidthOffsetA) {
        this->dstWidthOffsetA = new int [oWidth];
        memcpy(this->dstWidthOffsetA, other.dstWidthOffsetA, oWidthDataSize);
    }

    size_t oHeightDlDataSize = sizeof(size_t) * oHeight;

    if (other.srcHeightDlOffset) {
        this->srcHeightDlOffset = new size_t [oHeight];
        memcpy(this->srcHeightDlOffset, other.srcHeightDlOffset, oHeightDlDataSize);
    }

    if (other.srcHeightDlOffset_1) {
        this->srcHeightDlOffset_1 = new size_t [oHeight];
        memcpy(this->srcHeightDlOffset_1, other.srcHeightDlOffset_1, oHeightDlDataSize);
    }

    size_t iWidth_1 = this->inputFormat.width() + 1;
    size_t iHeight_1 = this->inputFormat.height() + 1;
    size_t integralImageSize = iWidth_1 * iHeight_1;
    size_t integralImageDataSize = sizeof(DlSumType) * integralImageSize;

    if (other.integralImageDataX) {
        this->integralImageDataX = new DlSumType [integralImageSize];
        memcpy(this->integralImageDataX, other.integralImageDataX, integralImageDataSize);
    }

    if (other.integralImageDataY) {
        this->integralImageDataY = new DlSumType [integralImageSize];
        memcpy(this->integralImageDataY, other.integralImageDataY, integralImageDataSize);
    }

    if (other.integralImageDataZ) {
        this->integralImageDataZ = new DlSumType [integralImageSize];
        memcpy(this->integralImageDataZ, other.integralImageDataZ, integralImageDataSize);
    }

    if (other.integralImageDataA) {
        this->integralImageDataA = new DlSumType [integralImageSize];
        memcpy(this->integralImageDataA, other.integralImageDataA, integralImageDataSize);
    }

    if (other.kx) {
        this->kx = new int64_t [oWidth];
        memcpy(this->kx, other.kx, sizeof(int64_t) * oWidth);
    }

    if (other.ky) {
        this->ky = new int64_t [oHeight];
        memcpy(this->ky, other.ky, sizeof(int64_t) * oHeight);
    }

    auto kdlSize = size_t(this->inputFormat.width()) * this->inputFormat.height();
    auto kdlDataSize = sizeof(DlSumType) * kdlSize;

    if (other.kdl) {
        this->kdl = new DlSumType [kdlSize];
        memcpy(this->kdl, other.kdl, kdlDataSize);
    }
}

AkVCam::FrameConvertParameters::~FrameConvertParameters()
{
    this->clearBuffers();
    this->clearDlBuffers();
}

AkVCam::FrameConvertParameters &AkVCam::FrameConvertParameters::operator =(const FrameConvertParameters &other)
{
    if (this != &other) {
        this->inputFormat = other.inputFormat;
        this->outputFormat = other.outputFormat;
        this->outputConvertFormat = other.outputConvertFormat;
        this->outputFrame = other.outputFrame;
        this->scalingMode = other.scalingMode;
        this->aspectRatioMode = other.aspectRatioMode;
        this->convertType = other.convertType;
        this->convertDataTypes = other.convertDataTypes;
        this->alphaMode = other.alphaMode;
        this->resizeMode = other.resizeMode;
        this->fastConvertion = other.fastConvertion;
        this->fromEndian = other.fromEndian;
        this->toEndian = other.toEndian;
        this->xmin = other.xmin;
        this->ymin = other.ymin;
        this->xmax = other.xmax;
        this->ymax = other.ymax;
        this->inputWidth = other.inputWidth;
        this->inputWidth_1 = other.inputWidth_1;
        this->inputHeight = other.inputHeight;
        this->planeXi = other.planeXi;
        this->planeYi = other.planeYi;
        this->planeZi = other.planeZi;
        this->planeAi = other.planeAi;
        this->compXi = other.compXi;
        this->compYi = other.compYi;
        this->compZi = other.compZi;
        this->compAi = other.compAi;
        this->planeXo = other.planeXo;
        this->planeYo = other.planeYo;
        this->planeZo = other.planeZo;
        this->planeAo = other.planeAo;
        this->compXo = other.compXo;
        this->compYo = other.compYo;
        this->compZo = other.compZo;
        this->compAo = other.compAo;
        this->xiOffset = other.xiOffset;
        this->yiOffset = other.yiOffset;
        this->ziOffset = other.ziOffset;
        this->aiOffset = other.aiOffset;
        this->xoOffset = other.xoOffset;
        this->yoOffset = other.yoOffset;
        this->zoOffset = other.zoOffset;
        this->aoOffset = other.aoOffset;
        this->xiShift = other.xiShift;
        this->yiShift = other.yiShift;
        this->ziShift = other.ziShift;
        this->aiShift = other.aiShift;
        this->xoShift = other.xoShift;
        this->yoShift = other.yoShift;
        this->zoShift = other.zoShift;
        this->aoShift = other.aoShift;
        this->maxXi = other.maxXi;
        this->maxYi = other.maxYi;
        this->maxZi = other.maxZi;
        this->maxAi = other.maxAi;
        this->maskXo = other.maskXo;
        this->maskYo = other.maskYo;
        this->maskZo = other.maskZo;
        this->maskAo = other.maskAo;
        this->alphaMask = other.alphaMask;

        this->clearBuffers();
        this->clearDlBuffers();

        auto oWidth = this->outputFormat.width();
        auto oHeight = this->outputFormat.height();

        size_t oWidthDataSize = sizeof(int) * oWidth;
        size_t oHeightDataSize = sizeof(int) * oHeight;

        if (other.srcWidth) {
            this->srcWidth = new int [oWidth];
            memcpy(this->srcWidth, other.srcWidth, oWidthDataSize);
        }

        if (other.srcWidth_1) {
            this->srcWidth_1 = new int [oWidth];
            memcpy(this->srcWidth_1, other.srcWidth_1, oWidthDataSize);
        }

        if (other.srcWidthOffsetX) {
            this->srcWidthOffsetX = new int [oWidth];
            memcpy(this->srcWidthOffsetX, other.srcWidthOffsetX, oWidthDataSize);
        }

        if (other.srcWidthOffsetY) {
            this->srcWidthOffsetY = new int [oWidth];
            memcpy(this->srcWidthOffsetY, other.srcWidthOffsetY, oWidthDataSize);
        }

        if (other.srcWidthOffsetZ) {
            this->srcWidthOffsetZ = new int [oWidth];
            memcpy(this->srcWidthOffsetZ, other.srcWidthOffsetZ, oWidthDataSize);
        }

        if (other.srcWidthOffsetA) {
            this->srcWidthOffsetA = new int [oWidth];
            memcpy(this->srcWidthOffsetA, other.srcWidthOffsetA, oWidthDataSize);
        }

        if (other.srcHeight) {
            this->srcHeight = new int [oHeight];
            memcpy(this->srcHeight, other.srcHeight, oHeightDataSize);
        }

        auto iWidth = this->inputFormat.width();
        size_t iWidthDataSize = sizeof(int) * iWidth;

        if (other.dlSrcWidthOffsetX) {
            this->dlSrcWidthOffsetX = new int [iWidth];
            memcpy(this->dlSrcWidthOffsetX, other.dlSrcWidthOffsetX, iWidthDataSize);
        }

        if (other.dlSrcWidthOffsetY) {
            this->dlSrcWidthOffsetY = new int [iWidth];
            memcpy(this->dlSrcWidthOffsetY, other.dlSrcWidthOffsetY, iWidthDataSize);
        }

        if (other.dlSrcWidthOffsetZ) {
            this->dlSrcWidthOffsetZ = new int [iWidth];
            memcpy(this->dlSrcWidthOffsetZ, other.dlSrcWidthOffsetZ, iWidthDataSize);
        }

        if (other.dlSrcWidthOffsetA) {
            this->dlSrcWidthOffsetA = new int [iWidth];
            memcpy(this->dlSrcWidthOffsetA, other.dlSrcWidthOffsetA, iWidthDataSize);
        }

        if (other.srcWidthOffsetX_1) {
            this->srcWidthOffsetX_1 = new int [oWidth];
            memcpy(this->srcWidthOffsetX_1, other.srcWidthOffsetX_1, oWidthDataSize);
        }

        if (other.srcWidthOffsetY_1) {
            this->srcWidthOffsetY_1 = new int [oWidth];
            memcpy(this->srcWidthOffsetY_1, other.srcWidthOffsetY_1, oWidthDataSize);
        }

        if (other.srcWidthOffsetZ_1) {
            this->srcWidthOffsetZ_1 = new int [oWidth];
            memcpy(this->srcWidthOffsetZ_1, other.srcWidthOffsetZ_1, oWidthDataSize);
        }

        if (other.srcWidthOffsetA_1) {
            this->srcWidthOffsetA_1 = new int [oWidth];
            memcpy(this->srcWidthOffsetA_1, other.srcWidthOffsetA_1, oWidthDataSize);
        }

        if (other.srcHeight_1) {
            this->srcHeight_1 = new int [oHeight];
            memcpy(this->srcHeight_1, other.srcHeight_1, oHeightDataSize);
        }

        if (other.dstWidthOffsetX) {
            this->dstWidthOffsetX = new int [oWidth];
            memcpy(this->dstWidthOffsetX, other.dstWidthOffsetX, oWidthDataSize);
        }

        if (other.dstWidthOffsetY) {
            this->dstWidthOffsetY = new int [oWidth];
            memcpy(this->dstWidthOffsetY, other.dstWidthOffsetY, oWidthDataSize);
        }

        if (other.dstWidthOffsetZ) {
            this->dstWidthOffsetZ = new int [oWidth];
            memcpy(this->dstWidthOffsetZ, other.dstWidthOffsetZ, oWidthDataSize);
        }

        if (other.dstWidthOffsetA) {
            this->dstWidthOffsetA = new int [oWidth];
            memcpy(this->dstWidthOffsetA, other.dstWidthOffsetA, oWidthDataSize);
        }

        size_t oHeightDlDataSize = sizeof(size_t) * oHeight;

        if (other.srcHeightDlOffset) {
            this->srcHeightDlOffset = new size_t [oHeight];
            memcpy(this->srcHeightDlOffset, other.srcHeightDlOffset, oHeightDlDataSize);
        }

        if (other.srcHeightDlOffset_1) {
            this->srcHeightDlOffset_1 = new size_t [oHeight];
            memcpy(this->srcHeightDlOffset_1, other.srcHeightDlOffset_1, oHeightDlDataSize);
        }

        size_t iWidth_1 = this->inputFormat.width() + 1;
        size_t iHeight_1 = this->inputFormat.height() + 1;
        size_t integralImageSize = iWidth_1 * iHeight_1;
        size_t integralImageDataSize = sizeof(DlSumType) * integralImageSize;

        if (other.integralImageDataX) {
            this->integralImageDataX = new DlSumType [integralImageSize];
            memcpy(this->integralImageDataX, other.integralImageDataX, integralImageDataSize);
        }

        if (other.integralImageDataY) {
            this->integralImageDataY = new DlSumType [integralImageSize];
            memcpy(this->integralImageDataY, other.integralImageDataY, integralImageDataSize);
        }

        if (other.integralImageDataZ) {
            this->integralImageDataZ = new DlSumType [integralImageSize];
            memcpy(this->integralImageDataZ, other.integralImageDataZ, integralImageDataSize);
        }

        if (other.integralImageDataA) {
            this->integralImageDataA = new DlSumType [integralImageSize];
            memcpy(this->integralImageDataA, other.integralImageDataA, integralImageDataSize);
        }

        if (other.kx) {
            this->kx = new int64_t [oWidth];
            memcpy(this->kx, other.kx, sizeof(int64_t) * oWidth);
        }

        if (other.ky) {
            this->ky = new int64_t [oHeight];
            memcpy(this->ky, other.ky, sizeof(int64_t) * oHeight);
        }

        auto kdlSize = size_t(this->inputFormat.width()) * this->inputFormat.height();
        auto kdlDataSize = sizeof(DlSumType) * kdlSize;

        if (other.kdl) {
            this->kdl = new DlSumType [kdlSize];
            memcpy(this->kdl, other.kdl, kdlDataSize);
        }
    }

    return *this;
}

void AkVCam::FrameConvertParameters::clearBuffers()
{
    if (this->srcWidth) {
        delete [] this->srcWidth;
        this->srcWidth = nullptr;
    }

    if (this->srcWidth_1) {
        delete [] this->srcWidth_1;
        this->srcWidth_1 = nullptr;
    }

    if (this->srcWidthOffsetX) {
        delete [] this->srcWidthOffsetX;
        this->srcWidthOffsetX = nullptr;
    }

    if (this->srcWidthOffsetY) {
        delete [] this->srcWidthOffsetY;
        this->srcWidthOffsetY = nullptr;
    }

    if (this->srcWidthOffsetZ) {
        delete [] this->srcWidthOffsetZ;
        this->srcWidthOffsetZ = nullptr;
    }

    if (this->srcWidthOffsetA) {
        delete [] this->srcWidthOffsetA;
        this->srcWidthOffsetA = nullptr;
    }

    if (this->srcHeight) {
        delete [] this->srcHeight;
        this->srcHeight = nullptr;
    }

    if (this->srcWidthOffsetX_1) {
        delete [] this->srcWidthOffsetX_1;
        this->srcWidthOffsetX_1 = nullptr;
    }

    if (this->srcWidthOffsetY_1) {
        delete [] this->srcWidthOffsetY_1;
        this->srcWidthOffsetY_1 = nullptr;
    }

    if (this->srcWidthOffsetZ_1) {
        delete [] this->srcWidthOffsetZ_1;
        this->srcWidthOffsetZ_1 = nullptr;
    }

    if (this->srcWidthOffsetA_1) {
        delete [] this->srcWidthOffsetA_1;
        this->srcWidthOffsetA_1 = nullptr;
    }

    if (this->srcHeight_1) {
        delete [] this->srcHeight_1;
        this->srcHeight_1 = nullptr;
    }

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

    if (this->kx) {
        delete [] this->kx;
        this->kx = nullptr;
    }

    if (this->ky) {
        delete [] this->ky;
        this->ky = nullptr;
    }
}

void AkVCam::FrameConvertParameters::clearDlBuffers()
{
    if (this->integralImageDataX) {
        delete [] this->integralImageDataX;
        this->integralImageDataX = nullptr;
    }

    if (this->integralImageDataY) {
        delete [] this->integralImageDataY;
        this->integralImageDataY = nullptr;
    }

    if (this->integralImageDataZ) {
        delete [] this->integralImageDataZ;
        this->integralImageDataZ = nullptr;
    }

    if (this->integralImageDataA) {
        delete [] this->integralImageDataA;
        this->integralImageDataA = nullptr;
    }

    if (this->kdl) {
        delete [] this->kdl;
        this->kdl = nullptr;
    }

    if (this->srcHeightDlOffset) {
        delete [] this->srcHeightDlOffset;
        this->srcHeightDlOffset = nullptr;
    }

    if (this->srcHeightDlOffset_1) {
        delete [] this->srcHeightDlOffset_1;
        this->srcHeightDlOffset_1 = nullptr;
    }

    if (this->dlSrcWidthOffsetX) {
        delete [] this->dlSrcWidthOffsetX;
        this->dlSrcWidthOffsetX = nullptr;
    }

    if (this->dlSrcWidthOffsetY) {
        delete [] this->dlSrcWidthOffsetY;
        this->dlSrcWidthOffsetY = nullptr;
    }

    if (this->dlSrcWidthOffsetZ) {
        delete [] this->dlSrcWidthOffsetZ;
        this->dlSrcWidthOffsetZ = nullptr;
    }

    if (this->dlSrcWidthOffsetA) {
        delete [] this->dlSrcWidthOffsetA;
        this->dlSrcWidthOffsetA = nullptr;
    }
}

void AkVCam::FrameConvertParameters::allocateBuffers(const VideoFormat &oformat)
{
    this->clearBuffers();

    this->srcWidth = new int [oformat.width()];
    this->srcWidth_1 = new int [oformat.width()];
    this->srcWidthOffsetX = new int [oformat.width()];
    this->srcWidthOffsetY = new int [oformat.width()];
    this->srcWidthOffsetZ = new int [oformat.width()];
    this->srcWidthOffsetA = new int [oformat.width()];
    this->srcHeight = new int [oformat.height()];

    this->srcWidthOffsetX_1 = new int [oformat.width()];
    this->srcWidthOffsetY_1 = new int [oformat.width()];
    this->srcWidthOffsetZ_1 = new int [oformat.width()];
    this->srcWidthOffsetA_1 = new int [oformat.width()];
    this->srcHeight_1 = new int [oformat.height()];

    this->dstWidthOffsetX = new int [oformat.width()];
    this->dstWidthOffsetY = new int [oformat.width()];
    this->dstWidthOffsetZ = new int [oformat.width()];
    this->dstWidthOffsetA = new int [oformat.width()];

    this->kx = new int64_t [oformat.width()];
    this->ky = new int64_t [oformat.height()];
}

void AkVCam::FrameConvertParameters::allocateDlBuffers(const VideoFormat &iformat,
                                                       const VideoFormat &oformat)
{
    size_t width_1 = iformat.width() + 1;
    size_t height_1 = iformat.height() + 1;
    auto integralImageSize = width_1 * height_1;

    this->integralImageDataX = new DlSumType [integralImageSize];
    this->integralImageDataY = new DlSumType [integralImageSize];
    this->integralImageDataZ = new DlSumType [integralImageSize];
    this->integralImageDataA = new DlSumType [integralImageSize];
    memset(this->integralImageDataX, 0, integralImageSize);
    memset(this->integralImageDataY, 0, integralImageSize);
    memset(this->integralImageDataZ, 0, integralImageSize);
    memset(this->integralImageDataA, 0, integralImageSize);

    auto kdlSize = size_t(iformat.width()) * iformat.height();
    this->kdl = new DlSumType [kdlSize];
    memset(this->kdl, 0, kdlSize);

    this->srcHeightDlOffset = new size_t [oformat.height()];
    this->srcHeightDlOffset_1 = new size_t [oformat.height()];

    this->dlSrcWidthOffsetX = new int [iformat.width()];
    this->dlSrcWidthOffsetY = new int [iformat.width()];
    this->dlSrcWidthOffsetZ = new int [iformat.width()];
    this->dlSrcWidthOffsetA = new int [iformat.width()];
}

#define DEFINE_CONVERT_TYPES(isize, osize) \
    if (ispecs.depth() == isize && ospecs.depth() == osize) \
        this->convertDataTypes = ConvertDataTypes_##isize##_##osize;

void AkVCam::FrameConvertParameters::configure(const VideoFormat &iformat,
                                               const VideoFormat &oformat,
                                               ColorConvert &colorConvert,
                                               ColorConvert::YuvColorSpace yuvColorSpace,
                                               ColorConvert::YuvColorSpaceType yuvColorSpaceType)
{
    auto ispecs = VideoFormat::formatSpecs(iformat.format());
    auto oFormat = oformat.format();

    if (oFormat == PixelFormat_none)
        oFormat = iformat.format();

    auto ospecs = VideoFormat::formatSpecs(oFormat);

    DEFINE_CONVERT_TYPES(8, 8);
    DEFINE_CONVERT_TYPES(8, 16);
    DEFINE_CONVERT_TYPES(8, 32);
    DEFINE_CONVERT_TYPES(16, 8);
    DEFINE_CONVERT_TYPES(16, 16);
    DEFINE_CONVERT_TYPES(16, 32);
    DEFINE_CONVERT_TYPES(32, 8);
    DEFINE_CONVERT_TYPES(32, 16);
    DEFINE_CONVERT_TYPES(32, 32);

    auto icomponents = ispecs.mainComponents();
    auto ocomponents = ospecs.mainComponents();

    if (icomponents == 3 && ispecs.type() == ospecs.type())
        this->convertType = ConvertType_Vector;
    else if (icomponents == 3 && ocomponents == 3)
        this->convertType = ConvertType_3to3;
    else if (icomponents == 3 && ocomponents == 1)
        this->convertType = ConvertType_3to1;
    else if (icomponents == 1 && ocomponents == 3)
        this->convertType = ConvertType_1to3;
    else if (icomponents == 1 && ocomponents == 1)
        this->convertType = ConvertType_1to1;

    this->fromEndian = ispecs.endianness();
    this->toEndian = ospecs.endianness();
    colorConvert.setYuvColorSpace(yuvColorSpace);
    colorConvert.setYuvColorSpaceType(yuvColorSpaceType);
    colorConvert.loadMatrix(ispecs, ospecs);

    switch (ispecs.type()) {
    case VideoFormatSpec::VFT_RGB:
        this->planeXi = ispecs.componentPlane(ColorComponent::CT_R);
        this->planeYi = ispecs.componentPlane(ColorComponent::CT_G);
        this->planeZi = ispecs.componentPlane(ColorComponent::CT_B);

        this->compXi = ispecs.component(ColorComponent::CT_R);
        this->compYi = ispecs.component(ColorComponent::CT_G);
        this->compZi = ispecs.component(ColorComponent::CT_B);

        break;

    case VideoFormatSpec::VFT_YUV:
        this->planeXi = ispecs.componentPlane(ColorComponent::CT_Y);
        this->planeYi = ispecs.componentPlane(ColorComponent::CT_U);
        this->planeZi = ispecs.componentPlane(ColorComponent::CT_V);

        this->compXi = ispecs.component(ColorComponent::CT_Y);
        this->compYi = ispecs.component(ColorComponent::CT_U);
        this->compZi = ispecs.component(ColorComponent::CT_V);

        break;

    case VideoFormatSpec::VFT_Gray:
        this->planeXi = ispecs.componentPlane(ColorComponent::CT_Y);
        this->compXi = ispecs.component(ColorComponent::CT_Y);

        break;

    default:
        break;
    }

    this->planeAi = ispecs.componentPlane(ColorComponent::CT_A);
    this->compAi = ispecs.component(ColorComponent::CT_A);

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

    this->xiOffset = this->compXi.offset();
    this->yiOffset = this->compYi.offset();
    this->ziOffset = this->compZi.offset();
    this->aiOffset = this->compAi.offset();

    this->xoOffset = this->compXo.offset();
    this->yoOffset = this->compYo.offset();
    this->zoOffset = this->compZo.offset();
    this->aoOffset = this->compAo.offset();

    this->xiShift = this->compXi.shift();
    this->yiShift = this->compYi.shift();
    this->ziShift = this->compZi.shift();
    this->aiShift = this->compAi.shift();

    this->xoShift = this->compXo.shift();
    this->yoShift = this->compYo.shift();
    this->zoShift = this->compZo.shift();
    this->aoShift = this->compAo.shift();

    this->maxXi = this->compXi.max<uint64_t>();
    this->maxYi = this->compYi.max<uint64_t>();
    this->maxZi = this->compZi.max<uint64_t>();
    this->maxAi = this->compAi.max<uint64_t>();

    this->maskXo = ~(this->compXo.max<uint64_t>() << this->compXo.shift());
    this->maskYo = ~(this->compYo.max<uint64_t>() << this->compYo.shift());
    this->maskZo = ~(this->compZo.max<uint64_t>() << this->compZo.shift());
    this->alphaMask = this->compAo.max<uint64_t>() << this->compAo.shift();
    this->maskAo = ~this->alphaMask;

    bool hasAlphaIn = ispecs.contains(ColorComponent::CT_A);
    bool hasAlphaOut = ospecs.contains(ColorComponent::CT_A);

    if (hasAlphaIn && hasAlphaOut)
        this->alphaMode = ConvertAlphaMode_AI_AO;
    else if (hasAlphaIn && !hasAlphaOut)
        this->alphaMode = ConvertAlphaMode_AI_O;
    else if (!hasAlphaIn && hasAlphaOut)
        this->alphaMode = ConvertAlphaMode_I_AO;
    else if (!hasAlphaIn && !hasAlphaOut)
        this->alphaMode = ConvertAlphaMode_I_O;

    this->fastConvertion = ispecs.isFast() && ospecs.isFast();
}

void AkVCam::FrameConvertParameters::configureScaling(const VideoFormat &iformat,
                                                      const VideoFormat &oformat,
                                                      const Rect &inputRect,
                                                      VideoConverter::AspectRatioMode aspectRatioMode)
{
    Rect irect(0, 0, iformat.width(), iformat.height());

    if (!inputRect.isEmpty())
        irect = irect.intersected(inputRect);

    this->outputConvertFormat = oformat;

    if (this->outputConvertFormat.format() == PixelFormat_none)
        this->outputConvertFormat.setFormat(iformat.format());

    int width = this->outputConvertFormat.width() > 1?
                    this->outputConvertFormat.width():
                    irect.width();
    int height = this->outputConvertFormat.height() > 1?
                     this->outputConvertFormat.height():
                     irect.height();
    int owidth = width;
    int oheight = height;

    if (aspectRatioMode == AkVCam::VideoConverter::AspectRatioMode_Keep
        || aspectRatioMode == AkVCam::VideoConverter::AspectRatioMode_Fit) {
        auto w = height * irect.width() / irect.height();
        auto h = width * irect.height() / irect.width();

        if (w > width)
            w = width;
        else if (h > height)
            h = height;

        owidth = w;
        oheight = h;

        if (aspectRatioMode == AkVCam::VideoConverter::AspectRatioMode_Keep) {
            width = owidth;
            height = oheight;
        }
    }

    this->outputConvertFormat.setWidth(width);
    this->outputConvertFormat.setHeight(height);
    this->outputConvertFormat.setFps(iformat.fps());

    this->xmin = (width - owidth) / 2;
    this->ymin = (height - oheight) / 2;
    this->xmax = (width + owidth) / 2;
    this->ymax = (height + oheight) / 2;

    if (owidth > irect.width()
        || oheight > irect.height())
        this->resizeMode = ResizeMode_Up;
    else if (owidth < irect.width()
             || oheight < irect.height())
        this->resizeMode = ResizeMode_Down;
    else
        this->resizeMode = ResizeMode_Keep;

    if (aspectRatioMode == AkVCam::VideoConverter::AspectRatioMode_Expanding) {
        auto w = irect.height() * owidth / oheight;
        auto h = irect.width() * oheight / owidth;

        if (w > irect.width())
            w = irect.width();

        if (h > irect.height())
            h = irect.height();

        auto x = (irect.x() + irect.width() - w) / 2;
        auto y = (irect.y() + irect.height() - h) / 2;
        irect = {x, y, w, h};
    }

    this->allocateBuffers(this->outputConvertFormat);

    auto &xomin = this->xmin;

    int wi_1 = std::max(1, irect.width() - 1);
    int wo_1 = std::max(1, owidth - 1);

    auto xSrcToDst = [&irect, &xomin, &wi_1, &wo_1] (int x) -> int {
        return ((x - irect.x()) * wo_1 + xomin * wi_1) / wi_1;
    };

    auto xDstToSrc = [&irect, &xomin, &wi_1, &wo_1] (int x) -> int {
        return ((x - xomin) * wi_1 + irect.x() * wo_1) / wo_1;
    };

    for (int x = 0; x < this->outputConvertFormat.width(); ++x) {
        auto xs = xDstToSrc(x);
        auto xs_1 = xDstToSrc(std::min(x + 1, this->outputConvertFormat.width() - 1));
        auto xmin = xSrcToDst(xs);
        auto xmax = xSrcToDst(xs + 1);

        this->srcWidth[x]   = xs;
        this->srcWidth_1[x] = std::min(xDstToSrc(x + 1), iformat.width());
        this->srcWidthOffsetX[x] = (xs >> this->compXi.widthDiv()) * this->compXi.step();
        this->srcWidthOffsetY[x] = (xs >> this->compYi.widthDiv()) * this->compYi.step();
        this->srcWidthOffsetZ[x] = (xs >> this->compZi.widthDiv()) * this->compZi.step();
        this->srcWidthOffsetA[x] = (xs >> this->compAi.widthDiv()) * this->compAi.step();

        this->srcWidthOffsetX_1[x] = (xs_1 >> this->compXi.widthDiv()) * this->compXi.step();
        this->srcWidthOffsetY_1[x] = (xs_1 >> this->compYi.widthDiv()) * this->compYi.step();
        this->srcWidthOffsetZ_1[x] = (xs_1 >> this->compZi.widthDiv()) * this->compZi.step();
        this->srcWidthOffsetA_1[x] = (xs_1 >> this->compAi.widthDiv()) * this->compAi.step();

        this->dstWidthOffsetX[x] = (x >> this->compXo.widthDiv()) * this->compXo.step();
        this->dstWidthOffsetY[x] = (x >> this->compYo.widthDiv()) * this->compYo.step();
        this->dstWidthOffsetZ[x] = (x >> this->compZo.widthDiv()) * this->compZo.step();
        this->dstWidthOffsetA[x] = (x >> this->compAo.widthDiv()) * this->compAo.step();

        if (xmax > xmin)
            this->kx[x] = SCALE_EMULT * (x - xmin) / (xmax - xmin);
        else
            this->kx[x] = 0;
    }

    auto &yomin = this->ymin;

    int hi_1 = std::max(1, irect.height() - 1);
    int ho_1 = std::max(1, oheight - 1);

    auto ySrcToDst = [&irect, &yomin, &hi_1, &ho_1] (int y) -> int {
        return ((y - irect.y()) * ho_1 + yomin * hi_1) / hi_1;
    };

    auto yDstToSrc = [&irect, &yomin, &hi_1, &ho_1] (int y) -> int {
        return ((y - yomin) * hi_1 + irect.y() * ho_1) / ho_1;
    };

    for (int y = 0; y < this->outputConvertFormat.height(); ++y) {
        if (this->resizeMode == ResizeMode_Down) {
            this->srcHeight[y] = yDstToSrc(y);
            this->srcHeight_1[y] = std::min(yDstToSrc(y + 1), iformat.height());
        } else {
            auto ys = yDstToSrc(y);
            auto ys_1 = yDstToSrc(std::min(y + 1, this->outputConvertFormat.height() - 1));
            auto ymin = ySrcToDst(ys);
            auto ymax = ySrcToDst(ys + 1);

            this->srcHeight[y] = ys;
            this->srcHeight_1[y] = ys_1;

            if (ymax > ymin)
                this->ky[y] = SCALE_EMULT * (y - ymin) / (ymax - ymin);
            else
                this->ky[y] = 0;
        }
    }

    this->inputWidth = iformat.width();
    this->inputWidth_1 = iformat.width() + 1;
    this->inputHeight = iformat.height();

    this->clearDlBuffers();

    if (this->resizeMode == ResizeMode_Down) {
        this->allocateDlBuffers(iformat, this->outputConvertFormat);

        for (int x = 0; x < iformat.width(); ++x) {
            this->dlSrcWidthOffsetX[x] = (x >> this->compXi.widthDiv()) * this->compXi.step();
            this->dlSrcWidthOffsetY[x] = (x >> this->compYi.widthDiv()) * this->compYi.step();
            this->dlSrcWidthOffsetZ[x] = (x >> this->compZi.widthDiv()) * this->compZi.step();
            this->dlSrcWidthOffsetA[x] = (x >> this->compAi.widthDiv()) * this->compAi.step();
        }

        for (int y = 0; y < this->outputConvertFormat.height(); ++y) {
            auto &ys = this->srcHeight[y];
            auto &ys_1 = this->srcHeight_1[y];

            this->srcHeightDlOffset[y] = size_t(ys) * this->inputWidth_1;
            this->srcHeightDlOffset_1[y] = size_t(ys_1) * this->inputWidth_1;

            auto diffY = ys_1 - ys;
            auto line = this->kdl + size_t(y) * iformat.width();

            for (int x = 0; x < this->outputConvertFormat.width(); ++x) {
                auto diffX = this->srcWidth_1[x] - this->srcWidth[x];
                line[x] = diffX * diffY;
            }
        }
    }

    this->outputFrame = {this->outputConvertFormat};

    if (aspectRatioMode == AkVCam::VideoConverter::AspectRatioMode_Fit)
        this->outputFrame.fillRgb(Color::rgb(0, 0, 0, 0));
}

void AkVCam::FrameConvertParameters::reset()
{
    this->inputFormat = VideoFormat();
    this->outputFormat = VideoFormat();
    this->outputConvertFormat = VideoFormat();
    this->outputFrame = VideoFrame();
    this->scalingMode = AkVCam::VideoConverter::ScalingMode_Fast;
    this->aspectRatioMode = AkVCam::VideoConverter::AspectRatioMode_Ignore;
    this->convertType = ConvertType_Vector;
    this->convertDataTypes = ConvertDataTypes_8_8;
    this->alphaMode = ConvertAlphaMode_AI_AO;
    this->resizeMode = ResizeMode_Keep;
    this->fastConvertion = false;

    this->fromEndian = ENDIANNESS_BO;
    this->toEndian = ENDIANNESS_BO;

    this->clearBuffers();
    this->clearDlBuffers();

    this->xmin = 0;
    this->ymin = 0;
    this->xmax = 0;
    this->ymax = 0;

    this->inputWidth = 0;
    this->inputWidth_1 = 0;
    this->inputHeight = 0;

    this->planeXi = 0;
    this->planeYi = 0;
    this->planeZi = 0;
    this->planeAi = 0;

    this->compXi = {};
    this->compYi = {};
    this->compZi = {};
    this->compAi = {};

    this->planeXo = 0;
    this->planeYo = 0;
    this->planeZo = 0;
    this->planeAo = 0;

    this->compXo = {};
    this->compYo = {};
    this->compZo = {};
    this->compAo = {};

    this->xiOffset = 0;
    this->yiOffset = 0;
    this->ziOffset = 0;
    this->aiOffset = 0;

    this->xoOffset = 0;
    this->yoOffset = 0;
    this->zoOffset = 0;
    this->aoOffset = 0;

    this->xiShift = 0;
    this->yiShift = 0;
    this->ziShift = 0;
    this->aiShift = 0;

    this->xoShift = 0;
    this->yoShift = 0;
    this->zoShift = 0;
    this->aoShift = 0;

    this->maxXi = 0;
    this->maxYi = 0;
    this->maxZi = 0;
    this->maxAi = 0;

    this->maskXo = 0;
    this->maskYo = 0;
    this->maskZo = 0;
    this->maskAo = 0;

    this->alphaMask = 0;
}
