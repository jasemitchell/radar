#include "fft.hpp"
#include "signalGen.hpp"
#include "xaxidma.h"
#include "xil_cache.h"
#include "xil_printf.h"
#include "xparameters.h"

#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdio>
#include <vector>

constexpr std::size_t N { 512 };

alignas(64) static int16_t  in[2 * N];   // 512 samples, Re & Im (16-bit each)
alignas(64) static int32_t out[2 * N];  // 512 bins, Re & Im (26-bit padded to 32)
static XAxiDma dma;

int main()
{
    // ---- DMA setup ----
    XAxiDma_Config *cfg { XAxiDma_LookupConfig(XPAR_XAXIDMA_0_BASEADDR) };
    if (cfg == nullptr) {
        xil_printf("No DMA config found\r\n");
        return 1;
    }

    if (XAxiDma_CfgInitialize(&dma, cfg) != XST_SUCCESS) {
        xil_printf("DMA init failed\r\n");
        return 1;
    }

    if (XAxiDma_HasSg(&dma)) {
        xil_printf("DMA is in Scatter Gather mode, needs Simple mode\r\n");
        return 1;
    }

    XAxiDma_IntrDisable(&dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
    XAxiDma_IntrDisable(&dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
    xil_printf("DMA ready\r\n");

    // ---- Test signal ----
    double   amplitude { 0.9 };
    double   fs        { 1000e3 };
    double   noise     { 0 };
    double   freq      { 19531.25 };   // bin 10 at fs = 1 MHz, N = 512, for freq = 19531.25 Hz
    double   phase_rad { 0 };
    unsigned seed      { 12345 };

    std::vector<std::complex<double>> signal { makeTones(amplitude, fs, freq, phase_rad, N, noise, seed) };

    // Quantise to 16-bit: scale first, then round
    for (std::size_t i = 0; i < N; ++i)
    {
        in[2 * i]     = static_cast<int16_t>(std::lround(signal[i].real() * 32767.0));
        in[2 * i + 1] = static_cast<int16_t>(std::lround(signal[i].imag() * 32767.0));
    }

    // ---- C++ reference: FFT of the same integers the hardware gets ----
    std::vector<std::complex<double>> q(N);
    for (std::size_t i = 0; i < N; ++i)
    {
        q[i] = { static_cast<double>(in[2 * i]), static_cast<double>(in[2 * i + 1]) };
    }
    std::vector<std::complex<double>> X { fft(q) };

    // ---- Hardware FFT via DMA ----
    Xil_DCacheFlushRange((UINTPTR)in, sizeof(in));

    // arm the receive first, then send
    if (XAxiDma_SimpleTransfer(&dma, (UINTPTR)out, sizeof(out), XAXIDMA_DEVICE_TO_DMA) != XST_SUCCESS) {
        xil_printf("S2MM transfer failed to start\r\n");
        return 1;
    }
    if (XAxiDma_SimpleTransfer(&dma, (UINTPTR)in, sizeof(in), XAXIDMA_DMA_TO_DEVICE) != XST_SUCCESS) {
        xil_printf("MM2S transfer failed to start\r\n");
        return 1;
    }

    std::uint32_t timeout { 10000000 };
    while (XAxiDma_Busy(&dma, XAXIDMA_DEVICE_TO_DMA) && --timeout) {}
    if (timeout == 0) {
        xil_printf("Timed out waiting for FFT output\r\n");
        return 1;
    }

    Xil_DCacheInvalidateRange((UINTPTR)out, sizeof(out));

    std::vector<std::complex<double>> Y(N);
    for (std::size_t k = 0; k < N; ++k)
    {
        Y[k] = { static_cast<double>(out[2 * k]), static_cast<double>(out[2 * k + 1]) };
    }

    // ---- Compare ----
    std::size_t peakHw { 0 };
    std::size_t peakSw { 0 };
    std::size_t errBin { 0 };
    double      maxErr { 0.0 };

    for (std::size_t k = 0; k < N; ++k)
    {
        if (std::abs(Y[k]) > std::abs(Y[peakHw])) peakHw = k;
        if (std::abs(X[k]) > std::abs(X[peakSw])) peakSw = k;

        double err { std::abs(Y[k] - X[k]) };
        if (err > maxErr) {
            maxErr = err;
            errBin = k;
        }
    }

    double errDb { 20.0 * std::log10(maxErr / std::abs(X[peakSw]) + 1e-20) };

    printf("Hardware peak: bin %u, |Y| = %.0f\r\n", static_cast<unsigned>(peakHw), std::abs(Y[peakHw]));
    printf("C++ peak:      bin %u, |X| = %.0f\r\n", static_cast<unsigned>(peakSw), std::abs(X[peakSw]));
    printf("Max error:     %.1f counts at bin %u (%.1f dB below peak)\r\n",
           maxErr, static_cast<unsigned>(errBin), -errDb);

    bool pass { peakHw == peakSw && errDb < -60.0 };
    printf("%s\r\n", pass ? "PASS" : "FAIL");

    return 0;
}