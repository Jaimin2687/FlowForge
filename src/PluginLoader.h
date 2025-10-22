#pragma once
#include "IAction.h"
#include <memory>
#include <string>
class PluginLoader {
public:
    std::unique_ptr<IAction> load(const std::string& pluginPath);
};
