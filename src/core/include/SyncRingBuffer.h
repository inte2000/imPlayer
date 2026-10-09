#ifndef SYNC_RING_BUFFER_H
#define SYNC_RING_BUFFER_H

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

class CSyncRingBuffer
{
public:
    explicit CSyncRingBuffer(std::size_t capacityBytes = 0);

    CSyncRingBuffer(const CSyncRingBuffer&) = delete;
    CSyncRingBuffer& operator=(const CSyncRingBuffer&) = delete;
    CSyncRingBuffer(CSyncRingBuffer&&) = delete;
    CSyncRingBuffer& operator=(CSyncRingBuffer&&) = delete;

    void Reset(std::size_t capacityBytes);
    void Clear();
    void Close();
    void SetProducerFinished();

    std::size_t Capacity() const;
    std::size_t Read(void* pBuf, std::size_t size, uint32_t timeoutSeconds);
    std::size_t Write(const uint8_t* pBuf, std::size_t size);

private:
    std::vector<uint8_t> m_buffer;
    std::size_t m_readPos;
    std::size_t m_writePos;
    std::size_t m_used;
    bool m_closed;
    bool m_producerFinished;

    mutable std::mutex m_mutex;
    std::condition_variable m_dataCv;
    std::condition_variable m_spaceCv;
};

#endif //SYNC_RING_BUFFER_H
