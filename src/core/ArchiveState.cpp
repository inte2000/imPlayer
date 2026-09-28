#include "ArchiveState.h"
#include "LibarchiveApi.h"

ArchiveState::ArchiveState()
    : m_archive(nullptr)
{
}

ArchiveState::~ArchiveState()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_archive != nullptr) {
        archive_read_free(m_archive);
        m_archive = nullptr;
    }
}

ArchiveState::ArchiveState(ArchiveState&& other) noexcept
    : m_archive(nullptr)
{
    std::lock_guard<std::mutex> lock(other.m_mutex);
    m_archivePath = std::move(other.m_archivePath);
    m_archive = other.m_archive;
    other.m_archive = nullptr;
}

ArchiveState& ArchiveState::operator=(ArchiveState&& other) noexcept
{
    if (this == &other) {
        return *this;
    }

    std::scoped_lock lock(m_mutex, other.m_mutex);

    if (m_archive != nullptr) {
        archive_read_free(m_archive);
    }

    m_archivePath = std::move(other.m_archivePath);
    m_archive = other.m_archive;
    other.m_archive = nullptr;
    return *this;
}
