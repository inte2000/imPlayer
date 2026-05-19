/*
20240425 初次生成
大模型：GPT 5.3 Codex
任务说明：todo_task_23.txt

20240425 增加 Name 静态函数，增加 m_decodeMtx 互斥体
大模型：GPT 5.3 Codex
任务说明：todo_task_24.txt
*/
#include <format>
#include <stdexcept>
#include "UnicodeConvert.h"
#include "PluginDecoder.h"

static const char* sPluginDecoderName = "Plugin Decoder";

std::string CPluginDecoder::Name()
{
    return sPluginDecoderName;
}

CPluginDecoder::CPluginDecoder(uint32_t streamFmt)
    : m_ctxHdr(nullptr)
    , m_streamFmt(streamFmt)
    , m_audioFmt({})
    , m_curFrames(0)
    , m_totalFrames(0)
{
    m_type = DECODE_TYPE_PLUGIN;
    m_name = sPluginDecoderName;
    m_curStreamIdx = static_cast<uint32_t>(-1);
    m_StreamCount = 1;
    InitEmptyAudioFormat(&m_audioFmt);
}

CPluginDecoder::~CPluginDecoder()
{
    UninitializePlugin();
}

void CPluginDecoder::AttachModule(const std::shared_ptr<CDecoderDllWrapper>& dll)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    m_dll = dll;
    if (!m_dll)
        throw std::runtime_error("Attach plugin module failed: empty wrapper.");

    PluginInfo info = {};
    info.size = sizeof(info);
    int code = m_dll->GetPluginInformation(&info);
    if (code != 0)
        ThrowPluginError("query plugin information", code);

    if (info.plug_type != PluginType::Decoder)
        throw std::runtime_error("Attach plugin module failed: plugin type is not decoder.");

    m_name = info.name;
    m_publisher = info.publisher;
    m_verMajor = info.ver_major;
    m_verMinor = info.ver_minor;
}

bool CPluginDecoder::InitDecode(const CDecodeInitCtx* decodeInit)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    (void)decodeInit;
    if (!m_dll)
        throw std::runtime_error("Initialize plugin decoder failed: module not attached.");

    if (m_pStream == nullptr)
        throw std::runtime_error("Initialize plugin decoder failed: data stream not attached.");

    UninitializePlugin();

    PluginInitialize init = {};
    init.pStream = m_pStream;
    init.streamFmt = m_streamFmt;
    init.mediaStreamIdx = 0;

    m_ctxHdr = m_dll->OnInitialize(&init);
    if (m_ctxHdr == nullptr)
    {
        std::string errormsg = Utf8ToLocalMBCS(m_dll->GetErrorMessage());
        throw std::runtime_error(std::format("Fail to init decoder plusin, error: {}", errormsg));
    }

    m_curStreamIdx = static_cast<uint32_t>(-1);
    m_curFrames = 0;
    m_totalFrames = 0;
    InitEmptyAudioFormat(&m_audioFmt);
    UpdateStatus(AudioInfoFlagFormat | AudioInfoFlagStreamCount | AudioInfoFlagTotalFrames);

    return true;
}

bool CPluginDecoder::StartStream(uint32_t streamIdx, std::size_t begin, std::size_t end, uint32_t loop)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    if (m_ctxHdr == nullptr)
        throw std::runtime_error("Start stream failed: plugin decoder not initialized.");

    PluginStart ps = {};
    ps.streamFmt = m_streamFmt;
    ps.mediaStreamIdx = streamIdx;
    int code = m_dll->StartStream(m_ctxHdr, &ps);
    if (code != 0)
        throw std::runtime_error("Plusin decoder fail to get audio status: " + m_dll->GetErrorMessage());

    m_curStreamIdx = streamIdx;
    UpdateStatus(AudioInfoFlagFormat | AudioInfoFlagActiveStream | AudioInfoFlagStreamCount | AudioInfoFlagTotalFrames);
    m_curFrames = 0;
    if (begin > 0)
    {
        m_curFrames = (begin <= m_totalFrames) ? begin : m_totalFrames;
    }
    if (end < m_totalFrames)
    {
        m_totalFrames = end;
    }
    m_dll->SeekToFrame(m_ctxHdr, m_curFrames);    

    return true;
}

void CPluginDecoder::StopStream(uint32_t streamIdx)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    if (m_ctxHdr == nullptr)
        throw std::runtime_error("Plusin decoder not initialized!");

    if (m_curStreamIdx != -1) //当前流已经打开
    {
        if ((streamIdx == -1) || (streamIdx == m_curStreamIdx))
        {
            int rtn = m_dll->StopStream(m_ctxHdr, m_curStreamIdx);
            if (rtn != 0)
                throw std::runtime_error("Plusin decoder fail to stop audio stream: " + m_dll->GetErrorMessage());
        }
        else
        {
            throw std::runtime_error("Plusin decoder fail to stop audio stream: stream index not match!");
        }
    }
}

CMediaTag CPluginDecoder::GetTags(uint32_t streamIdx)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    if (m_ctxHdr == nullptr)
        throw std::runtime_error("Query plugin tags failed: decoder not initialized.");

    AudioMetaTags meta = {};
    meta.size = sizeof(meta);
    if (streamIdx == static_cast<uint32_t>(-1))
        streamIdx = (m_curStreamIdx == static_cast<uint32_t>(-1)) ? 0 : m_curStreamIdx;

    int code = m_dll->QueryMetaInfo(m_ctxHdr, streamIdx, &meta);
    if (code != 0)
        throw std::runtime_error("Plusin decoder fail query tags: " + m_dll->GetErrorMessage());

    return meta.tags;
}

AudioFormat CPluginDecoder::GetAudioFormat(uint32_t streamIdx) const
{
    (void)streamIdx;
    return m_audioFmt;
}

uint32_t CPluginDecoder::Decode(void* pBuf, uint32_t bufSize, uint32_t frames, const AudioFormat* audioFmt)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    if ((m_ctxHdr == nullptr) || (pBuf == nullptr) || (audioFmt == nullptr) || (frames == 0))
    {
        return 0;
    }
    if (bufSize < frames * audioFmt->blockAlign)
    {
        return 0;
    }

    uint32_t readFrames = m_dll->DecodeFrames(m_ctxHdr, pBuf, frames, audioFmt);
    m_curFrames += readFrames;
    if (m_totalFrames > 0)
    {
        m_curFrames = (m_curFrames > m_totalFrames) ? m_totalFrames : m_curFrames;
    }

    return readFrames;
}

void CPluginDecoder::SeekTo(std::size_t frames)
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    if (m_ctxHdr == nullptr)
        return;

    m_dll->SeekToFrame(m_ctxHdr, frames);
    UpdateStatus(AudioInfoFlagCurrentFrames | AudioInfoFlagTotalFrames);
}

bool CPluginDecoder::IsSupportOutput(const AudioFormat* audioFmt) const
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    if ((m_ctxHdr == nullptr) || (audioFmt == nullptr))
    {
        return false;
    }

    int code = m_dll->IsSupportOutput(m_ctxHdr, m_curStreamIdx, audioFmt);
    if (code < 0)
    {
        ThrowPluginError("query plugin output support", code);
    }

    return (code != 0);
}

bool CPluginDecoder::IsCanSeeking(uint32_t streamIdx) const
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    if (m_ctxHdr == nullptr)
    {
        return false;
    }

    int code = m_dll->IsCanSeeking(m_ctxHdr, streamIdx);
    if (code < 0)
    {
        ThrowPluginError("query plugin seek support", code);
    }

    return (code != 0);
}

void CPluginDecoder::Reset()
{
    std::lock_guard<std::mutex> guard(m_decodeMtx);

    if (m_ctxHdr == nullptr)
        return;

    m_dll->ResetDecoder(m_ctxHdr);
    UpdateStatus(AudioInfoFlagCurrentFrames | AudioInfoFlagTotalFrames);
}

void CPluginDecoder::UninitializePlugin()
{
    if ((m_dll != nullptr) && (m_ctxHdr != nullptr))
    {
        m_dll->OnUninitialize(m_ctxHdr);
    }
    m_ctxHdr = nullptr;
}

void CPluginDecoder::ThrowPluginError(const char* action, int code) const
{
    std::string err = (m_dll != nullptr) ? m_dll->GetErrorMessage() : "";
    if (err.empty())
    {
        err = "unknown error";
    }

    throw std::runtime_error(std::format("{} failed (code={}): {}", action, code, err));
}

void CPluginDecoder::UpdateStatus(uint32_t flags)
{
    if ((m_dll == nullptr) || (m_ctxHdr == nullptr))
        return;

    PluginAudioInfo info = {};
    info.size = sizeof(info);
    info.flags = flags;
    int code = m_dll->GetAudioStatusInfo(m_ctxHdr, &info);
    if (code != 0)
    {
        ThrowPluginError("query plugin audio status", code);
    }

    if ((flags & AudioInfoFlagFormat) != 0)
    {
        m_audioFmt = info.audioFmt;
    }
    if ((flags & AudioInfoFlagActiveStream) != 0)
    {
        m_curStreamIdx = info.mediaStreamIdx;
    }
    if ((flags & AudioInfoFlagStreamCount) != 0)
    {
        m_StreamCount = info.mediaStreamCount;
    }
    if ((flags & AudioInfoFlagCurrentFrames) != 0)
    {
        m_curFrames = info.currentFrames;
    }
    if ((flags & AudioInfoFlagTotalFrames) != 0)
    {
        m_totalFrames = info.totalFrames;
    }
}
