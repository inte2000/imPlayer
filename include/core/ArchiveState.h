#ifndef ARCHIVE_STATE_H
#define ARCHIVE_STATE_H

#include <mutex>

extern "C" {
#include "unarr.h"
}

struct ArchiveState
{
    ArchiveState();
    ~ArchiveState();

    ArchiveState(const ArchiveState&) = delete;
    ArchiveState& operator=(const ArchiveState&) = delete;

    ArchiveState(ArchiveState&& other) noexcept;
    ArchiveState& operator=(ArchiveState&& other) noexcept;

    mutable std::mutex m_mutex;
    ar_stream* m_stream;
    ar_archive* m_archive;
};

#endif // ARCHIVE_STATE_H
