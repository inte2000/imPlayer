#include <algorithm>
#include <cmath>
#include <utility>

#include "SpectrumDisplay.h"

static_assert(std::atomic<float>::is_always_lock_free);

CSpectrumDisplay::~CSpectrumDisplay()
{
    StopRefresh();
}

void CSpectrumDisplay::SetPowerBand(const float* powerBands, std::size_t bands)
{
    if (!powerBands) {
        return;
    }
    for (std::size_t band = 0; band < std::min(bands, SPECTRUM_BAND_COUNT); ++band) {
        const float value = std::isfinite(powerBands[band])
            ? std::clamp(powerBands[band], 0.0f, 100.0f) : 0.0f;
        float current = m_powerBands[band].load(std::memory_order_relaxed);
        while (current < value && !m_powerBands[band].compare_exchange_weak(
            current, value, std::memory_order_relaxed)) {
        }
    }
}

void CSpectrumDisplay::Reset()
{
    for (auto& band : m_powerBands) {
        band.store(0.0f, std::memory_order_relaxed);
    }
}

SpectrumPowerBands CSpectrumDisplay::GetPowerBands() const
{
    SpectrumPowerBands result{};
    for (std::size_t band = 0; band < result.size(); ++band) {
        result[band] = m_powerBands[band].load(std::memory_order_relaxed);
    }
    return result;
}

ftxui::Element CSpectrumDisplay::Render(int height) const
{
    using namespace ftxui;
    Elements columns;
    const auto powerBands = GetPowerBands();
    for (std::size_t band = 0; band < powerBands.size(); ++band) {
        if (band != 0) {
            columns.push_back(text(" "));
        }
        const auto bandColor = powerBands[band] > 60.0f ? Color::Red : Color::Green;
        columns.push_back(gaugeUp(powerBands[band] / 100.0f)
            | size(HEIGHT, EQUAL, std::max(1, height)) | xflex | color(bandColor));
    }
    return vbox({
        hbox(std::move(columns)),
        hbox({text("Low"), filler(), text("High")}) | dim,
    }) | bgcolor(Color::Black) | border;
}

void CSpectrumDisplay::Advance(std::chrono::milliseconds elapsed)
{
    if (elapsed.count() <= 0) {
        return;
    }
    m_decayElapsed += elapsed;
    const auto ticks = m_decayElapsed / REFRESH_INTERVAL;
    m_decayElapsed %= REFRESH_INTERVAL;
    const float decay = static_cast<float>(std::min<int64_t>(ticks, 10)) * DECAY_PER_TICK;
    if (decay == 0.0f) {
        return;
    }
    for (auto& band : m_powerBands) {
        float current = band.load(std::memory_order_relaxed);
        while (!band.compare_exchange_weak(current, std::max(0.0f, current - decay),
            std::memory_order_relaxed)) {
        }
    }
}

void CSpectrumDisplay::StartRefresh(std::function<void()> refresh)
{
    StopRefresh();
    m_decayElapsed = std::chrono::milliseconds(0);
    m_timer = std::jthread([this, refresh = std::move(refresh)](std::stop_token stop) {
        auto previous = std::chrono::steady_clock::now();
        auto nextTick = previous + REFRESH_INTERVAL;
        while (!stop.stop_requested()) {
            std::unique_lock lock(m_timerMutex);
            if (m_timerWake.wait_until(lock, nextTick, [&] { return stop.stop_requested(); })) {
                break;
            }
            lock.unlock();
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - previous);
            previous += elapsed;
            Advance(elapsed);
            if (refresh) {
                refresh();
            }
            nextTick += REFRESH_INTERVAL;
            if (nextTick <= now) {
                nextTick = now + REFRESH_INTERVAL;
            }
        }
    });
}

void CSpectrumDisplay::StopRefresh()
{
    if (m_timer.joinable()) {
        m_timer.request_stop();
        // Synchronize with the wait predicate to avoid a lost stop notification.
        {
            std::lock_guard lock(m_timerMutex);
        }
        m_timerWake.notify_all();
        m_timer.join();
    }
}
