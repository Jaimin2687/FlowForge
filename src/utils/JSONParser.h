#pragma once
#include "json.hpp"
#include <string>
class JSONParser {
public:
    static nlohmann::json parseFile(const std::string& path);
    static void writeFile(const std::string& path, const nlohmann::json& j);
};
