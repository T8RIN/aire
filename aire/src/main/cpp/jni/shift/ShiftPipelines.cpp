#include "../JNIUtils.h"
#include "../AcquireBitmapPixels.h"
#include "shift/TiltShift.h"
#include "blur/GaussBlur.h"
#include "shift/Glitch.h"

//
// Created by Radzivon Bartoshyk on 01/02/2024.
//

extern "C"
JNIEXPORT jobject JNICALL
Java_com_awxkee_aire_pipeline_ShiftPipelineImpl_tiltShiftImpl(JNIEnv *env, jobject thiz,
                                                              jobject bitmap, jfloat radius,
                                                              jfloat sigma, jfloat anchorX,
                                                              jfloat anchorY, jfloat tiltRadius) {
    try {
        std::vector<AcquirePixelFormat> formats;
        formats.insert(formats.begin(), APF_RGBA8888);
        jobject newBitmap = AcquireBitmapPixels(env,
                                                bitmap,
                                                formats,
                                                true,
                                                [radius, sigma, anchorX, anchorY, tiltRadius](
                                                        std::vector<uint8_t> &input, int stride,
                                                        int width, int height,
                                                        AcquirePixelFormat fmt) -> BuiltImagePresentation {
                                                    std::vector<uint8_t> blurred(input.size());
                                                    std::copy(input.begin(), input.end(),
                                                              blurred.begin());
                                                    std::vector<uint8_t> output(input.size());
                                                    if (fmt == APF_RGBA8888) {
                                                        aire::gaussBlurU8(blurred.data(),
                                                                          stride, width,
                                                                          height, radius,
                                                                          sigma);
                                                        aire::tiltShift(output.data(), input.data(),
                                                                        blurred,
                                                                        stride, width,
                                                                        height,
                                                                        anchorX, anchorY,
                                                                        tiltRadius);
                                                    }
                                                    blurred.clear();
                                                    return {
                                                            .data = std::move(output),
                                                            .stride = stride,
                                                            .width = width,
                                                            .height = height,
                                                            .pixelFormat = fmt
                                                    };
                                                });
        return newBitmap;
    } catch (AireError &err) {
        std::string msg = err.what();
        throwException(env, msg);
        return nullptr;
    }
}
extern "C"
JNIEXPORT jobject JNICALL
Java_com_awxkee_aire_pipeline_ShiftPipelineImpl_glitchImpl(JNIEnv *env, jobject thiz,
                                                           jobject bitmap, jfloat shiftX,
                                                           jfloat shiftY, jfloat corruptionSize,
                                                           jint corruptions, jfloat cShiftX,
                                                           jfloat cShiftY) {
    try {
        std::vector<AcquirePixelFormat> formats;
        formats.insert(formats.begin(), APF_RGBA8888);
        jobject newBitmap = AcquireBitmapPixels(env,
                                                bitmap,
                                                formats,
                                                true,
                                                [shiftX, shiftY, corruptionSize, corruptions, cShiftX, cShiftY](
                                                        std::vector<uint8_t> &input, int stride,
                                                        int width, int height,
                                                        AcquirePixelFormat fmt) -> BuiltImagePresentation {
                                                    if (fmt == APF_RGBA8888) {
                                                        aire::glitchEffect(input.data(),
                                                                           stride, width,
                                                                           height, shiftX, shiftY,
                                                                           corruptionSize,
                                                                           corruptions,
                                                                           cShiftX,
                                                                           cShiftY);
                                                    }
                                                    return {
                                                            .data = input,
                                                            .stride = stride,
                                                            .width = width,
                                                            .height = height,
                                                            .pixelFormat = fmt
                                                    };
                                                });
        return newBitmap;
    } catch (AireError &err) {
        std::string msg = err.what();
        throwException(env, msg);
        return nullptr;
    }
}