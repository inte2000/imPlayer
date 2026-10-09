#ifndef ENCODING_PARAMS_H
#define ENCODING_PARAMS_H

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

enum class EncoderParamType : uint32_t
{
    None = 0,
    String,
    UnsignedInt,
    SignedInt,
    Bool,
    Float,
    Custom
};

struct EncoderParamter
{
public:
    using ValueType = std::variant<std::monostate, std::string, uint32_t, int32_t, bool, double, std::vector<uint8_t>>;

    EncoderParamter() = default;
    EncoderParamter(std::string name, EncoderParamType type, ValueType value)
        : m_name(std::move(name))
        , m_type(type)
        , m_value(std::move(value))
    {
    }

    EncoderParamter(const EncoderParamter&) = default;
    EncoderParamter(EncoderParamter&&) noexcept = default;
    EncoderParamter& operator=(const EncoderParamter&) = default;
    EncoderParamter& operator=(EncoderParamter&&) noexcept = default;

    const std::string& GetName() const { return m_name; }
    void SetName(std::string name) { m_name = std::move(name); }

    EncoderParamType GetType() const { return m_type; }
    void SetType(EncoderParamType type) { m_type = type; }

    const ValueType& GetValue() const { return m_value; }
    ValueType& GetValue() { return m_value; }
    void SetValue(ValueType value) { m_value = std::move(value); }

    template <typename TValue>
    std::optional<TValue> GetValue() const
    {
        const TValue* value = std::get_if<TValue>(&m_value);
        if (value == nullptr) {
            return std::nullopt;
        }

        return *value;
    }

    void SetString(std::string value)
    {
        m_type = EncoderParamType::String;
        m_value = std::move(value);
    }

    void SetUnsignedInt(uint32_t value)
    {
        m_type = EncoderParamType::UnsignedInt;
        m_value = value;
    }

    void SetSignedInt(int32_t value)
    {
        m_type = EncoderParamType::SignedInt;
        m_value = value;
    }

    void SetBool(bool value)
    {
        m_type = EncoderParamType::Bool;
        m_value = value;
    }

    void SetFloat(double value)
    {
        m_type = EncoderParamType::Float;
        m_value = value;
    }

    void SetCustom(std::vector<uint8_t> value)
    {
        m_type = EncoderParamType::Custom;
        m_value = std::move(value);
    }

private:
    std::string m_name;
    EncoderParamType m_type = EncoderParamType::None;
    ValueType m_value = std::monostate{};
};

inline const EncoderParamter* FindEncoderParamter(const std::vector<EncoderParamter>& params, const std::string& name)
{
    for (const auto& param : params) {
        if (param.GetName() == name) {
            return &param;
        }
    }

    return nullptr;
}

inline EncoderParamter* FindEncoderParamter(std::vector<EncoderParamter>& params, const std::string& name)
{
    for (auto& param : params) {
        if (param.GetName() == name) {
            return &param;
        }
    }

    return nullptr;
}

inline bool HasEncoderParamter(const std::vector<EncoderParamter>& params, const std::string& name)
{
    return FindEncoderParamter(params, name) != nullptr;
}

inline void AddEncoderParamter(std::vector<EncoderParamter>& params, std::string name, EncoderParamType type, EncoderParamter::ValueType value)
{
    params.emplace_back(std::move(name), type, std::move(value));
}

template <typename TValue>
inline std::optional<TValue> GetEncoderParamterValue(const std::vector<EncoderParamter>& params, const std::string& name)
{
    const auto* found = FindEncoderParamter(params, name);
    if (found == nullptr) {
        return std::nullopt;
    }

    const TValue* value = std::get_if<TValue>(&found->GetValue());
    if (value == nullptr) {
        return std::nullopt;
    }

    return *value;
}

#endif //ENCODING_PARAMS_H
