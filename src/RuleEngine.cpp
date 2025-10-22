#include "RuleEngine.h"
#include <sys/statvfs.h>
#include <unistd.h>
#include <ctime>
#include <regex>
#include <iostream>
#include <mach/mach.h>  // For macOS memory info

using namespace std;

bool RuleEngine::evaluate(const nlohmann::json& rule) {
    if (!rule.contains("if")) return true;
    
    return evaluateCondition(rule["if"]);
}

bool RuleEngine::evaluateCondition(const nlohmann::json& condition) {
    if (condition.is_string()) {
        // Simple string condition (backward compatibility)
        return evaluateSimpleCondition(condition);
    } else if (condition.is_object()) {
        // Complex condition object
        if (condition.contains("and")) {
            for (const auto& sub : condition["and"]) {
                if (!evaluateCondition(sub)) return false;
            }
            return true;
        } else if (condition.contains("or")) {
            for (const auto& sub : condition["or"]) {
                if (evaluateCondition(sub)) return true;
            }
            return false;
        } else if (condition.contains("not")) {
            return !evaluateCondition(condition["not"]);
        } else {
            // Single condition object
            return evaluateSingleCondition(condition);
        }
    }
    return true;
}

bool RuleEngine::evaluateSingleCondition(const nlohmann::json& cond) {
    if (cond.contains("disk")) {
        return evaluateDiskCondition(cond["disk"]);
    } else if (cond.contains("cpu")) {
        return evaluateCpuCondition(cond["cpu"]);
    } else if (cond.contains("memory")) {
        return evaluateMemoryCondition(cond["memory"]);
    } else if (cond.contains("file")) {
        return evaluateFileCondition(cond["file"]);
    } else if (cond.contains("time")) {
        return evaluateTimeCondition(cond["time"]);
    }
    return true;
}

bool RuleEngine::evaluateSimpleCondition(const string& cond) {
    if (cond.find("disk") != string::npos) {
        struct statvfs stat;
        if (statvfs(".", &stat) != 0) return false;
        double used = 1.0 - (double)stat.f_bavail / stat.f_blocks;
        regex pattern(R"(disk\s*>\s*(\d+)%?)");
        smatch match;
        if (regex_search(cond, match, pattern)) {
            int percent = stoi(match[1]);
            return (used * 100) > percent;
        }
    }
    return true;
}

bool RuleEngine::evaluateDiskCondition(const string& cond) {
    struct statvfs stat;
    if (statvfs(".", &stat) != 0) return false;
    double used = 1.0 - (double)stat.f_bavail / stat.f_blocks;
    
    regex pattern(R"(>\s*(\d+)%?)");
    smatch match;
    if (regex_search(cond, match, pattern)) {
        int percent = stoi(match[1]);
        return (used * 100) > percent;
    }
    return false;
}

bool RuleEngine::evaluateCpuCondition(const string& cond) {
    // Note: This is a simplified CPU usage check
    // In a real implementation, you'd use system calls or libraries like sys/stat.h
    // For demonstration, we'll return true if condition is "> 50%"
    regex pattern(R"(>\s*(\d+)%?)");
    smatch match;
    if (regex_search(cond, match, pattern)) {
        int threshold = stoi(match[1]);
        // Placeholder: In real code, calculate actual CPU usage
        double cpuUsage = 60.0; // Simulated
        return cpuUsage > threshold;
    }
    return false;
}

bool RuleEngine::evaluateMemoryCondition(const string& cond) {
    // macOS memory info using mach
    mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
    vm_statistics_data_t vmstat;
    if (host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmstat, &count) != KERN_SUCCESS) {
        return false;
    }

    double used = 1.0 - (double)vmstat.free_count / (vmstat.free_count + vmstat.active_count + vmstat.inactive_count + vmstat.wire_count);

    regex pattern(R"(>\s*(\d+)%?)");
    smatch match;
    if (regex_search(cond, match, pattern)) {
        int percent = stoi(match[1]);
        return (used * 100) > percent;
    }
    return false;
}

bool RuleEngine::evaluateFileCondition(const string& cond) {
    // Example: "exists /path/to/file"
    if (cond.find("exists") == 0) {
        string path = cond.substr(7);
        return access(path.c_str(), F_OK) == 0;
    }
    return false;
}

bool RuleEngine::evaluateTimeCondition(const string& cond) {
    // Example: "between 09:00 and 17:00"
    time_t now = time(nullptr);
    tm* local = localtime(&now);
    int currentHour = local->tm_hour;
    
    regex pattern(R"(between\s+(\d+):(\d+)\s+and\s+(\d+):(\d+))");
    smatch match;
    if (regex_search(cond, match, pattern)) {
        int startHour = stoi(match[1]);
        int endHour = stoi(match[3]);
        return currentHour >= startHour && currentHour <= endHour;
    }
    return false;
}
