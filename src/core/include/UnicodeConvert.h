/*
20250726 AI 生成（Web 问答，手工粘贴代码）
大模型：ChatGPT 4
*/
#ifndef UNICODE_CONVERT_H
#define UNICODE_CONVERT_H

#include <string>
#include <vector>
#include <unicode/translit.h>

// UTF-8 → UTF-16
std::u16string UTtf8ToUtf16(const std::u8string& input);
std::u16string UTtf8ToUtf16_2(const std::string& input);
std::wstring UTtf8ToUtf16Le(const std::string& input);

// UTF-16 → UTF-8
std::u8string Utf16ToUtf8(const std::u16string& input);
std::string Utf16ToUtf8_2(const std::u16string& input);
std::string Utf16ToUtf8(const std::wstring& input);

// UTF-8 → UTF-32
std::u32string Utf8ToUtf32(const std::u8string& input);
std::u32string Utf8ToUtf32_2(const std::string& input);

// UTF-32 → UTF-8
std::u8string Utf32ToUtf8(const std::u32string& input);
std::string Utf32ToUtf8_2(const std::u32string& input);

// UTF-8 → Latin1
std::string UTtf8ToLatin(const std::u8string& input, const char* coding = "Latin1");
std::string UTtf8ToLatin(const std::string& input, const char* coding = "Latin1");

//coding: "GB2312"，只能表示国标 2312 范围的简体中文（不含繁体和扩展字符）
//coding: "GBK"，GB2312 的超集，常用在 Windows，支持更多汉字
//coding: "GB18030"，国家标准超集，支持所有 Unicode 字符，Windows 新系统默认中文编码
//coding: "BIG5"，台湾/港澳地区常用繁体中文编码
//coding: "Big5-HKSCS"，Big5 扩展，支持香港字符集

// UTF-8 → 本地多字节扩展 ASCII
std::string Utf8ToLocalMBCS(const std::string& utf8_str, const std::string& coding = "");

// 本地多字节扩展 ASCII → UTF-8
std::string LocalMBCSToUtf8(const std::string& mbcs_str, const std::string& coding = "");

// UTF-16LE → 本地多字节扩展 ASCII
std::string Utf16LeToLocalMBCS(const std::wstring& utf16_str, const std::string& coding = "");

// 本地多字节扩展 ASCII → UTF-q6LE
std::wstring LocalMBCSToUtf16Le(const std::string& mbcs_str, const std::string& coding = "");


bool IsValidUtf8Chars(const std::string& data);
bool IsValidExtAsciiChars(const std::string& data);

std::pair<std::string, uint32_t> DetectEncoding(const std::string& data);
std::string AdaptiveToUtf8(const std::string& sfString);

enum class CharsetType {
    ASCII,
    EXTENDED_ASCII,
    LATIN,
    UNKNOWN
};
CharsetType DetectSpecialCharset(const std::string& data);

std::string DetectSystemEncoding();

//繁体中文与简体中文互转
class ChineseTransliterator
{
public:
    ChineseTransliterator(bool bTradToSim = true);
    std::u8string TranslateUtf8(const std::u8string& utf8Str);
    std::string TranslateUtf8_2(const std::string& utf8Str);
    std::string TranslateMbcs(const std::string& mbcsStr);
private:
    std::unique_ptr<icu::Transliterator> m_translit;
};

bool IaAsciiOrLatinString(const std::wstring& s);
std::string CasefoldUtf8(const std::string& s);

#endif //UNICODE_CONVERT_H
