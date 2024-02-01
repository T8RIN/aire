//
// Created by Radzivon Bartoshyk on 31/01/2024.
//

#include "BilateralBlur.h"
#include <vector>
#include "MathUtils.hpp"
#include <thread>

using namespace std;

namespace aire {
    template<class V>
    void
    bilateralBlurPass(V *data, V* transient, int y, int stride, int width, int height, float radius,
                      float rangeSigma,
                      float spatialSigma) {
        const float dRangeSigma = 2.f * rangeSigma * rangeSigma;
        const float dSpatialSigma = 2.f * spatialSigma * spatialSigma;

        float rStore = 0.f;
        float gStore = 0.f;
        float bStore = 0.f;
        float aStore = 0.f;
        const float lumaPrimaries[3] = {0.2126, 0.7152, 0.0722};
        const int iRadius = ceil(radius);
        auto src = reinterpret_cast<V *>(reinterpret_cast<uint8_t *>(data) + y * stride);
        auto dst = reinterpret_cast<V *>(reinterpret_cast<uint8_t *>(transient) +
                                         y * stride);
        for (int x = 0; x < width; ++x) {

            rStore = 0.f;
            gStore = 0.f;
            bStore = 0.f;
            aStore = 0.f;

            int currentPixelPosition = x*4;

            V currentR = src[currentPixelPosition];
            V currentG = src[currentPixelPosition + 1];
            V currentB = src[currentPixelPosition + 2];

//            float currentIntensity = (src[x] + src[x + 1] + src[x + 2]) / 3.f;
            float kernelSumR = 0.f;
            float kernelSumG = 0.f;
            float kernelSumB = 0.f;

            for (int j = -iRadius; j <= iRadius; ++j) {
                for (int i = -iRadius; i <= iRadius; ++i) {
                    int px = clamp(x + i, 0, width - 1);
                    int py = clamp(y + j, 0, height - 1);
                    auto mSrc = reinterpret_cast<V *>(reinterpret_cast<uint8_t *>(data) + py * stride);
                    float dx = (float(px) - float(x));
                    float dy = (float(py) - float(y));
                    float distance = std::sqrt(dx * dx + dy * dy);

                    int srcX = px * 4;

                    float drIntensity = mSrc[srcX] - currentR;
                    float weightR = std::exp(
                            -distance / dSpatialSigma -
                            (drIntensity * drIntensity) / dRangeSigma);

                    float dgIntensity = mSrc[srcX + 1] - currentG;
                    float weightG = std::exp(
                            -distance / dSpatialSigma -
                            (dgIntensity * dgIntensity) / dRangeSigma);

                    float dbIntensity = mSrc[srcX + 1] - currentB;
                    float weightB = std::exp(
                            -distance / dSpatialSigma -
                            (dbIntensity * dbIntensity) / dRangeSigma);

                    kernelSumR += weightR;
                    kernelSumG += weightG;
                    kernelSumB += weightB;

                    rStore += mSrc[srcX] * weightR;
                    gStore += mSrc[srcX + 1] * weightG;
                    bStore += mSrc[srcX + 2] * weightB;
                }
            }

            dst[0] = ceil(rStore / kernelSumR);
            dst[1] = ceil(gStore / kernelSumG);
            dst[2] = ceil(bStore / kernelSumB);
            dst[3] = src[currentPixelPosition + 3];

            dst += 4;
        }
    }

    template<class V>
    void bilateralBlur(V *data, int stride, int width, int height, float radius, float rangeSigma,
                       float spatialSigma) {
        int threadCount = clamp(min(static_cast<int>(std::thread::hardware_concurrency()),
                                    height * width / (256 * 256)), 1, 12);
        vector<thread> workers;

        int segmentHeight = height / threadCount;

        std::vector<V> transient(stride * height);

        for (int i = 0; i < threadCount; i++) {
            int start = i * segmentHeight;
            int end = (i + 1) * segmentHeight;
            if (i == threadCount - 1) {
                end = height;
            }
            workers.emplace_back(
                    [start, end, width, height, stride, data, radius, rangeSigma, spatialSigma, &transient]() {
                        for (int y = start; y < end; ++y) {
                            bilateralBlurPass(data, transient.data(), y,
                                              stride, width, height, radius, rangeSigma,
                                              spatialSigma);
                        }
                    });
        }

        for (std::thread &thread: workers) {
            thread.join();
        }

        std::copy(transient.begin(), transient.end(), data);
    }

    template void
    bilateralBlur(uint8_t *data, int stride, int width, int height, float radius, float sigma,
                  float spatialSigma);

    template void
    bilateralBlur(uint16_t *data, int stride, int width, int height, float radius, float sigma,
                  float spatialSigma);
}