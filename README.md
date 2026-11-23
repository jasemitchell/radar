# FPGA FMCW Radar Signal Processing on the PYNQ-Z2

My year-3 project: a 24 GHz FMCW radar front end with the signal processing running on a Zynq-7020 FPGA. The beat signal from a K-LC5 radar module is amplified, sampled by the Zynq's XADC and passed to a hardware FFT. The FFT output goes to DDR memory by AXI DMA. A bare-metal C++ application on the ARM core checks the hardware FFT against a C++ reference model.

**Status:** the Xilinx FFT core has been checked against the C++ model on the board, and the results agree closely (see [Results](#results)). The XADC and `ADC_formatter` are in the block design, but the formatter's output isn't connected to the FFT yet. For now the FFT gets its input from the DMA.

## Signal chain

```
 K-LC5 24 GHz      IF          XADC            ADC_formatter        Xilinx FFT       AXI DMA       DDR / ARM
 FMCW module  ──▶  amplifier ──▶ VAUX1 (I)  ──▶ (custom SV)  ─ ─ ─ ▶ 512-point   ──▶ S2MM   ──▶  bare-metal C++
 I/Q beat out                    VAUX9 (Q)      I/Q pairing,  not   unscaled          ▲             app
                                 simultaneous   framing      wired                    │
                                 sampling                    yet                     MM2S ◀── test vectors
                                                                                     (current test path)
```

| Stage | What it does |
|---|---|
| **K-LC5** | 24 GHz FMCW transceiver with I and Q beat-frequency outputs. The beat frequency is proportional to target range. |
| **IF amplifier** | Raises the low-level beat signal to the XADC's input range. |
| **XADC** (`xadc_wiz` 3.3) | Samples the I channel on VAUX1 and the Q channel on VAUX9 at the same instant, and streams the samples out over AXI-Stream. |
| **ADC_formatter** ([`hw/rtl/ADC_formatter.sv`](hw/rtl/ADC_formatter.sv)) | Pairs each I sample with its Q sample using the XADC channel ID, and flips the sign bit so the samples become two's complement. It packs each pair into a 32-bit `{Q, I}` word and sends frames of 512 samples, marked with `TLAST`, each time the `capture` input rises. |
| **FFT** (`xfft` 9.1) | 512-point radix-2 lite burst I/O, unscaled, convergent rounding, output in natural order, clocked at 100 MHz. |
| **AXI DMA** (simple mode) | MM2S sends the input samples to the FFT, and S2MM writes the spectrum back to DDR through HP0. |
| **Bare-metal app** ([`vitis_ws/fft_dma_test/src/main.cpp`](vitis_ws/fft_dma_test/src/main.cpp)) | Generates a test tone, runs it through both the hardware FFT and the C++ FFT, and compares the results. |

## Repository layout

```
.
├── hw/                      Vivado sources
│   ├── bd/system.tcl        Block design (exported with write_bd_tcl)
│   ├── rtl/ADC_formatter.sv XADC → FFT AXI-Stream formatter
│   └── export/system.xsa    Exported hardware platform, including the bitstream
├── fft/                     Host-side C++ model and comparison
│   ├── fft.hpp              Recursive radix-2 decimation-in-time FFT
│   ├── signalGen.hpp        Complex tone and noise generator (I/Q, as from the K-LC5)
│   ├── gen_vectors.cpp      Writes the FFT input (stimulus.hex) and the expected output (expected.txt)
│   ├── compare_fft.cpp      Compares expected.txt with the FFT core's output (ip_output.txt)
│   └── Makefile
└── vitis_ws/                Vitis workspace
    ├── pynq_platform/       Platform (standalone BSP and FSBL) generated from the XSA
    └── fft_dma_test/        Bare-metal application for the hardware FFT test
```

## Hardware sources

`hw/` holds copies of the Vivado design files, exported from Vivado 2026.1:

- `hw/bd/system.tcl`: the block design, exported with `write_bd_tcl`
- `hw/rtl/ADC_formatter.sv`: the formatter RTL
- `hw/export/system.xsa`: the exported hardware platform, including the bitstream, from which the Vitis platform in `vitis_ws/pynq_platform` was built

To recreate the design, create a PYNQ-Z2 (`xc7z020clg400-1`) project in Vivado 2026.1, add `ADC_formatter.sv`, and run `source hw/bd/system.tcl` in the Tcl console. The formatter has to be added first, because the block design uses it as an RTL module.

## Building and running the Vitis app

1. Open `vitis_ws/` as the workspace in the Vitis unified IDE (2026.1).
2. Build the `pynq_platform` component, then `fft_dma_test`. If the hardware has changed, first point the platform at the new XSA under *Switch XSA*.
3. Connect the PYNQ-Z2 by USB, set its boot jumper to JTAG, and open a serial terminal on the board's USB-UART (usually 115200 baud).
4. Run `fft_dma_test` on hardware. The app programs the FPGA, sends a 512-sample test tone through the FFT core by DMA, and prints:

```
DMA ready
Hardware peak: bin 10, |Y| = ...
C++ peak:      bin 10, |X| = ...
Max error:     ... counts at bin 10 (97.8 dB below peak)
PASS
```

The test passes if the hardware and C++ peaks are in the same bin and the largest error is more than 60 dB below the peak.

## Host-side model

```sh
cd fft
make check     # rebuilds the test vectors from the C++ model, then compares with ip_output.txt
```

`gen_vectors` produces a 19.53125 kHz complex tone at 0.9 full scale, with fs = 1 MHz and N = 512, which puts it exactly on bin 10. It converts the tone to 16-bit I/Q and writes `stimulus.hex` (one `{Q, I}` word per line, the same format the formatter produces). It also writes `expected.txt`, the C++ FFT of those same 16-bit samples. `compare` reads `expected.txt` and `ip_output.txt`, the FFT core's output for the same input.

## Results

Test: a 512-sample single tone on bin 10, quantised to 16 bits and run through the FFT core unscaled.

| Metric | Value |
|---|---|
| Peak bin, hardware vs C++ model | 10 vs 10 |
| Peak magnitude | 1.510 × 10⁷ |
| Largest error, all bins | 193 counts (bin 10), **97.8 dB below the peak** |
| Signal-to-quantisation-noise ratio (SQNR) | **93.7 dB** |
| Largest spur in the hardware output | 106 counts (bin 266), 103 dB below the peak |

The FFT core agrees with the double-precision C++ model to well within the 60 dB pass threshold. The remaining error comes from the core's fixed-point arithmetic and rounding.

## Next steps

- Connect `ADC_formatter` to the FFT, drive `capture` from the PS (through an AXI GPIO, for example), and route VAUX1 and VAUX9 to the board's analogue header.
- Capture real beat signals from the K-LC5 and turn the FFT bin numbers into range.
- Add windowing, and a second FFT stage for range-Doppler processing.
