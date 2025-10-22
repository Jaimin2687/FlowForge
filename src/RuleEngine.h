#pragma once
#include <string>
#include "utils/json.hpp"

class RuleEngine {
public:
    static bool evaluate(const nlohmann::json& rule);
    
private:
    static bool evaluateCondition(const nlohmann::json& condition);
    static bool evaluateSingleCondition(const nlohmann::json& cond);
    static bool evaluateSimpleCondition(const std::string& cond);
    static bool evaluateDiskCondition(const std::string& cond);
    static bool evaluateCpuCondition(const std::string& cond);
    static bool evaluateMemoryCondition(const std::string& cond);
    static bool evaluateFileCondition(const std::string& cond);
    static bool evaluateTimeCondition(const std::string& cond);
};
