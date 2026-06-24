#ifndef PLAY_LIST_H
#define PLAY_LIST_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Music.h"
#include "MusicItem.h"

class CPlayList
{
public:
    const std::wstring& GetName() const;
    void SetName(const std::wstring& name);
    uint32_t GetCount() const;
    std::unique_ptr<CMusic> GetMusic(uint32_t index);
    bool GetItem(uint32_t index, MusicItem& item) const;
    bool AddItem(const MusicItem& item);
    bool AddItem(MusicItem&& item);
    void RemoveItem(uint32_t index);
    bool Copy(const std::vector<MusicItem>& items);

protected:
    std::wstring m_name;
    std::vector<MusicItem> m_items;
};

#endif // PLAY_LIST_H
