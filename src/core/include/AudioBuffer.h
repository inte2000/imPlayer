#pragma once


#include <atomic>
#include "AudioInfo.h"


class AudioBuffer final
{
public:

	// Constructs a RingBufferT with size = 0
	AudioBuffer() : m_pData(nullptr), m_capacity(0), m_frameSize(0), m_WriteIndex(0), m_ReadIndex(0)
	{
	}

    // Constructs a RingBufferT with \a count maximum elements.
	AudioBuffer(std::size_t frames, uint32_t frameSize) : m_pData(nullptr), m_WriteIndex(0), m_ReadIndex(0)
	{ 
		m_capacity = frames;
		m_frameSize = frameSize;
		AllocBuffer(frames, frameSize);
	}

	AudioBuffer(const AudioBuffer& other) = delete;
	AudioBuffer(AudioBuffer&& other) = delete;

	~AudioBuffer() {
		ReleaseBuffer();
	}

	bool ResetBuffer(std::size_t frames, uint32_t frameSize)
	{
		ReleaseBuffer();
		m_capacity = frames;
		m_frameSize = frameSize;
		AllocBuffer(frames, frameSize);

		return true;
	}

	void Clear()
	{
		// SPSC：producer / consumer 都可调用，但要“重置流”时使用
		m_ReadIndex.store(0, std::memory_order_relaxed);
		m_WriteIndex.store(0, std::memory_order_relaxed);
	}

	std::size_t GetAvailableRead() const
	{
		//std::lock_guard guard(m_mtx);

		const uint64_t write = m_WriteIndex.load(std::memory_order_acquire);
		const uint64_t read = m_ReadIndex.load(std::memory_order_acquire);
		return static_cast<std::size_t>(write - read);
	}

	std::size_t GetAvailableWrite() const
	{
		return m_capacity - GetAvailableRead();
	}

	// Producer thread only
	bool Write(const uint8_t* pData, std::size_t frames)
	{
		//std::lock_guard guard(m_mtx);

		const uint64_t read = m_ReadIndex.load(std::memory_order_acquire);
		const uint64_t write = m_WriteIndex.load(std::memory_order_relaxed);

		if (frames > m_capacity - (write - read))
			return false;

		std::size_t writePos = static_cast<std::size_t>(write % m_capacity);
 		std::size_t firstPart = std::min(frames, m_capacity - writePos);
		std::memcpy(&m_pData[writePos * m_frameSize], pData, firstPart * m_frameSize);
		if (frames > firstPart)
		{
			std::memcpy(&m_pData[0], pData + firstPart * m_frameSize, (frames - firstPart) * m_frameSize);
		}

		// 发布写入
		m_WriteIndex.store(write + frames, std::memory_order_release);
		return true;
	}

	// Consumer thread only
	bool Read(uint8_t* pData, std::size_t frames, AudioDataFormat format)
	{
		//std::lock_guard guard(m_mtx);

		const uint64_t write = m_WriteIndex.load(std::memory_order_acquire);
		const uint64_t read = m_ReadIndex.load(std::memory_order_relaxed);

		if (frames > (write - read))
			return false;

		std::size_t readPos = static_cast<std::size_t>(read % m_capacity);
		std::size_t firstPart =	std::min(frames, m_capacity - readPos);

		uint32_t channels = m_frameSize / sizeof(float);
		std::memcpy(pData, &m_pData[readPos * m_frameSize], firstPart * m_frameSize);
		if (frames > firstPart)
		{
			std::memcpy(pData + firstPart * m_frameSize, &m_pData[0], (frames - firstPart) * m_frameSize);
		}

		m_ReadIndex.store(read + frames, std::memory_order_release);
		return true;
	}

private:
	void AllocBuffer(std::size_t frames, uint32_t frameSize)
	{
		m_pData = new uint8_t[frames * frameSize];

		Clear();
	}
	void ReleaseBuffer()
	{
		if (m_pData)
			delete[] m_pData;

		m_capacity = 0;
		Clear();
	}

	uint8_t *m_pData;
	std::size_t m_capacity;   // frames
	uint32_t m_frameSize;  // bytes per frame
	std::atomic<std::size_t> m_WriteIndex, m_ReadIndex;
	//mutable std::mutex m_mtx;
};

