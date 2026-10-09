#include "Music.h"

#include <filesystem>
#include <format>
#include <utility>

#include "AudioSource.h"

std::wstring ResolveMusicItemName(const MusicItem& item)
{
    if (!item.item_name.empty()) {
        return item.item_name;
    }

    if (item.itemType == MUSIC_ITEM_TYPE_FILE) {
        return std::filesystem::path(item.res_url).filename().wstring();
    }
    if (item.itemType == MUSIC_ITEM_TYPE_CD_TRACK) {
        return std::format(L"CD Track {}", item.track);
    }
    if (item.itemType == MUSIC_ITEM_TYPE_NETWORK_STREAM) {
        return item.title;
    }

    return {};
}

void EnsureMusicItemName(MusicItem& item)
{
    if (item.item_name.empty()) {
        item.item_name = ResolveMusicItemName(item);
    }
}

CFileMusic::CFileMusic(const MusicItem& item)
    : m_item(item)
{
    EnsureMusicItemName(m_item);
}

CFileMusic::CFileMusic(MusicItem&& item)
    : m_item(std::move(item))
{
    EnsureMusicItemName(m_item);
}

uint32_t CFileMusic::GetType() const
{
    return static_cast<uint32_t>(m_item.itemType);
}

std::wstring CFileMusic::GetResUrl() const
{
    return m_item.res_url;
}

std::wstring CFileMusic::GetItemName() const
{
    return m_item.item_name;
}

std::unique_ptr<CAudioSource> CFileMusic::MakeAudioSource() const
{
    return MakeFileAudioSource(m_item.res_url);
}

int32_t CFileMusic::GetTrack() const
{
    return m_item.track;
}

float CFileMusic::GetDuration() const
{
    return m_item.duration;
}

std::wstring CFileMusic::GetTitle() const
{
    return m_item.title;
}

std::wstring CFileMusic::GetArtists() const
{
    return m_item.artists;
}

std::wstring CFileMusic::GetAlbum() const
{
    return m_item.album;
}

std::wstring CFileMusic::GetLyricsFilePath() const
{
    return m_item.lyricsFilePath;
}

std::wstring CFileMusic::GetAlbumArtFilePath() const
{
    return m_item.albumArtFilePath;
}

CCDTrackMusic::CCDTrackMusic(const MusicItem& item)
    : m_item(item)
{
    EnsureMusicItemName(m_item);
}

CCDTrackMusic::CCDTrackMusic(MusicItem&& item)
    : m_item(std::move(item))
{
    EnsureMusicItemName(m_item);
}

uint32_t CCDTrackMusic::GetType() const
{
    return static_cast<uint32_t>(m_item.itemType);
}

std::wstring CCDTrackMusic::GetResUrl() const
{
    return m_item.res_url;
}

std::wstring CCDTrackMusic::GetItemName() const
{
    return m_item.item_name;
}

std::unique_ptr<CAudioSource> CCDTrackMusic::MakeAudioSource() const
{
    return MakeCDTrackAudioSource(m_item.res_url, static_cast<uint32_t>(m_item.track));
}

int32_t CCDTrackMusic::GetTrack() const
{
    return m_item.track;
}

float CCDTrackMusic::GetDuration() const
{
    return m_item.duration;
}

std::wstring CCDTrackMusic::GetTitle() const
{
    return m_item.title;
}

std::wstring CCDTrackMusic::GetArtists() const
{
    return m_item.artists;
}

std::wstring CCDTrackMusic::GetAlbum() const
{
    return m_item.album;
}

std::wstring CCDTrackMusic::GetLyricsFilePath() const
{
    return m_item.lyricsFilePath;
}

std::wstring CCDTrackMusic::GetAlbumArtFilePath() const
{
    return m_item.albumArtFilePath;
}

CNetStreamMusic::CNetStreamMusic(const MusicItem& item)
    : m_item(item)
{
    EnsureMusicItemName(m_item);
}

CNetStreamMusic::CNetStreamMusic(MusicItem&& item)
    : m_item(std::move(item))
{
    EnsureMusicItemName(m_item);
}

uint32_t CNetStreamMusic::GetType() const
{
    return static_cast<uint32_t>(m_item.itemType);
}

std::wstring CNetStreamMusic::GetResUrl() const
{
    return m_item.res_url;
}

std::wstring CNetStreamMusic::GetItemName() const
{
    return m_item.item_name;
}

std::unique_ptr<CAudioSource> CNetStreamMusic::MakeAudioSource() const
{
    return MakeNetStreamAudioSource(m_item.res_url);
}

CArchiveMusic::CArchiveMusic(const MusicItem& item)
    : m_item(item)
{
}

CArchiveMusic::CArchiveMusic(MusicItem&& item)
    : m_item(std::move(item))
{
}

uint32_t CArchiveMusic::GetType() const
{
    return static_cast<uint32_t>(m_item.itemType);
}

std::wstring CArchiveMusic::GetResUrl() const
{
    return m_item.res_url;
}

std::wstring CArchiveMusic::GetItemName() const
{
    return m_item.item_name;
}

std::unique_ptr<CAudioSource> CArchiveMusic::MakeAudioSource() const
{
    return MakeArchiveFileAudioSource(m_item.res_url, m_item.item_name);
}

int32_t CArchiveMusic::GetTrack() const
{
    return m_item.track;
}

float CArchiveMusic::GetDuration() const
{
    return m_item.duration;
}

std::wstring CArchiveMusic::GetTitle() const
{
    return m_item.title;
}

std::wstring CArchiveMusic::GetArtists() const
{
    return m_item.artists;
}

std::wstring CArchiveMusic::GetAlbum() const
{
    return m_item.album;
}

std::wstring CArchiveMusic::GetLyricsFilePath() const
{
    return m_item.lyricsFilePath;
}

std::wstring CArchiveMusic::GetAlbumArtFilePath() const
{
    return m_item.albumArtFilePath;
}

int32_t CNetStreamMusic::GetTrack() const
{
    return m_item.track;
}

float CNetStreamMusic::GetDuration() const
{
    return m_item.duration;
}

std::wstring CNetStreamMusic::GetTitle() const
{
    return m_item.title;
}

std::wstring CNetStreamMusic::GetArtists() const
{
    return m_item.artists;
}

std::wstring CNetStreamMusic::GetAlbum() const
{
    return m_item.album;
}

std::wstring CNetStreamMusic::GetLyricsFilePath() const
{
    return m_item.lyricsFilePath;
}

std::wstring CNetStreamMusic::GetAlbumArtFilePath() const
{
    return m_item.albumArtFilePath;
}
