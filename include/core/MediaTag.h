#pragma once

#include <string>
#include <optional>
#include <format>
#include <unordered_map>
#include "MediaTagNames.h"

enum class MediaTagType
{
    Integer, String, Decimal
};

//template<typename Func>
//concept TagEnumCallback = std::invocable<Func, const std::string&, const std::string&>;
template<typename Func>
concept TagEnumCallback = requires(Func func, const std::string & k, MediaTagType t, const std::string & v) {
    func(k, t, v);  //
};

struct MediaTagPair
{
    MediaTagType type;
    std::string value;
};

class CMediaTag
{
public:
    void AddTagString(const std::string& name, const std::string& value) {
        AddTag(name, MediaTagType::String, value);
    }
    void AddTagInteger(const std::string& name, int32_t value) {
        std::string strValue = std::format("{}", value);
        AddTag(name, MediaTagType::Integer, strValue);
    }
    void AddTagDecimal(const std::string& name, double value) {
        std::string strValue = std::format("{:.6f}", value);
        AddTag(name, MediaTagType::Decimal, strValue);
    }

    std::optional<std::string> QueryTagString(const std::string& name) const;
    std::optional<int32_t> QueryTagInteger(const std::string& name) const;
    std::optional<double> QueryTagDecimal(const std::string& name) const;

    template<TagEnumCallback Func>
    void EnumTags(Func&& func) {
        for (const auto& [k, v] : m_tags)
            func(k, v);
    }
    bool IsEmpty() const { return (m_tags.size() == 0); }
    void Clear() { m_tags.clear(); }
protected:
    void AddTag(const std::string& name, MediaTagType type, const std::string& value);
    std::optional<MediaTagPair> QueryTag(const std::string& name) const;
private:
    std::unordered_map<std::string, MediaTagPair> m_tags;
};

