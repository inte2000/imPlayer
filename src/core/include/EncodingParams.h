#ifndef ENCODING_PARAMS_H
#define ENCODING_PARAMS_H

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include "EncoderValue.h"

#pragma pack(push, 4)
struct EncoderParamter
{
public:
    using ValueType = EncoderValueType;

    EncoderParamter() = default;
    EncoderParamter(std::string name, EncoderParamType type, ValueType value)
        : m_type(type), m_value(MakeEncoderValue(value))
    {
        SetName(std::move(name));
    }

    EncoderParamter(const EncoderParamter&) = default;
    EncoderParamter(EncoderParamter&&) noexcept = default;
    EncoderParamter& operator=(const EncoderParamter&) = default;
    EncoderParamter& operator=(EncoderParamter&&) noexcept = default;

    std::string GetName() const { return ReadEncoderString(m_name); }
    void SetName(std::string name) { CopyEncoderString(m_name, name); }

    EncoderParamType GetType() const { return m_type; }
    void SetType(EncoderParamType type) { m_type = type; }

    ValueType GetValue() const { return ReadEncoderValue(m_value); }
    void SetValue(ValueType value) { m_value = MakeEncoderValue(value); }

    template <typename TValue>
    std::optional<TValue> GetValue() const
    {
        const auto storedValue = GetValue();
        const TValue* value = std::get_if<TValue>(&storedValue);
        if (value == nullptr) {
            return std::nullopt;
        }

        return *value;
    }

    void SetString(std::string value)
    {
        SetValue(std::move(value));
        m_type = EncoderParamType::String;
    }

    void SetUnsignedInt(uint32_t value)
    {
        SetValue(value);
        m_type = EncoderParamType::UnsignedInt;
    }

    void SetSignedInt(int32_t value)
    {
        SetValue(value);
        m_type = EncoderParamType::SignedInt;
    }

    void SetBool(bool value)
    {
        SetValue(value);
        m_type = EncoderParamType::Bool;
    }

    void SetFloat(double value)
    {
        SetValue(value);
        m_type = EncoderParamType::Float;
    }

    void SetCustom(std::vector<uint8_t> value)
    {
        SetValue(std::move(value));
        m_type = EncoderParamType::Custom;
    }

    void Validate() const
    {
        (void)GetName();
        if (m_type != m_value.type) throw std::runtime_error("Encoder parameter type does not match its value.");
        (void)GetValue();
    }

private:
    char m_name[EncoderParamNameLimit]{};
    EncoderParamType m_type = EncoderParamType::None;
    EncoderValue m_value{};
};
#pragma pack(pop)

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

    return found->GetValue<TValue>();
}

#endif //ENCODING_PARAMS_H
