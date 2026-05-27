/*
20260526 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_74.txt

修改记录：
大模型：ChatGPT 5.3 Codex
todo_task_75.txt
todo_task_77.txt
*/
#include <fstream>
#include <cstring>

#include "UnicodeConvert.h"
#include "AudioInfo.h"
#include "Mpg123Func.h"

uint32_t ParseStreamFormatByMpg123(const char* filenameUtf8)
{
    if ((filenameUtf8 == nullptr) || (filenameUtf8[0] == '\0')) {
        return StreamFormatUnknown;
    }

    const std::wstring filename = UTtf8ToUtf16Le(filenameUtf8);
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return StreamFormatUnknown;
    }

    unsigned char header[10] = {};
    file.read(reinterpret_cast<char*>(header), static_cast<std::streamsize>(sizeof(header)));
    const uint32_t readCount = static_cast<uint32_t>(file.gcount());
    if (readCount < sizeof(header)) {
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
