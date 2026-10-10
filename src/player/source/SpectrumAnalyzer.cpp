#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>
#include <numbers>
#include <numeric>
#include <stdexcept>

#include <fftw3.h>

#include "SpectrumAnalyzer.h"

namespace {

constexpr std::size_t MAX_ANALYSIS_CHANNELS = 32;
constexpr std::size_t FFT_BIN_COUNT = SPECTRUM_FFT_SIZE / 2 + 1;
constexpr double MIN_POWER_DB = -60.0;

std::mutex& GetFftPlannerMutex()
{
    // FFTW planning/destruction are serialized; execution never takes this lock.
    static std::mutex mutex;
    return mutex;
}

template<typename T>
T ReadPcm(const uint8_t* sample)
{
    T value;
    std::memcpy(&value, sample, sizeof(value));
    return value;
}

double ReadNormalizedSample(const uint8_t* sample, AudioDataFormat format)
{
    double value = 0.0;
    switch (format) {
    case AudioDataFormat::PCM_U8:
        value = (static_cast<int>(*sample) - 128) / 128.0;
        break;
    case AudioDataFormat::PCM_S8:
        value = ReadPcm<int8_t>(sample) / 128.0;
        break;
    case AudioDataFormat::PCM_S16:
        value = ReadPcm<int16_t>(sample) / 32768.0;
        break;
    case AudioDataFormat::PCM_S24: {
        const uint32_t packed = static_cast<uint32_t>(sample[0])
            | (static_cast<uint32_t>(sample[1]) << 8)
            | (static_cast<uint32_t>(sample[2]) << 16);
        const int32_t signedSample = (packed & 0x800000)
            ? static_cast<int32_t>(packed) - 0x1000000 : static_cast<int32_t>(packed);
        value = signedSample / 8388608.0;
        break;
    }
    case AudioDataFormat::PCM_S24_32:
        // Windows device PCM stores valid 24-bit samples left-aligned in 32 bits.
    case AudioDataFormat::PCM_S32:
        value = ReadPcm<int32_t>(sample) / 2147483648.0;
        break;
    case AudioDataFormat::PCM_64:
        value = static_cast<double>(ReadPcm<int64_t>(sample)) / 9223372036854775808.0;
        break;
    case AudioDataFormat::Float32:
        value = ReadPcm<float>(sample);
        break;
    case AudioDataFormat::Float64:
        value = ReadPcm<double>(sample);
        break;
    default:
        break;
    }
    return std::isfinite(value) ? std::clamp(value, -1.0, 1.0) : 0.0;
}

} // namespace

struct CSpectrumAnalyzer::CImpl
{
    std::array<std::array<double, SPECTRUM_FFT_SIZE>, MAX_ANALYSIS_CHANNELS> samples{};
    std::array<double, SPECTRUM_FFT_SIZE> window{};
    std::array<double, SPECTRUM_FFT_SIZE> fftInput{};
    fftw_complex fftOutput[FFT_BIN_COUNT]{};
    std::array<std::size_t, SPECTRUM_BAND_COUNT + 1> bandEdges{};
    fftw_plan plan = nullptr;
    double windowEnergy = 0.0;
    std::size_t sampleCount = 0;
    AudioFormat audioFmt{};

    CImpl()
    {
        for (std::size_t i = 0; i < SPECTRUM_FFT_SIZE; ++i) {
            window[i] = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * i / (SPECTRUM_FFT_SIZE - 1));
            windowEnergy += window[i] * window[i];
        }
        // Log-spaced bin ranges: exclude DC, keep every positive-frequency bin once.
        bandEdges.front() = 1;
        for (std::size_t i = 1; i < SPECTRUM_BAND_COUNT; ++i) {
            const auto edge = static_cast<std::size_t>(std::pow(
                static_cast<double>(FFT_BIN_COUNT), static_cast<double>(i) / SPECTRUM_BAND_COUNT));
            bandEdges[i] = std::max(bandEdges[i - 1] + 1, edge);
        }
        bandEdges.back() = FFT_BIN_COUNT;

        std::lock_guard lock(GetFftPlannerMutex());
        plan = fftw_plan_dft_r2c_1d(static_cast<int>(SPECTRUM_FFT_SIZE),
            fftInput.data(), fftOutput, FFTW_ESTIMATE);
        if (!plan) {
            throw std::runtime_error("Failed to create spectrum FFT plan");
        }
    }

    ~CImpl()
    {
        std::lock_guard lock(GetFftPlannerMutex());
        fftw_destroy_plan(plan);
    }

    SpectrumPowerBands CalculateBands()
    {
        std::array<double, SPECTRUM_BAND_COUNT> bandPower{};
        const double normalization = SPECTRUM_FFT_SIZE * windowEnergy * audioFmt.numChannels;
        for (uint32_t channel = 0; channel < audioFmt.numChannels; ++channel) {
            const auto& channelSamples = samples[channel];
            const double mean = std::accumulate(channelSamples.begin(), channelSamples.end(), 0.0)
                / SPECTRUM_FFT_SIZE;
            for (std::size_t i = 0; i < SPECTRUM_FFT_SIZE; ++i) {
                fftInput[i] = (channelSamples[i] - mean) * window[i];
            }
            fftw_execute(plan);
            for (std::size_t band = 0; band < SPECTRUM_BAND_COUNT; ++band) {
                for (std::size_t bin = bandEdges[band]; bin < bandEdges[band + 1]; ++bin) {
                    const double real = fftOutput[bin][0];
                    const double imag = fftOutput[bin][1];
                    const double sidedFactor = (bin == SPECTRUM_FFT_SIZE / 2) ? 1.0 : 2.0;
                    bandPower[band] += sidedFactor * (real * real + imag * imag) / normalization;
                }
            }
        }

        SpectrumPowerBands result{};
        for (std::size_t band = 0; band < SPECTRUM_BAND_COUNT; ++band) {
            // Summed energy in logarithmic ranges balances pink/music-like spectra.
            // Map -60..0 dBFS to 0..100, keeping silence exactly zero.
            if (bandPower[band] > 0.0) {
                const double db = 10.0 * std::log10(bandPower[band]);
                result[band] = static_cast<float>(std::clamp(
                    (db - MIN_POWER_DB) * 100.0 / -MIN_POWER_DB, 0.0, 100.0));
            }
        }
        return result;
    }
};

CSpectrumAnalyzer::CSpectrumAnalyzer() : m_impl(std::make_unique<CImpl>())
{
}

CSpectrumAnalyzer::~CSpectrumAnalyzer() = default;

void CSpectrumAnalyzer::Reset()
{
    m_impl->sampleCount = 0;
    m_impl->audioFmt = {};
}

bool CSpectrumAnalyzer::Analyze(const void* audioBuf, uint32_t frames,
    const AudioFormat& format, SpectrumPowerBands& powerBands)
{
    powerBands.fill(0.0f);
    const uint32_t sampleBytes = GetBitsPerSampleByFormat(format.format) / 8;
    // Only the device's interleaved PCM is accepted, not compressed/planar input.
    if (!audioBuf || frames == 0 || sampleBytes == 0 || format.sampleRate == 0
        || format.numChannels == 0 || format.numChannels > MAX_ANALYSIS_CHANNELS
        || format.format < AudioDataFormat::PCM_U8 || format.format > AudioDataFormat::Float64
        || format.blockAlign < sampleBytes * format.numChannels) {
        Reset();
        return false;
    }
    if (!IsSameAudioFormat(&format, &m_impl->audioFmt)) {
        m_impl->sampleCount = 0;
        m_impl->audioFmt = format;
    }

    bool updated = false;
    const auto* pcm = static_cast<const uint8_t*>(audioBuf);
    for (uint32_t frame = 0; frame < frames; ++frame) {
        for (uint32_t channel = 0; channel < format.numChannels; ++channel) {
            m_impl->samples[channel][m_impl->sampleCount] = ReadNormalizedSample(
                pcm + static_cast<std::size_t>(frame) * format.blockAlign + channel * sampleBytes, format.format);
        }
        if (++m_impl->sampleCount == SPECTRUM_FFT_SIZE) {
            const auto result = m_impl->CalculateBands();
            for (std::size_t band = 0; band < SPECTRUM_BAND_COUNT; ++band) {
                powerBands[band] = std::max(powerBands[band], result[band]);
            }
            m_impl->sampleCount = 0;
            updated = true;
        }
    }
    return updated;
}
