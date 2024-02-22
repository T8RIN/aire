//
// Created by Radzivon Bartoshyk on 04/02/2024.
//

#include "PoissonBlur.h"
#include "base/Convolve1D.h"
#include "base/Convolve1Db16.h"
#include "base/Convolve2D.h"
#include "Eigen/Eigen"
#include <vector>
#include <random>
#include <chrono>

namespace aire {

    void poissonBlurF16(uint16_t *data, int stride, int width, int height, int radius) {
        auto kernel = generatePoissonBlur(radius);
        Convolve1Db16 convolution(kernel, kernel);
        convolution.convolve(data, stride, width, height);
    }

    Eigen::MatrixXf generatePoissonBlur2D(int size) {
        std::poisson_distribution<> d(size);
        Eigen::MatrixXf kernel(size, size);

        std::random_device rd;
        std::mt19937 gen(rd());
        gen.seed(std::chrono::system_clock::now().time_since_epoch().count());

        for (int j = 0; j < size; ++j) {
            for (int i = 0; i < size; ++i) {
                kernel(j, i) = d(gen);
            }
        }

        float sum = kernel.sum();

        int maxIter = 0;
        while (sum == 0.f && maxIter < 50) {
            for (int j = 0; j < size; ++j) {
                for (int i = 0; i < size; ++i) {
                    kernel(j, i) = d(gen);
                }
            }
            sum = kernel.sum();
            maxIter++;
        }

        if (sum != 0.f) {
            kernel /= sum;
        }

        return kernel;
    }

    void poissonBlur(uint8_t *data, int stride, int width, int height, int kernelSize) {
        auto kernel = generatePoissonBlur(kernelSize);
        convolve1D(data, stride, width, height, kernel, kernel);
    }

    std::vector<float> generatePoissonBlur(const int kernelSize) {
        std::poisson_distribution<> d(kernelSize);
        std::vector<float> kernel(kernelSize, 1.0f / kernelSize);

        std::random_device rd;
        std::mt19937 gen(rd());
        gen.seed(std::chrono::system_clock::now().time_since_epoch().count());

        for (int i = 0; i < kernelSize; ++i) {
            kernel[i] = d(gen);
        }

        float sum = 0.f;
        for (int i = 0; i < kernelSize; ++i) {
            sum += kernel[i];
        }

        int maxIter = 0;
        while (sum == 0.f && maxIter < 50) {
            kernel = generatePoissonBlur(kernelSize);
            maxIter++;
        }

        if (sum != 0.f) {
            for (int i = 0; i < kernelSize; ++i) {
                kernel[i] /= sum;
            }
        }

        return std::move(kernel);
    }
}
