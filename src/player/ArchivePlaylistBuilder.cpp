#include <filesystem>
#include <memory>
#include <vector>

#include "ArchiveFileStream.h"
#include "ArchivePackage.h"
#include "ArchivePlaylistBuilder.h"
#include "DecoderFactory.h"
#include "PlayList.h"
#include "StringEx.h"

bool BuildArchivePlaylist(const std::wstring& archiveName, CPlayList& playlist)
{
    CArchivePackage archive;
    if (!archive.Open(archiveName)) {
        return false;
    }

    std::wstring playlistName = std::filesystem::path(archiveName).stem().wstring();
    if (playlistName.empty()) {
        playlistName = std::filesystem::path(archiveName).filename().wstring();
    }
    if (playlistName.empty()) {
        playlistName = L"Archive";
    }
    playlist.SetName(playlistName);

    CDecoderFactory& factory = CDecoderFactory::GetInstance();
    const std::vector<std::wstring> fileList = archive.GetFileList();
    for (const std::wstring& entryName : fileList)
    {
        std::unique_ptr<CDataStream> entryStream = MakeArchiveFileStream(archiveName, entryName, true);
        if (!entryStream) {
            continue;
        }

        const uint32_t fmt = factory.ParseFileFormat(entryStream.get());
        if (fmt == StreamFormatUnknown) {
            continue;
        }

        MusicItem item;
        item.itemType = MUSIC_ITEM_TYPE_ARCHIVE;
        item.res_url = archiveName;
        item.item_name = entryName;
        item.title = GetFileNamePart(entryName);
        playlist.AddItem(std::move(item));
    }

    return (playlist.GetCount() > 0);
}
