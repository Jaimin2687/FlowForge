#include "WorkflowManager.h"
#include "Logger.h"
#include "PathUtils.h"
#include <iostream>
#include <string>
#include <limits>
#include <filesystem>
#include <thread>
#include "Storage.h"
#include "utils/json.hpp"
#include <algorithm>
#include <vector>
#include <cctype>
#include <curl/curl.h> // Add curl header for global init

using namespace std;

static void printHeader() {
    cout << "\nFlowForge Workflow Engine\n";
    cout << "--------------------------\n";
}

// ANSI color helpers
namespace color {
    const char* RESET = "\033[0m";
    const char* RED = "\033[31m";
    const char* GREEN = "\033[32m";
    const char* YELLOW = "\033[33m";
}

static vector<int> parseSelectionList(const string &s, size_t max) {
    vector<int> result;
    stringstream ss(s);
    string item;
    while(getline(ss, item, ',')) {
        try {
            int num = stoi(item);
            if(num > 0 && num <= static_cast<int>(max)) {
                result.push_back(num - 1);
            }
        } catch(...) {}
    }
    return result;
}

// Helper function to ensure data directories exist
static void ensureDirectoriesExist() {
    namespace fs = std::filesystem;
    try {
        fs::create_directories("data/backups");
        fs::create_directories("data/uploads");
        fs::create_directories("logs");
    } catch (const std::exception& e) {
        cerr << "Warning: Could not create directories: " << e.what() << endl;
    }
}

int main(int argc, char* argv[]) {
    // Initialize libcurl globally (prevent segfaults in MessageAction)
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // Ensure required directories exist
    ensureDirectoriesExist();
    
    // Check for command-line arguments to run a workflow directly
    if (argc >= 3 && (string(argv[1]) == "run" || string(argv[1]) == "r")) {
        string workflowName = argv[2];
        WorkflowManager manager;
        // Locate config/workflows.json from several likely locations so running from build/ works
        namespace fs = std::filesystem;
        std::vector<std::string> candidates = {
            "config/workflows.json",
            "../config/workflows.json",
            "./config/workflows.json",
            "../../config/workflows.json"
        };
        std::string usedConfig;
        for (const auto &c : candidates) {
            if (fs::exists(c)) { usedConfig = c; break; }
        }
        if (usedConfig.empty()) {
            cout << "No workflow config found (tried config/workflows.json and parent dirs).\n";
            cout << "Please run from repository root or place config/workflows.json accordingly.\n";
            curl_global_cleanup();
            return 1;
        }
        cout << "Using workflow config: " << usedConfig << "\n";
        manager.loadWorkflows(usedConfig);

        auto names = manager.listWorkflowNames();
        if (find(names.begin(), names.end(), workflowName) == names.end()) {
            cout << "Workflow '" << workflowName << "' not found.\n";
            curl_global_cleanup();
            return 1;
        }

        cout << "Running workflow: " << workflowName << "\n";
        manager.startWorkflow(workflowName);
        Logger::instance().log("Engine exited after running workflow: " + workflowName);
        curl_global_cleanup();
        return 0;
    }

    printHeader();

    WorkflowManager manager;
    // Locate config/workflows.json from several likely locations so running from build/ works
    namespace fs = std::filesystem;
    std::vector<std::string> candidates = {
        "config/workflows.json",
        "../config/workflows.json",
        "./config/workflows.json",
        "../../config/workflows.json"
    };
    std::string usedConfig;
    for (const auto &c : candidates) {
        if (fs::exists(c)) { usedConfig = c; break; }
    }
    if (usedConfig.empty()) {
        cout << "No workflow config found (tried config/workflows.json and parent dirs).\n";
        cout << "Please run from repository root or place config/workflows.json accordingly.\n";
        curl_global_cleanup();
        return 0;
    }
    cout << "Using workflow config: " << usedConfig << "\n";
    manager.loadWorkflows(usedConfig);

    // Main menu loop
    bool running = true;
    while(running) {
        cout << "\nOptions:\n";
        cout << "1. List available workflows\n";
        cout << "2. Run a workflow\n";
        cout << "3. Run a workflow with parameter overrides\n";
        cout << "4. Exit\n";
        cout << "Enter choice: ";

        int choice;
        cin >> choice;
        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number between 1 and 4.\n";
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');  // Clear input buffer
        
        switch(choice) {
            case 1: {
                auto names = manager.listWorkflowNames();
                cout << "\nAvailable workflows:\n";
                for(size_t i = 0; i < names.size(); ++i) {
                    cout << (i+1) << ". " << names[i] << "\n";
                }
                break;
            }
            case 2: {
                auto names = manager.listWorkflowNames();
                cout << "\nAvailable workflows:\n";
                for(size_t i = 0; i < names.size(); ++i) {
                    cout << (i+1) << ". " << names[i] << "\n";
                }
                
                cout << "Enter workflow number(s) (comma-separated for multiple): ";
                string input;
                getline(cin, input);
                
                auto selected = parseSelectionList(input, names.size());
                if(selected.empty()) {
                    cout << "No valid workflow selected.\n";
                    break;
                }
                
                for(auto idx : selected) {
                    cout << "Running workflow: " << names[idx] << "\n";
                    manager.startWorkflow(names[idx]);
                }
                break;
            }
            case 3: {
                auto names = manager.listWorkflowNames();
                cout << "\nAvailable workflows:\n";
                for(size_t i = 0; i < names.size(); ++i) {
                    cout << (i+1) << ". " << names[i] << "\n";
                }
                
                cout << "Enter workflow number: ";
                string input;
                getline(cin, input);
                
                try {
                    int idx = stoi(input) - 1;
                    if(idx < 0 || idx >= static_cast<int>(names.size())) {
                        cout << "Invalid workflow number.\n";
                        break;
                    }
                    
                    string workflowName = names[idx];
                    auto actionSummaries = manager.getActionSummaries(workflowName);
                    
                    cout << "\nWorkflow '" << workflowName << "' has " << actionSummaries.size() << " actions.\n";
                    for(size_t i = 0; i < actionSummaries.size(); ++i) {
                        cout << (i+1) << ". " << actionSummaries[i] << "\n";
                    }
                    
                    vector<string> overrides;
                    for(size_t i = 0; i < actionSummaries.size(); ++i) {
                        cout << "Enter override for action " << (i+1) << " (leave blank to use default): ";
                        string override;
                        getline(cin, override);
                        overrides.push_back(override);
                    }
                    
                    cout << "Running workflow: " << workflowName << " with overrides\n";
                    manager.startWorkflowWithOverrides(workflowName, overrides);
                    
                } catch(const exception& e) {
                    cout << "Error: " << e.what() << "\n";
                }
                break;
            }
            case 4:
                running = false;
                break;
            default:
                cout << "Invalid choice. Please enter one of the listed options.\n";
        }
    }

    Logger::instance().log("Engine exited.");
    curl_global_cleanup(); // Clean up curl on exit
    return 0;
}
