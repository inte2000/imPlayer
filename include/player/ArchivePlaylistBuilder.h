#pragma once

#include <string>

class CPlayList;

bool BuildArchivePlaylist(const std::wstring& archiveName, CPlayList& playlist);
