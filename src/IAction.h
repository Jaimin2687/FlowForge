#pragma once
#include <string>
class IAction {
public:
    virtual void execute(const std::string& params) = 0;
    virtual ~IAction() = default;
};
