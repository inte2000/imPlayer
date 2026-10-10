#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

#include <ftxui/dom/elements.hpp>

#include "SpectrumAnalyzer.h"

class CSpectrumDisplay final
{
public:
    static constexpr std::chrono::milliseconds REFRESH_INTERVAL{50};
    static constexpr float DECAY_PER_TICK = 10.0f;

    CSpectrumDisplay() = default;
    ~CSpectrumDisplay();
    CSpectrumDisplay(const CSpectrumDisplay&) = delete;
    CSpectrumDisplay& operator=(const CSpectrumDisplay&) = delete;

    // Audio-thread producer: retain peaks without locks or allocations.
    void SetPowerBand(const float* powerBands, std::size_t bands);
    void Reset();
    SpectrumPowerBands GetPowerBands() const;
    ftxui::Element Render(int height = 5) const;

    void StartRefresh(std::function<void()> refresh);
    void StopRefresh();
    // Timer-thread consumer; also permits deterministic elapsed-time tests.
    void Advance(std::chrono::milliseconds elapsed);

private:
    std::array<std::atomic<float>, SPECTRUM_BAND_COUNT> m_powerBands{};
    std::chrono::milliseconds m_decayElapsed{0};
    std::jthread m_timer;
    std::mutex m_timerMutex;
    std::condition_variable m_timerWake;
};
