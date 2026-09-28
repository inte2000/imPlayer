#ifndef ARCHIVE_STATE_H
#define ARCHIVE_STATE_H

#include <mutex>
#include <string>

extern "C" {
struct archive;
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
    std::wstring m_archivePath;
    archive* m_archive;
};

#endif // ARCHIVE_STATE_H
