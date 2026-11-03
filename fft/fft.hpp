#pragma once

#include <complex>
#include <stdexcept>
#include <vector>

std::vector<std::complex<double>> fft(std::vector<std::complex<double>> x)
{
    const std::size_t N = x.size();
    if (N == 0 || (N & (N - 1)) != 0) 
        throw std::invalid_argument("N is not a power of two");

    if (N == 1)
        return x;

    // Split into even and odd samples (decimation in time)
    std::vector<std::complex<double>> x_even(N/2);
    std::vector<std::complex<double>> x_odd(N/2);
    for (std::size_t n = 0; n < N / 2; ++n)
    {
        x_even[n] = x[2 * n];
        x_odd [n] = x[2 * n + 1];
    }
    
    std::vector<std::complex<double>> A = fft(x_even);
    std::vector<std::complex<double>> B = fft(x_odd);

    const double pi = 3.14159265358979323846;
    std::vector<std::complex<double>> X(N);

    for (std::size_t m = 0; m < N / 2; ++m)
    {
        const double angle = -2.0 * pi * static_cast<double>(m) / static_cast<double>(N);
        const std::complex<double> Wn = std::polar(1.0, angle);
        const std::complex<double> t  = Wn * B[m];
        X[m]         = A[m] + t;
        X[m + N / 2] = A[m] - t;
    }
    return X;
}
