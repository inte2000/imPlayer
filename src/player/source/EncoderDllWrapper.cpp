#include <filesystem>
#include <utility>

#include "EncoderDllWrapper.h"

CEncoderDllWrapper::CEncoderDllWrapper(const std::wstring& dllname)
{
    Load(dllname);
}

CEncoderDllWrapper::CEncoderDllWrapper(CEncoderDllWrapper&& other) noexcept
{
    *this = std::move(other);
}

CEncoderDllWrapper& CEncoderDllWrapper::operator=(CEncoderDllWrapper&& other) noexcept
{
    if (this != &other) {
        Unload();
        m_hModule = std::exchange(other.m_hModule, nullptr);
        m_dllHost = std::move(other.m_dllHost);
        m_functions = std::exchange(other.m_functions, Functions{});
        other.m_dllHost.clear();
    }
    return *this;
}

CEncoderDllWrapper::~CEncoderDllWrapper()
{
    Unload();
}

bool CEncoderDllWrapper::Load(const std::wstring& dllname)
{
    Unload();
    if (dllname.empty()) {
        return false;
    }
    std::error_code error;
    const auto path = std::filesystem::absolute(dllname, error);
    if (error) {
        return false;
    }
    m_hModule = ::LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!m_hModule) {
        return false;
    }

    m_dllHost = path.wstring();
    m_functions.onRegister = reinterpret_cast<decltype(m_functions.onRegister)>(::GetProcAddress(m_hModule, "Plug_OnRegister"));
    m_functions.getErrMessage = reinterpret_cast<decltype(m_functions.getErrMessage)>(::GetProcAddress(m_hModule, "Plug_GetErrMessage"));
    m_functions.getPluginInformation = reinterpret_cast<decltype(m_functions.getPluginInformation)>(::GetProcAddress(m_hModule, "Plug_GetPluginInformation"));
    m_functions.getEncodingFormatsDefine = reinterpret_cast<decltype(m_functions.getEncodingFormatsDefine)>(::GetProcAddress(m_hModule, "Plug_GetEncodingFormatsDefine"));
    m_functions.getEncodingParamtersDefine = reinterpret_cast<decltype(m_functions.getEncodingParamtersDefine)>(::GetProcAddress(m_hModule, "Plug_GetEncodingParamtersDefine"));
    m_functions.onInitialize = reinterpret_cast<decltype(m_functions.onInitialize)>(::GetProcAddress(m_hModule, "Plug_OnInitialize"));
    m_functions.onRelease = reinterpret_cast<decltype(m_functions.onRelease)>(::GetProcAddress(m_hModule, "Plug_OnRelease"));
    m_functions.openEncoder = reinterpret_cast<decltype(m_functions.openEncoder)>(::GetProcAddress(m_hModule, "Plug_OpenEncoder"));
    m_functions.closeEncoder = reinterpret_cast<decltype(m_functions.closeEncoder)>(::GetProcAddress(m_hModule, "Plug_CloseEncoder"));
    m_functions.setMetaInfo = reinterpret_cast<decltype(m_functions.setMetaInfo)>(::GetProcAddress(m_hModule, "Plug_SetMetaInfo"));
    m_functions.startEncoding = reinterpret_cast<decltype(m_functions.startEncoding)>(::GetProcAddress(m_hModule, "Plug_StartEncoding"));
    m_functions.encodeFrames = reinterpret_cast<decltype(m_functions.encodeFrames)>(::GetProcAddress(m_hModule, "Plug_EncodeFrames"));
    m_functions.stopEncoding = reinterpret_cast<decltype(m_functions.stopEncoding)>(::GetProcAddress(m_hModule, "Plug_StopEncoding"));
    if (!static_cast<bool>(*this)) {
        Unload();
        return false;
    }
    return true;
}

void CEncoderDllWrapper::Unload()
{
    m_functions = {};
    if (m_hModule) {
        ::FreeLibrary(m_hModule);
        m_hModule = nullptr;
    }
    m_dllHost.clear();
}

CEncoderDllWrapper::operator bool() const
{
    return m_hModule && m_functions.onRegister && m_functions.getErrMessage
        && m_functions.getPluginInformation && m_functions.getEncodingFormatsDefine
        && m_functions.getEncodingParamtersDefine && m_functions.onInitialize
        && m_functions.onRelease && m_functions.openEncoder && m_functions.closeEncoder
        && m_functions.setMetaInfo && m_functions.startEncoding
        && m_functions.encodeFrames && m_functions.stopEncoding;
}

int CEncoderDllWrapper::OnRegister(const ApplicationConfig* app, PluginRegister* regInfo)
{
    return m_functions.onRegister ? m_functions.onRegister(app, regInfo) : -101;
}

std::string CEncoderDllWrapper::GetErrorMessage()
{
    char message[256] = {};
    if (m_functions.getErrMessage) {
        m_functions.getErrMessage(message, static_cast<uint32_t>(sizeof(message)));
        message[sizeof(message) - 1] = '\0';
    }
    return message;
}

int CEncoderDllWrapper::GetPluginInformation(PluginInfo* info)
{
    return m_functions.getPluginInformation ? m_functions.getPluginInformation(info) : -101;
}

int CEncoderDllWrapper::GetEncodingFormatsDefine(PluginEncodingFormatsDefine* fmt)
{
    return m_functions.getEncodingFormatsDefine ? m_functions.getEncodingFormatsDefine(fmt) : -101;
}

int CEncoderDllWrapper::GetEncodingParamtersDefine(uint32_t streamFmt, PluginEncodingParamtersDefine* param)
{
    return m_functions.getEncodingParamtersDefine ? m_functions.getEncodingParamtersDefine(streamFmt, param) : -101;
}

void* CEncoderDllWrapper::OnInitialize(const EncoderPluginInitialize* init)
{
    return m_functions.onInitialize ? m_functions.onInitialize(init) : nullptr;
}

void CEncoderDllWrapper::OnRelease(void* ctx)
{
    if (m_functions.onRelease) {
        m_functions.onRelease(ctx);
    }
}

int CEncoderDllWrapper::OpenEncoder(void* ctx, const EncoderPluginOpen* open)
{
    return m_functions.openEncoder ? m_functions.openEncoder(ctx, open) : -101;
}

void CEncoderDllWrapper::CloseEncoder(void* ctx, const EncoderPluginClose* close)
{
    if (m_functions.closeEncoder) {
        m_functions.closeEncoder(ctx, close);
    }
}

int CEncoderDllWrapper::SetMetaInfo(void* ctx, const AudioMetaTags* metaTags)
{
    return m_functions.setMetaInfo ? m_functions.setMetaInfo(ctx, metaTags) : -101;
}

int CEncoderDllWrapper::StartEncoding(void* ctx, const AudioFormat* audioFmt)
{
    return m_functions.startEncoding ? m_functions.startEncoding(ctx, audioFmt) : -101;
}

uint32_t CEncoderDllWrapper::EncodeFrames(void* ctx, const void* pBuf, uint32_t frames)
{
    return m_functions.encodeFrames ? m_functions.encodeFrames(ctx, pBuf, frames) : 0;
}

int CEncoderDllWrapper::StopEncoding(void* ctx)
{
    return m_functions.stopEncoding ? m_functions.stopEncoding(ctx) : -101;
}

std::shared_ptr<CEncoderDllWrapper> MakeEncoderWrapper(const std::wstring& dllname)
{
    auto dll = std::make_shared<CEncoderDllWrapper>(dllname);
    return static_cast<bool>(*dll) ? dll : nullptr;
}
