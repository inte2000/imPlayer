#ifndef ENCODER_PARAMTER_DEFINE_H
#define ENCODER_PARAMTER_DEFINE_H

#include <cstdint>
#include <algorithm>
#include <iterator>
#include <initializer_list>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "EncodingParams.h"

#pragma pack(push, 4)
struct EncoderParamterDefine
{
public:
    using ValueType = EncoderValueType;
    using RangeType = std::variant<std::monostate,
        std::pair<uint32_t, uint32_t>,
        std::pair<int32_t, int32_t>,
        std::pair<double, double>>;

    EncoderParamterDefine() = default;
    EncoderParamterDefine(std::string name, EncoderParamType type, ValueType defaultValue)
        : m_type(type), m_defaultValue(MakeEncoderValue(defaultValue))
    {
        SetName(std::move(name));
    }

    EncoderParamterDefine(std::string name, EncoderParamType type, ValueType defaultValue, std::vector<ValueType> optionValues)
        : EncoderParamterDefine(std::move(name), type, std::move(defaultValue))
    {
        SetOptionValues(std::move(optionValues));
    }

    EncoderParamterDefine(std::string name, EncoderParamType type, ValueType defaultValue, std::initializer_list<ValueType> optionValues)
        : EncoderParamterDefine(std::move(name), type, std::move(defaultValue), std::vector<ValueType>(optionValues))
    {
    }

    std::string GetName() const { return ReadEncoderString(m_name); }
    void SetName(std::string name) { CopyEncoderString(m_name, name); }

    EncoderParamType GetType() const { return m_type; }
    void SetType(EncoderParamType type) { m_type = type; }

    ValueType GetDefaultValue() const { return ReadEncoderValue(m_defaultValue); }
    void SetDefaultValue(ValueType value) { m_defaultValue = MakeEncoderValue(value); }

    RangeType GetRange() const
    {
        switch (m_rangeType) {
        case EncoderParamType::None: return std::monostate{};
        case EncoderParamType::UnsignedInt: return std::pair(m_range.unsignedRange.minimum, m_range.unsignedRange.maximum);
        case EncoderParamType::SignedInt: return std::pair(m_range.signedRange.minimum, m_range.signedRange.maximum);
        case EncoderParamType::Float: return std::pair(m_range.floatRange.minimum, m_range.floatRange.maximum);
        default: throw std::runtime_error("Invalid encoder parameter range type.");
        }
    }

    void SetUnsignedIntRange(uint32_t minValue, uint32_t maxValue)
    {
        m_rangeType = EncoderParamType::UnsignedInt;
        m_range.unsignedRange = { minValue, maxValue };
    }

    void SetSignedIntRange(int32_t minValue, int32_t maxValue)
    {
        m_rangeType = EncoderParamType::SignedInt;
        m_range.signedRange = { minValue, maxValue };
    }

    void SetFloatRange(double minValue, double maxValue)
    {
        m_rangeType = EncoderParamType::Float;
        m_range.floatRange = { minValue, maxValue };
    }

    void ClearRange()
    {
        m_rangeType = EncoderParamType::None;
        m_range = {};
    }

    std::vector<ValueType> GetOptionValues() const
    {
        if (m_optionCount > EncoderParamOptionsLimit) throw std::runtime_error("Invalid encoder option count.");
        std::vector<ValueType> result;
        result.reserve(m_optionCount);
        for (uint32_t i = 0; i < m_optionCount; ++i) result.push_back(ReadEncoderValue(m_optionValues[i]));
        return result;
    }
    void SetOptionValues(std::vector<ValueType> optionValues)
    {
        if (optionValues.size() > EncoderParamOptionsLimit) throw std::runtime_error("Encoder option count exceeds its limit.");
        EncoderValue values[EncoderParamOptionsLimit]{};
        for (std::size_t i = 0; i < optionValues.size(); ++i) values[i] = MakeEncoderValue(optionValues[i]);
        std::copy(std::begin(values), std::end(values), std::begin(m_optionValues));
        m_optionCount = static_cast<uint32_t>(optionValues.size());
    }

    void Validate() const
    {
        (void)GetName();
        if (static_cast<uint32_t>(m_type) > static_cast<uint32_t>(EncoderParamType::Custom))
            throw std::runtime_error("Invalid encoder parameter type.");
        (void)GetDefaultValue();
        (void)GetRange();
        (void)GetOptionValues();
    }

private:
    char m_name[EncoderParamNameLimit]{};
    EncoderParamType m_type = static_cast<EncoderParamType>(0);
    EncoderValue m_defaultValue{};
    EncoderParamType m_rangeType = EncoderParamType::None;
    union {
        struct { uint32_t minimum, maximum; } unsignedRange;
        struct { int32_t minimum, maximum; } signedRange;
        struct { double minimum, maximum; } floatRange;
    } m_range{};
    EncoderValue m_optionValues[EncoderParamOptionsLimit]{};
    uint32_t m_optionCount = 0;
};
#pragma pack(pop)

#endif //ENCODER_PARAMTER_DEFINE_H
