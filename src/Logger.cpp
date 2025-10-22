#include "Logger.h"
#include <iostream>
#include <ctime>
#include <filesystem>
#include <cstdlib>
using namespace std;
Logger::Logger() {
    namespace fs = std::filesystem;
    const char* env = std::getenv("FLOWFORGE_LOG_DIR");
    fs::path logdir = env ? fs::path(env) : fs::path("logs");
    try {
        fs::create_directories(logdir);
    } catch(...) {
        // ignore - we'll attempt to open file and let ofstream report errors
    }
    fs::path logfile = logdir / "engine.log";
    file_.open(logfile.string(), ios::app);
}
Logger& Logger::instance() {
    static Logger inst;
    return inst;
}
void Logger::log(const string& msg) {
    lock_guard<mutex> lock(mtx_);
    time_t now = time(nullptr);
    file_ << std::ctime(&now) << ": " << msg << endl;
    file_.flush();
    cout << msg << endl;
}
