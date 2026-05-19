/*
20250726 AI 生成（Web 问答，手工粘贴代码）
大模型：ChatGPT 4
*/
#include <cassert>
#include <algorithm>
#include <iterator>
#include "Utf8String.h"
#include "UnicodeNormalize.h"
#include "UnicodeConvert.h"

bool is_valid_utf8(const std::string& data) {
    size_t i = 0;
    const size_t len = data.size();

    while ((i < len) && (data[i] != 0)) {
        uint8_t c = static_cast<uint8_t>(data[i]);
        size_t seq_len = 0;

        if (c <= 0x7F) {
            seq_len = 1;
        }
        else if ((c >> 5) == 0x6) { // 110xxxxx
            seq_len = 2;
            if (c < 0xC2) return false; // 超短编码
        }
        else if ((c >> 4) == 0xE) { // 1110xxxx
            seq_len = 3;
        }
        else if ((c >> 3) == 0x1E) { // 11110xxx
            seq_len = 4;
            if (c > 0xF4) return false; // 超出 U+10FFFF
        }
        else {
            return false;
        }

        if (i + seq_len > len) return false;

        for (size_t j = 1; j < seq_len; ++j) {
            if ((static_cast<uint8_t>(data[i + j]) >> 6) != 0x2) {
                return false; // continuation byte 必须是 10xxxxxx
            }
        }

        // 检查 surrogate 范围
        if (seq_len == 3) {
            uint8_t c1 = static_cast<uint8_t>(data[i]);
            uint8_t c2 = static_cast<uint8_t>(data[i + 1]);
            if (c1 == 0xED && (c2 & 0xE0) == 0xA0) {
                return false;
            }
        }

        i += seq_len;
    }
    return true;
}

// 检查 GBK 合法性
bool is_valid_gbk(const std::string& data) {
    size_t i = 0;
    const size_t len = data.size();

    while ((i < len) && (data[i] != 0)) {
        uint8_t c = static_cast<uint8_t>(data[i]);
        if (c <= 0x7F) {
            // 单字节 ASCII
            ++i;
        }
        else {
            if (i + 1 >= len) return false;
            uint8_t c2 = static_cast<uint8_t>(data[i + 1]);
            // GBK 双字节范围：0x81-0xFE 0x40-0xFE (不包括 0x7F)
            if (c >= 0x81 && c <= 0xFE &&
                ((c2 >= 0x40 && c2 <= 0xFE) && c2 != 0x7F)) {
                i += 2;
            }
            else {
                return false;
            }
        }
    }
    return true;
}

std::string DecodeUnknownString(const std::string& input)
{
    if (is_valid_utf8(input))
        return NormalizeUtf8(input);

    //其他情况尝试用本地编码转成 utf8 
    return LocalMBCSToUtf8(input);
}

