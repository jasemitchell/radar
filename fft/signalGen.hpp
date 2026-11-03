#pragma once

#include <cmath>
#include <complex>
#include <cstddef>
#include <random>
#include <vector>

// x[n] = sum_i A_i * exp(j*(2*pi*f_i*n/fs + phi_i)) + complex Gaussian noise.
// I = real part, Q = imaginary part, as the K-LC5 I/Q outputs would give.

std::vector<std::complex<double>> makeTones(double amplitude, double fs, double freq, 
    double phase_rad, std::size_t N, double noise, unsigned seed) 
{
    const double pi { 3.14159265358979323846 };
    std::vector<std::complex<double>> x(N, std::complex<double>(0.0, 0.0));
    for (std::size_t n = 0; n < N; ++n)
    {
        double ph = 2 * pi * freq * static_cast<double>(n) / fs + phase_rad;
        x[n] += amplitude * std::complex<double>(std::cos(ph), std::sin(ph));
    }
    if (noise > 0.0)
    {
        std::mt19937 rng(seed);
        std::normal_distribution<double> g(0.0, noise / std::sqrt(2.0));
        for (auto& v : x)
        {
            double nI = g(rng);
            double nQ = g(rng);
            v += std::complex<double>(nI, nQ);
        }
    }
    return x;
}
