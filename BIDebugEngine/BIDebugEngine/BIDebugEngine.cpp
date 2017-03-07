//BI Debug Engine Interface


#include "BIDebugEngine.h"
#include <windows.h>
#include "EngineHook.h"
static DllInterface dllIface;
static EngineInterface* engineIface;
extern EngineHook GlobalEngineHook;
uintptr_t engineAlloc;
#include <Psapi.h>
#pragma comment (lib, "Psapi.lib")//GetModuleInformation
void DllInterface::Startup() {
    GlobalEngineHook.placeHooks();



    //Get engine allocator - From my Intercept fork
    //Find the allocator base
    MODULEINFO modInfo = { 0 };
    HMODULE hModule = GetModuleHandleA(nullptr);
    GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));

    auto findInMemory = [&modInfo](char* pattern, size_t patternLength) ->uintptr_t {
        uintptr_t base = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
        uintptr_t size = static_cast<uintptr_t>(modInfo.SizeOfImage);
        for (DWORD i = 0; i < size - patternLength; i++) {
            bool found = true;
            for (DWORD j = 0; j < patternLength; j++) {
                found &= pattern[j] == *reinterpret_cast<char*>(base + i + j);
                if (!found)
                    break;
            }
            if (found)
                return base + i;
        }
        return 0;
    };

    auto getRTTIName = [](uintptr_t vtable) -> const char* {
        uintptr_t typeBase = *((uintptr_t*) (vtable - 4));
        uintptr_t type = *((uintptr_t*) (typeBase + 0xC));
        return (char*) (type + 9);
    };



    uintptr_t stringOffset = findInMemory("tbb4malloc_bi", 13);

    uintptr_t allocatorVtablePtr = (findInMemory((char*) &stringOffset, 4) - 4);
    const char* test = getRTTIName(*reinterpret_cast<uintptr_t*>(allocatorVtablePtr));
    engineAlloc = allocatorVtablePtr;
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
