#pragma once
#include <vector>
#include <string>
#include <memory>
#include "utils/json.hpp"
struct ActionConfig {
    std::string type;
    std::string params;
};
class Workflow {
public:
    Workflow(const std::string& name, const std::vector<ActionConfig>& actions, const nlohmann::json& rule = {});
    void execute();
    void executeWithOverrides(const std::vector<std::string>& overrides);
    std::string getName() const;
    const std::vector<ActionConfig>& getActions() const { return actions_; }
private:
    std::string name_;
    std::vector<ActionConfig> actions_;
    nlohmann::json rule_;
};
