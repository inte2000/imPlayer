#include <filesystem>
#include "AudioSource.h"
#include "FileStream.h"
//#include "Utf8String.h"
#include "UnicodeConvert.h"
#include "DecoderFactory.h"
#include "DecoderParametersMgmt.h" 

static std::runtime_error MakeRuntimeError(const char* msg, const std::wstring& filename)
{
    std::string strMsg = msg;
    strMsg += Utf16LeToLocalMBCS(filename);

    return std::runtime_error(strMsg);
}

std::unique_ptr<CAudioSource> MakeFileAudioSource(const std::wstring& filename)
{
    std::unique_ptr<CDataStream> streamPtr = MakeFileStream(filename, true);
    if (!streamPtr)
        throw MakeRuntimeError("Fail to open file: ", filename);

    uint32_t fileFmt = ParseAudioFileFormat(filename);
    std::unique_ptr<CAudioDecoder> decoderPtr = CDecoderFactory::GetInstance().MakeAudioDecoder(fileFmt);
    if(!decoderPtr)
        throw MakeRuntimeError("Fail to generate decoder for: ", filename);

    return std::make_unique<CAudioSource>(std::move(streamPtr), std::move(decoderPtr), fileFmt);
    //return std::unique_ptr<CAudioSource>(new CAudioSource{});
}

CAudioSource::CAudioSource()
{
    std::memset(&m_audioInfo, 0, sizeof(AudioInfo));
    InitEmptyAudioFormat(&m_outFmt);
    m_bDeviceOutput = true;
    m_needConvert = false;
}

CAudioSource::CAudioSource(std::unique_ptr<CDataStream> stream, std::unique_ptr<CAudioDecoder> decoder, uint32_t streamFmt)
{
    m_stream = std::move(stream);
    m_decoder = std::move(decoder);
    m_decoder->Attach(m_stream.get()); 
    InitEmptyAudioFormat(&m_outFmt);
    m_bDeviceOutput = true;
    m_needConvert = false;
    CDecoderParametersMgmt& mgmt = CDecoderParametersMgmt::GetInstance();
    const CDecodeInitCtx* decodeInit = mgmt.GetDecodeParamter(streamFmt);
    m_audioInfo = m_decoder->InitDecode(decodeInit);      
}

std::wstring CAudioSource::GetName() const
{
    if (!m_stream || !m_decoder)
        return L"";

    std::wstring name = m_stream->GetName();

    return name;
}

static AudioFormat GetMostMatchDecoderFormat(CAudioDecoder *pDecoder, const AudioFormat& audioFmt, const AudioFormat& deviceFmt)
{
    AudioFormat decodeFmt = audioFmt;
    if (deviceFmt.format == AudioDataFormat::PCM_S16)
    {
        decodeFmt.format = AudioDataFormat::PCM_S16;
        decodeFmt.bytesPerSample = sizeof(int16_t);
        decodeFmt.blockAlign = decodeFmt.bytesPerSample * decodeFmt.numChannels;
        if (pDecoder->IsSupportOutput(&decodeFmt))
            return decodeFmt;
    }
    else if (deviceFmt.format == AudioDataFormat::PCM_S32)
    {
        decodeFmt.format = AudioDataFormat::PCM_S32;
        decodeFmt.bytesPerSample = sizeof(int32_t);
        decodeFmt.blockAlign = decodeFmt.bytesPerSample * decodeFmt.numChannels;
        if (pDecoder->IsSupportOutput(&decodeFmt))
            return decodeFmt;
    }
    else if (deviceFmt.format == AudioDataFormat::PCM_S24)
    {
        decodeFmt.format = AudioDataFormat::PCM_S24;
        decodeFmt.bytesPerSample = 3;
        decodeFmt.blockAlign = decodeFmt.bytesPerSample * decodeFmt.numChannels;
        if (pDecoder->IsSupportOutput(&decodeFmt))
            return decodeFmt;

        decodeFmt.format = AudioDataFormat::PCM_S24_32;
        decodeFmt.bytesPerSample = sizeof(int32_t);
        decodeFmt.blockAlign = decodeFmt.bytesPerSample * decodeFmt.numChannels;
        if (pDecoder->IsSupportOutput(&decodeFmt))
            return decodeFmt;
    }

    decodeFmt.format = AudioDataFormat::Float32;
    decodeFmt.bytesPerSample = GetBitsPerSampleByFormat(decodeFmt.format) / 8;
    decodeFmt.blockAlign = decodeFmt.bytesPerSample * decodeFmt.numChannels;

    return decodeFmt;
}

bool CAudioSource::SetOutputFormat(const AudioFormat& outFmt, bool bDevide)
{
    m_needConvert = false;
    m_outFmt = outFmt;
    m_bDeviceOutput = bDevide;
    if (!m_decoder->IsSupportOutput(&outFmt))
    {
        m_needConvert = true;

        m_decodeFmt = GetMostMatchDecoderFormat(m_decoder.get(), m_audioInfo.m_audioFmt, outFmt);
        if (!bDevide)
        {
            m_outFmt.format = AudioDataFormat::Float32;
            m_outFmt.bytesPerSample = GetBitsPerSampleByFormat(m_outFmt.format) / 8;
        }

        uint32_t totalBytes = m_decodeFmt.blockAlign * DECODE_BUF_FRAMES;
        m_tmpBuf.reset();
        m_tmpBuf = std::make_unique<uint8_t[]>(totalBytes);
        m_tmpSize = totalBytes; 

        m_Effects.Release();
        if (!m_Effects.Init(m_decodeFmt, m_outFmt)) 
            return false;
    }
    else
    {
        m_decodeFmt = m_outFmt;

        uint32_t bytesPerSample = GetBitsPerSampleByFormat(m_outFmt.format) / 8;
        uint32_t totalBytes = bytesPerSample * m_outFmt.numChannels * DECODE_BUF_FRAMES;
        m_tmpBuf.reset();
        m_tmpBuf = std::make_unique<uint8_t[]>(totalBytes);
        m_tmpSize = totalBytes; 
    }

    return true;
}

uint32_t CAudioSource::ReadBuffer(uint8_t* buf, uint32_t bufSize, uint32_t frames)
{
    if (!m_stream || !m_decoder)
        return 0;

    if (m_decoder->GetCurrentFrame() >= m_decoder->GetTotalFrame())
        return 0;
    
    if (m_outFmt.format == AudioDataFormat::UNKNOWN)
    {
        InitEmptyAudioFormat(&m_outFmt);
        return m_decoder->Decode(buf, bufSize, frames, &m_audioInfo.m_audioFmt);
    }
    if (!m_needConvert)
    {
        return m_decoder->Decode(buf, bufSize, frames, &m_decodeFmt); 
    }
    else
    {
        uint32_t readFrames = m_decoder->Decode(m_tmpBuf.get(), m_tmpSize, frames, &m_decodeFmt);
        if (readFrames > 0)
        {
            uint32_t inSamples = readFrames * m_decodeFmt.numChannels;
            double ch_rate = double(m_outFmt.numChannels) / m_decodeFmt.numChannels;
            uint32_t outSamples = uint32_t((double(inSamples) * m_outFmt.sampleRate / m_decodeFmt.sampleRate + 0.5) * ch_rate);
            if (bufSize < (outSamples * m_outFmt.bytesPerSample))
                return 0;

            bool bFlushLast = m_decoder->GetCurrentFrame() >= m_decoder->GetTotalFrame();
            uint32_t realSamples = m_Effects.Process(m_tmpBuf.get(), inSamples, buf, outSamples, bFlushLast);
            return (realSamples / m_outFmt.numChannels);
        }
    }

    return 0;
}

std::size_t CAudioSource::SourceFrameToDeviceFrame(std::size_t srcFrame) const 
{
    if (m_outFmt.format == AudioDataFormat::UNKNOWN) //
        return srcFrame;

    if (m_outFmt.sampleRate == m_audioInfo.m_audioFmt.sampleRate)
        return srcFrame;

    double rate = double(m_outFmt.sampleRate) / m_audioInfo.m_audioFmt.sampleRate;

    return static_cast<std::size_t>(rate * srcFrame);
}

std::size_t CAudioSource::DeviceFrameToSourceFrame(std::size_t devFrame) const 
{
    if (m_outFmt.format == AudioDataFormat::UNKNOWN) //
        return devFrame;

    if (m_outFmt.sampleRate == m_audioInfo.m_audioFmt.sampleRate)
        return devFrame;

    double rate = double(m_audioInfo.m_audioFmt.sampleRate) / m_outFmt.sampleRate;

    return static_cast<std::size_t>(rate * devFrame);
}

void CAudioSource::SeekToFrame(std::size_t frames)
{
    if (!m_stream || !m_decoder)
        return;

    std::size_t sourceFrames = DeviceFrameToSourceFrame(frames);
    m_decoder->SeekTo(sourceFrames);
}

std::size_t CAudioSource::GetCurrentFrame() const
{
    if (!m_decoder)
        return 0;

    std::size_t srcFrames = m_decoder->GetCurrentFrame();
    return SourceFrameToDeviceFrame(srcFrames);
}

std::size_t CAudioSource::GetTotalFrames() const 
{ 
    //return SourceFrameToDeviceFrame(m_audioInfo.m_totalFrames);
    if (!m_decoder)
        return 0;

    std::size_t srcFrames = m_decoder->GetTotalFrame();
    return SourceFrameToDeviceFrame(srcFrames);
}

float CAudioSource::FramesToSeconds(std::size_t frames) const 
{
    if (m_outFmt.format == AudioDataFormat::UNKNOWN) //
        return float(frames) / float(m_audioInfo.m_audioFmt.sampleRate);

    //if (m_devFmt.sampleRate == m_audioInfo.m_audioFmt.sampleRate)
    //    return float(frames) / float(m_devFmt.sampleRate);

    return float(frames) / float(m_outFmt.sampleRate);
}

std::size_t CAudioSource::SecondsToFrames(float seconds) const
{
    if (m_outFmt.format == AudioDataFormat::UNKNOWN) //
        return static_cast<std::size_t>(m_audioInfo.m_audioFmt.sampleRate * seconds);

    //if (m_devFmt.sampleRate == m_audioInfo.m_audioFmt.sampleRate)
    //    return static_cast<std::size_t>(m_devFmt.sampleRate * seconds);

    return static_cast<std::size_t>(m_outFmt.sampleRate * seconds);
}
