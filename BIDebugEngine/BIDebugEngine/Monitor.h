#pragma once
#include <cstdint>
#include <map>
#include "RVClasses.h"
#include <unordered_set>
class Debugger;
struct DebuggerInstructionInfo;
//Classes that hook into events and.. do stuff.

class IMonitorBase {
public:
    virtual ~IMonitorBase(){}
    virtual void onInstruction(Debugger*, const DebuggerInstructionInfo&) = 0;
    virtual void onShutdown() = 0;
};

class Monitor_knownScriptFiles {
public:
    virtual ~Monitor_knownScriptFiles() {}
    virtual void onInstruction(Debugger*, const DebuggerInstructionInfo&);
    virtual void onShutdown();
private:
    std::map<RString, std::unordered_set<uint32_t>> scriptLines;
};


class Monitor {
public:
    Monitor();
    ~Monitor();
};

