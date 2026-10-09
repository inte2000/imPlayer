/*
20260527 ��������
��ģ�ͣ�ChatGPT 5.3 Codex
����������todo_task_84.txt
*/
#include <array>
#include <fstream>

#include "UnicodeConvert.h"
#include "AudioInfo.h"
#include "WavpackFunc.h"

namespace {

constexpr uint32_t WAVPACK_FMT_WV_OFFSET = 0;
uint32_t g_formatIdBase = StreamFormatPlusBegin;

uint32_t ParseHeader(const std::array<unsigned char, 4>& header, std::size_t readCount)
{
    if (readCount != header.size()) {
        return StreamFormatUnknown;
    }

    if ((header[0] == 'w') && (header[1] == 'v') && (header[2] == 'p') && (header[3] == 'k')) {
        return ::WavpackFormatWv();
    }

    return StreamFormatUnknown;
}

} // namespace

void SetWavpackCustomFormatBase(uint32_t formatIdBase)
{
    g_formatIdBase = formatIdBase;
}

uint32_t WavpackFormatWv()
{
    return g_formatIdBase + WAVPACK_FMT_WV_OFFSET;
}

uint32_t ParseStreamFormatByWavpack(const char* filenameUtf8, CDataStream* pStream)
{
    std::array<unsigned char, 4> header = {};
    if ((filenameUtf8 != nullptr) && (filenameUtf8[0] != '\0')) {
        const std::wstring filename = UTtf8ToUtf16Le(filenameUtf8);
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return StreamFormatUnknown;
        }

        file.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
        return ParseHeader(header, static_cast<std::size_t>(file.gcount()));
    }

    if (pStream == nullptr) {
        return StreamFormatUnknown;
    }
    const DataStreamStyle style = pStream->GetStyle();
    if (((style & dsStyleSeekable) == 0) || ((style & dsStyleTellPos) == 0)) {
        return StreamFormatUnknown;
    }

    const std::size_t oldPos = pStream->Tell();
    pStream->Seek(SeekBase::Begin, 0);
    const uint32_t readCount = pStream->Read(header.data(), static_cast<uint32_t>(header.size()));
    pStream->Seek(SeekBase::Begin, static_cast<long long>(oldPos));
    return ParseHeader(header, static_cast<std::size_t>(readCount));
}
