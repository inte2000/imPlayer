#include <filesystem>
#include "FileStream.h"
#include "EncoderFactory.h"
#include "Utf8String.h"
#include "AudioTarget.h"

static std::runtime_error MakeRuntimeError(const char *msg, const std::wstring& filename)
{
    std::string strMsg = msg;
    strMsg += LocalMBCSFromUTF16LEStr(filename);

    return std::runtime_error(strMsg);
}

std::unique_ptr<CAudioTarget> MakeFileAudioTarget(const std::wstring& filename, uint32_t typeFmt, const std::string& encoder)
{
    std::unique_ptr<CDataStream> streamPtr = MakeFileStream(filename, false);
    if (!streamPtr)
        throw MakeRuntimeError("Fail to open file: ", filename);

    CEncoderFactory& encFactory = CEncoderFactory::GetInstance();
    std::unique_ptr<CAudioEncoder> encoderPtr = encFactory.MakeAudioEncoder(typeFmt, encoder);
    if(!encoderPtr)
        throw MakeRuntimeError("Fail to generate decoder for: ", filename);

    return std::make_unique<CAudioTarget>(std::move(streamPtr), std::move(encoderPtr));
}

CAudioTarget::CAudioTarget()
{
    InitEmptyAudioFormat(&m_devFmt);
}

CAudioTarget::CAudioTarget(std::unique_ptr<CDataStream> stream, std::unique_ptr<CAudioEncoder> encoder)
{
    m_stream = std::move(stream);
    m_encoder = std::move(encoder);
    m_encoder->Attach(m_stream.get()); 
    InitEmptyAudioFormat(&m_devFmt);
}

std::wstring CAudioTarget::GetName() const
{
    if (!m_stream || !m_encoder)
        return L"";

    std::wstring name = m_stream->GetName();

    return name;
}

bool CAudioTarget::InitEncoder(const std::string& jsonParams)
{
    if (!m_encoder)
        return false;

    EncodingBaseParams params = ExtractBaseParams(jsonParams);
    InitAudioFormat(&m_devFmt, params.format, params.numChannels, params.sampleRate, params.bitsPerSample / 8, 0, params.chLayout);
    return m_encoder->Init(jsonParams);
}

void CAudioTarget::SetExtraInformation(const std::string& jsonMeta)
{
    if (m_encoder)
        m_encoder->SetMetaInfo(jsonMeta);
}

bool CAudioTarget::SetOutputFormat(const AudioFormat& devFmt)
{
    m_devFmt = devFmt;
    /*
    if (!m_encoder->IsSupportOutput(&devFmt))
    {
    }
    else
    {
    }
    */
    return true;
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

    return m_encoder->Flush();
}