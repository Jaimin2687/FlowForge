#include "PluginLoader.h"
#include <dlfcn.h>
#include <stdexcept>
#include <vector>
#include <string>
using namespace std;
typedef IAction* (*CreateActionFunc)();
unique_ptr<IAction> PluginLoader::load(const string& pluginPath) {
    // Try a variety of likely paths so plugins can be found whether
    // running from repo root or build directory, and handle lib prefix/suffix variations.
    vector<string> candidates;
    candidates.push_back(pluginPath); // as given (e.g. plugins/MessageAction.so)
    candidates.push_back("./" + pluginPath);
    candidates.push_back("../" + pluginPath);
    candidates.push_back("./plugins/" + pluginPath.substr(pluginPath.find_last_of('/')+1));
    candidates.push_back("../plugins/" + pluginPath.substr(pluginPath.find_last_of('/')+1));

    // Also try lib-prefixed and .dylib variants (macOS)
    string base = pluginPath.substr(pluginPath.find_last_of('/')+1);
    if (base.rfind("lib", 0) != 0) {
        candidates.push_back("plugins/lib" + base);
        candidates.push_back("./plugins/lib" + base);
        candidates.push_back("../plugins/lib" + base);
    }
    // Try .dylib if .so didn't work
    if (base.size() > 3 && base.substr(base.size()-3) == ".so") {
        string without = base.substr(0, base.size()-3);
        candidates.push_back("plugins/" + without + ".dylib");
        candidates.push_back("./plugins/" + without + ".dylib");
        candidates.push_back("../plugins/" + without + ".dylib");
    }

    string lastErrors;
    for (const auto &cand : candidates) {
        void* handle = dlopen(cand.c_str(), RTLD_LAZY);
        if (!handle) {
            const char* err = dlerror();
            if (err) {
                lastErrors += string(cand) + ": " + err + "\n";
            }
            continue;
        }
        auto create = (CreateActionFunc)dlsym(handle, "create_action");
        if (!create) {
            const char* err = dlerror();
            dlclose(handle);
            if (err) {
                lastErrors += string(cand) + ": " + err + "\n";
            }
            continue;
        }
        return unique_ptr<IAction>(create());
    }

    throw runtime_error(string("Failed to load plugin '") + pluginPath + "'. Tried candidates:\n" + lastErrors);
}
