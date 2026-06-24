#include <fstream>
#include <utility>

#include <nlohmann/json.hpp>

#include "PlayList.h"
#include "PlayListFile.h"
#include "UnicodeConvert.h"

using json = nlohmann::json;

namespace {

std::string WideToUtf8(const std::wstring& value)
{
    return Utf16ToUtf8(value);
}

std::wstring Utf8ToWide(const std::string& value)
{
    return UTtf8ToUtf16Le(value);
}

bool ParsePlaylistJson(const json& playlistJson, CPlayList& playlist)
{
    if (!playlistJson.contains("name") || !playlistJson.contains("items"))
        return false;
    if (!playlistJson["name"].is_string() || !playlistJson["items"].is_array())
        return false;

    std::vector<MusicItem> items;

    try {
        playlist.SetName(Utf8ToWide(playlistJson["name"].get<std::string>()));

        for (const auto& node : playlistJson["items"])
        {
            MusicItem item;
            item.itemType = node.value("itemType", MUSIC_ITEM_TYPE_UNKNOWN);
            item.res_url = Utf8ToWide(node.value("res_url", std::string()));
            item.track = node.value("track", 0);
            item.duration = node.value("duration", 0.0f);
            item.title = Utf8ToWide(node.value("title", std::string()));
            item.artists = Utf8ToWide(node.value("artists", std::string()));
            item.album = Utf8ToWide(node.value("album", std::string()));
            item.lyricsFilePath = Utf8ToWide(node.value("lyricsFilePath", std::string()));
            item.albumArtFilePath = Utf8ToWide(node.value("albumArtFilePath", std::string()));
            items.push_back(std::move(item));
        }
    }
    catch (...) {
        return false;
    }

    return playlist.Copy(items);
}

bool BuildPlaylistJson(const CPlayList& playlist, json& playlistJson)
{
    try {
        playlistJson = json::object();
        playlistJson["name"] = WideToUtf8(playlist.GetName());
        playlistJson["items"] = json::array();

        const uint32_t count = playlist.GetCount();
        for (uint32_t i = 0; i < count; ++i)
        {
            MusicItem item;
            if (!playlist.GetItem(i, item))
                return false;

            json node;
            node["itemType"] = item.itemType;
            node["res_url"] = WideToUtf8(item.res_url);
            node["track"] = item.track;
            node["duration"] = item.duration;
            node["title"] = WideToUtf8(item.title);
            node["artists"] = WideToUtf8(item.artists);
            node["album"] = WideToUtf8(item.album);
            node["lyricsFilePath"] = WideToUtf8(item.lyricsFilePath);
            node["albumArtFilePath"] = WideToUtf8(item.albumArtFilePath);
            playlistJson["items"].push_back(std::move(node));
        }
    }
    catch (...) {
        return false;
    }

    return true;
}

}

bool LoadPlaylistFile(const std::string& filename, CPlayList& playlist)
{
    std::ifstream ifs(filename);
    if (!ifs.is_open())
        return false;

    json playlistJson;
    try {
        ifs >> playlistJson;
    }
    catch (...) {
        return false;
    }

    return ParsePlaylistJson(playlistJson, playlist);
}

bool SavePlaylistFile(const std::string& filename, const CPlayList& playlist)
{
    json playlistJson;
    if (!BuildPlaylistJson(playlist, playlistJson))
        return false;

    std::ofstream ofs(filename);
    if (!ofs.is_open())
        return false;

    try {
        ofs << playlistJson.dump(4);
    }
    catch (...) {
        return false;
    }

    return true;
}
