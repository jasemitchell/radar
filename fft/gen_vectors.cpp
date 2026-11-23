#include "fft.hpp"
#include "signalGen.hpp"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdint>
#include <cstdio>
#include <vector>

int main()
{
    double      amplitude { 0.9 };
    double      fs        { 1000e3 };
    double      noise     { 0 };
    double      freq      { 19531.25 };   // bin 10 at fs = 1 MHz, N = 512
    double      phase_rad { 0 };
    std::size_t N         { 512 };
    unsigned    seed      { 12345 };

    std::vector<std::complex<double>> X =
        fft(makeTones(amplitude, fs, freq, phase_rad, N, noise, seed));

    // Find and print the peak
    std::size_t peak = 0;
    for (std::size_t k = 1; k < X.size(); ++k)
        if (std::abs(X[k]) > std::abs(X[peak])) peak = k;

    std::printf("expected bin %.2f\n", freq * N / fs);
    std::printf("peak bin %zu (%.2f Hz), |X| = %.1f\n",
                peak, peak * fs / N, std::abs(X[peak]));

    // C++ test vectors to Vivado IP
    std::vector<std::complex<double>> x { makeTones(amplitude, fs, freq, phase_rad, N, noise, seed) };

    const double scale { 32767.0 };
    std::vector<std::complex<double>> xq(N);

    std::FILE* stim { std::fopen("stimulus.hex", "w") };
    for (std::size_t n = 0; n < N; ++n)
    {
        long Ir { std::clamp(std::lround(x[n].real() * scale), -32768L, 32767L) };
        long Qr { std::clamp(std::lround(x[n].imag() * scale), -32768L, 32767L) };
        auto I  { static_cast<int16_t>(Ir) };
        auto Q  { static_cast<int16_t>(Qr) };

        uint32_t word { (uint32_t(uint16_t(Q)) << 16) | uint16_t(I) };
        std::fprintf(stim, "%08X\n", word);

        xq[n] = { double(I), double(Q) };
    }
    std::fclose(stim);

    // Expected IP output
    std::vector<std::complex<double>> Xe { fft(xq) };
    std::FILE* exp { std::fopen("expected.txt", "w") };
    for (std::size_t k = 0; k < N; ++k)
      std::fprintf(exp, "%zu %.3f %.3f\n", k, Xe[k].real(), Xe[k].imag());
    std::fclose(exp);
    return 0;
}
