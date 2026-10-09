/*
20260526 ��������
��ģ�ͣ�ChatGPT 5.3 Codex
����������todo_task_74.txt

�޸ļ�¼��
��ģ�ͣ�ChatGPT 5.3 Codex
todo_task_75.txt
todo_task_77.txt
*/
#include <fstream>
#include <cstring>

#include "UnicodeConvert.h"
#include "AudioInfo.h"
#include "Mpg123Func.h"

namespace {

uint32_t ParseHeader(const unsigned char* header, uint32_t readCount)
{
    if ((header == nullptr) || (readCount < 10)) {
        return StreamFormatUnknown;
    }

    if (std::memcmp(header, "ID3", 3) == 0) {
        return StreamFormatMp3;
    }

    if ((header[0] == 0xFF) && ((header[1] & 0xE0) == 0xE0)) {
        const uint8_t layerBits = static_cast<uint8_t>((header[1] >> 1) & 0x03);
        if (layerBits == 0x01) {
            return StreamFormatMp3;
        }
        if (layerBits == 0x02) {
            return StreamFormatMp2;
        }
        if (layerBits == 0x03) {
            return StreamFormatMp1;
        }
    }

    return StreamFormatUnknown;
}

}

uint32_t ParseStreamFormatByMpg123(const char* filenameUtf8, CDataStream* pStream)
{
    unsigned char header[10] = {};
    if ((filenameUtf8 != nullptr) && (filenameUtf8[0] != '\0')) {
        const std::wstring filename = UTtf8ToUtf16Le(filenameUtf8);
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return StreamFormatUnknown;
        }

        file.read(reinterpret_cast<char*>(header), static_cast<std::streamsize>(sizeof(header)));
        return ParseHeader(header, static_cast<uint32_t>(file.gcount()));
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
    const uint32_t readCount = pStream->Read(header, static_cast<uint32_t>(sizeof(header)));
    pStream->Seek(SeekBase::Begin, static_cast<long long>(oldPos));
    return ParseHeader(header, readCount);
}
