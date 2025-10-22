#include "Workflow.h"
#include "PluginLoader.h"
#include "RuleEngine.h"
#include <iostream>
using namespace std;
Workflow::Workflow(const string& name, const vector<ActionConfig>& actions, const nlohmann::json& rule)
    : name_(name), actions_(actions), rule_(rule) {}
void Workflow::execute() {
    cout << "Starting workflow: " + name_ << endl;
    if (!RuleEngine::evaluate(rule_)) {
        cout << "Rule not satisfied for workflow: " + name_ << endl;
        return;
    }
    PluginLoader loader;
    for (const auto& action : actions_) {
        cout << "  Executing action: " + action.type + " with params: " + action.params << endl;
        try {
            auto plugin = loader.load("plugins/" + action.type + ".so");
            plugin->execute(action.params);
            cout << "  Action " + action.type + " completed." << endl;
        } catch (const exception& e) {
            cout << "  Action " + action.type + " failed: " + e.what() << endl;
        }
    }
    cout << "Workflow completed: " + name_ << endl;
}

void Workflow::executeWithOverrides(const std::vector<std::string>& overrides) {
    cout << "Starting workflow (with overrides): " + name_ << endl;
    if (!RuleEngine::evaluate(rule_)) {
        cout << "Rule not satisfied for workflow: " + name_ << endl;
        return;
    }
    PluginLoader loader;
    for (size_t i = 0; i < actions_.size(); ++i) {
        const auto& action = actions_[i];
        string params = (i < overrides.size() && !overrides[i].empty()) ? overrides[i] : action.params;
        cout << "  Executing action: " + action.type + " with params: " + params << endl;
        try {
            auto plugin = loader.load("plugins/" + action.type + ".so");
            plugin->execute(params);
            cout << "  Action " + action.type + " completed." << endl;
        } catch (const exception& e) {
            cout << "  Action " + action.type + " failed: " + e.what() << endl;
        }
    }
    cout << "Workflow completed: " + name_ << endl;
}
string Workflow::getName() const { return name_; }
