#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include "RVClasses.h"
#include "BreakPoint.h"
#include "Monitor.h"
struct RV_VMContext;
class Script;
class VMContext;

struct DebuggerInstructionInfo {
    RV_GameInstruction* instruction;
    RV_VMContext* context;
};
                   
extern scriptExecutionContext currentContext;
extern MissionEventType currentEventHandler;
       
class Debugger {
public:
    Debugger();
    ~Debugger();


    void clear();
    std::shared_ptr<VMContext> getVMContext(RV_VMContext* vm);
    void writeFrameToFile(uint32_t frameCounter);
    void onInstruction(DebuggerInstructionInfo& instructionInfo);//#TODO add gameState
    void checkForBreakpoint(DebuggerInstructionInfo& instructionInfo);
    void onShutdown();
    std::map<uintptr_t, std::shared_ptr<VMContext>> VMPtrToScript;
    std::map<std::string, std::vector<BreakPoint>> breakPoints;

    std::vector<std::shared_ptr<IMonitorBase>> monitors;
};

