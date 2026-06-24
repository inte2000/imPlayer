#include "PlayList.h"

#include <utility>

const std::wstring& CPlayList::GetName() const
{
    return m_name;
}

void CPlayList::SetName(const std::wstring& name)
{
    m_name = name;
}

uint32_t CPlayList::GetCount() const
{
    return static_cast<uint32_t>(m_items.size());
}

std::unique_ptr<CMusic> CPlayList::GetMusic(uint32_t index)
{
    if (index >= m_items.size())
        return nullptr;

    const MusicItem& item = m_items[index];
    if (item.itemType == MUSIC_ITEM_TYPE_FILE)
        return std::make_unique<CFileMusic>(item);

    return nullptr;
}

bool CPlayList::GetItem(uint32_t index, MusicItem& item) const
{
    if (index >= m_items.size())
        return false;

    item = m_items[index];
    return true;
}

bool CPlayList::AddItem(const MusicItem& item)
{
    m_items.push_back(item);
    return true;
}

bool CPlayList::AddItem(MusicItem&& item)
{
    m_items.push_back(std::move(item));
    return true;
}

void CPlayList::RemoveItem(uint32_t index)
{
    if (index >= m_items.size())
        return;

    m_items.erase(m_items.begin() + index);
}

bool CPlayList::Copy(const std::vector<MusicItem>& items)
{
    m_items = items;
    return true;
}
