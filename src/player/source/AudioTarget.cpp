#include <stdexcept>

#include "AudioTarget.h"
#include "EncoderFactory.h"
#include "FileStream.h"

std::unique_ptr<CAudioTarget> MakeFileAudioTarget(const std::wstring& filename, uint32_t streamFmt, const std::string& encoder)
{
    auto encoderObj = CEncoderFactory::GetInstance().MakeAudioEncoder(streamFmt, encoder);
    if (!encoderObj) {
        return nullptr;
    }

    auto stream = MakeFileStream(filename, false);
    return std::make_unique<CAudioTarget>(std::move(stream), std::move(encoderObj));
}

CAudioTarget::CAudioTarget()
{
    InitEmptyAudioFormat(&m_inputFmt);
    InitEmptyAudioFormat(&m_startedFmt);
}

CAudioTarget::CAudioTarget(std::unique_ptr<CDataStream> stream, std::unique_ptr<CAudioEncoder> encoder)
{
    m_stream = std::move(stream);
    m_encoder = std::move(encoder);
    if (m_encoder) {
        m_encoder->Init();
        m_encoder->Attach(m_stream.get());
    }
    InitEmptyAudioFormat(&m_inputFmt);
    InitEmptyAudioFormat(&m_startedFmt);
}

CAudioTarget::~CAudioTarget()
{
    Close();
}

std::wstring CAudioTarget::GetName() const
{
    if (!m_stream || !m_encoder)
        return L"";

    std::wstring name = m_stream->GetName();

    return name;
}

bool CAudioTarget::Open(const std::vector<EncoderParamter>& params)
{
    if (m_opened || !m_stream || !m_encoder) {
        return false;
    }

    try {
        if (!m_encoder->Open(params)) {
            m_encoder->Close();
            return false;
        }
        m_opened = true;
        if (m_metaInfo.has_value() && !m_encoder->SetMetaInfo(*m_metaInfo)) {
            Close();
            return false;
        }
        m_inputFmt = m_encoder->GetTransFormat();
    }
    catch (...) {
        m_encoder->Close();
        m_opened = false;
        InitEmptyAudioFormat(&m_inputFmt);
        throw;
    }

    return true;
}

bool CAudioTarget::Start(const AudioFormat& audioFmt)
{
    if (!m_opened || m_started || m_stopped) {
        return false;
    }
    if (!m_encoder->Start(&audioFmt)) {
        return false;
    }
    m_startedFmt = audioFmt;
    m_started = true;
    return true;
}

void CAudioTarget::Close()
{
    if (m_opened) {
        m_encoder->Close();
    }
    m_opened = false;
    m_started = false;
    m_stopped = false;
    InitEmptyAudioFormat(&m_inputFmt);
    InitEmptyAudioFormat(&m_startedFmt);
}

void CAudioTarget::SetMetaInformation(const CMediaTag& metaInfo)
{
    m_metaInfo = metaInfo;
    if (m_opened && !m_encoder->SetMetaInfo(metaInfo)) {
        throw std::runtime_error("failed to set encoder metadata");
    }
}

uint32_t CAudioTarget::WriteBuffer(const uint8_t* buf, uint32_t frames, const AudioFormat& audioFmt)
{
    if (!m_started || m_stopped)
        return 0;

    if (!IsSameAudioFormat(&audioFmt, &m_startedFmt)) {
        throw std::runtime_error("audio target input format mismatch");
    }
    return m_encoder->Encode(buf, frames);
}

uint32_t CAudioTarget::FlushBuffer()
{
    if (!m_started)
        return 0;

    m_stopped = true;
    return m_encoder->Stop() ? 1u : 0u;
}
