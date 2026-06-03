#ifndef ENCODER_PARAMTER_DEFINE_H
#define ENCODER_PARAMTER_DEFINE_H

#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

enum class EncoderParamType : uint32_t;

struct EncoderParamterDefine
{
public:
    using ValueType = std::variant<std::monostate, std::string, uint32_t, int32_t, bool, double, std::vector<uint8_t>>;
    using RangeType = std::variant<std::monostate,
        std::pair<uint32_t, uint32_t>,
        std::pair<int32_t, int32_t>,
        std::pair<double, double>>;

    EncoderParamterDefine() = default;
    EncoderParamterDefine(std::string name, EncoderParamType type, ValueType defaultValue)
        : m_name(std::move(name))
        , m_type(type)
        , m_defaultValue(std::move(defaultValue))
    {
    }

    const std::string& GetName() const { return m_name; }
    void SetName(std::string name) { m_name = std::move(name); }

    EncoderParamType GetType() const { return m_type; }
    void SetType(EncoderParamType type) { m_type = type; }

    const ValueType& GetDefaultValue() const { return m_defaultValue; }
    ValueType& GetDefaultValue() { return m_defaultValue; }
    void SetDefaultValue(ValueType value) { m_defaultValue = std::move(value); }

    const RangeType& GetRange() const { return m_range; }
    RangeType& GetRange() { return m_range; }

    void SetUnsignedIntRange(uint32_t minValue, uint32_t maxValue)
    {
        m_range = std::pair<uint32_t, uint32_t>(minValue, maxValue);
    }

    void SetSignedIntRange(int32_t minValue, int32_t maxValue)
    {
        m_range = std::pair<int32_t, int32_t>(minValue, maxValue);
    }

    void SetFloatRange(double minValue, double maxValue)
    {
        m_range = std::pair<double, double>(minValue, maxValue);
    }

    void ClearRange()
    {
        m_range = std::monostate{};
    }

    const std::vector<ValueType>& GetOptionValues() const { return m_optionValues; }
    std::vector<ValueType>& GetOptionValues() { return m_optionValues; }
    void SetOptionValues(std::vector<ValueType> optionValues) { m_optionValues = std::move(optionValues); }

private:
    std::string m_name;
    EncoderParamType m_type = static_cast<EncoderParamType>(0);
    ValueType m_defaultValue = std::monostate{};
    RangeType m_range = std::monostate{};
    std::vector<ValueType> m_optionValues;
};

#endif //ENCODER_PARAMTER_DEFINE_H
