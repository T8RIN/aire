//
// Created by Radzivon Bartoshyk on 04/02/2024.
//

#pragma once

#include <cstdint>
#include <vector>

namespace aire {
    void poissonBlur(uint8_t *data, int stride, int width, int height, const int kernelSize);

    void poissonBlurF16(uint16_t *data, int stride, int width, int height, const int kernelSize);

    std::vector<float> generatePoissonBlur(const int kernelSize);
}
