#ifndef UNICODE_NORMALIZE_H
#define UNICODE_NORMALIZE_H

#include <string>
#include <vector>

enum NormalizerType
{
    NFC,
    NFD,
    NFKC,
    NFKD
};

//input 应该是 utf16 
std::u16string NormalizeUnicode(const std::u16string& input, NormalizerType type = NFC);

//input 应该是 utf8
std::string NormalizeUtf8(const std::string& input, NormalizerType type = NFC);
std::u8string NormalizeUtf8(const std::u8string& input, NormalizerType type = NFC);


#endif //UNICODE_NORMALIZE_H
