#include <chrono>
#include <cstring>
#include <cwctype>
#include <mutex>

#include <curl/curl.h>

#include "NetStream.h"
#include "UnicodeConvert.h"

namespace {

std::once_flag g_curlInitFlag;
bool g_curlInitOk = false;

std::size_t MinSize(std::size_t lhs, std::size_t rhs)
{
    return (lhs < rhs) ? lhs : rhs;
}

void InitCurlOnce()
{
    g_curlInitOk = (curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
}

} // namespace

std::unique_ptr<CDataStream> MakeNetStream(const std::wstring& url, NetStreamType type)
{
    auto stream = std::make_unique<CNetStream>();
    if (!stream->Open(url, type)) {
        return nullptr;
    }
    return stream;
}

CNetStream::CNetStream()
    : m_length(0)
    , m_curPos(0)
    , m_ringSizeBytes(NetStreamDefaultRingBufferBytes)
    , m_ringBuffer()
    , m_ringReadPos(0)
    , m_ringWritePos(0)
    , m_ringUsedBytes(0)
    , m_readerThread()
    , m_stopRequested(false)
    , m_readerFinished(true)
    , m_opened(false)
    , m_proxy{NetProxyType::none, "", 0, "", ""}
{
    m_style = dsStyleFixedLength | dsStyleTellPos;
    m_ringBuffer.resize(m_ringSizeBytes);
}

CNetStream::~CNetStream()
{
    Close();
}

bool CNetStream::Open(const std::wstring& url, NetStreamType type)
{
    Close();

    if (type != NetStreamType::Http) {
        return false;
    }
    if (!IsHttpUrl(url)) {
        return false;
    }

    std::call_once(g_curlInitFlag, InitCurlOnce);
    if (!g_curlInitOk) {
        return false;
    }

    if (m_ringSizeBytes == 0) {
        m_ringSizeBytes = NetStreamDefaultRingBufferBytes;
    }

    {
        std::lock_guard<std::mutex> guard(m_ringMutex);
        m_ringBuffer.assign(m_ringSizeBytes, 0);
        m_ringReadPos = 0;
        m_ringWritePos = 0;
        m_ringUsedBytes = 0;
    }

    m_name = url;
    m_length = static_cast<std::size_t>(NetStreamLengthLimit);
    m_curPos = 0;
    m_stopRequested.store(false, std::memory_order_release);
    m_readerFinished = false;
    m_opened = true;

    try {
        m_readerThread = std::thread(&CNetStream::ReaderThreadProc, this, url, type);
    }
    catch (...) {
        m_opened = false;
        m_readerFinished = true;
        m_stopRequested.store(true, std::memory_order_release);
        return false;
    }

    return true;
}

void CNetStream::Close()
{
    m_stopRequested.store(true, std::memory_order_release);
    m_dataCv.notify_all();
    m_spaceCv.notify_all();

    if (m_readerThread.joinable()) {
        m_readerThread.join();
    }

    {
        std::lock_guard<std::mutex> guard(m_ringMutex);
        m_ringReadPos = 0;
        m_ringWritePos = 0;
        m_ringUsedBytes = 0;
        m_readerFinished = true;
    }

    m_opened = false;
    m_curPos = 0;
    m_length = 0;
    m_name.clear();
}

void CNetStream::SetProxy(const NetProxy& proxy)
{
    if (m_opened) {
        return;
    }
    m_proxy = proxy;
}

void CNetStream::SetRingBufferSize(std::size_t bytes)
{
    if (bytes == 0) {
        return;
    }
    if (m_opened) {
        return;
    }

    m_ringSizeBytes = bytes;
    m_ringBuffer.assign(m_ringSizeBytes, 0);
    m_ringReadPos = 0;
    m_ringWritePos = 0;
    m_ringUsedBytes = 0;
}

uint32_t CNetStream::Read(void* pBuf, uint32_t size, uint32_t timeout)
{
    if ((pBuf == nullptr) || (size == 0) || !m_opened) {
        return 0;
    }

    auto* out = static_cast<uint8_t*>(pBuf);
    std::size_t totalRead = 0;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout);

    while (totalRead < size) {
        std::unique_lock<std::mutex> lock(m_ringMutex);

        auto ready = [&]() {
            return (m_ringUsedBytes > 0) || m_readerFinished || m_stopRequested.load(std::memory_order_acquire);
        };

        if (m_ringUsedBytes == 0) {
            if (timeout == 0) {
                m_dataCv.wait(lock, ready);
            }
            else if (!m_dataCv.wait_until(lock, deadline, ready)) {
                break;
            }
        }

        if (m_ringUsedBytes == 0) {
            if (m_readerFinished || m_stopRequested.load(std::memory_order_acquire)) {
                break;
            }
            if (timeout != 0) {
                break;
            }
            continue;
        }

        const std::size_t expected = static_cast<std::size_t>(size) - totalRead;
        const std::size_t copyBytes = MinSize(expected, m_ringUsedBytes);
        const std::size_t firstPart = MinSize(copyBytes, m_ringBuffer.size() - m_ringReadPos);

        std::memcpy(out + totalRead, m_ringBuffer.data() + m_ringReadPos, firstPart);
        if (copyBytes > firstPart) {
            std::memcpy(out + totalRead + firstPart, m_ringBuffer.data(), copyBytes - firstPart);
        }

        m_ringReadPos = (m_ringReadPos + copyBytes) % m_ringBuffer.size();
        m_ringUsedBytes -= copyBytes;
        totalRead += copyBytes;

        lock.unlock();
        m_spaceCv.notify_one();
    }

    m_curPos += totalRead;
    return static_cast<uint32_t>(totalRead);
}

uint32_t CNetStream::Write(const void* pBuf, uint32_t size, uint32_t timeout)
{
    (void)pBuf;
    (void)size;
    (void)timeout;
    return 0;
}

std::size_t CNetStream::GetLength() const
{
    return m_length;
}

void CNetStream::Seek(SeekBase base, long long off)
{
    (void)base;
    (void)off;
}

std::size_t CNetStream::Tell()
{
    return m_curPos;
}

std::unique_ptr<CDataStream> CNetStream::CreateMateStream(const wchar_t* name)
{
    (void)name;
    return nullptr;
}

std::size_t CNetStream::CurlWriteCallback(char* ptr, std::size_t size, std::size_t nmemb, void* userdata)
{
    if ((ptr == nullptr) || (userdata == nullptr)) {
        return 0;
    }

    auto* stream = static_cast<CNetStream*>(userdata);
    return stream->OnCurlWrite(reinterpret_cast<uint8_t*>(ptr), size * nmemb);
}

std::size_t CNetStream::OnCurlWrite(const uint8_t* data, std::size_t bytes)
{
    if ((data == nullptr) || (bytes == 0)) {
        return 0;
    }

    std::size_t wrote = 0;
    while (wrote < bytes) {
        std::unique_lock<std::mutex> lock(m_ringMutex);

        m_spaceCv.wait(lock, [&]() {
            return (m_ringUsedBytes < m_ringBuffer.size()) || m_stopRequested.load(std::memory_order_acquire);
        });

        if (m_stopRequested.load(std::memory_order_acquire)) {
            break;
        }

        const std::size_t writable = m_ringBuffer.size() - m_ringUsedBytes;
        if (writable == 0) {
            continue;
        }

        const std::size_t willWrite = MinSize(bytes - wrote, writable);
        const std::size_t firstPart = MinSize(willWrite, m_ringBuffer.size() - m_ringWritePos);

        std::memcpy(m_ringBuffer.data() + m_ringWritePos, data + wrote, firstPart);
        if (willWrite > firstPart) {
            std::memcpy(m_ringBuffer.data(), data + wrote + firstPart, willWrite - firstPart);
        }

        m_ringWritePos = (m_ringWritePos + willWrite) % m_ringBuffer.size();
        m_ringUsedBytes += willWrite;
        wrote += willWrite;

        lock.unlock();
        m_dataCv.notify_one();
    }

    return wrote;
}

void CNetStream::ReaderThreadProc(std::wstring url, NetStreamType type)
{
    if (type != NetStreamType::Http) {
        std::lock_guard<std::mutex> guard(m_ringMutex);
        m_readerFinished = true;
        m_dataCv.notify_all();
        m_spaceCv.notify_all();
        return;
    }

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        std::lock_guard<std::mutex> guard(m_ringMutex);
        m_readerFinished = true;
        m_dataCv.notify_all();
        m_spaceCv.notify_all();
        return;
    }

    const std::string u8Url = Utf16ToUtf8(url);
    curl_easy_setopt(curl, CURLOPT_URL, u8Url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CNetStream::CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);

    if (m_proxy.type != NetProxyType::none) {
        const std::string proxyAddress = BuildProxyAddress();
        if (!proxyAddress.empty()) {
            curl_easy_setopt(curl, CURLOPT_PROXY, proxyAddress.c_str());
            if (m_proxy.serverPort > 0) {
                curl_easy_setopt(curl, CURLOPT_PROXYPORT, static_cast<long>(m_proxy.serverPort));
            }

            const long proxyType = ConvertProxyType(m_proxy.type);
            if (proxyType >= 0) {
                curl_easy_setopt(curl, CURLOPT_PROXYTYPE, proxyType);
            }

            if (!m_proxy.userName.empty() || !m_proxy.password.empty()) {
                const std::string proxyUserPwd = m_proxy.userName + ":" + m_proxy.password;
                curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxyUserPwd.c_str());
            }
        }
    }

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    {
        std::lock_guard<std::mutex> guard(m_ringMutex);
        m_readerFinished = true;
    }
    m_dataCv.notify_all();
    m_spaceCv.notify_all();
}

bool CNetStream::IsHttpUrl(const std::wstring& url) const
{
    constexpr const wchar_t* kHttpPrefix = L"http://";
    const std::size_t prefixLen = 7;
    if (url.size() < prefixLen) {
        return false;
    }
    for (std::size_t i = 0; i < prefixLen; ++i) {
        if (std::towlower(kHttpPrefix[i]) != std::towlower(url[i])) {
            return false;
        }
    }
    return true;
}

std::string CNetStream::BuildProxyAddress() const
{
    if (m_proxy.serverName.empty()) {
        return "";
    }

    if (m_proxy.serverPort > 0) {
        return m_proxy.serverName + ":" + std::to_string(m_proxy.serverPort);
    }
    return m_proxy.serverName;
}

long CNetStream::ConvertProxyType(NetProxyType type) const
{
    if (type == NetProxyType::http) {
        return CURLPROXY_HTTP;
    }
    if (type == NetProxyType::https) {
        return CURLPROXY_HTTPS;
    }
    if (type == NetProxyType::socks4) {
        return CURLPROXY_SOCKS4;
    }
    if (type == NetProxyType::socks5) {
        return CURLPROXY_SOCKS5;
    }
    return -1;
}
