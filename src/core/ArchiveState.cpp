#include "ArchiveState.h"

ArchiveState::ArchiveState()
    : m_stream(nullptr)
    , m_archive(nullptr)
{
}

ArchiveState::~ArchiveState()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_archive != nullptr) {
        ar_close_archive(m_archive);
        m_archive = nullptr;
    }
    if (m_stream != nullptr) {
        ar_close(m_stream);
        m_stream = nullptr;
    }
}

ArchiveState::ArchiveState(ArchiveState&& other) noexcept
    : m_stream(nullptr)
    , m_archive(nullptr)
{
    std::lock_guard<std::mutex> lock(other.m_mutex);
    m_stream = other.m_stream;
    m_archive = other.m_archive;
    other.m_stream = nullptr;
    other.m_archive = nullptr;
}

ArchiveState& ArchiveState::operator=(ArchiveState&& other) noexcept
{
    if (this == &other) {
        return *this;
    }

    std::scoped_lock lock(m_mutex, other.m_mutex);

    if (m_archive != nullptr) {
        ar_close_archive(m_archive);
    }
    if (m_stream != nullptr) {
        ar_close(m_stream);
    }

    m_stream = other.m_stream;
    m_archive = other.m_archive;
    other.m_stream = nullptr;
    other.m_archive = nullptr;
    return *this;
}
