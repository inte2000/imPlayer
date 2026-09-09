#include <cwctype>

#include <curl/curl.h>

#include "LibCurlInit.h"
#include "NetStream.h"
#include "UnicodeConvert.h"

namespace {

CLibCurlInit g_libCurlInit;

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
    , m_ringBuffer(NetStreamDefaultRingBufferBytes)
    , m_readerThread()
    , m_stopRequested(false)
    , m_opened(false)
    , m_proxy{NetProxyType::none, "", 0, "", ""}
{
    m_style = dsStyleFixedLength | dsStyleTellPos;
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

    if (!g_libCurlInit.IsInitialized()) {
        return false;
    }

    if (m_ringSizeBytes == 0) {
        m_ringSizeBytes = NetStreamDefaultRingBufferBytes;
    }

    m_ringBuffer.Reset(m_ringSizeBytes);

    m_name = url;
    m_length = static_cast<std::size_t>(NetStreamLengthLimit);
    m_curPos = 0;
    m_stopRequested.store(false, std::memory_order_release);
    m_opened = true;

    try {
        m_readerThread = std::thread(&CNetStream::ReaderThreadProc, this, url, type);
    }
    catch (...) {
        m_opened = false;
        m_stopRequested.store(true, std::memory_order_release);
        m_ringBuffer.Close();
        return false;
    }

    return true;
}

void CNetStream::Close()
{
    m_stopRequested.store(true, std::memory_order_release);
    m_ringBuffer.Close();

    if (m_readerThread.joinable()) {
        m_readerThread.join();
    }

    m_ringBuffer.Clear();

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
    m_ringBuffer.Reset(m_ringSizeBytes);
}

uint32_t CNetStream::Read(void* pBuf, uint32_t size, uint32_t timeout)
{
    if ((pBuf == nullptr) || (size == 0) || !m_opened) {
        return 0;
    }

    const std::size_t totalRead = m_ringBuffer.Read(pBuf, static_cast<std::size_t>(size), timeout);
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
    if (m_stopRequested.load(std::memory_order_acquire)) {
        return 0;
    }
    return m_ringBuffer.Write(data, bytes);
}

void CNetStream::ReaderThreadProc(std::wstring url, NetStreamType type)
{
    if (type != NetStreamType::Http) {
        m_ringBuffer.SetProducerFinished();
        return;
    }

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        m_ringBuffer.SetProducerFinished();
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
    m_ringBuffer.SetProducerFinished();
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
