//
// Created by Radzivon Bartoshyk on 01/02/2024.
//

#pragma once

#include <cstdint>
#include <vector>

namespace aire {
    template<class V>
    void glitchEffect(V *data, int stride, int width, int height, float channelsShiftX,
                      float channelsShiftY, float corruptionSize, int corruptionCount,
                      float cShiftX, float cShiftY);
}
