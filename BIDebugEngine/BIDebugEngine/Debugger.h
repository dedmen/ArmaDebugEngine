#pragma once
#include <cstdint>
#include <map>
#include <memory>
#include "RVClasses.h"
#include "BreakPoint.h"
#include "Monitor.h"
#include "NetworkController.h"
struct RV_VMContext;
class Script;
class VMContext;

struct DebuggerInstructionInfo {
    RV_GameInstruction* instruction;
    RV_VMContext* context;
    GameState* gs;
};
                   
extern scriptExecutionContext currentContext;
extern MissionEventType currentEventHandler;
 
enum class DebuggerState {
    Uninitialized,
    running,
    breakState
};


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
    void onStartup();
    void onHalt(HANDLE waitEvent, BreakPoint* bp, const DebuggerInstructionInfo& info); //Breakpoint is halting engine
    void onContinue(); //Breakpoint has stopped halting
    void commandContinue(); //Tells Breakpoint in breakState to Stop halting
    std::map<uintptr_t, std::shared_ptr<VMContext>> VMPtrToScript;
    std::map<std::string, std::vector<BreakPoint>> breakPoints;

    std::vector<std::shared_ptr<IMonitorBase>> monitors;
    NetworkController nController;
    HANDLE breakStateContinueEvent{ 0 };
    DebuggerState state;
};

