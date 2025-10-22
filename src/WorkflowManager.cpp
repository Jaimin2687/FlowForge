#include "WorkflowManager.h"
#include "utils/JSONParser.h"
#include "ThreadPool.h"
#include <iostream>
#include <future>

using namespace std;

void WorkflowManager::loadWorkflows(const string& configPath) {
    auto j = JSONParser::parseFile(configPath);

    // Guard: file could be empty or not contain "workflows"
    if (!j.is_object() || !j.contains("workflows") || !j["workflows"].is_array()) {
        cerr << "WorkflowManager::loadWorkflows: no 'workflows' array found in " << configPath << "\n";
        return;
    }

    for (const auto& wf : j["workflows"]) {
        // Basic validation
        if (!wf.is_object() || !wf.contains("name") || !wf.contains("actions")) {
            cerr << "Skipping invalid workflow entry in config (missing name/actions)\n";
            continue;
        }

        string name;
        try {
            name = wf["name"].get<std::string>();
        } catch (...) {
            cerr << "Skipping workflow with invalid name type\n";
            continue;
        }

        vector<ActionConfig> actions;
        if (!wf["actions"].is_array()) {
            cerr << "Workflow '" << name << "' has invalid 'actions' (not an array). Skipping.\n";
            continue;
        }

        for (const auto& act : wf["actions"]) {
            if (!act.is_object() || !act.contains("type")) {
                cerr << "Skipping invalid action in workflow '" << name << "' (missing type)\n";
                continue;
            }

            string type;
            try {
                type = act["type"].get<std::string>();
            } catch (...) {
                cerr << "Skipping action with invalid type in workflow '" << name << "'\n";
                continue;
            }

            // Ensure params is passed to plugins as a string. If params missing => empty string.
            std::string paramsStr;
            if (act.contains("params")) {
                try {
                    if (act["params"].is_string()) {
                        paramsStr = act["params"].get<std::string>();
                    } else {
                        paramsStr = act["params"].dump();
                    }
                } catch (...) {
                    paramsStr = "";
                }
            } else {
                paramsStr = "";
            }

            actions.push_back({ type, paramsStr });
        }

        nlohmann::json rule = wf.contains("rule") ? wf["rule"] : nlohmann::json{};
        workflows_.push_back(std::make_unique<Workflow>(name, actions, rule));
    }
}

void WorkflowManager::startAll() {
    // If no workflows, nothing to do
    if (workflows_.empty()) return;

    // Create a thread pool (size can be configured; 4 is default here)
    ThreadPool pool(4);
    vector<future<void>> futures;
    futures.reserve(workflows_.size());

    // Important: capture a stable pointer to each workflow for use inside the lambda.
    // Capturing the loop variable by reference would be UB because the reference
    // would refer to a variable that changes in the next iteration.
    for (const auto& wfPtr : workflows_) {
        Workflow* raw = wfPtr.get();
        if (!raw) continue;
        futures.push_back(pool.enqueue([raw]() {
            raw->execute();
        }));
    }

    // Wait for all workflows to finish
    for (auto& f : futures) {
        try {
            f.get();
        } catch (const std::exception& e) {
            cerr << "Exception from workflow thread: " << e.what() << "\n";
        } catch (...) {
            cerr << "Unknown exception from workflow thread\n";
        }
    }
}

void WorkflowManager::startWorkflow(const string& name) {
    for (const auto& wf : workflows_) {
        if (wf && wf->getName() == name) {
            wf->execute();
            return;
        }
    }
    cout << "Workflow not found: " << name << endl;
}

void WorkflowManager::startWorkflowWithOverrides(const string& name, const std::vector<string>& overrides) {
    for (const auto& wf : workflows_) {
        if (wf && wf->getName() == name) {
            wf->executeWithOverrides(overrides);
            return;
        }
    }
    cout << "Workflow not found: " << name << endl;
}

std::vector<std::string> WorkflowManager::listWorkflowNames() const {
    std::vector<std::string> names;
    names.reserve(workflows_.size());
    for (const auto& wf : workflows_) {
        if (wf) names.push_back(wf->getName());
    }
    return names;
}

std::vector<std::string> WorkflowManager::getActionSummaries(const std::string& name) const {
    for (const auto& wf : workflows_) {
        if (wf && wf->getName() == name) {
            std::vector<std::string> sums;
            for (const auto& act : wf->getActions()) {
                // act.type and act.params assumed to be std::string
                sums.push_back(act.type + ": " + act.params);
            }
            return sums;
        }
    }
    return {};
}
