/*
20260526 ��������
��ģ�ͣ�ChatGPT 5.3 Codex
����������todo_task_78.txt
*/
#include <array>
#include <fstream>

#include "UnicodeConvert.h"
#include "AudioInfo.h"
#include "FlacFunc.h"

namespace {

uint32_t ParseHeader(const std::array<unsigned char, 64>& header, std::size_t readCount)
{
    if (readCount < 4) {
        return StreamFormatUnknown;
    }

    if ((header[0] == 'f') && (header[1] == 'L') && (header[2] == 'a') && (header[3] == 'C')) {
        return StreamFormatFlac;
    }

    if ((readCount >= 36) && (header[0] == 'O') && (header[1] == 'g') && (header[2] == 'g') && (header[3] == 'S')) {
        for (std::size_t i = 0; i + 4 <= readCount; ++i)
        {
            if ((header[i] == 'F') && (header[i + 1] == 'L') && (header[i + 2] == 'A') && (header[i + 3] == 'C')) {
                return StreamFormatFlac;
            }
        }
    }

    return StreamFormatUnknown;
}

}

uint32_t ParseStreamFormatByLibflac(const char* filenameUtf8, CDataStream* pStream)
{
    std::array<unsigned char, 64> header = {};
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
