/*
本文件内容有 AI 生成，未作手工修改
大模型：GPT 5.3 Codex
任务说明：todo_task_1.txt
*/
#include "UnicodeConvert.h"
#include "DecoderDllWrapper.h"

CDecoderDllWrapper::CDecoderDllWrapper()
    : m_hModule(nullptr)
{
    ResetFunctionPointers();
}

CDecoderDllWrapper::CDecoderDllWrapper(const std::wstring& dllname)
    : m_hModule(nullptr)
{
    ResetFunctionPointers();
    Load(dllname);
}

CDecoderDllWrapper::CDecoderDllWrapper(CDecoderDllWrapper&& ddr) noexcept
    : m_hModule(ddr.m_hModule)
    , m_dllHost(std::move(ddr.m_dllHost))
    , m_OnRegisterFunc(ddr.m_OnRegisterFunc)
    , m_GetErrMessage(ddr.m_GetErrMessage)
    , m_ParseFileTypeID(ddr.m_ParseFileTypeID)
    , m_GetPluginInformation(ddr.m_GetPluginInformation)
    , m_OnInitialize(ddr.m_OnInitialize)
    , m_StartStream(ddr.m_StartStream)
    , m_StopStream(ddr.m_StopStream)
    , m_IsCanSeeking(ddr.m_IsCanSeeking)
    , m_IsSupportOutput(ddr.m_IsSupportOutput)
    , m_OnUninitialize(ddr.m_OnUninitialize)
    , m_DecodeFrames(ddr.m_DecodeFrames)
    , m_SeekToFrame(ddr.m_SeekToFrame)
    , m_QueryMetaInfo(ddr.m_QueryMetaInfo)
    , m_GetAudioStatusInfo(ddr.m_GetAudioStatusInfo)
    , m_ResetDecoder(ddr.m_ResetDecoder)
    , m_ConfigPlugin(ddr.m_ConfigPlugin)
{
    ddr.m_hModule = nullptr;
    ddr.ResetFunctionPointers();
}

CDecoderDllWrapper& CDecoderDllWrapper::operator=(CDecoderDllWrapper&& ddr) noexcept
{
    if (this != &ddr)
    {
        Unload();

        m_hModule = ddr.m_hModule;
        m_dllHost = std::move(ddr.m_dllHost);
        m_OnRegisterFunc = ddr.m_OnRegisterFunc;
        m_GetErrMessage = ddr.m_GetErrMessage;
        m_ParseFileTypeID = ddr.m_ParseFileTypeID;
        m_GetPluginInformation = ddr.m_GetPluginInformation;
        m_OnInitialize = ddr.m_OnInitialize;
        m_StartStream = ddr.m_StartStream;
        m_StopStream = ddr.m_StopStream;
        m_IsCanSeeking = ddr.m_IsCanSeeking;
        m_IsSupportOutput = ddr.m_IsSupportOutput;
        m_OnUninitialize = ddr.m_OnUninitialize;
        m_DecodeFrames = ddr.m_DecodeFrames;
        m_SeekToFrame = ddr.m_SeekToFrame;
        m_QueryMetaInfo = ddr.m_QueryMetaInfo;
        m_GetAudioStatusInfo = ddr.m_GetAudioStatusInfo;
        m_ResetDecoder = ddr.m_ResetDecoder;
        m_ConfigPlugin = ddr.m_ConfigPlugin;

        ddr.m_hModule = nullptr;
        ddr.ResetFunctionPointers();
    }

    return *this;
}

CDecoderDllWrapper::~CDecoderDllWrapper()
{
    Unload();
}

bool CDecoderDllWrapper::Load(const std::wstring& dllname)
{
    Unload();

    m_hModule = ::LoadLibraryExW(dllname.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (m_hModule == nullptr)
    {
        return false;
    }

    m_dllHost = dllname;
    m_OnRegisterFunc = reinterpret_cast<OnRegisterFuncPtr>(::GetProcAddress(m_hModule, "Plus_OnRegister"));
    m_GetErrMessage = reinterpret_cast<GetErrMessageFuncPtr>(::GetProcAddress(m_hModule, "Plus_GetErrMessage"));
    m_ParseFileTypeID = reinterpret_cast<ParseFileTypeIDFuncPtr>(::GetProcAddress(m_hModule, "Plus_ParseFileTypeID"));
    m_GetPluginInformation = reinterpret_cast<GetPluginInformationFuncPtr>(::GetProcAddress(m_hModule, "Plus_GetPluginInformation"));
    m_OnInitialize = reinterpret_cast<OnInitializeFuncPtr>(::GetProcAddress(m_hModule, "Plus_OnInitialize"));
    m_StartStream = reinterpret_cast<StartStreamFuncPtr>(::GetProcAddress(m_hModule, "Plus_StartStream"));
    m_StopStream = reinterpret_cast<StopStreamFuncPtr>(::GetProcAddress(m_hModule, "Plus_StopStream"));
    m_IsCanSeeking = reinterpret_cast<IsCanSeekingFuncPtr>(::GetProcAddress(m_hModule, "Plus_IsCanSeeking"));
    m_IsSupportOutput = reinterpret_cast<IsSupportOutputFuncPtr>(::GetProcAddress(m_hModule, "Plus_IsSupportOutput"));
    m_OnUninitialize = reinterpret_cast<OnUninitializeFuncPtr>(::GetProcAddress(m_hModule, "Plus_OnUninitialize"));
    m_DecodeFrames = reinterpret_cast<DecodeFramesFuncPtr>(::GetProcAddress(m_hModule, "Plus_DecodeFrames"));
    m_SeekToFrame = reinterpret_cast<SeekToFrameFuncPtr>(::GetProcAddress(m_hModule, "Plus_SeekToFrame"));
    m_QueryMetaInfo = reinterpret_cast<QueryMetaInfoFuncPtr>(::GetProcAddress(m_hModule, "Plus_QueryMetaInfo"));
    m_GetAudioStatusInfo = reinterpret_cast<GetAudioStatusInfoFuncPtr>(::GetProcAddress(m_hModule, "Plus_GetAudioStatusInfo"));
    m_ResetDecoder = reinterpret_cast<ResetDecoderFuncPtr>(::GetProcAddress(m_hModule, "Plus_ResetDecoder"));
    m_ConfigPlugin = reinterpret_cast<ConfigPluginFuncPtr>(::GetProcAddress(m_hModule, "Plus_ConfigPlugin"));

    return static_cast<bool>(*this);
}

void CDecoderDllWrapper::Unload()
{
    ResetFunctionPointers();

    if (m_hModule != nullptr)
    {
        ::FreeLibrary(m_hModule);
        m_hModule = nullptr;
    }

    m_dllHost.clear();
}

CDecoderDllWrapper::operator bool() const
{
    return (m_hModule != nullptr
        && m_OnRegisterFunc != nullptr
        && m_GetErrMessage != nullptr
        && m_ParseFileTypeID != nullptr
        && m_GetPluginInformation != nullptr
        && m_OnInitialize != nullptr
        && m_StartStream != nullptr
        && m_StopStream != nullptr
        && m_IsCanSeeking != nullptr
        && m_IsSupportOutput != nullptr
        && m_OnUninitialize != nullptr
        && m_DecodeFrames != nullptr
        && m_SeekToFrame != nullptr
        && m_QueryMetaInfo != nullptr
        && m_GetAudioStatusInfo != nullptr
        && m_ResetDecoder != nullptr
        && m_ConfigPlugin != nullptr);
}

int CDecoderDllWrapper::OnRegister(const ApplicationConfig* app, PluginRegister* regInfo)
{
    if (m_OnRegisterFunc == nullptr)
    {
        return -101;
    }

    return m_OnRegisterFunc(app, regInfo);
}

std::string CDecoderDllWrapper::GetErrorMessage()
{
    if (m_GetErrMessage == nullptr)
    {
        return "";
    }

    char msgBuf[256] = {};
    m_GetErrMessage(msgBuf, static_cast<uint32_t>(sizeof(msgBuf)));
    return msgBuf;
}

uint32_t CDecoderDllWrapper::ParseFileTypeID(const std::wstring& filename)
{
    if (m_ParseFileTypeID == nullptr)
    {
        return 0;
    }

    std::string utf8name = Utf16ToUtf8(filename);
    return m_ParseFileTypeID(utf8name.c_str());
}

int CDecoderDllWrapper::GetPluginInformation(PluginInfo* info)
{
    if (m_GetPluginInformation == nullptr)
    {
        return -101;
    }

    return m_GetPluginInformation(info);
}

void* CDecoderDllWrapper::OnInitialize(const PluginInitialize* init)
{
    if (m_OnInitialize == nullptr)
    {
        return nullptr;
    }

    return m_OnInitialize(init);
}

int CDecoderDllWrapper::StartStream(void* ctx, const PluginStart* param)
{
    if (m_StartStream == nullptr)
    {
        return -101;
    }

    return m_StartStream(ctx, param);
}

int CDecoderDllWrapper::StopStream(void* ctx, uint32_t mediaStreamIdx)
{
    if (m_StopStream == nullptr)
    {
        return -101;
    }

    return m_StopStream(ctx, mediaStreamIdx);
}

int CDecoderDllWrapper::IsCanSeeking(void* ctx, uint32_t mediaStreamIdx)
{
    if (m_IsCanSeeking == nullptr)
    {
        return 0;
    }

    return m_IsCanSeeking(ctx, mediaStreamIdx);
}

int CDecoderDllWrapper::IsSupportOutput(void* ctx, uint32_t mediaStreamIdx, const AudioFormat* audioFmt)
{
    if (m_IsSupportOutput == nullptr)
    {
        return 0;
    }

    return m_IsSupportOutput(ctx, mediaStreamIdx, audioFmt);
}

void CDecoderDllWrapper::OnUninitialize(void* ctx)
{
    if (m_OnUninitialize != nullptr)
    {
        m_OnUninitialize(ctx);
    }
}

uint32_t CDecoderDllWrapper::DecodeFrames(void* ctx, void* pBuf, uint32_t frames, const AudioFormat* audioFmt)
{
    if (m_DecodeFrames == nullptr)
    {
        return 0;
    }

    return m_DecodeFrames(ctx, pBuf, frames, audioFmt);
}

void CDecoderDllWrapper::SeekToFrame(void* ctx, std::size_t frames)
{
    if (m_SeekToFrame != nullptr)
    {
        m_SeekToFrame(ctx, frames);
    }
}

int CDecoderDllWrapper::QueryMetaInfo(void* ctx, uint32_t streamIdx, AudioMetaTags* metaTags)
{
    if (m_QueryMetaInfo == nullptr)
    {
        return -101;
    }

    return m_QueryMetaInfo(ctx, streamIdx, metaTags);
}

int CDecoderDllWrapper::GetAudioStatusInfo(void* ctx, PluginAudioInfo* info)
{
    if (m_GetAudioStatusInfo == nullptr)
    {
        return -101;
    }

    return m_GetAudioStatusInfo(ctx, info);
}

void CDecoderDllWrapper::ResetDecoder(void* ctx)
{
    if (m_ResetDecoder != nullptr)
    {
        m_ResetDecoder(ctx);
    }
}

void CDecoderDllWrapper::ConfigPlugin(HWND hWnd)
{
    if (m_ConfigPlugin != nullptr)
    {
        m_ConfigPlugin(hWnd);
    }
}

void CDecoderDllWrapper::ResetFunctionPointers()
{
    m_OnRegisterFunc = nullptr;
    m_GetErrMessage = nullptr;
    m_ParseFileTypeID = nullptr;
    m_GetPluginInformation = nullptr;
    m_OnInitialize = nullptr;
    m_StartStream = nullptr;
    m_StopStream = nullptr;
    m_IsCanSeeking = nullptr;
    m_IsSupportOutput = nullptr;
    m_OnUninitialize = nullptr;
    m_DecodeFrames = nullptr;
    m_SeekToFrame = nullptr;
    m_QueryMetaInfo = nullptr;
    m_GetAudioStatusInfo = nullptr;
    m_ResetDecoder = nullptr;
    m_ConfigPlugin = nullptr;
}

std::shared_ptr<CDecoderDllWrapper> MakeDecoderWrapper(const std::wstring& dllname)
{
    std::shared_ptr<CDecoderDllWrapper> dll = std::make_shared<CDecoderDllWrapper>(dllname);
    if (dll && static_cast<bool>(*dll))
    {
        return dll;
    }

    return nullptr;
}
