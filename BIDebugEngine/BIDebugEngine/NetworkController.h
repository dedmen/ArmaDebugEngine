#pragma once
#include "NamedPipeServer.h"
#include <thread>

enum class NC_CommandType {
    


};

class NetworkController {
public:
    NetworkController();
    ~NetworkController();

    void incomingMessage(const std::string& message);
private:
    NamedPipeServer server;
    std::thread pipeThread;
};

