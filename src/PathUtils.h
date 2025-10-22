#pragma once
#include <string>
#include <filesystem>
#include <cstdlib>
#include <pwd.h>
#include <unistd.h>

namespace PathUtils {
    // Expand leading ~ to the user's home directory, then return absolute normalized path
    inline std::string expandAndNormalizePath(const std::string &input) {
        if (input.empty()) return input;
        std::string p = input;
        if (p[0] == '~') {
            const char* home = std::getenv("HOME");
            if (!home) {
                struct passwd *pw = getpwuid(getuid());
                if (pw) home = pw->pw_dir;
            }
            if (home) {
                if (p.size() == 1) p = std::string(home);
                else if (p[1] == '/') p = std::string(home) + p.substr(1);
            }
        }
        try {
            std::filesystem::path fp = std::filesystem::u8path(p);
            fp = std::filesystem::absolute(fp);
            fp = fp.lexically_normal();
            return fp.u8string();
        } catch (...) {
            return p;
        }
    }
}
