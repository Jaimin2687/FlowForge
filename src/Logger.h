#pragma once
#include <string>
#include <mutex>
#include <fstream>
class Logger {
public:
    static Logger& instance();
    void log(const std::string& msg);
private:
    Logger();
    std::mutex mtx_;
    std::ofstream file_;
};
