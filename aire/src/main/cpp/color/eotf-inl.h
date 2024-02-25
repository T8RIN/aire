/*
 *
 *  * MIT License
 *  *
 *  * Copyright (c) 2024 Radzivon Bartoshyk
 *  * aire [https://github.com/awxkee/aire]
 *  *
 *  * Created by Radzivon Bartoshyk on 14/01/24, 6:13 PM
 *  *
 *  * Permission is hereby granted, free of charge, to any person obtaining a copy
 *  * of this software and associated documentation files (the "Software"), to deal
 *  * in the Software without restriction, including without limitation the rights
 *  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  * copies of the Software, and to permit persons to whom the Software is
 *  * furnished to do so, subject to the following conditions:
 *  *
 *  * The above copyright notice and this permission notice shall be included in all
 *  * copies or substantial portions of the Software.
 *  *
 *  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  * SOFTWARE.
 *  *
 *
 */

#if defined(HIGHWAY_HWY_EOTF_INL_H_) == defined(HWY_TARGET_TOGGLE)
#ifdef HIGHWAY_HWY_EOTF_INL_H_
#undef HIGHWAY_HWY_EOTF_INL_H_
#else
#define HIGHWAY_HWY_EOTF_INL_H_
#endif

#include "hwy/highway.h"
#include <algorithm>
#include "algo/fast_math-inl.h"
#include "Eigen/Eigen"

using namespace std;

HWY_BEFORE_NAMESPACE();
namespace aire::HWY_NAMESPACE {

    static const float betaRec2020 = 0.018053968510807f;
    static const float alphaRec2020 = 1.09929682680944f;

    using namespace hwy;
    using namespace hwy::HWY_NAMESPACE;

    HWY_API float bt2020GammaCorrection(float linear) {
        if (0 <= betaRec2020 && linear < betaRec2020) {
            return 4.5f * linear;
        } else if (betaRec2020 <= linear && linear < 1) {
            return alphaRec2020 * powf(linear, 0.45f) - (alphaRec2020 - 1.0f);
        }
        return linear;
    }

    HWY_API float ToLinearPQ(float v, const float sdrReferencePoint) {
        float o = v;
        v = max(0.0f, v);
        const float m1 = (2610.0f / 4096.0f) / 4.0f;
        const float m2 = (2523.0f / 4096.0f) * 128.0f;
        const float c1 = 3424.0f / 4096.0f;
        const float c2 = (2413.0f / 4096.0f) * 32.0f;
        const float c3 = (2392.0f / 4096.0f) * 32.0f;
        float p = pow(v, 1.0f / m2);
        v = pow(max(p - c1, 0.0f) / (c2 - c3 * p), 1.0f / m1);
        v *= 10000.0f / sdrReferencePoint;
        return copysign(v, o);
    }

    using hwy::HWY_NAMESPACE::Zero;
    using hwy::HWY_NAMESPACE::Vec;
    using hwy::HWY_NAMESPACE::TFromD;

    template<class DF, class V, HWY_IF_FLOAT(TFromD<DF>)>

    HWY_API V
    HLGEotf(const DF df, V v) {
        v = Max(Zero(df), v);
        using VF32 = Vec<decltype(df)>;
        using T = hwy::HWY_NAMESPACE::TFromD<DF>;
        const VF32 a = Set(df, static_cast<T>(0.17883277f));
        const VF32 b = Set(df, static_cast<T>(0.28466892f));
        const VF32 c = Set(df, static_cast<T>(0.55991073f));
        const VF32 mm = Set(df, static_cast<T>(0.5f));
        const VF32 inversed3 = ApproximateReciprocal(Set(df, static_cast<T>(3.f)));
        const VF32 inversed12 = ApproximateReciprocal(Set(df, static_cast<T>(12.0f)));
        const auto cmp = v < mm;
        auto branch1 = Mul(Mul(v, v), inversed3);
        auto branch2 = Mul(aire::HWY_NAMESPACE::sleef::Exp(df, Add(Div(Sub(v, c), a), b)),
                           inversed12);
        return IfThenElse(cmp, branch1, branch2);
    }

    template<class D, class V, HWY_IF_FLOAT(TFromD<D>)>
    HWY_API V ToLinearPQ(const D df, V v, const TFromD<D> sdrReferencePoint) {
        const V zeros = Zero(df);
        v = Max(zeros, v);
        using T = hwy::HWY_NAMESPACE::TFromD<D>;
        const T m1 = static_cast<T>((2610.0f / 4096.0f) / 4.0f);
        const T m2 = static_cast<T>((2523.0f / 4096.0f) * 128.0f);
        const V c1 = Set(df, static_cast<T>(3424.0f / 4096.0f));
        const V c2 = Set(df, static_cast<T>((2413.0f / 4096.0f) * 32.0f));
        const V c3 = Set(df, static_cast<T>((2392.0f / 4096.0f) * 32.0f));
        const V p1Power = Set(df, static_cast<T>(1.0f / m2));
        const V p2Power = Set(df, static_cast<T>(1.0f / m1));
        const V p3Power = Set(df, static_cast<T>(10000.0f / sdrReferencePoint));
        const V p = aire::HWY_NAMESPACE::Pow(df, v, p1Power);
        v = aire::HWY_NAMESPACE::Pow(df, Div(Max(Sub(p, c1), zeros), Sub(c2, Mul(c3, p))),
                                     p2Power);
        v = Mul(v, p3Power);
        return v;
    }

    template<class D, typename V = Vec<D>, HWY_IF_FLOAT(TFromD<D>)>
    HWY_API V bt2020GammaCorrection(const D d, V color) {
        const V bt2020 = Set(d, betaRec2020);
        const V alpha2020 = Set(d, alphaRec2020);
        using T = hwy::HWY_NAMESPACE::TFromD<D>;
        const auto cmp = color < bt2020;
        const auto fourAndHalf = Set(d, static_cast<T>(4.5f));
        const auto branch1 = Mul(color, fourAndHalf);
        const V power1 = Set(d, static_cast<T>(0.45f));
        const V ones = Set(d, static_cast<T>(1.0f));
        const V power2 = Sub(alpha2020, ones);
        const auto branch2 =
                Sub(Mul(alpha2020, aire::HWY_NAMESPACE::Pow(d, color, power1)), power2);
        return IfThenElse(cmp, branch1, branch2);
    }

    template<class D, typename V = Vec<D>, HWY_IF_FLOAT(TFromD<D>)>

    HWY_API V LinearITUR709ToITUR709(const D df, V value) {
        const auto minCurve = Set(df, static_cast<TFromD<D>>(0.018f));
        const auto minPowValue = Set(df, static_cast<TFromD<D>>(4.5f));
        const auto minLane = Mul(value, minPowValue);
        const auto subValue = Set(df, static_cast<TFromD<D>>(0.099f));
        const auto scalePower = Set(df, static_cast<TFromD<D>>(1.099f));
        const auto pwrValue = Set(df, static_cast<TFromD<D>>(0.45f));
        const auto maxLane = MulSub(aire::HWY_NAMESPACE::Pow(df, value, pwrValue), scalePower,
                                    subValue);
        return IfThenElse(value < minCurve, minLane, maxLane);
    }

    HWY_API float SRGBToLinear(float v) {
        if (v <= 0.045f) {
            return v / 12.92f;
        } else {
            return std::pow((v + 0.055f) / 1.055f, 2.4f);
        }
    }

    HWY_API float LinearSRGBTosRGB(const float linear) {
        if (linear <= 0.0031308f) {
            return 12.92f * linear;
        } else {
            return 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
        }
    }

    template<class D, typename V = Vec<D>, HWY_IF_FLOAT(TFromD<D>)>

    HWY_API V LinearSRGBTosRGB(const D df, V value) {
        const auto minCurve = Set(df, static_cast<TFromD<D>>(0.0031308f));
        const auto minPowValue = Set(df, static_cast<TFromD<D>>(12.92f));
        const auto minLane = Mul(value, minPowValue);
        const auto subValue = Set(df, static_cast<TFromD<D>>(0.055f));
        const auto scalePower = Set(df, static_cast<TFromD<D>>(1.055f));
        const auto pwrValue = Set(df, static_cast<TFromD<D>>(1.0f / 2.4f));
        const auto maxLane = MulSub(aire::HWY_NAMESPACE::Pow(df, value, pwrValue), scalePower,
                                    subValue);
        return IfThenElse(value <= minCurve, minLane, maxLane);
    }

    HWY_API float LinearITUR709ToITUR709(const float linear) {
        if (linear <= 0.018f) {
            return 4.5f * linear;
        } else {
            return 1.099f * pow(linear, 0.45f) - 0.099f;
        }
    }

    template<class D, typename V = Vec<D>, HWY_IF_FLOAT(TFromD<D>)>
    HWY_API V SMPTE428Eotf(const D df, V value) {
        const auto zeros = Zero(df);
        const auto ones = Set(df, static_cast<TFromD<D>>(1.0f));
        const auto scale = Set(df, static_cast<TFromD<D>>(52.37f / 48.0f));
        auto mask = value < zeros;
        auto result = IfThenElse(mask, ones, value);
        const auto twoPoint6 = Set(df, static_cast<TFromD<D>>(2.6f));
        result = Mul(aire::HWY_NAMESPACE::Pow(df, value, twoPoint6), scale);
        result = IfThenElse(mask, zeros, result);
        return result;
    }

    HWY_API float SMPTE428Eotf(const float value) {
        if (value < 0.0f) {
            return 0.0f;
        }
        constexpr float scale = 52.37f / 48.0f;
        return pow(value, 2.6f) * scale;
    }

    HWY_API float HLGEotf(float v) {
        v = max(0.0f, v);
        constexpr float a = 0.17883277f;
        constexpr float b = 0.28466892f;
        constexpr float c = 0.55991073f;
        if (v <= 0.5f)
            v = v * v / 3.0f;
        else
            v = (exp((v - c) / a) + b) / 12.f;
        return v;
    }

    template<class D, HWY_IF_F32_D(D), typename T = TFromD<D>, typename V = VFromD<D>>
    HWY_FAST_MATH_INLINE V SRGBToLinear(D d, V v) {
        const auto lowerValueThreshold = Set(d, T(0.045f));
        const auto lowValueDivider = ApproximateReciprocal(Set(d, T(12.92f)));
        const auto lowMask = v <= lowerValueThreshold;
        const auto lowValue = Mul(v, lowValueDivider);
        const auto powerStatic = Set(d, T(2.4f));
        const auto addStatic = Set(d, T(0.055f));
        const auto scaleStatic = ApproximateReciprocal(Set(d, T(1.055f)));
        const auto highValue = Pow(d, Mul(Add(v, addStatic), scaleStatic), powerStatic);
        return IfThenElse(lowMask, lowValue, highValue);
    }

    template<class D, typename T = Vec<D>, HWY_IF_FLOAT(TFromD<D>)>

    HWY_API T dciP3PQGammaCorrection(const D d, T color) {
        const auto pw = Set(d, 1 / 2.6f);
        return aire::HWY_NAMESPACE::Pow(d, color, pw);
    }

    template<class D, typename T = Vec<D>, HWY_IF_FLOAT(TFromD<D>)>

    HWY_API T gammaEotf(const D d, T color, TFromD<D> gamma) {
        const auto pw = Set(d, 1 / gamma);
        return aire::HWY_NAMESPACE::Pow(d, color, pw);
    }

    HWY_API float dciP3PQGammaCorrection(float linear) {
        return powf(linear, 1 / 2.6f);
    }

    HWY_API float gammaEotf(float linear, const float gamma) {
        return powf(linear, 1 / gamma);
    }

//
//    template<typename D>
//    class Rec2408PQToneMapper : public ToneMapper<D> {
//    private:
//        using V = Vec<D>;
//        static D df_;
//
//    public:
//        Rec2408PQToneMapper(const TFromD <D> contentMaxBrightness,
//                            const TFromD <D> displayMaxBrightness,
//                            const TFromD <D> whitePoint,
//                            const TFromD <D> lumaCoefficients[3]) : ToneMapper<D>() {
//            this->Ld = contentMaxBrightness / whitePoint;
//            this->a = (displayMaxBrightness / whitePoint) / (Ld * Ld);
//            this->b = 1.0f / (displayMaxBrightness / whitePoint);
//            std::copy(lumaCoefficients, lumaCoefficients + 3, this->lumaCoefficients);
//            this->lumaCoefficients[3] = 0.0f;
//        }
//
//        ~Rec2408PQToneMapper() override = default;
//
//        HWY_API void Execute(V &R, V &G, V &B) {
//            const V lumaR = Set(df_, lumaCoefficients[0]);
//            const V lumaG = Set(df_, lumaCoefficients[1]);
//            const V lumaB = Set(df_, lumaCoefficients[1]);
//            const V aVec = Set(df_, this->a);
//            const V bVec = Set(df_, this->b);
//
//            V rLuma = Mul(R, lumaR);
//            V gLuma = Mul(G, lumaG);
//            V bLuma = Mul(B, lumaB);
//
//            const V ones = Set(df_, static_cast<TFromD<D>>(1.0f));
//
//            V Lin = Add(Add(rLuma, gLuma), bLuma);
//            V scales = Div(MulAdd(aVec, Lin, ones),
//                           MulAdd(bVec, Lin, ones));
//            R = Mul(R, scales);
//            G = Mul(G, scales);
//            B = Mul(B, scales);
//        }
//
//        HWY_API void Execute(TFromD <D> &r, TFromD <D> &g, TFromD <D> &b) {
//            const float Lin =
//                    r * lumaCoefficients[0] + g * lumaCoefficients[1] + b * lumaCoefficients[2];
//            if (Lin == 0) {
//                return;
//            }
//            const TFromD<D> shScale = (1.f + this->a * Lin) / (1.f + this->b * Lin);
//            r = r * shScale;
//            g = g * shScale;
//            b = b * shScale;
//        }
//
//    private:
//        TFromD <D> Ld;
//        TFromD <D> a;
//        TFromD <D> b;
//        TFromD <D> lumaCoefficients[4];
//    };

}
HWY_AFTER_NAMESPACE();
#endif