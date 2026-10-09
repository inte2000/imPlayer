#ifndef STREAM_META_SOURCE_H
#define STREAM_META_SOURCE_H

#include <string>

#include "AudioInfo.h"
#include "MediaTag.h"

struct DsMetaInfo
{
    uint32_t itemSequence;
    CMediaTag itemTag;
    std::string itemMediaType;
    AudioFormat itemFormat;
};

struct MetaSource
{
    virtual ~MetaSource() = default;
    virtual const DsMetaInfo* GetMetaInformation() const = 0;
};

#endif // STREAM_META_SOURCE_H
