#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <string>

extern "C" {
#include "unarr.h"
}

struct ArchiveState
{
    ArchiveState();
    ~ArchiveState();

    bool Open(const std::wstring& archivePath);
    bool IsOpen() const;
    bool QueryEntrySize(const std::string& entryNameUtf8, std::size_t* entrySize);
    bool ReadEntryChunk(const std::string& entryNameUtf8, std::size_t offset, void* buf, std::size_t size, std::size_t* readSize);

private:
    bool OpenArchiveFromStream();

private:
    mutable std::mutex m_mutex;
    ar_stream* m_stream;
    ar_archive* m_archive;
};
