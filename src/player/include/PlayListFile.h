#ifndef PLAY_LIST_FILE_H
#define PLAY_LIST_FILE_H

#include <string>

class CPlayList;

bool LoadPlaylistFile(const std::string& filename, CPlayList& playlist);
bool SavePlaylistFile(const std::string& filename, const CPlayList& playlist);

#endif // PLAY_LIST_FILE_H
