#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

enum class EncoderParamType : uint32_t
{
    None = 0, String, UnsignedInt, SignedInt, Bool, Float, Custom
};

constexpr uint32_t EncoderParamNameLimit = 64;
constexpr uint32_t EncoderParamStringLimit = 256;
constexpr uint32_t EncoderParamCustomLimit = 256;
constexpr uint32_t EncoderParamOptionsLimit = 64;
constexpr uint32_t EncoderParamItemsLimit = 32;

// Convenience values are local to a module, never members of ABI structures.
using EncoderValueType = std::variant<std::monostate, std::string, uint32_t, int32_t, bool, double, std::vector<uint8_t>>;

template <std::size_t N>
inline void CopyEncoderString(char (&output)[N], const std::string& input)
{
    if (input.size() >= N || input.find('\0') != std::string::npos)
        throw std::runtime_error("Encoder string exceeds its limit or contains NUL.");
    std::memcpy(output, input.data(), input.size());
    output[input.size()] = '\0';
}

template <std::size_t N>
inline std::string ReadEncoderString(const char (&input)[N])
{
    const auto* end = static_cast<const char*>(std::memchr(input, '\0', N));
    if (!end) throw std::runtime_error("Encoder string is not NUL terminated.");
    return std::string(input, end);
}

#pragma pack(push, 4)
struct EncoderValue
{
    EncoderParamType type;
    uint32_t length; // Bytes, excluding the string's trailing NUL.
    union {
        char stringValue[EncoderParamStringLimit];
        uint8_t customValue[EncoderParamCustomLimit];
        uint32_t unsignedValue;
        int32_t signedValue;
        uint32_t boolValue;
        double floatValue;
    } data;
};
#pragma pack(pop)

inline EncoderValue MakeEncoderValue(const EncoderValueType& input)
{
    EncoderValue result{};
    result.type = static_cast<EncoderParamType>(input.index());
    switch (result.type) {
    case EncoderParamType::None: break;
    case EncoderParamType::String: {
        const auto& text = std::get<std::string>(input);
        if (text.size() >= EncoderParamStringLimit) throw std::runtime_error("Encoder string value exceeds its limit.");
        std::memcpy(result.data.stringValue, text.data(), text.size());
        result.data.stringValue[text.size()] = '\0';
        result.length = static_cast<uint32_t>(text.size());
        break;
    }
    case EncoderParamType::UnsignedInt: result.data.unsignedValue = std::get<uint32_t>(input); break;
    case EncoderParamType::SignedInt: result.data.signedValue = std::get<int32_t>(input); break;
    case EncoderParamType::Bool: result.data.boolValue = std::get<bool>(input) ? 1 : 0; break;
    case EncoderParamType::Float: result.data.floatValue = std::get<double>(input); break;
    case EncoderParamType::Custom: {
        const auto& bytes = std::get<std::vector<uint8_t>>(input);
        if (bytes.size() > EncoderParamCustomLimit) throw std::runtime_error("Encoder custom value exceeds its limit.");
        if (!bytes.empty()) std::memcpy(result.data.customValue, bytes.data(), bytes.size());
        result.length = static_cast<uint32_t>(bytes.size());
        break;
    }
    default: throw std::runtime_error("Invalid encoder value type.");
    }
    return result;
}

inline EncoderValueType ReadEncoderValue(const EncoderValue& value)
{
    switch (value.type) {
    case EncoderParamType::None: return std::monostate{};
    case EncoderParamType::String:
        if (value.length >= EncoderParamStringLimit || value.data.stringValue[value.length] != '\0')
            throw std::runtime_error("Invalid encoder string value length.");
        return std::string(value.data.stringValue, value.length);
    case EncoderParamType::UnsignedInt: return value.data.unsignedValue;
    case EncoderParamType::SignedInt: return value.data.signedValue;
    case EncoderParamType::Bool:
        if (value.data.boolValue > 1) throw std::runtime_error("Invalid encoder boolean value.");
        return value.data.boolValue != 0;
    case EncoderParamType::Float: return value.data.floatValue;
    case EncoderParamType::Custom:
        if (value.length > EncoderParamCustomLimit) throw std::runtime_error("Invalid encoder custom value length.");
        return std::vector<uint8_t>(value.data.customValue, value.data.customValue + value.length);
    default: throw std::runtime_error("Invalid encoder value type.");
    }
}
