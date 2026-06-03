#include "AudioTarget.h"

std::unique_ptr<CAudioTarget> MakeFileAudioTarget(const std::wstring& filename, uint32_t streamFmt, const std::string& encoder)
{
    (void)filename;
    (void)streamFmt;
    (void)encoder;
    return nullptr;
}

CAudioTarget::CAudioTarget()
{
    InitEmptyAudioFormat(&m_devFmt);
}

CAudioTarget::CAudioTarget(std::unique_ptr<CDataStream> stream, std::unique_ptr<CAudioEncoder> encoder)
{
    m_stream = std::move(stream);
    m_encoder = std::move(encoder);
    if (m_encoder) {
        m_encoder->Attach(m_stream.get());
    }
    InitEmptyAudioFormat(&m_devFmt);
}

std::wstring CAudioTarget::GetName() const
{
    if (!m_stream || !m_encoder)
        return L"";

    std::wstring name = m_stream->GetName();

    return name;
}

bool CAudioTarget::InitEncoder(const std::vector<EncoderParamter>& params)
{
    if ((!m_stream) || (!m_encoder)) {
        return false;
    }

    m_encoder->Attach(m_stream.get());
    if (!m_encoder->Init(params)) {
        return false;
    }

    m_devFmt = m_encoder->GetTransFormat();
    return true;
}

void CAudioTarget::SetMetaInformation(const CMediaTag& metaInfo)
{
    if (m_encoder) {
        m_encoder->SetMetaInfo(metaInfo);
    }
}

uint32_t CAudioTarget::WriteBuffer(const uint8_t* buf, uint32_t frames, const AudioFormat& audioFmt)
{
    if (!m_stream || !m_encoder)
        return 0;

    return m_encoder->Encode(buf, frames, &audioFmt);
}

uint32_t CAudioTarget::FlushBuffer()
{
    if (!m_stream || !m_encoder)
        return 0;

    return m_encoder->Flush() ? 1u : 0u;
}
