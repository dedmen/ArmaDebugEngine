#pragma once
#include "WebSocketServer.h"
#include <thread>

enum class NC_CommandType {
    invalid,
    getVersionInfo,
    addBreakpoint,
    delBreakpoint,
    BPContinue,//Tell's breakpoint to leave breakState
    MonitorDump, //Dump's all Monitors
    setHookEnable,
    getVariable,
    getCurrentCode, //While in breakState returns full preproced code of last Instructions script file
    getAllScriptCommands,
    getAvailableVariables,
    haltNow, //Triggers halt on next possible instruction
    ExecuteCode, //Executes code while halted, in current context and returns result
    LoadFile, //literally runs loadFile command and returns result
    clearAllBreakpoints,
    clearFileBreakpoints,
    SetExceptionFilter = 16
};

enum class NC_OutgoingCommandType {
    invalid,
    versionInfo,
    halt_breakpoint,
    halt_step,
    halt_error,
    halt_scriptAssert,
    halt_scriptHalt,
    halt_placeholder,
    ContinueExecution,
    VariableReturn, //returning from getVariable
    AvailableVariablesReturn, //returning from getAvailableVariables
    BreakpointLog, //A log breakpoint was triggered
    LogMessage, //A log message from the game, for example from echo script command
    ExecuteCodeResult, //Result of ExecuteCode command
    LoadFileResult //Result of LoadFile command
};

class NetworkController {
public:
    NetworkController();
    ~NetworkController();
    void init();
    void incomingMessage(const std::string& message);
    void sendMessage(const std::string& message);
    bool isClientConnected() { return clientConnected; }

    void onShutdown();
private:
    WebSocketServer server;
    bool pipeThreadShouldRun{ true };
    std::thread* pipeThread{ nullptr };
    bool clientConnected{ false };
};

