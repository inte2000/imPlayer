/*
20250709 AI 生成（Web 问答，手工粘贴代码）
大模型：ChatGPT 4
*/
#include <filesystem>
#include <stdexcept>
#include <unicode/unistr.h>       // icu::UnicodeString
#include <unicode/normalizer2.h>  // icu::Normalizer2
#include <unicode/errorcode.h>    // icu::ErrorCode
#include "UnicodeNormalize.h"

static const icu::Normalizer2* GetNormalizer(NormalizerType type)
{
    UErrorCode status = U_ZERO_ERROR;
    const icu::Normalizer2* normalizer = nullptr;

    switch (type) 
    {
    case NormalizerType::NFC:
        normalizer = icu::Normalizer2::getNFCInstance(status);
        break;
    case NormalizerType::NFD:
        normalizer = icu::Normalizer2::getNFDInstance(status);
        break;
    case NormalizerType::NFKC:
        normalizer = icu::Normalizer2::getNFKCInstance(status);
        break;
    case NormalizerType::NFKD:
        normalizer = icu::Normalizer2::getNFKDInstance(status);
        break;
    default:        
        break;
    }

    return normalizer;
}

// 使用 ICU 进行 Unicode 正规化
std::u16string NormalizeUnicode(const std::u16string& input, NormalizerType type)
{
    const icu::Normalizer2* normalizer = GetNormalizer(type);
    if (!normalizer) 
        throw std::runtime_error("Failed to get Normalizer2 instance");

    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString src(input.data(), static_cast<int32_t>(input.size()));
    icu::UnicodeString dest = normalizer->normalize(src, status);

    if (U_FAILURE(status))
        throw std::runtime_error("Normalization failed");

    return std::u16string(dest.getBuffer(), dest.length());
}

std::string NormalizeUtf8(const std::string& input, NormalizerType type)
{
    const icu::Normalizer2* normalizer = GetNormalizer(type);
    if (!normalizer)
        throw std::runtime_error("Failed to get Normalizer2 instance");

    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(input);
    icu::UnicodeString dest = normalizer->normalize(ustr, status);
    std::string out;
    dest.toUTF8String(out);

    return out;
}

std::u8string NormalizeUtf8(const std::u8string& input, NormalizerType type)
{
    const icu::Normalizer2* normalizer = GetNormalizer(type);
    if (!normalizer)
        throw std::runtime_error("Failed to get Normalizer2 instance");

    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(
        icu::StringPiece(reinterpret_cast<const char*>(input.data()), static_cast<int32_t>(input.size()))
    );
    icu::UnicodeString dest = normalizer->normalize(ustr, status);
    std::string out;
    dest.toUTF8String(out);

    return std::u8string(reinterpret_cast<const char8_t*>(out.data()), out.size());
}

#if 0
int main() {
    // 举例：U+00E9（é） vs. U+0065 + U+0301（e + 组合重音）
    std::u16string s = u"e\u0301";  // 分解形式：e + ́
    std::u16string normalized = normalize_unicode(s, "nfc");

    std::cout << "Original length: " << s.size() << "\n";
    std::cout << "NFC length: " << normalized.size() << "\n";

    // 输出 UTF-8 方便查看
    icu::UnicodeString u8(normalized.data(), normalized.size());
    std::string utf8;
    u8.toUTF8String(utf8);
    std::cout << "NFC normalized: " << utf8 << "\n";
}

Original length : 2
NFC length : 1
NFC normalized : é

#endif

#if 0
UTF - 8 只是把 Unicode 码点编码成字节序列的一种方式，不会改变字符的组合方式。
在 Unicode 中，同一个“字”可能有不同的码点序列表示。例如：
文字    Unicode 码点序列     名称                                             UTF-8 字节序列
é       U + 00E9            LATIN SMALL LETTER E WITH ACUTE                  0xC3 0xA9
é       U + 0065 U + 0301   LATIN SMALL LETTER E + COMBINING ACUTE ACCENT    0x65 0xCC 0x81

不做正规化可能导致的问题：
std::string("é") != std::string("é")，因为字节序列不一样

#endif

#if 0
//使用 C API（底层操作）,适合大量文本操作，效率高
int main() {
    const char* utf8Text = u8"你好，ICU！";
    UErrorCode status = U_ZERO_ERROR;

    // UTF-8 → UTF-16
    UChar utf16Buffer[100];
    int32_t utf16Len;
    u_strFromUTF8(utf16Buffer, 100, &utf16Len, utf8Text, -1, &status);

    // UTF-16 → UTF-8
    char utf8Buffer[100];
    int32_t utf8Len;
    status = U_ZERO_ERROR;
    u_strToUTF8(utf8Buffer, 100, &utf8Len, utf16Buffer, utf16Len, &status);

    std::cout << "Converted: " << utf8Buffer << "\n";
}
#endif
