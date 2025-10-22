#pragma once
#include "Workflow.h"
#include <vector>
#include <memory>
#include <string>
class WorkflowManager {
public:
    void loadWorkflows(const std::string& configPath);
    void startAll();
    void startWorkflow(const std::string& name);
    void startWorkflowWithOverrides(const std::string& name, const std::vector<std::string>& overrides);
    std::vector<std::string> listWorkflowNames() const;
    // Return action descriptions (type + params) for a workflow by name
    std::vector<std::string> getActionSummaries(const std::string& name) const;
private:
    std::vector<std::unique_ptr<Workflow>> workflows_;
};
