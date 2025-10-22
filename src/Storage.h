#pragma once
#include <string>
#include "utils/json.hpp"
class Storage {
public:
    static void saveState(const nlohmann::json& state, const std::string& path);
    static nlohmann::json loadState(const std::string& path);
};
