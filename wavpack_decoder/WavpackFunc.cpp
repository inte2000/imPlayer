/*
20260527 初次生成
大模型：ChatGPT 5.3 Codex
任务描述：todo_task_84.txt
*/
#include <array>
#include <fstream>

#include "UnicodeConvert.h"
#include "AudioInfo.h"
#include "WavpackFunc.h"

namespace {

constexpr uint32_t WAVPACK_FMT_WV_OFFSET = 0;
uint32_t g_formatIdBase = StreamFormatPlusBegin;

} // namespace

void SetWavpackCustomFormatBase(uint32_t formatIdBase)
{
    g_formatIdBase = formatIdBase;
}

uint32_t WavpackFormatWv()
{
    return g_formatIdBase + WAVPACK_FMT_WV_OFFSET;
}

uint32_t ParseStreamFormatByWavpack(const char* filenameUtf8)
{
    if ((filenameUtf8 == nullptr) || (filenameUtf8[0] == '\0')) {
        return StreamFormatUnknown;
    }

    const std::wstring filename = UTtf8ToUtf16Le(filenameUtf8);
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return StreamFormatUnknown;
    }

    std::array<unsigned char, 4> header = {};
    file.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
    if (file.gcount() != static_cast<std::streamsize>(header.size())) {
        return StreamFormatUnknown;
    }

    if ((header[0] == 'w') && (header[1] == 'v') && (header[2] == 'p') && (header[3] == 'k')) {
        return WavpackFormatWv();
    }

    return StreamFormatUnknown;
}
