#pragma once

#include <cassert>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include "factorize.h"
#include "fftw3.h"

namespace aire {

    class FF2DWorkspace {
    public:
        FF2DWorkspace(int hSrc, int wSrc, int hKernel, int wKernel) {
            h_src = hSrc;
            w_src = wSrc;
            h_kernel = hKernel;
            w_kernel = wKernel;

            h_fftw = find_closest_factor(h_src + h_kernel * 2 - 1, const_cast<int *>(FFTW_FACTORS));
            w_fftw = find_closest_factor(w_src + w_kernel * 2 - 1, const_cast<int *>(FFTW_FACTORS));
            h_dst = h_src + h_kernel * 2 - 1;
            w_dst = w_src + w_kernel * 2 - 1;

            in_src = new double[h_fftw * w_fftw];
            out_src = (double *) fftw_malloc(sizeof(fftw_complex) * h_fftw * (w_fftw / 2 + 1));
            in_kernel = new double[h_fftw * w_fftw];
            out_kernel = (double *) fftw_malloc(sizeof(fftw_complex) * h_fftw * (w_fftw / 2 + 1));
            dst_fft = new double[h_fftw * w_fftw];
            dst = new double[h_dst * w_dst];

            p_forw_src = fftw_plan_dft_r2c_2d(h_fftw, w_fftw, in_src, (fftw_complex *) out_src, FFTW_ESTIMATE);
            p_forw_kernel = fftw_plan_dft_r2c_2d(h_fftw, w_fftw, in_kernel, (fftw_complex *) out_kernel, FFTW_ESTIMATE);
            p_back = fftw_plan_dft_c2r_2d(h_fftw, w_fftw, (fftw_complex *) out_kernel, dst_fft, FFTW_ESTIMATE);
        }

        double *getOutput() {
            return dst;
        }

        int getDstWidth() {
            return w_dst;
        }

        int getDstHeight() {
            return h_dst;
        }

        void convolveWorkspace(double *src, double *kernel) {
            if (h_fftw <= 0 || w_fftw <= 0)
                return;

            fftw_circular_convolution(src, kernel);

            for (int i = 0; i < h_dst; ++i)
                memcpy(&dst[i * w_dst], &dst_fft[i * w_fftw], w_dst * sizeof(double));
        }

        ~FF2DWorkspace() {
            delete[] in_src;
            fftw_free((fftw_complex *) out_src);
            delete[] in_kernel;
            fftw_free((fftw_complex *) out_kernel);

            delete[] dst_fft;
            delete[] dst;

            fftw_destroy_plan(p_forw_src);
            fftw_destroy_plan(p_forw_kernel);
            fftw_destroy_plan(p_back);
        }

    private:

        void fftw_circular_convolution(double *src, double *kernel) {
            double *ptr, *ptr_end, *ptr2;

            // Reset the content of ws.in_src
            for (ptr = in_src, ptr_end = in_src + h_fftw * w_fftw; ptr != ptr_end; ++ptr)
                *ptr = 0.0;
            for (ptr = in_kernel, ptr_end = in_kernel + h_fftw * w_fftw; ptr != ptr_end; ++ptr)
                *ptr = 0.0;

            for (int i = 0; i < h_fftw; ++i)
                for (int j = 0; j < w_fftw; ++j, ++ptr) {
                    int reflected_x = std::min(std::max(j, 0), w_src - 1);
                    if (j < 0)
                        reflected_x = -j;
                    else if (j >= w_src)
                        reflected_x = 2 * w_src - j - 2;

                    int reflected_y = std::min(std::max(i, 0), h_src - 1);
                    if (i < 0)
                        reflected_y = -i;
                    else if (i >= h_src)
                        reflected_y = 2 * h_src - i - 2;

                    in_src[(i) * w_fftw + j] += src[std::clamp(reflected_y, 0, h_src - 1) * w_src + std::clamp(reflected_x, 0, w_src - 1)];
                }

            for (int i = 0; i < h_kernel; ++i)
                for (int j = 0; j < w_kernel; ++j, ++ptr)
                    in_kernel[(i % h_fftw) * w_fftw + (j % w_fftw)] += kernel[i * w_kernel + j];

            // And we compute their packed FFT
            fftw_execute(p_forw_src);
            fftw_execute(p_forw_kernel);

            // Compute the element-wise product on the packed terms
            // Let's put the element wise products in ws.in_kernel
            double re_s, im_s, re_k, im_k;
            for (ptr = out_src, ptr2 = out_kernel, ptr_end = out_src + 2 * h_fftw * (w_fftw / 2 + 1); ptr != ptr_end; ++ptr, ++ptr2) {
                re_s = *ptr;
                im_s = *(++ptr);
                re_k = *ptr2;
                im_k = *(++ptr2);
                *(ptr2 - 1) = re_s * re_k - im_s * im_k;
                *ptr2 = re_s * im_k + im_s * re_k;
            }

            // Compute the backward FFT
            // Carefull, The backward FFT does not preserve the output
            fftw_execute(p_back);
            // Scale the transform

            double max = 0;
            for (ptr = dst_fft, ptr_end = dst_fft + w_fftw * h_fftw; ptr != ptr_end; ++ptr)
                if (max < *ptr) {
                    max = *ptr;
                }

            for (ptr = dst_fft, ptr_end = dst_fft + w_fftw * h_fftw; ptr != ptr_end; ++ptr)
                *ptr /= max;
        }


        double *in_src, *out_src, *in_kernel, *out_kernel;
        int h_src, w_src, h_kernel, w_kernel;
        int w_fftw, h_fftw;
        double *dst_fft;
        double *dst; // The array containing the result
        int h_dst, w_dst; // its size ; This is automatically set by init_workspace
        fftw_plan p_forw_src;
        fftw_plan p_forw_kernel;
        fftw_plan p_back;

        const int FFTW_FACTORS[7] = {13, 11, 7, 5, 3, 2, 0};;
    };

}