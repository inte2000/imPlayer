#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <limits>
#include <mutex>
#include <numbers>
#include <vector>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include "SpectrumAnalyzer.h"
#include "SpectrumDisplay.h"

using namespace std::chrono_literals;

namespace {

AudioFormat MakeFormat(AudioDataFormat sampleFormat = AudioDataFormat::Float32,
    uint32_t channels = 1, uint32_t sampleRate = 48000)
{
    AudioFormat format{};
    InitAudioFormat(&format, sampleFormat, channels, sampleRate);
    return format;
}

std::vector<float> MakeTone(std::size_t bin, float amplitude = 0.5f,
    uint32_t channels = 1, bool oppositePhase = false)
{
    std::vector<float> samples(SPECTRUM_FFT_SIZE * channels);
    for (std::size_t frame = 0; frame < SPECTRUM_FFT_SIZE; ++frame) {
        const float sample = amplitude * static_cast<float>(std::sin(
            2.0 * std::numbers::pi * bin * frame / SPECTRUM_FFT_SIZE));
        for (uint32_t channel = 0; channel < channels; ++channel) {
            samples[frame * channels + channel] = oppositePhase && channel % 2 ? -sample : sample;
        }
    }
    return samples;
}

std::size_t PeakBand(const SpectrumPowerBands& bands)
{
    return static_cast<std::size_t>(std::distance(bands.begin(), std::max_element(bands.begin(), bands.end())));
}

void CheckFiniteBands(const SpectrumPowerBands& bands)
{
    for (float value : bands) {
        CHECK(std::isfinite(value));
        CHECK(value >= 0.0f);
        CHECK(value <= 100.0f);
    }
}

template<typename T>
void StoreSample(uint8_t* destination, T value)
{
    std::memcpy(destination, &value, sizeof(value));
}

std::vector<uint8_t> EncodeTone(AudioDataFormat sampleFormat)
{
    const auto tone = MakeTone(32);
    const auto format = MakeFormat(sampleFormat);
    std::vector<uint8_t> pcm(tone.size() * format.blockAlign);
    for (std::size_t i = 0; i < tone.size(); ++i) {
        auto* destination = pcm.data() + i * format.blockAlign;
        const double sample = tone[i];
        switch (sampleFormat) {
        case AudioDataFormat::PCM_U8:
            StoreSample(destination, static_cast<uint8_t>(std::lround(sample * 128.0 + 128.0)));
            break;
        case AudioDataFormat::PCM_S8:
            StoreSample(destination, static_cast<int8_t>(std::lround(sample * 128.0)));
            break;
        case AudioDataFormat::PCM_S16:
            StoreSample(destination, static_cast<int16_t>(std::lround(sample * 32768.0)));
            break;
        case AudioDataFormat::PCM_S24: {
            const auto packed = static_cast<uint32_t>(static_cast<int32_t>(std::lround(sample * 8388608.0)));
            destination[0] = static_cast<uint8_t>(packed);
            destination[1] = static_cast<uint8_t>(packed >> 8);
            destination[2] = static_cast<uint8_t>(packed >> 16);
            break;
        }
        case AudioDataFormat::PCM_S24_32:
            StoreSample(destination, static_cast<int32_t>(std::lround(sample * 8388608.0)) * 256);
            break;
        case AudioDataFormat::PCM_S32:
            StoreSample(destination, static_cast<int32_t>(std::llround(sample * 2147483648.0)));
            break;
        case AudioDataFormat::PCM_64:
            StoreSample(destination, static_cast<int64_t>(std::llround(sample * 9223372036854775808.0)));
            break;
        case AudioDataFormat::Float32:
            StoreSample(destination, static_cast<float>(sample));
            break;
        case AudioDataFormat::Float64:
            StoreSample(destination, sample);
            break;
        default:
            break;
        }
    }
    return pcm;
}

} // namespace

TEST_CASE("Spectrum orders low middle and high tones in logarithmic bands", "[player][spectrum]")
{
    CSpectrumAnalyzer analyzer;
    SpectrumPowerBands bands{};
    std::array<std::size_t, 3> peaks{};
    const std::array<std::size_t, 3> toneBins{3, 32, 350};
    for (std::size_t i = 0; i < toneBins.size(); ++i) {
        const auto samples = MakeTone(toneBins[i]);
        REQUIRE(analyzer.Analyze(samples.data(), SPECTRUM_FFT_SIZE, MakeFormat(), bands));
        CheckFiniteBands(bands);
        peaks[i] = PeakBand(bands);
        CHECK(bands[peaks[i]] > 60.0f);
    }
    CHECK(peaks[0] < peaks[1]);
    CHECK(peaks[1] < peaks[2]);
    CHECK(peaks[2] == SPECTRUM_BAND_COUNT - 1);
}

TEST_CASE("Spectrum accumulates partial device buffers without changing FFT results", "[player][spectrum]")
{
    const auto tone = MakeTone(32);
    CSpectrumAnalyzer whole;
    CSpectrumAnalyzer split;
    SpectrumPowerBands expected{};
    SpectrumPowerBands actual{};
    REQUIRE(whole.Analyze(tone.data(), SPECTRUM_FFT_SIZE, MakeFormat(), expected));
    CHECK_FALSE(split.Analyze(tone.data(), 137, MakeFormat(), actual));
    CHECK_FALSE(split.Analyze(tone.data() + 137, 211, MakeFormat(), actual));
    REQUIRE(split.Analyze(tone.data() + 348, SPECTRUM_FFT_SIZE - 348, MakeFormat(), actual));
    for (std::size_t i = 0; i < expected.size(); ++i) {
        CHECK(actual[i] == Catch::Approx(expected[i]).margin(0.001));
    }

    auto multipleWindows = tone;
    multipleWindows.resize(SPECTRUM_FFT_SIZE * 2, 0.0f);
    split.Reset();
    REQUIRE(split.Analyze(multipleWindows.data(), SPECTRUM_FFT_SIZE * 2, MakeFormat(), actual));
    CHECK(actual[PeakBand(expected)] == Catch::Approx(expected[PeakBand(expected)]).margin(0.001));
}

TEST_CASE("Spectrum handles device PCM formats including packed and container 24 bit", "[player][spectrum]")
{
    const std::array formats{
        AudioDataFormat::PCM_U8, AudioDataFormat::PCM_S8, AudioDataFormat::PCM_S16,
        AudioDataFormat::PCM_S24, AudioDataFormat::PCM_S24_32, AudioDataFormat::PCM_S32,
        AudioDataFormat::PCM_64, AudioDataFormat::Float32, AudioDataFormat::Float64,
    };
    CSpectrumAnalyzer analyzer;
    SpectrumPowerBands reference{};
    const auto tone = MakeTone(32);
    REQUIRE(analyzer.Analyze(tone.data(), SPECTRUM_FFT_SIZE, MakeFormat(), reference));
    for (auto format : formats) {
        INFO("PCM format: " << static_cast<int>(format));
        const auto pcm = EncodeTone(format);
        SpectrumPowerBands actual{};
        auto audioFormat = MakeFormat(format);
        if (format == AudioDataFormat::PCM_S24_32) {
            audioFormat.bitsPerSample = 24; // WAVEFORMATEXTENSIBLE valid bits, 4-byte storage.
        }
        REQUIRE(analyzer.Analyze(pcm.data(), SPECTRUM_FFT_SIZE, audioFormat, actual));
        CHECK(PeakBand(actual) == PeakBand(reference));
        CHECK(actual[PeakBand(reference)] == Catch::Approx(reference[PeakBand(reference)]).margin(0.5));
        CheckFiniteBands(actual);
    }
}

TEST_CASE("Spectrum ignores silence DC and nonfinite samples and responds to level", "[player][spectrum]")
{
    CSpectrumAnalyzer analyzer;
    SpectrumPowerBands bands{};
    std::vector<float> samples(SPECTRUM_FFT_SIZE, 0.0f);
    for (float value : {0.0f, 0.25f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()}) {
        std::fill(samples.begin(), samples.end(), value);
        REQUIRE(analyzer.Analyze(samples.data(), SPECTRUM_FFT_SIZE, MakeFormat(), bands));
        CHECK(std::all_of(bands.begin(), bands.end(), [](float band) { return band == 0.0f; }));
    }
    auto tone = MakeTone(32, 0.5f);
    REQUIRE(analyzer.Analyze(tone.data(), SPECTRUM_FFT_SIZE, MakeFormat(), bands));
    const float loud = bands[PeakBand(bands)];
    tone = MakeTone(32, 0.05f);
    REQUIRE(analyzer.Analyze(tone.data(), SPECTRUM_FFT_SIZE, MakeFormat(), bands));
    CHECK(bands[PeakBand(bands)] < loud);
    CHECK(bands[PeakBand(bands)] < 60.0f);
    tone = MakeTone(32, 0.000001f);
    REQUIRE(analyzer.Analyze(tone.data(), SPECTRUM_FFT_SIZE, MakeFormat(), bands));
    CHECK(std::all_of(bands.begin(), bands.end(), [](float band) { return band == 0.0f; }));
}

TEST_CASE("Spectrum preserves energy of opposite phase stereo", "[player][spectrum]")
{
    CSpectrumAnalyzer analyzer;
    SpectrumPowerBands mono{};
    SpectrumPowerBands stereo{};
    const auto monoTone = MakeTone(32);
    const auto stereoTone = MakeTone(32, 0.5f, 2, true);
    REQUIRE(analyzer.Analyze(monoTone.data(), SPECTRUM_FFT_SIZE, MakeFormat(), mono));
    REQUIRE(analyzer.Analyze(stereoTone.data(), SPECTRUM_FFT_SIZE, MakeFormat(AudioDataFormat::Float32, 2), stereo));
    CHECK(stereo[PeakBand(mono)] == Catch::Approx(mono[PeakBand(mono)]).margin(0.001));
}

TEST_CASE("Spectrum format changes and reset discard incomplete old audio", "[player][spectrum]")
{
    CSpectrumAnalyzer analyzer;
    SpectrumPowerBands bands{};
    const auto tone = MakeTone(32);
    std::vector<float> silence(SPECTRUM_FFT_SIZE, 0.0f);
    CHECK_FALSE(analyzer.Analyze(tone.data(), 512, MakeFormat(), bands));
    const auto changedFormat = MakeFormat(AudioDataFormat::Float32, 1, 44100);
    CHECK_FALSE(analyzer.Analyze(silence.data(), 512, changedFormat, bands));
    REQUIRE(analyzer.Analyze(silence.data(), 512, changedFormat, bands));
    CHECK(std::all_of(bands.begin(), bands.end(), [](float value) { return value == 0.0f; }));

    CHECK_FALSE(analyzer.Analyze(tone.data(), 512, changedFormat, bands));
    analyzer.Reset();
    CHECK_FALSE(analyzer.Analyze(silence.data(), 512, changedFormat, bands));
    CHECK_FALSE(analyzer.Analyze(nullptr, 512, changedFormat, bands));
    auto invalidFormat = MakeFormat();
    invalidFormat.blockAlign = 1;
    CHECK_FALSE(analyzer.Analyze(tone.data(), SPECTRUM_FFT_SIZE, invalidFormat, bands));
    invalidFormat = MakeFormat();
    invalidFormat.format = AudioDataFormat::Float32P;
    CHECK_FALSE(analyzer.Analyze(tone.data(), SPECTRUM_FFT_SIZE, invalidFormat, bands));
}

TEST_CASE("Spectrum display retains peaks and decays ten points per forty milliseconds", "[player][spectrum][display]")
{
    CSpectrumDisplay display;
    SpectrumPowerBands values{};
    values.fill(100.0f);
    display.SetPowerBand(values.data(), values.size());
    display.Advance(39ms);
    CHECK(display.GetPowerBands()[0] == 100.0f);
    display.Advance(1ms);
    CHECK(display.GetPowerBands()[0] == 90.0f);
    values.fill(50.0f);
    display.SetPowerBand(values.data(), values.size());
    CHECK(display.GetPowerBands()[0] == 90.0f);
    display.Advance(80ms);
    CHECK(display.GetPowerBands()[0] == 70.0f);
    display.Advance(400ms);
    CHECK(display.GetPowerBands() == SpectrumPowerBands{});
}

TEST_CASE("Spectrum display clamps input handles missing bands and resets", "[player][spectrum][display]")
{
    CSpectrumDisplay display;
    const std::array<float, 4> values{-2.0f, 200.0f, std::numeric_limits<float>::quiet_NaN(), 55.0f};
    display.SetPowerBand(values.data(), values.size());
    const auto bands = display.GetPowerBands();
    CHECK(bands[0] == 0.0f);
    CHECK(bands[1] == 100.0f);
    CHECK(bands[2] == 0.0f);
    CHECK(bands[3] == 55.0f);
    CHECK(bands[9] == 0.0f);
    display.SetPowerBand(nullptr, SPECTRUM_BAND_COUNT);
    CHECK(display.GetPowerBands() == bands);
    display.Reset();
    CHECK(display.GetPowerBands() == SpectrumPowerBands{});
}

TEST_CASE("Spectrum renderer colors high bands red lower bands green and clears zero bars", "[player][spectrum][display]")
{
    using namespace ftxui;
    CSpectrumDisplay display;
    SpectrumPowerBands values{};
    values.fill(60.0f);
    values[0] = 70.0f;
    display.SetPowerBand(values.data(), values.size());
    Screen screen(41, 8);
    Render(screen, display.Render());
    bool red = false;
    bool green = false;
    for (int y = 1; y <= 5; ++y) {
        for (int x = 1; x < screen.dimx() - 1; ++x) {
            const auto& pixel = screen.PixelAt(x, y);
            if (!pixel.character.empty() && pixel.character != " ") {
                red |= pixel.foreground_color == Color::Red;
                green |= pixel.foreground_color == Color::Green;
            }
        }
    }
    CHECK(red);
    CHECK(green);

    int visibleBars = 0;
    bool previousFilled = false;
    for (int x = 1; x < screen.dimx() - 1; ++x) {
        const auto& pixel = screen.PixelAt(x, 5);
        const bool filled = !pixel.character.empty() && pixel.character != " ";
        if (filled && !previousFilled) {
            ++visibleBars;
        }
        previousFilled = filled;
    }
    CHECK(visibleBars == SPECTRUM_BAND_COUNT);

    display.Advance(400ms);
    screen.Clear();
    Render(screen, display.Render());
    for (int y = 1; y <= 5; ++y) {
        for (int x = 1; x < screen.dimx() - 1; ++x) {
            const auto& pixel = screen.PixelAt(x, y);
            CHECK((pixel.character.empty() || pixel.character == " "));
        }
    }
}

TEST_CASE("Spectrum timer refreshes and joins before display teardown", "[player][spectrum][display]")
{
    CSpectrumDisplay display;
    SpectrumPowerBands values{};
    values.fill(100.0f);
    display.SetPowerBand(values.data(), values.size());
    std::mutex mutex;
    std::condition_variable wake;
    int refreshes = 0;
    display.StartRefresh([&] {
        std::lock_guard lock(mutex);
        ++refreshes;
        wake.notify_one();
    });
    bool refreshed = false;
    {
        std::unique_lock lock(mutex);
        refreshed = wake.wait_for(lock, 2s, [&] { return refreshes > 0; });
    }
    display.StopRefresh();
    REQUIRE(refreshed);
    CHECK(display.GetPowerBands()[0] <= 90.0f);
    // StopRefresh has joined the callback thread, so captured objects can be destroyed.
    display.StopRefresh();
}
