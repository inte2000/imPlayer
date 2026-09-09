#ifndef NET_DATA_STREAM_H
#define NET_DATA_STREAM_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <thread>

#include "DataStream.h"
#include "NetProxyDef.h"
#include "StreamMateSource.h"
#include "SyncRingBuffer.h"

static constexpr uint64_t NetStreamLengthLimit = 3048192000000ull;
static constexpr std::size_t NetStreamDefaultRingBufferBytes = 8 * 1024 * 1024;

enum class NetStreamType
{
    Http = 0
};

class CNetStream : public CDataStream, public MateSource
{
public:
    CNetStream();
    ~CNetStream() override;

    bool Open(const std::wstring& url, NetStreamType type);
    void Close();

    void SetProxy(const NetProxy& proxy);
    void SetRingBufferSize(std::size_t bytes);

    uint32_t Read(void* pBuf, uint32_t size, uint32_t timeout = 0) override;
    uint32_t Write(const void* pBuf, uint32_t size, uint32_t timeout = 0) override;
    std::size_t GetLength() const override;
    void Seek(SeekBase base, long long off) override;
    std::size_t Tell() override;

    std::unique_ptr<CDataStream> CreateMateStream(const wchar_t* name) override;

private:
    static std::size_t CurlWriteCallback(char* ptr, std::size_t size, std::size_t nmemb, void* userdata);
    std::size_t OnCurlWrite(const uint8_t* data, std::size_t bytes);
    void ReaderThreadProc(std::wstring url, NetStreamType type);
    bool IsHttpUrl(const std::wstring& url) const;
    std::string BuildProxyAddress() const;
    long ConvertProxyType(NetProxyType type) const;

private:
    std::size_t m_length;
    std::size_t m_curPos;

    std::size_t m_ringSizeBytes;
    CSyncRingBuffer m_ringBuffer;
    std::thread m_readerThread;
    std::atomic<bool> m_stopRequested;
    bool m_opened;

    NetProxy m_proxy;
};

std::unique_ptr<CDataStream> MakeNetStream(const std::wstring& url, NetStreamType type);

#endif //NET_DATA_STREAM_H
