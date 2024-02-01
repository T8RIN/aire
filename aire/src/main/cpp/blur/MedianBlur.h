//
// Created by Radzivon Bartoshyk on 31/01/2024.
//

#ifndef JXLCODER_MEDIANBLUR_H
#define JXLCODER_MEDIANBLUR_H

#include <cstdint>

template<class V>
void medianBlur(V* data, int stride, int width, int height, int radius);

#endif //JXLCODER_MEDIANBLUR_H
