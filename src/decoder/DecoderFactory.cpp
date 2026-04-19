
#include <vector>
#include <utility>
#include <unordered_map>
#include "DecoderFactory.h"
#include "Mp3Decoder.h"
#include "NativeDecoder.h"


CDecoderFactory::CDecoderFactory()
{
    //MP3 优先选择 libmpg123 解码
    RegisterDecoder(StreamFormatMp3, []() { return new CMp3Decoder{ StreamFormatMp3 }; });
    //RegisterDecoder(StreamFormatMp3, []() { return new CNativeDecoder{ StreamFormatMp3 }; });

    RegisterDecoder(StreamFormatOgg, []() { return new CNativeDecoder{ StreamFormatOgg }; });
    RegisterDecoder(StreamFormatCaf, []() { return new CNativeDecoder{ StreamFormatCaf }; });
    RegisterDecoder(StreamFormatCaf, []() { return new CFfmpegDecoder{ StreamFormatCaf }; });
    RegisterDecoder(StreamFormatAiff, []() { return new CNativeDecoder{ StreamFormatAiff }; });
    RegisterDecoder(StreamFormatVoc, []() { return new CNativeDecoder{ StreamFormatVoc }; });
    RegisterDecoder(StreamFormatW64, []() { return new CNativeDecoder{ StreamFormatW64 }; });
    RegisterDecoder(StreamFormatMidSds, []() { return new CNativeDecoder{ StreamFormatMidSds }; }); //midi sds

    RegisterDecoder(StreamFormatWav, []() { return new CNativeDecoder{ StreamFormatWav }; });

    RegisterDecoder(StreamFormatRaw, []() { return new CNativeDecoder{ StreamFormatRaw }; });
    RegisterDecoder(StreamFormatWve, []() { return new CNativeDecoder{ StreamFormatWve }; });
    RegisterDecoder(StreamFormatWav64, []() { return new CNativeDecoder{ StreamFormatWav64 }; });
    RegisterDecoder(StreamFormatPaf, []() { return new CNativeDecoder{ StreamFormatPaf }; });
    RegisterDecoder(StreamFormatSvx, []() { return new CNativeDecoder{ StreamFormatSvx }; });
    RegisterDecoder(StreamFormatNist, []() { return new CNativeDecoder{ StreamFormatNist }; });
    RegisterDecoder(StreamFormatIrcam, []() { return new CNativeDecoder{ StreamFormatIrcam }; });
    RegisterDecoder(StreamFormatMat4, []() { return new CNativeDecoder{ StreamFormatMat4 }; });
    RegisterDecoder(StreamFormatMat5, []() { return new CNativeDecoder{ StreamFormatMat5 }; });
    RegisterDecoder(StreamFormatPvf, []() { return new CNativeDecoder{ StreamFormatPvf }; });
    RegisterDecoder(StreamFormatXi, []() { return new CNativeDecoder{ StreamFormatXi }; });
    RegisterDecoder(StreamFormatHtk, []() { return new CNativeDecoder{ StreamFormatHtk }; });
    RegisterDecoder(StreamFormatAvr, []() { return new CNativeDecoder{ StreamFormatAvr }; });
    RegisterDecoder(StreamFormatSd2, []() { return new CNativeDecoder{ StreamFormatSd2 }; });
    RegisterDecoder(StreamFormatMPC2k, []() { return new CNativeDecoder{ StreamFormatMPC2k }; });
}

std::unique_ptr<CAudioDecoder> CDecoderFactory::MakeAudioDecoder(uint32_t fileFmt)
{
    auto it = m_decoders.find(fileFmt);
    if (it == m_decoders.end())
        return nullptr;

    return std::unique_ptr<CAudioDecoder>(it->second());
}

bool CDecoderFactory::RegisterDecoder(uint32_t fileFmt, DecoderMaker maker)
{
    if (m_decoders.find(fileFmt) != m_decoders.end())
        return false;

    m_decoders[fileFmt] = std::move(maker);
    return true;
}

uint32_t ParseAudioFileFormat(const std::wstring& filename)
{
    uint32_t type = LibSndfileQueryFileType(filename);
    if (type == StreamFormatUnknown)
    {
        /*
        type = LibavQueryFileType(filename);
        if (type == StreamFormatUnknown)
        {
            type = LibAdplugQueryFileType(filename);
        }
        */
    }

    return type;
}
