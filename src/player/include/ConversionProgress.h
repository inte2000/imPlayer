#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <format>
#include <limits>
#include <ostream>
#include <string>

class CConversionProgress final
{
public:
    explicit CConversionProgress(std::ostream& output, uint32_t totalStreams,
        std::chrono::milliseconds refreshInterval = std::chrono::milliseconds(100))
        : m_output(output), m_totalStreams((std::max)(1u, totalStreams)), m_refreshInterval(refreshInterval)
    {
    }

    CConversionProgress(const CConversionProgress&) = delete;
    CConversionProgress& operator=(const CConversionProgress&) = delete;

    ~CConversionProgress()
    {
        // 异常退出只结束当前显示行，不把尚未成功收尾的输出标记为完成。
        if (m_visible && !m_finished) {
            m_output << '\n';
        }
    }

    void Update(uint32_t streamIndex, uint64_t writtenFrames, std::size_t totalFrames)
    {
        if (m_finished) {
            return;
        }
        const auto now = std::chrono::steady_clock::now();
        const bool newStream = !m_visible || streamIndex != m_streamIndex;
        if (!newStream && now - m_lastUpdate < m_refreshInterval) {
            return;
        }

        m_streamIndex = streamIndex;
        m_lastUpdate = now;
        if (totalFrames == 0 || totalFrames == (std::numeric_limits<std::size_t>::max)()) {
            Print(std::format("Conversion progress: stream {}/{}, {} frames written",
                streamIndex + 1, m_totalStreams, writtenFrames));
            return;
        }

        // 各音频流等权累计；使用输出帧数计算，避免重采样造成进度失真。
        const auto fraction = (std::min)(1.0L, static_cast<long double>(writtenFrames) / totalFrames);
        const int percent = (std::max)(m_lastPercent, (std::min)(99,
            static_cast<int>(100.0L * (streamIndex + fraction) / m_totalStreams)));
        if (newStream || percent != m_lastPercent) {
            Print(std::format("Conversion progress: {:3}% (stream {}/{})",
                percent, streamIndex + 1, m_totalStreams));
            m_lastPercent = percent;
        }
    }

    void Finish()
    {
        if (!m_finished) {
            Print("Conversion progress: 100% (complete)");
            m_output << std::endl;
            m_finished = true;
        }
    }

private:
    void Print(const std::string& text)
    {
        m_output << '\r' << text;
        if (text.size() < m_lineLength) {
            m_output << std::string(m_lineLength - text.size(), ' ');
        }
        m_output << std::flush;
        m_lineLength = text.size();
        m_visible = true;
    }

    std::ostream& m_output;
    uint32_t m_totalStreams;
    std::chrono::milliseconds m_refreshInterval;
    std::chrono::steady_clock::time_point m_lastUpdate{};
    uint32_t m_streamIndex = 0;
    int m_lastPercent = -1;
    std::size_t m_lineLength = 0;
    bool m_visible = false;
    bool m_finished = false;
};
