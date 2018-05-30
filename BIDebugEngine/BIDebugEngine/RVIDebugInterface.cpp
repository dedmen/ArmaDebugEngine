#include "RVIDebugInterface.h"
#include <vector>
#include <windows.h>

char VarBuffer[1024];
void printAllVariables(const intercept::types::vm_context::IDebugScope& s) {


    auto varC = s.varCount();
    std::vector<IDebugVariable*> vars;
    vars.resize(varC);
    IDebugVariable** vbase = vars.data();
    s.getVariables(const_cast<const IDebugVariable**>(vbase), varC);

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

std::string allVariablesToString(const intercept::types::vm_context::IDebugScope& s) {
    std::string output;
    auto varC = s.varCount();
    std::vector<IDebugVariable*> vars;
    vars.resize(varC);
    IDebugVariable** vbase = vars.data();
    s.getVariables(const_cast<const IDebugVariable**>(vbase), varC);

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
