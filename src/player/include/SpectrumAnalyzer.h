#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "AudioInfo.h"

inline constexpr std::size_t SPECTRUM_BAND_COUNT = 10;
inline constexpr std::size_t SPECTRUM_FFT_SIZE = 1024;
using SpectrumPowerBands = std::array<float, SPECTRUM_BAND_COUNT>;

class CSpectrumAnalyzer final
{
public:
    CSpectrumAnalyzer();
    ~CSpectrumAnalyzer();
    CSpectrumAnalyzer(const CSpectrumAnalyzer&) = delete;
    CSpectrumAnalyzer& operator=(const CSpectrumAnalyzer&) = delete;

    // Single audio-consumer thread only. Returns true when a complete FFT is available.
    bool Analyze(const void* audioBuf, uint32_t frames, const AudioFormat& format,
        SpectrumPowerBands& powerBands);
    void Reset();

private:
    struct CImpl;
    std::unique_ptr<CImpl> m_impl;
};
