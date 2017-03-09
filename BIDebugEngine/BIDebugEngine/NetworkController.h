#pragma once
#include "NamedPipeServer.h"
#include <thread>

class NetworkController {
public:
    NetworkController();
    ~NetworkController();

    void incomingMessage(const std::string& message);
private:
    NamedPipeServer server;
    std::thread pipeThread;
};

