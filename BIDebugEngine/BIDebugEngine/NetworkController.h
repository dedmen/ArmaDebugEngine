#pragma once
#include "NamedPipeServer.h"
#include <thread>

enum class NC_CommandType {
    invalid,
    addBreakpoint,
    delBreakpoint,
    BPContinue,//Tell's breakpoint to leave breakState
    MonitorDump, //Dump's all Monitors
    setHookEnable,
    getVariable
};

enum class NC_OutgoingCommandType {
    invalid,
    BreakpointHalt,
    ContinueExecution
};

class NetworkController {
public:
    NetworkController();
    ~NetworkController();
    void init();
    void incomingMessage(const std::string& message);
    void sendMessage(const std::string& message);
private:
    NamedPipeServer server;
    std::thread* pipeThread{ nullptr };
};

