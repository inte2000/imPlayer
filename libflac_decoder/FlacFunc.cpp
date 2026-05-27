/*
20260526 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_78.txt
*/
#include <array>
#include <fstream>

#include "UnicodeConvert.h"
#include "AudioInfo.h"
#include "FlacFunc.h"

uint32_t ParseStreamFormatByLibflac(const char* filenameUtf8)
{
    if ((filenameUtf8 == nullptr) || (filenameUtf8[0] == '\0')) {
        return StreamFormatUnknown;
    }

    const std::wstring filename = UTtf8ToUtf16Le(filenameUtf8);
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return StreamFormatUnknown;
    }

    std::array<unsigned char, 64> header = {};
    file.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
    const std::size_t readCount = static_cast<std::size_t>(file.gcount());
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
