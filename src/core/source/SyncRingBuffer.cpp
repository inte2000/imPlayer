#include <chrono>
#include <cstring>

#include "SyncRingBuffer.h"

namespace {

std::size_t MinSize(std::size_t lhs, std::size_t rhs)
{
    return (lhs < rhs) ? lhs : rhs;
}

} // namespace

CSyncRingBuffer::CSyncRingBuffer(std::size_t capacityBytes)
    : m_buffer(capacityBytes)
    , m_readPos(0)
    , m_writePos(0)
    , m_used(0)
    , m_closed(false)
    , m_producerFinished(false)
{
}

void CSyncRingBuffer::Reset(std::size_t capacityBytes)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    m_buffer.assign(capacityBytes, 0);
    m_readPos = 0;
    m_writePos = 0;
    m_used = 0;
    m_closed = false;
    m_producerFinished = false;

    m_dataCv.notify_all();
    m_spaceCv.notify_all();
}

void CSyncRingBuffer::Clear()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    m_readPos = 0;
    m_writePos = 0;
    m_used = 0;
}

void CSyncRingBuffer::Close()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_closed = true;

    m_dataCv.notify_all();
    m_spaceCv.notify_all();
}

void CSyncRingBuffer::SetProducerFinished()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_producerFinished = true;

    m_dataCv.notify_all();
    m_spaceCv.notify_all();
}

std::size_t CSyncRingBuffer::Capacity() const
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_buffer.size();
}

std::size_t CSyncRingBuffer::Read(void* pBuf, std::size_t size, uint32_t timeoutSeconds)
{
    if ((pBuf == nullptr) || (size == 0)) {
        return 0;
    }

    uint8_t* out = static_cast<uint8_t*>(pBuf);
    std::size_t totalRead = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeoutSeconds);

    while (totalRead < size) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_buffer.empty() || m_closed) {
            break;
        }

        auto ready = [&]() {
            return (m_used > 0) || m_closed || m_producerFinished;
        };

        if (m_used == 0) {
            if (timeoutSeconds == 0) {
                m_dataCv.wait(lock, ready);
            }
            else if (!m_dataCv.wait_until(lock, deadline, ready)) {
                break;
            }
        }

        if (m_closed || (m_used == 0)) {
            if (m_closed || m_producerFinished) {
                break;
            }
            if (timeoutSeconds != 0) {
                break;
            }
            continue;
        }

        const std::size_t expected = size - totalRead;
        const std::size_t willRead = MinSize(expected, m_used);
        const std::size_t firstPart = MinSize(willRead, m_buffer.size() - m_readPos);

        std::memcpy(out + totalRead, m_buffer.data() + m_readPos, firstPart);
        if (willRead > firstPart) {
            std::memcpy(out + totalRead + firstPart, m_buffer.data(), willRead - firstPart);
        }

        m_readPos = (m_readPos + willRead) % m_buffer.size();
        m_used -= willRead;
        totalRead += willRead;

        lock.unlock();
        m_spaceCv.notify_one();
    }

    return totalRead;
}

std::size_t CSyncRingBuffer::Write(const uint8_t* pBuf, std::size_t size)
{
    if ((pBuf == nullptr) || (size == 0)) {
        return 0;
    }

    std::size_t totalWrote = 0;
    while (totalWrote < size) {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_buffer.empty() || m_closed) {
            break;
        }

        m_spaceCv.wait(lock, [&]() {
            return (m_used < m_buffer.size()) || m_closed;
        });

        if (m_closed) {
            break;
        }

        const std::size_t writable = m_buffer.size() - m_used;
        if (writable == 0) {
            continue;
        }

        const std::size_t willWrite = MinSize(size - totalWrote, writable);
        const std::size_t firstPart = MinSize(willWrite, m_buffer.size() - m_writePos);

        std::memcpy(m_buffer.data() + m_writePos, pBuf + totalWrote, firstPart);
        if (willWrite > firstPart) {
            std::memcpy(m_buffer.data(), pBuf + totalWrote + firstPart, willWrite - firstPart);
        }

        m_writePos = (m_writePos + willWrite) % m_buffer.size();
        m_used += willWrite;
        totalWrote += willWrite;

        lock.unlock();
        m_dataCv.notify_one();
    }

    return totalWrote;
}
