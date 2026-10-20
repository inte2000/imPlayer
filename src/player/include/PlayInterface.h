#ifndef _PLAY_INTERFACE_H
#define _PLAY_INTERFACE_H


#include <string>
#include <cstdint>
#include <optional>

void SetupDevice(const std::string& type, const std::string& deviceName, const std::string& deviceId);
int MakePlayListFileInterface(const std::string& folder, bool recursion, const std::string& playlistFile);
int MakeArchivePlayListFileInterface(const std::string& archiveFile, const std::string& playlistFile);
int MakeCDPlayListFileInterface(const std::string& sourceName, const std::string& defaultPlaylistName, const std::string& playlistFile);
void StartPlayingInterface(const std::string& filename,
    bool bPlaylist,
    bool bCdSource,
    bool bArchiveSource,
    int sequenceMode,
    const std::string& speakerLayout);
void StartPlayingTuiInterface(const std::string& filename,
    bool bPlaylist,
    bool bCdSource,
    bool bArchiveSource,
    int sequenceMode,
    const std::string& speakerLayout);
// 未指定的参数保留编码器默认值，Open 后使用 GetTransFormat 与音源协商。
int ConvertMediaFileInterface(const std::string& srcFilename,
    const std::string& outFilename,
    const std::string& formatName,
    std::optional<uint32_t> sampleRate = std::nullopt,
    const std::optional<std::string>& cfmt = std::nullopt,
    std::optional<uint32_t> channels = std::nullopt);

#endif //_PLAY_INTERFACE_H
