#include <complex>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

std::vector<std::complex<double>> readSpectrum(const std::string& path)
{
    std::vector<std::complex<double>> X;
    std::ifstream in(path);
    if (!in)
    {
        std::cout << "could not open " << path << std::endl;
        return X;
    }

    std::size_t k;
    double re, im;
    while (in >> k >> re >> im)
        X.push_back({re, im});

    return X;
}

int main()
{
    auto expected { readSpectrum("expected.txt") };
    auto output { readSpectrum("ip_output.txt") };

    if (expected.empty() || expected.size() != output.size())
    {
        std::cout << "files missing or different lengths\n";
        return 1;
    }

    std::size_t peakExp = 0, peakOut = 0;
    for (std::size_t k = 1; k < output.size(); ++k)
    {
        if (std::abs(expected[k]) > std::abs(expected[peakExp])) peakExp = k;
        if (std::abs(output[k])   > std::abs(output[peakOut]))   peakOut = k;
    }

    double signalPower {}, errorPower {};
    for (std::size_t k = 0; k < output.size(); ++k)
    {
        signalPower += std::norm(expected[k]);
        errorPower  += std::norm(expected[k] - output[k]);
    }
    double sqnr_db = 10.0 * std::log10(signalPower / errorPower);

    std::cout << "Peak bin  expected: " << peakExp << "  |X| = " << std::abs(expected[peakExp]) << "\n";
    std::cout << "Peak bin  output:   " << peakOut << "  |X| = " << std::abs(output[peakOut])   << "\n";
    std::cout << (peakExp == peakOut ? "PEAK MATCH\n" : "PEAK MISMATCH\n");
    std::cout << "Peak difference: " << std::abs(expected[peakExp]) - std::abs(output[peakOut]) << "\n";
    std::cout << "SQNR: " << sqnr_db << "\n";
    return 0;
}