//
// Created by Radzivon Bartoshyk on 02/02/2024.
//

#include "WindStagger.h"
#include <algorithm>
#include <random>
#include "scale/sampler.h"
#include "scale/sampler-inl.h"
#include "hwy/highway.h"
#include "algo/support-inl.h"
#include "blur/ShgStackBlur.h"
#include "base/Arithmetics.h"
#include "algo/MathUtils.hpp"

using namespace std;

namespace aire {

    using namespace hwy;
    using namespace hwy::HWY_NAMESPACE;

    void horizontalWindStagger(uint8_t *data, uint8_t *source, int stride, int width, int height,
                               float windStrength, int streamsCount, int clearColor) {
        int staggerWidth = width * abs(windStrength);
        const FixedTag<uint8_t, 4> du8;
        const FixedTag<float32_t, 4> dfx4;
        using VU = Vec<decltype(du8)>;
        using VF = Vec<decltype(dfx4)>;

        const FixedTag<uint32_t, 1> du32x1;
        const FixedTag<uint32_t, 4> du32x4;

        fillSurface(data, clearColor, stride, width, height);

        int streamSize = float(height) / float(streamsCount);

        std::default_random_engine generator;
        std::uniform_int_distribution<> wind(width * 0.0001f, clamp(staggerWidth, 0, width - 1));
        int lastY = 0;
        int passedY = 0;
        vector<float> kernel = compute1DGaussianKernel(streamSize, 1.0f);
        for (int stream = 0; stream < streamsCount; ++stream) {
            int fullStreamLength = wind(generator);
            for (int y = passedY, streamHeight = 0;
                 streamHeight < streamSize && y < height; ++y, ++streamHeight) {
                auto dst = reinterpret_cast<uint8_t *>(
                        reinterpret_cast<uint8_t *>(data) + y * stride);
                passedY += 1;
                lastY = y;

                int usePX = streamHeight;
                if (streamHeight >= kernel.size()) {
                    usePX = kernel.size() - 1;
                }

                float part = kernel[usePX];
                int streamLength = part * fullStreamLength;

                float scale = width / float(width - streamLength);

                if (windStrength >= 0) {
                    for (int x = 0; x < clamp(width - streamLength, 0, width); ++x) {

                        float srcX = (float) x * scale;
                        float srcY = (float) y;

                        int x1 = static_cast<int>(srcX);
                        int y1 = static_cast<int>(srcY);

                        int x2 = min(x1 + 1, width - 1);
                        int y2 = min(y1 + 1, height - 1);

                        float dx = max((float) srcX - (float) x1, 0.0f);
                        float dy = max((float) srcY - (float) y1, 0.0f);

                        auto row1 = reinterpret_cast<const uint8_t *>(
                                reinterpret_cast<uint8_t *>(source) + y1 * stride);
                        auto row2 = reinterpret_cast<const uint8_t *>(
                                reinterpret_cast<uint8_t *>(source) + y2 * stride);

                        auto c1 = ConvertToFloat(dfx4, LoadU(du8, &row1[x1 * 4]));;
                        auto c2 = ConvertToFloat(dfx4, LoadU(du8, &row1[x2 * 4]));
                        auto c3 = ConvertToFloat(dfx4, LoadU(du8, &row2[x1 * 4]));
                        auto c4 = ConvertToFloat(dfx4, LoadU(du8, &row2[x2 * 4]));

                        auto result = Blerp(dfx4, c1, c2, c3, c4, Set(dfx4, dx), Set(dfx4, dy));
                        auto px = DemoteToU8(du8, result);
                        StoreU(px, du8, reinterpret_cast<uint8_t *>(&dst[x * 4]));
                    }
                } else {
                    for (int x = streamLength; x < width; ++x) {

                        float srcX = (float) x * scale;
                        float srcY = (float) y;

                        int x1 = clamp(static_cast<int>(srcX), 0, width - 1);
                        int y1 = clamp(static_cast<int>(srcY), 0, height - 1);

                        int x2 = clamp(x1 + 1, 0, width - 1);
                        int y2 = clamp(y1 + 1, 0, height - 1);

                        float dx = max((float) srcX - (float) x1, 0.0f);
                        float dy = max((float) srcY - (float) y1, 0.0f);

                        auto row1 = reinterpret_cast<const uint8_t *>(
                                reinterpret_cast<uint8_t *>(source) + y1 * stride);
                        auto row2 = reinterpret_cast<const uint8_t *>(
                                reinterpret_cast<uint8_t *>(source) + y2 * stride);

                        auto c1 = ConvertToFloat(dfx4, LoadU(du8, &row1[x1 * 4]));;
                        auto c2 = ConvertToFloat(dfx4, LoadU(du8, &row1[x2 * 4]));
                        auto c3 = ConvertToFloat(dfx4, LoadU(du8, &row2[x1 * 4]));
                        auto c4 = ConvertToFloat(dfx4, LoadU(du8, &row2[x2 * 4]));

                        auto result = Blerp(dfx4, c1, c2, c3, c4, Set(dfx4, dx), Set(dfx4, dy));
                        auto px = DemoteToU8(du8, result);
                        StoreU(px, du8, reinterpret_cast<uint8_t *>(&dst[x * 4]));
                    }
                }
            }
        }
        for (int y = lastY; y < height; ++y) {
            auto dst = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(data) +
                                                    y * stride);
            auto src = reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(source) +
                                                    y * stride);
            int x = 0;
            for (; x + 4 < width; x += 4) {
                StoreU(LoadU(du32x4, src), du32x4, dst);
                src += 4;
                dst += 4;
            }
            for (; x < width; ++x) {
                StoreU(LoadU(du32x1, src), du32x1, dst);
                src += 1;
                dst += 1;
            }
        }
    }

}