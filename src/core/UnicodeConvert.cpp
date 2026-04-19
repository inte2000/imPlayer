#include <filesystem>
#include <stdexcept>
#include <map>
#include <unicode/unistr.h>       // icu::UnicodeString
#include <unicode/errorcode.h>    // icu::ErrorCode
#include <unicode/ucnv.h>
#include <unicode/ucsdet.h> //字符检测
#include <unicode/utypes.h>
#include <unicode/uversion.h>
#include "ScopeGuard.h"
#include "UnicodeConvert.h"


// UTF-8 → UTF-16
std::u16string UTtf8ToUtf16(const std::u8string& input) 
{
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(
        icu::StringPiece(reinterpret_cast<const char*>(input.data()), static_cast<int32_t>(input.size()))
    );
    return std::u16string(reinterpret_cast<const char16_t*>(ustr.getBuffer()), ustr.length());
}

std::u16string UTtf8ToUtf16_2(const std::string& input)
{
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(input);
    return std::u16string(reinterpret_cast<const char16_t*>(ustr.getBuffer()), ustr.length());
}

std::wstring UTtf8ToUtf16Le(const std::string& input)
{
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(input);
    return std::wstring(reinterpret_cast<const wchar_t*>(ustr.getBuffer()), ustr.length());
}



// UTF-16 → UTF-8
std::u8string Utf16ToUtf8(const std::u16string& input)
{
    icu::UnicodeString ustr(reinterpret_cast<const UChar*>(input.data()), static_cast<int32_t>(input.size()));
    std::string utf8;
    ustr.toUTF8String(utf8);

    return std::u8string(reinterpret_cast<const char8_t*>(utf8.data()), utf8.size());
}

std::string Utf16ToUtf8_2(const std::u16string& input)
{
    icu::UnicodeString ustr(reinterpret_cast<const UChar*>(input.data()), static_cast<int32_t>(input.size()));
    std::string utf8;
    ustr.toUTF8String(utf8);

    return utf8;
}

std::string Utf16ToUtf8(const std::wstring& input)
{
    icu::UnicodeString ustr(reinterpret_cast<const UChar*>(input.data()), static_cast<int32_t>(input.size()));
    std::string utf8;
    ustr.toUTF8String(utf8);

    return utf8;
}

// UTF-8 → UTF-32
std::u32string Utf8ToUtf32(const std::u8string& input) 
{
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(
        icu::StringPiece(reinterpret_cast<const char*>(input.data()), static_cast<int32_t>(input.size()))
    );

    std::u32string result;
    result.reserve(ustr.length());

    for (int32_t i = 0; i < ustr.length();) 
    {
        UChar32 cp = ustr.char32At(i);
        result.push_back(static_cast<char32_t>(cp));
        i += U16_LENGTH(cp); // 跳过这个码点占用的 UTF-16 单元数
    }

    return result;
}

std::u32string Utf8ToUtf32_2(const std::string& input)
{
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(input);

    std::u32string result;
    result.reserve(ustr.length());

    for (int32_t i = 0; i < ustr.length();)
    {
        UChar32 cp = ustr.char32At(i);
        result.push_back(static_cast<char32_t>(cp));
        i += U16_LENGTH(cp); // 跳过这个码点占用的 UTF-16 单元数
    }

    return result;
}


// UTF-32 → UTF-8
std::u8string Utf32ToUtf8(const std::u32string& input)
{
    icu::UnicodeString ustr;

    for (char32_t cp : input) 
    {
        ustr.append(static_cast<UChar32>(cp));
    }

    std::string utf8;
    ustr.toUTF8String(utf8);

    return std::u8string(reinterpret_cast<const char8_t*>(utf8.data()), utf8.size());
}

std::string Utf32ToUtf8_2(const std::u32string& input)
{
    icu::UnicodeString ustr;

    for (char32_t cp : input)
    {
        ustr.append(static_cast<UChar32>(cp));
    }

    std::string utf8;
    ustr.toUTF8String(utf8);

    return utf8;
}

// UTF-8 → Latin1

static std::string GetIsoEncoding(const char* coding)
{
    static const std::map<const char*, const char*> encodingMap = {
                {"Latin1", "ISO-8859-1"},
                {"Latin2", "ISO-8859-2"},
                {"Latin3", "ISO-8859-3"},
                {"Latin4", "ISO-8859-4"},
                {"Latin5", "ISO-8859-5"},
                {"Latin6", "ISO-8859-6"},
                {"Latin7", "ISO-8859-7"},
                {"Latin8", "ISO-8859-8"},
                {"Latin9", "ISO-8859-9"},
                {"Latin10", "ISO-8859-10"},
                {"Windows1252", "windows-1252"}
    };
    auto it = encodingMap.find(coding);
    if (it == encodingMap.end()) {
        throw std::runtime_error("Not support Latin encoding!");
    }

    return it->second;
}

std::string UTtf8ToLatin(const std::u8string& input, const char* coding)
{
    std::string utf8str(reinterpret_cast<const char*>(input.data()), static_cast<int32_t>(input.size()));

    return UTtf8ToLatin(utf8str, coding);
}

std::string UTtf8ToLatin(const std::string& input, const char* coding)
{
    UErrorCode err = U_ZERO_ERROR;
    // 打开 UTF-8 转换器（源）
    UConverter* src = ucnv_open("UTF-8", &err);
    if (U_FAILURE(err))
        throw std::runtime_error("Failed to open UTF-8 converter");

    CScopeGuard guard_src([&src]() { ucnv_close(src); });

    // 打开 GBK 转换器（目标）
    std::string latinEncoding = GetIsoEncoding(coding);
    err = U_ZERO_ERROR;
    UConverter* dst = ucnv_open(latinEncoding.c_str(), &err);
    if (U_FAILURE(err))
        throw std::runtime_error("Failed to open latin encoding converter");

    CScopeGuard guard_dst([&dst]() { ucnv_close(dst); });
    
    //不设置替换，使用默认的 0x1a 填充
    //ucnv_setSubstChars(dst, "?", 1, &err);

    const char* source = input.c_str();
    err = U_ZERO_ERROR;
    // Latin-1 每个字符最多1字节，所以不会超过源长度
    std::vector<char> buffer(input.size() + 1); // +1 用于安全
    char* target = buffer.data();
    ucnv_convertEx(dst, src, &target, target + buffer.size(), &source, source + input.size(), nullptr, nullptr, nullptr, nullptr, 1, 1, &err);

    if (U_FAILURE(err) && (err != U_BUFFER_OVERFLOW_ERROR)) 
        throw std::runtime_error("Failed to convert encoding: " + std::string(u_errorName(err)));

    std::string result(buffer.data(), target - buffer.data());
    return result;
}

std::string Utf8ToLocalMBCS(const std::string& utf8_str, const std::string& coding)
{
    std::string sysEncoding;
    if (coding.empty())
        sysEncoding = DetectSystemEncoding();
    else
        sysEncoding = coding;

    UErrorCode err = U_ZERO_ERROR;

    // 打开 UTF-8 转换器（源）
    UConverter* src = ucnv_open("UTF-8", &err);
    if (U_FAILURE(err))
        throw std::runtime_error("Failed to open UTF-8 converter");

    CScopeGuard guard_src([&src]() { ucnv_close(src); });

    // 打开 GBK 转换器（目标）
    err = U_ZERO_ERROR;
    UConverter* dst = ucnv_open(sysEncoding.c_str(), &err);
    if (U_FAILURE(err)) 
        throw std::runtime_error("Failed to open coding converter");

    CScopeGuard guard_dst([&dst]() { ucnv_close(dst); });

    // 转换 UTF-8 -> Unicode（UChar）
    int32_t unicode_len = static_cast<uint32_t>(utf8_str.size() * 2);
    std::vector<UChar> unicode_buf(unicode_len);

    const char* src_ptr = utf8_str.data();
    const char* src_limit = src_ptr + utf8_str.size();
    UChar* unicode_ptr = unicode_buf.data();
    UChar* unicode_limit = unicode_ptr + unicode_len;

    err = U_ZERO_ERROR;
    ucnv_toUnicode(src, &unicode_ptr, unicode_limit, &src_ptr, src_limit, nullptr, 1, &err);
    if (U_FAILURE(err)) 
        throw std::runtime_error("Failed to convert UTF-8 to Unicode");

    int32_t ulen = static_cast<uint32_t>(unicode_ptr - unicode_buf.data());

    // 转换 Unicode -> LocalMBCS
    std::vector<char> mbcs_buf(ulen * 2);
    const UChar* uni_ptr = unicode_buf.data();
    const UChar* uni_limit = uni_ptr + ulen;
    char* mbcs_ptr = mbcs_buf.data();
    char* mbcs_limit = mbcs_ptr + mbcs_buf.size();

    err = U_ZERO_ERROR;
    ucnv_fromUnicode(dst, &mbcs_ptr, mbcs_limit, &uni_ptr, uni_limit, nullptr, 1, &err);
    if (U_FAILURE(err)) 
        throw std::runtime_error("Failed to convert Unicode to LocalMBCS");

    return std::string(mbcs_buf.data(), mbcs_ptr - mbcs_buf.data());
}

std::string LocalMBCSToUtf8(const std::string& mbcs_str, const std::string& coding)
{
    std::string sysEncoding;
    if (coding.empty())
        sysEncoding = DetectSystemEncoding();
    else
        sysEncoding = coding;

    UErrorCode err = U_ZERO_ERROR;
    UConverter* utf8Conv = ucnv_open("UTF-8", &err);
    if (U_FAILURE(err)) 
        throw std::runtime_error("Failed to open UTF-8 converter");

    CScopeGuard guard_utf8Conv([&utf8Conv]() { ucnv_close(utf8Conv); });

    UConverter* mbcsConv = ucnv_open(sysEncoding.c_str(), &err);
    if (U_FAILURE(err))
        throw std::runtime_error("Failed to open coding converter");

    CScopeGuard guard_mbcsConv([&mbcsConv]() { ucnv_close(mbcsConv); });

    std::string utf8str;
    int32_t targetCap = static_cast<uint32_t>(mbcs_str.size() * 3); // UTF-8 最多3字节
    utf8str.resize(targetCap);

    const char* src = mbcs_str.data();
    const char* srcLimit = src + mbcs_str.size();
    char* tgt = utf8str.data();
    const char* tgtLimit = tgt + targetCap;

    ucnv_convertEx(utf8Conv, mbcsConv, &tgt, tgtLimit, &src, srcLimit, nullptr, nullptr, nullptr, nullptr, 1, 1, &err);
    if (U_FAILURE(err)) 
        throw std::runtime_error("Conversion failed");

    utf8str.resize(tgt - utf8str.data());

    return utf8str;
    //return std::u8string(reinterpret_cast<const char8_t*>(utf8str.data()), utf8str.size());
}

std::string Utf16LeToLocalMBCS(const std::wstring& utf16_str, const std::string& coding)
{
    std::string sysEncoding;
    if (coding.empty())
        sysEncoding = DetectSystemEncoding();
    else
        sysEncoding = coding;

    UErrorCode err = U_ZERO_ERROR;
    UConverter* utf16Conv = ucnv_open("UTF-16LE", &err);
    if (U_FAILURE(err))
        throw std::runtime_error("Failed to open UTF-16LE converter");

    CScopeGuard guard_utf16Conv([&utf16Conv]() { ucnv_close(utf16Conv); });

    UConverter* mbcsConv = ucnv_open(sysEncoding.c_str(), &err);
    if (U_FAILURE(err))
        throw std::runtime_error("Failed to open coding converter");

    CScopeGuard guard_mbcsConv([&mbcsConv]() { ucnv_close(mbcsConv); });

    // 准备输入输出缓冲区
    const char* source = reinterpret_cast<const char*>(utf16_str.data());
    const char* sourceLimit = source + utf16_str.size() * sizeof(wchar_t);
    
    std::string result;
    result.resize(utf16_str.size() * 4); // 预估：UTF-16 -> GBK 最多 4 倍空间
    char* target = result.data();
    const char* targetStart = target;
    const char* targetLimit = target + result.size();

    err = U_ZERO_ERROR;
    ucnv_convertEx(mbcsConv, utf16Conv, &target, targetLimit, &source, sourceLimit, nullptr, nullptr, nullptr, nullptr, 1, 1, &err);
    if (U_FAILURE(err))
        throw std::runtime_error("Conversion failed");
    
    result.resize(target - targetStart);

    return result;
}

std::wstring LocalMBCSToUtf16Le(const std::string& mbcs_str, const std::string& coding)
{
    std::string sysEncoding;
    if (coding.empty())
        sysEncoding = DetectSystemEncoding();
    else
        sysEncoding = coding;

    UErrorCode err = U_ZERO_ERROR;
    UConverter* utf16Conv = ucnv_open("UTF-16LE", &err);
    if (U_FAILURE(err))
        throw std::runtime_error("Failed to open UTF-16LE converter");

    CScopeGuard guard_utf16Conv([&utf16Conv]() { ucnv_close(utf16Conv); });

    UConverter* mbcsConv = ucnv_open(sysEncoding.c_str(), &err);
    if (U_FAILURE(err))
        throw std::runtime_error("Failed to open coding converter");

    CScopeGuard guard_mbcsConv([&mbcsConv]() { ucnv_close(mbcsConv); });

    std::wstring utf16str;
    int32_t targetCap = static_cast<uint32_t>(mbcs_str.size() * 2 + 2); // UTF-16 最多2字节
    utf16str.resize(targetCap);

    const char* src = mbcs_str.data();
    const char* srcLimit = src + mbcs_str.size();
    char* tgt = (char *)utf16str.data();
    const char* tgtLimit = tgt + targetCap;

    ucnv_convertEx(utf16Conv, mbcsConv, &tgt, tgtLimit, &src, srcLimit, nullptr, nullptr, nullptr, nullptr, 1, 1, &err);
    if (U_FAILURE(err))
        throw std::runtime_error("Conversion failed");

    utf16str.resize((wchar_t *)tgt - utf16str.data());

    return utf16str;
    //return std::u8string(reinterpret_cast<const char8_t*>(utf8str.data()), utf8str.size());
}

bool IsValidUtf8Chars(const std::string& data) 
{
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

// 检查是否是扩展 ascii 和 ascii 字符序列
bool IsValidExtAsciiChars(const std::string& data) 
{
    size_t i = 0;
    const size_t len = data.size();

    while ((i < len) && (data[i] != 0)) {
        uint8_t c = static_cast<uint8_t>(data[i]);
        if ((c >= 0x20) && (c <= 0x7F)) {
            ++i;// 单字节 ASCII
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

std::pair<std::string, uint32_t> DetectEncoding(const std::string& data)
{
    std::string encoding_name;

    UErrorCode status = U_ZERO_ERROR;
    UCharsetDetector* csd = ucsdet_open(&status);
    if (U_FAILURE(status))
        return {encoding_name, 0};

    CScopeGuard guard_csd([&csd]() { ucsdet_close(csd); });

    // 设置待检测的字节数据
    ucsdet_setText(csd, data.c_str(), static_cast<int32_t>(data.size()), &status);
    if (U_FAILURE(status))
        return { encoding_name, 0 };

    // 检测最可能的编码
    const UCharsetMatch* match = ucsdet_detect(csd, &status);
    if (match && U_SUCCESS(status)) 
    {
        encoding_name = ucsdet_getName(match, &status);
        int32_t confidence = ucsdet_getConfidence(match, &status);
        return {encoding_name, confidence };
    }

    return { encoding_name, 0 };
}

std::string AdaptiveToUtf8(const std::string& sfString)
{
    std::string result;

    if (!sfString.empty())
    {
        if (IsValidUtf8Chars(sfString))
        {
            result = sfString;
        }
        else
        {
            result = LocalMBCSToUtf8(sfString);
        }
    }

    return result;
}

CharsetType DetectSpecialCharset(const std::string& data)
{
    UErrorCode status = U_ZERO_ERROR;
    UCharsetDetector* csd = ucsdet_open(&status);
    if (U_FAILURE(status))
        return CharsetType::UNKNOWN;

    CScopeGuard guard_csd([&csd]() { ucsdet_close(csd); });

    // 设置待检测的字节数据
    ucsdet_setText(csd, data.c_str(), static_cast<int32_t>(data.size()), &status);
    if (U_FAILURE(status))
        return CharsetType::UNKNOWN;

    CharsetType result = CharsetType::UNKNOWN;
    // 获取所有可能的匹配
    int32_t match_count = 0;
    const UCharsetMatch** matches = ucsdet_detectAll(csd, &match_count, &status);
    for (int32_t i = 0; i < match_count; i++)
    {
        const char* charset_name = ucsdet_getName(matches[i], &status);
        int32_t confidence = ucsdet_getConfidence(matches[i], &status);
        std::string charset(charset_name);
        
        // 检查字符集类型
        if (charset == "ASCII" && confidence > 50) {
            result = CharsetType::ASCII;
            break;
        }
        else if ((charset == "ISO-8859-1" || charset == "windows-1252" ||
            charset == "ISO-8859-15") && confidence > 50) {
            result = CharsetType::EXTENDED_ASCII;
            break;
        }
        else if (charset.find("8859") != std::string::npos || // ISO-8859 series
            charset.find("1252") != std::string::npos || // Windows Latin
            charset.find("Latin") != std::string::npos) {
            result = CharsetType::LATIN;
            break;
        }
    }

    return result;
}


#if defined(_WIN32) || defined(_WIN64)
static std::string GetWindowsEncoding()
{
    // Windows系统通常使用代码页
    const char* defaultConverter = ucnv_getDefaultName();
    if (defaultConverter) 
    {
        std::string encoding(defaultConverter);
        // 映射Windows代码页到标准编码名称
        if (encoding.find("1252") != std::string::npos) 
        {
            return "Windows-1252";
        }
        else if (encoding.find("936") != std::string::npos) 
        {
            return "GBK";
        }
        else if (encoding.find("950") != std::string::npos) 
        {
            return "Big5";
        }
        else if (encoding.find("932") != std::string::npos) 
        {
            return "Shift_JIS";
        }

        return encoding;
    }

    return "Windows-1252"; // 西欧系统默认
}

#elif defined(__APPLE__)
static std::string GetMacOSEncoding() 
{
    // macOS通常使用UTF-8
    return "UTF-8";
}
#elif defined(__linux__)
static std::string GetLinuxEncoding() 
{
    UErrorCode status = U_ZERO_ERROR;

    // 检查locale环境变量
    const char* lang = getenv("LANG");
    if (lang)
    {
        std::string locale(lang);
        // 解析locale中的编码信息
        size_t dotPos = locale.find('.');
        if (dotPos != std::string::npos)
        {
            std::string encoding = locale.substr(dotPos + 1);
            // 常见编码映射
            if (encoding == "UTF-8" || encoding == "utf8") 
            {
                return "UTF-8";
            }
            else if (encoding == "ISO-8859-1" || encoding == "latin1") 
            {
                return "ISO-8859-1";
            }
            else if (encoding == "GBK" || encoding == "gbk") 
            {
                return "GBK";
            }
        }
    }

    return "UTF-8"; // Linux默认UTF-8
}
#else
return "UTF-8"; // 其他系统默认UTF-8
#endif

std::string DetectSystemEncoding()
{
    // 获取ICU版本信息（包含平台信息）
    UVersionInfo icuVersion;
    u_getVersion(icuVersion);
    // 检查操作系统类型（通过编译时定义或运行时检测）
#if defined(_WIN32) || defined(_WIN64)
    return GetWindowsEncoding();
#elif defined(__APPLE__)
    return GetMacOSEncoding();
#elif defined(__linux__)
    return getLinuxEncoding();
#else
    return "UTF-8"; // 其他系统默认UTF-8
#endif
}

ChineseTransliterator::ChineseTransliterator(bool bTradToSim)
{
    UErrorCode ec = U_ZERO_ERROR;
    if(bTradToSim)
        m_translit.reset(icu::Transliterator::createInstance("Traditional-Simplified", UTRANS_FORWARD, ec));
    else
        m_translit.reset(icu::Transliterator::createInstance("Simplified-Traditional", UTRANS_FORWARD, ec));
    
    if (U_FAILURE(ec) || !m_translit)
        throw std::runtime_error("Failed to create Traditional<->Simplified transliterator");
}

std::u8string ChineseTransliterator::TranslateUtf8(const std::u8string& utf8Str)
{
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(
        icu::StringPiece(reinterpret_cast<const char*>(utf8Str.data()), static_cast<int32_t>(utf8Str.size()))
    );
    m_translit->transliterate(ustr);

    // 输出为 UTF-8
    std::string out;
    ustr.toUTF8String(out);

    return std::u8string(reinterpret_cast<const char8_t*>(out.data()), out.size());
}

std::string ChineseTransliterator::TranslateUtf8_2(const std::string& utf8Str)
{
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(utf8Str);
    m_translit->transliterate(ustr);

    // 输出为 UTF-8
    std::string out;
    ustr.toUTF8String(out);

    return out;
}

std::string ChineseTransliterator::TranslateMbcs(const std::string& mbcsStr)
{
    std::string utf8Str = LocalMBCSToUtf8(mbcsStr, "GB18030");
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(utf8Str);
    m_translit->transliterate(ustr);

    // 输出为 UTF-8
    std::string out;
    ustr.toUTF8String(out);

    return Utf8ToLocalMBCS(out, "GB18030");
}

bool IaAsciiOrLatinString(const std::wstring& s)
{
    for (wchar_t wc : s) 
    {
        uint32_t cp = static_cast<uint32_t>(wc);

        // ASCII: U+0000 – U+007F
        if (cp <= 0x7F)
            continue;

        // Latin-1 Supplement: U+0080 – U+00FF
        if (cp >= 0x80 && cp <= 0x00FF)
            continue;

        // Latin Extended-A / B: U+0100 – U+024F
        if (cp >= 0x0100 && cp <= 0x024F)
            continue;

        // 其他字符 -> 非 ASCII/Latin
        return false;
    }

    return true;
}

std::string CasefoldUtf8(const std::string& s)
{
    icu::UnicodeString us = icu::UnicodeString::fromUTF8(s);
    us.foldCase();  // ICU Unicode case folding
    std::string out;
    us.toUTF8String(out);
    return out;
}