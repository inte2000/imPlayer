#include <cassert>
#include <algorithm>
#include <format>
#include "MemBufStream.h"

const uint32_t MEM_GRANULARITY = 4 * 1024 * 1024;

std::unique_ptr<CDataStream> MakeMemoryBufStream(std::size_t totalSize, bool bReadOnly)
{
    auto ptr = std::make_unique<CMemoryBufStream>(bReadOnly);
    if (ptr)
    {
        ptr->Open(totalSize);
    }

    return ptr;
}


bool CMemoryBufStream::Open(std::size_t totalSize)
{
    assert(!m_pBuf);

    m_pBuf = std::make_unique<uint8_t[]>(totalSize);
    if (!m_pBuf)
        return false;

    m_length = 0;
    m_curPos = 0;
    m_totalSize = totalSize;
    m_name = std::format(L"Memory Buffer: {}", reinterpret_cast<long long>(m_pBuf.get()));

    return true;
}

uint32_t CMemoryBufStream::Read(void* pBuf, uint32_t size, uint32_t timeout)
{
    if (!m_pBuf)
        return 0;
    if (m_curPos >= m_length)
        return 0;

    std::size_t remained = m_length - m_curPos;
    std::size_t readed = (remained > size) ? size : remained;
    std::memcpy(pBuf, m_pBuf.get() + m_curPos, readed);
    m_curPos += readed;

    return static_cast<uint32_t>(readed);
}

uint32_t CMemoryBufStream::Write(const void* pBuf, uint32_t size, uint32_t timeout)
{
    if (m_bReadOnly || !m_pBuf)
        return 0;

    std::size_t remained = m_totalSize - m_curPos;
    if (remained < static_cast<std::size_t>(size))
    {
        std::size_t newSize = m_totalSize + std::max(size, MEM_GRANULARITY);
        if (!ResetSize(newSize))
            return 0;
    }

    std::memcpy(m_pBuf.get() + m_curPos, pBuf, size);

    m_curPos += size;
    if (m_curPos > m_length)
        m_length = m_curPos;

    return size;
}

void CMemoryBufStream::Seek(SeekBase base, long long off)
{
    if (!m_pBuf)
        return;

    bool bNegLeft = (off < 0);
    std::size_t poff = std::abs(off);

    if (base == SeekBase::Begin)
        m_curPos = bNegLeft ? 0 : poff;
    else if (base == SeekBase::End)
        m_curPos = bNegLeft ? (m_length - poff) : 0;
    else
        m_curPos = bNegLeft ? (m_curPos - poff) : (m_curPos + poff);

    m_curPos = std::clamp(m_curPos, (std::size_t)0, m_length);
}

bool CMemoryBufStream::ResetSize(std::size_t newSize)
{
    uint8_t* pnewBuf = new (std::nothrow) uint8_t[newSize];
    if (!pnewBuf)
        return false;
    
    std::memcpy(pnewBuf, m_pBuf.get(), m_length);
    m_pBuf.reset(pnewBuf);

    m_totalSize = newSize;
    return true;
}


#if 0
void CFileStream::Seek(SeekBase base, long long off)
{
    if (m_file)
    {
        if(base == SeekBase::Begin)
            m_file.seekg(off, std::ios::beg);
        else if (base == SeekBase::End)
            m_file.seekg(off, std::ios::end);
        else
            m_file.seekg(off, std::ios::cur);
    }
}
#endif
