#ifndef STREAM_META_SOURCE_H
#define STREAM_META_SOURCE_H

#include <string>

#include "AudioInfo.h"

struct DsMetaInfo
{
    uint32_t itemSequence;
    std::wstring itemTitle;
    std::wstring itemName;
    std::wstring itemArtist;
    std::wstring itemPerformer;
    std::wstring itemAlbum;
    std::string itemMediaType;
    AudioFormat itemFormat;
};

struct MetaSource
{
    virtual ~MetaSource() = default;
    virtual const DsMetaInfo* GetMetaInformation() const = 0;
};

#endif // STREAM_META_SOURCE_H
