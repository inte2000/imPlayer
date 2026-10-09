#include <stdexcept>
#include <functional>
#include <fstream>
#include "AudioDecoderMap.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;


CAudioDecoderMap::CAudioDecoderMap()
{
}

void CAudioDecoderMap::SetDecoderMap(uint32_t st, const std::string& desc, const std::string& decoderName)
{
    auto it = m_DecoderMap.find(st);
    if (it == m_DecoderMap.end())
    {
        DecoderDesc item = { st, desc, decoderName };
        m_DecoderMap[st] = std::move(item);
    }
}

void CAudioDecoderMap::RemoveDecoderMap(const std::string& decoderName)
{
    std::erase_if(m_DecoderMap, [&decoderName](const auto& pair) {
        return pair.second.decodername == decoderName;
        });
}

std::string CAudioDecoderMap::GetDecoderName(uint32_t fileFmt)
{
    if (m_DecoderMap.empty())
        throw std::runtime_error("imPlayer decoder map is empty!");

    auto it = m_DecoderMap.find(fileFmt);
    if (it == m_DecoderMap.end())
        throw std::runtime_error(std::format("no decoder config for: {} file type", fileFmt));

    std::string decodername = it->second.decodername;
    if (m_CustomMap.size() > 0) //如果有自定义关系，就查找一下，看看是否需要修正
    {
        auto it = std::find_if(m_CustomMap.begin(), m_CustomMap.end(),
            [fileFmt](auto item) {
                return (item.st == fileFmt);
            });
        if (it != m_CustomMap.end())
        {
            decodername = it->decodername;
        }
    }

    return decodername;
}

bool CAudioDecoderMap::AddCustomDecoderMap(uint32_t st, const std::string& desc, const std::string& decoderName)
{
    auto it = std::find_if(m_CustomMap.begin(), m_CustomMap.end(),
        [st, &desc](auto item) {
            //return ((item.desc == desc) && (item.st == st));
            return (item.st == st);
        });

    if (it != m_CustomMap.end())
    {
        it->decodername = decoderName;
    }
    else
        m_CustomMap.emplace_back(st, desc, decoderName);

    return true;
}

bool CAudioDecoderMap::RemoveCustomDecoderMap(const std::string& decoderName)
{
    auto rmit = std::remove_if(m_CustomMap.begin(), m_CustomMap.end(),
        [&decoderName](const auto& item) {
            return (item.decodername == decoderName);
        });

    m_CustomMap.erase(rmit, m_CustomMap.end());
    
    return true;
}

bool LoadDecoderMapFile(const std::string& filename, std::vector<DecoderDesc>& decodermap)
{
    std::ifstream ifs(filename);
    if (!ifs.is_open())
        return false;

    json j;
    try
    {
        ifs >> j;
        decodermap.clear();

        if (!j.contains("decoders"))
            return false;

        for (auto& item : j["decoders"])
        {
            uint32_t st = item.value("StreamType", 0);
            std::string desc = item.value("desc", "");
            std::string decoder = item.value("decoder", "");

            decodermap.emplace_back(st, std::move(desc), std::move(decoder));
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool SaveDecoderMapFile(const std::string& filename, const std::vector<DecoderDesc>& decodermap)
{
    std::ofstream ofs(filename);
    if (!ofs.is_open())
        return false;

    try
    {
        json j;
        j["decoders"] = json::array();

        for (const auto& kv : decodermap)
        {
            json node;
            node["StreamType"] = kv.st;
            node["desc"] = kv.desc;
            node["decoder"] = kv.decodername;

            j["decoders"].push_back(node);
        }

        ofs << j.dump(4);
    }
    catch (...)
    {
        return false;
    }

    return true;
}

