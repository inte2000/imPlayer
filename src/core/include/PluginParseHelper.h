#pragma once

#include <string>

#include <windows.h>

#include "AudioInfo.h"
#include "DataStream.h"

namespace detail {

inline std::string StreamNameToUtf8(const std::wstring& value)
{
    if (value.empty()) {
        return {};
    }

    const int required = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (required <= 0) {
        return {};
    }

    std::string result(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), required, nullptr, nullptr);
    return result;
}

}

template<typename StreamParser, typename FileParser>
uint32_t ParsePluginFileTypeID(CDataStream* pStream, StreamParser&& parseStream, FileParser&& parseFile)
{
    if (pStream == nullptr) {
        return StreamFormatUnknown;
    }

    const DataStreamStyle style = pStream->GetStyle();
    if (((style & dsStyleSeekable) != 0) && ((style & dsStyleTellPos) != 0))
    {
        const uint32_t streamFmt = parseStream(pStream);
        pStream->Seek(SeekBase::Begin, 0);
        return streamFmt;
    }

    const std::wstring& streamName = pStream->GetName();
    if (streamName.empty()) {
        return StreamFormatUnknown;
    }

    const std::string utf8Name = detail::StreamNameToUtf8(streamName);
    if (utf8Name.empty()) {
        return StreamFormatUnknown;
    }

    return parseFile(utf8Name.c_str());
}
