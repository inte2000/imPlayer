#include <algorithm>
#include <format>
#include <stdexcept>

#include "PluginEncoder.h"
#include "StdFileSystem.h"

CPluginEncoder::CPluginEncoder(uint32_t streamFmt) : m_streamFmt(streamFmt)
{
    m_name = Name();
    m_type = ENCODE_TYPE_PLUGIN;
    InitEmptyAudioFormat(&m_transFmt);
}

CPluginEncoder::~CPluginEncoder()
{
    ReleasePlugin();
}

std::string CPluginEncoder::Name()
{
    return "Plugin Encoder";
}

void CPluginEncoder::AttachModule(const std::shared_ptr<CEncoderDllWrapper>& dll)
{
    if (!dll || !static_cast<bool>(*dll)) {
        throw std::runtime_error("Attach plugin encoder failed: invalid wrapper.");
    }
    PluginInfo info{};
    info.size = sizeof(info);
    if (dll->GetPluginInformation(&info) != 0 || info.plug_type != PluginType::Encoder) {
        throw std::runtime_error("Attach plugin encoder failed: invalid encoder plugin information.");
    }
    info.name[sizeof(info.name) - 1] = '\0';
    info.publisher[sizeof(info.publisher) - 1] = '\0';
    ReleasePlugin();
    m_dllWrapper = dll;
    m_name = info.name;
    m_publisher = info.publisher;
    m_verMajor = info.ver_major;
    m_verMinor = info.ver_minor;
}

void CPluginEncoder::Init()
{
    if (!m_dllWrapper) {
        throw std::runtime_error("Initialize plugin encoder failed: module not attached.");
    }
    ReleasePlugin();
    EncoderPluginInitialize init{};
    init.size = sizeof(init);
    const auto hostPath = GetApplicationBasePath().wstring();
    wcsncpy_s(init.hostpath, hostPath.c_str(), _TRUNCATE);
    m_pContext = m_dllWrapper->OnInitialize(&init);
    if (!m_pContext) {
        ThrowPluginError("initialize plugin encoder", -1);
    }
}

bool CPluginEncoder::Open(const std::vector<EncoderParamter>& params)
{
    if (!m_pContext || !m_pStream || m_opened) {
        return false;
    }
    EncoderPluginOpen open{};
    open.size = sizeof(open);
    open.streamFmt = m_streamFmt;
    open.mediaStreamIdx = 0;
    open.pStream = m_pStream;
    if (params.size() > EncoderParamItemsLimit) throw std::runtime_error("Encoder parameter count exceeds its limit.");
    for (const auto& param : params) param.Validate();
    std::copy(params.begin(), params.end(), open.params);
    open.count = static_cast<uint32_t>(params.size());
    const int code = m_dllWrapper->OpenEncoder(m_pContext, &open);
    if (code != 0) {
        // 先保存插件错误，清理调用可能覆盖插件的线程局部错误信息。
        const auto message = m_dllWrapper->GetErrorMessage();
        EncoderPluginClose close{};
        close.size = sizeof(close);
        m_dllWrapper->CloseEncoder(m_pContext, &close);
        throw std::runtime_error(std::format("open plugin encoder failed (code={}): {}", code, message));
    }
    m_opened = true;
    const auto* header = static_cast<const AudioEncodeContextHeader*>(m_pContext);
    if (header->size != sizeof(AudioEncodeContextHeader)) {
        Close();
        throw std::runtime_error("Invalid plugin encoder context header.");
    }
    m_transFmt = header->transFmt;
    return true;
}

bool CPluginEncoder::SetMetaInfo(const CMediaTag& metaTags)
{
    if (!m_pContext) {
        return false;
    }
    AudioMetaTags tags{};
    tags.size = sizeof(tags);
    tags.streamIdx = 0;
    tags.tags = metaTags;
    const int code = m_dllWrapper->SetMetaInfo(m_pContext, &tags);
    if (code != 0) {
        ThrowPluginError("set plugin encoder metadata", code);
    }
    return true;
}

bool CPluginEncoder::Start(const AudioFormat* audioFmt)
{
    if (!m_opened || m_started || m_stopped || !audioFmt) {
        return false;
    }
    const int code = m_dllWrapper->StartEncoding(m_pContext, audioFmt);
    if (code != 0) {
        ThrowPluginError("start plugin encoder", code);
    }
    m_started = true;
    return true;
}

uint32_t CPluginEncoder::Encode(const void* pData, uint32_t frames)
{
    if (!m_started || !pData || frames == 0) {
        return 0;
    }
    return m_dllWrapper->EncodeFrames(m_pContext, pData, frames);
}

bool CPluginEncoder::Stop()
{
    if (m_stopped) {
        return m_stopResult;
    }
    if (!m_started) {
        return false;
    }
    const int code = m_dllWrapper->StopEncoding(m_pContext);
    m_started = false;
    m_stopped = true;
    m_stopResult = (code == 0);
    if (code != 0) {
        ThrowPluginError("stop plugin encoder", code);
    }
    return true;
}

void CPluginEncoder::Close()
{
    if (m_opened) {
        EncoderPluginClose close{};
        close.size = sizeof(close);
        close.mediaStreamIdx = 0;
        m_dllWrapper->CloseEncoder(m_pContext, &close);
    }
    m_opened = false;
    m_started = false;
    m_stopped = false;
    m_stopResult = true;
    InitEmptyAudioFormat(&m_transFmt);
}

void CPluginEncoder::ReleasePlugin()
{
    Close();
    if (m_pContext) {
        m_dllWrapper->OnRelease(m_pContext);
        m_pContext = nullptr;
    }
}

void CPluginEncoder::ThrowPluginError(const char* action, int code) const
{
    auto message = m_dllWrapper->GetErrorMessage();
    if (message.empty()) {
        message = "unknown error";
    }
    throw std::runtime_error(std::format("{} failed (code={}): {}", action, code, message));
}
