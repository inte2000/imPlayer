#include <algorithm>
#include "MediaTag.h"

std::optional<std::string> CMediaTag::QueryTagString(const std::string& name) const 
{
    auto result = QueryTag(name);
    if (result && result.value().type == MediaTagType::String)
    {
        return result.value().value;
    }

    return std::nullopt;
}

std::optional<int32_t> CMediaTag::QueryTagInteger(const std::string& name) const
{
    auto result = QueryTag(name);
    if (result && result.value().type == MediaTagType::Integer)
    {
        return std::stoi(result.value().value);
    }

    return std::nullopt;
}

std::optional<double> CMediaTag::QueryTagDecimal(const std::string& name) const
{
    auto result = QueryTag(name);
    if (result && result.value().type == MediaTagType::Decimal)
    {
        return std::stod(result.value().value);
    }

    return std::nullopt;
}

void CMediaTag::AddTag(const std::string& name, MediaTagType type, const std::string& value)
{
    std::string tagName;
    tagName.resize(name.size());
    std::transform(name.begin(), name.end(), tagName.begin(), ::tolower);

    m_tags[tagName] = MediaTagPair{ type, value };
}

std::optional<MediaTagPair> CMediaTag::QueryTag(const std::string& name) const
{
    std::string tagName;
    tagName.resize(name.size());
    std::transform(name.begin(), name.end(), tagName.begin(), ::tolower);

    auto it = m_tags.find(tagName);
    if (it != m_tags.end())
        return it->second;

    return std::nullopt;
}
