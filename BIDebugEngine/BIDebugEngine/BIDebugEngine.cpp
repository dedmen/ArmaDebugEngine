//BI Debug Engine Interface


#include "BIDebugEngine.h"
#include <windows.h>
#include "EngineHook.h"
static DllInterface dllIface;
static EngineInterface* engineIface;
extern EngineHook GlobalEngineHook;
uintptr_t engineAlloc;

void DllInterface::Startup() {  
    GlobalEngineHook.placeHooks();    
}

void DllInterface::Shutdown() {
    Sleep(10);
}

void DllInterface::ScriptLoaded(IDebugScript *script, const char *name) {

}

void DllInterface::ScriptEntered(IDebugScript *script) {

}

void DllInterface::ScriptTerminated(IDebugScript *script) {

}

void DllInterface::FireBreakpoint(IDebugScript *script, unsigned int bp) {

}

void DllInterface::Breaked(IDebugScript *script) {

}

void DllInterface::DebugEngineLog(const char *str) {

}

DllInterface* Connect(EngineInterface *engineInt) {
    while (!IsDebuggerPresent()) Sleep(1);
    engineIface = engineInt;
    return &dllIface;
}
char VarBuffer[1024];
void IDebugScope::printAllVariables() {
    

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
        value->getType(VarBuffer, 1023);
        std::string valueType(VarBuffer);
        value->getValue(10, VarBuffer, 1023);
        OutputDebugStringA((name + " " + valueType + ": " + std::string(VarBuffer) + "\n").c_str());
    }
}

std::string IDebugScope::allVariablesToString() {
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
        value->getType(VarBuffer, 1023);
        std::string valueType(VarBuffer);
        if(valueType == "code" || valueType == "Array") {
            output += (name + " : " + valueType + "; ");
            continue;
        }
        value->getValue(10, VarBuffer, 1023);
        output += (name + "=" + std::string(VarBuffer) + " : " + valueType + "; ");
    }
    return output;
}

void RV_ScriptVM::debugPrint(std::string prefix) {
    std::string title = _displayName;
    std::string filename = _doc._fileName.isNull() ? _docpos._sourceFile : _doc._fileName;
    std::string data;// = _doc._content.data();
    OutputDebugStringA((prefix + " " + title + "F " + filename +" "+ data + "\n").c_str());
}
