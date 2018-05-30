#include "RVIDebugInterface.h"
#include <vector>

char VarBuffer[1024];
void intercept::types::vm_context::IDebugScope::printAllVariables() {


    auto varC = varCount();
    std::vector<IDebugVariable*> vars;
    vars.resize(varC);
    IDebugVariable** vbase = vars.data();
    getVariables(const_cast<const IDebugVariable**>(vbase), varC);

    for (auto& var : vars) {
        var->getName(VarBuffer, 1023);
        std::string name(VarBuffer);
        auto value = var->getValue();
        if (!value) continue;
        value->getTypeStr(VarBuffer, 1023);
        std::string valueType(VarBuffer);
        value->getValue(10, VarBuffer, 1023);
        OutputDebugStringA((name + " " + valueType + ": " + std::string(VarBuffer) + "\n").c_str());
    }
}

std::string intercept::types::vm_context::IDebugScope::allVariablesToString() {
    std::string output;
    auto varC = varCount();
    std::vector<IDebugVariable*> vars;
    vars.resize(varC);
    IDebugVariable** vbase = vars.data();
    getVariables(const_cast<const IDebugVariable**>(vbase), varC);

    for (auto& var : vars) {
        var->getName(VarBuffer, 1023);
        std::string name(VarBuffer);
        auto value = var->getValue();
        if (!value) continue;
        value->getTypeStr(VarBuffer, 1023);
        std::string valueType(VarBuffer);
        if (valueType == "code" || valueType == "Array") {
            output += (name + " : " + valueType + "; ");
            continue;
        }
        value->getValue(10, VarBuffer, 1023);
        output += (name + "=" + std::string(VarBuffer) + " : " + valueType + "; ");
    }
    return output;
}
