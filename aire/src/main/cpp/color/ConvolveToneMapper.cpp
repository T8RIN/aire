//
// Created by Radzivon Bartoshyk on 04/02/2024.
//

#include "ConvolveToneMapper.h"
#include "hwy/highway.h"
#include "color/eotf-inl.h"
#include "tone/LogarithmicToneMapper.hpp"
#include "tone/AcesFilmicToneMapper.hpp"
#include "algo/support-inl.h"

namespace aire {

    using namespace hwy;
    using namespace hwy::HWY_NAMESPACE;
    using namespace aire::HWY_NAMESPACE;

    void convolveToneMapper(uint8_t *data, int stride, int width, int height, ToneMapper<FixedTag<float32_t, 4>> *toneMapper) {
        const float rPrimary = 0.299f;
        const float gPrimary = 0.587f;
        const float bPrimary = 0.114f;

        const FixedTag<uint8_t, 4> du;
        const FixedTag<uint32_t, 4> du32x4;
        const FixedTag<float32_t, 4> dfx4;
        const FixedTag<uint8_t, 4> du16;
        using VF = Vec<decltype(dfx4)>;
        using VU = Vec<decltype(du)>;
        using VU4 = Vec<decltype(du16)>;

        const auto vScale = Set(dfx4, 255.f);
        const auto vRevertScale = ApproximateReciprocal(vScale);

        const float lumaPrimaries[4] = {rPrimary, gPrimary, bPrimary, 0.f};
        const VF vLumaPrimaries = LoadU(dfx4, lumaPrimaries);

        const auto zeros = Zero(dfx4);

        for (int y = 0; y < height; ++y) {
            auto pixels = reinterpret_cast<uint8_t *>(reinterpret_cast<uint8_t *>(data) + y * stride);
            int x = 0;

            for (; x + 4 < width; x += 4) {
                VU4 ru4, gu4, bu4, au4;
                LoadInterleaved4(du16, &pixels[0], ru4, gu4, bu4, au4);
                VF rf4 = Mul(ConvertToFloat(dfx4, ru4), vRevertScale);
                VF gf4 = Mul(ConvertToFloat(dfx4, gu4), vRevertScale);
                VF bf4 = Mul(ConvertToFloat(dfx4, bu4), vRevertScale);

                rf4 = aire::HWY_NAMESPACE::SRGBToLinear(dfx4, rf4);
                gf4 = aire::HWY_NAMESPACE::SRGBToLinear(dfx4, gf4);
                bf4 = aire::HWY_NAMESPACE::SRGBToLinear(dfx4, bf4);

                toneMapper->Execute(rf4, gf4, bf4);

                rf4 = aire::HWY_NAMESPACE::LinearSRGBTosRGB(dfx4,rf4);
                gf4 = aire::HWY_NAMESPACE::LinearSRGBTosRGB(dfx4,gf4);
                bf4 = aire::HWY_NAMESPACE::LinearSRGBTosRGB(dfx4,bf4);

                rf4 = Clamp(Mul(rf4, vScale), zeros, vScale);
                gf4 = Clamp(Mul(gf4, vScale), zeros, vScale);
                bf4 = Clamp(Mul(bf4, vScale), zeros, vScale);

                ru4 = DemoteToU8(du, rf4);
                gu4 = DemoteToU8(du, gf4);
                bu4 = DemoteToU8(du, bf4);

                StoreInterleaved4(ru4, gu4, bu4, au4, du16, &pixels[0]);

                pixels += 16;
            }

            for (; x < width; ++x) {
                VF local = Mul(ConvertTo(dfx4, PromoteTo(du32x4, LoadU(du, &pixels[0]))), vRevertScale);
                VF color = aire::HWY_NAMESPACE::SRGBToLinear(dfx4, local);
                float r = ExtractLane(color, 0);
                float g = ExtractLane(color, 1);
                float b = ExtractLane(color, 2);
                toneMapper->Execute(r, g, b);
                pixels[0] = std::clamp(LinearSRGBTosRGB(r) * 255.f, 0.f, 255.f);
                pixels[1] = std::clamp(LinearSRGBTosRGB(g) * 255.f, 0.f, 255.f);
                pixels[2] = std::clamp(LinearSRGBTosRGB(b) * 255.f, 0.f, 255.f);
                pixels += 4;
            }
        }
    }

    void logarithmic(uint8_t *data, int stride, int width, int height, float exposure) {
        const float rPrimary = 0.299f;
        const float gPrimary = 0.587f;
        const float bPrimary = 0.114f;

        const float coeffs[3] = {rPrimary, gPrimary, bPrimary};
        LogarithmicToneMapper<FixedTag<float32_t, 4>> toneMapper(coeffs, exposure);
        convolveToneMapper(data, stride, width, height, &toneMapper);
    }

    void acesFilm(uint8_t *data, int stride, int width, int height, float exposure) {
        AcesFilmicToneMapper<FixedTag<float32_t, 4>> toneMapper(exposure);
        convolveToneMapper(data, stride, width, height, &toneMapper);
    }
}