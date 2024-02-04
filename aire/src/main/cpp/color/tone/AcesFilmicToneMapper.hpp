//
// Created by Radzivon Bartoshyk on 04/02/2024.
//

#pragma once

#include "ToneMapper.h"
#include <fast_math-inl.h>

namespace aire {
    template<typename D>
    class AcesFilmicToneMapper : public ToneMapper<D> {
    private:
        using V = Vec<D>;
        D df_;
        TFromD<D> den;
        TFromD<D> exposure;

    public:
        AcesFilmicToneMapper(const TFromD<D> exposure = 1.0f) : exposure(exposure), ToneMapper<D>() {
            TFromD<D> Lmax = 1;
            den = static_cast<TFromD<D>>(1) / log(static_cast<TFromD<D>>(1 + Lmax * exposure));
        }

        ~AcesFilmicToneMapper() override = default;

        HWY_FAST_MATH_INLINE void Execute(V &R, V &G, V &B) {
            const V mExposure = Set(df_, exposure);
            R = ACESFilm(Mul(R, mExposure));
            G = ACESFilm(Mul(G, mExposure));
            B = ACESFilm(Mul(B, mExposure));
        }

        HWY_FAST_MATH_INLINE void Execute(TFromD<D> &r, TFromD<D> &g, TFromD<D> &b) {
            r = ACESFilm(r * exposure);
            g = ACESFilm(g * exposure);
            b = ACESFilm(b * exposure);
        }

    private:
        HWY_FAST_MATH_INLINE V ACESFilm(V x) {
            const V a = Set(df_, 2.51f);
            const V b = Set(df_, 0.03f);
            const V c = Set(df_, 2.43f);
            const V d = Set(df_, 0.59f);
            const V e = Set(df_, 0.14f);

            const V zeros = Zero(df_);
            const V ones = Set(df_, 1.0f);

            V v = Div(Mul(MulAdd(a, x, b), x), MulAdd(x, MulAdd(c, x, d), e));
            return Clamp(v, zeros, ones);
        }

        float ACESFilm(float x) {
            constexpr float a = 2.51f;
            constexpr float b = 0.03f;
            constexpr float c = 2.43f;
            constexpr float d = 0.59f;
            constexpr float e = 0.14f;
            return std::clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.f, 1.0f);
        }
    };
}