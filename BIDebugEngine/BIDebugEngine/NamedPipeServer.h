#pragma once

#include <string>
#include <array>
#include "SignalSlot.h"
typedef void *HANDLE;

class NamedPipeServer {
public:
    NamedPipeServer();
    ~NamedPipeServer();
    void open();
    void close();
    void writeMessage(std::string message);
    std::string readMessageBlocking();
    Signal<void(std::string)> messageRead;                             
private:
    void transactMessage(char *output, int outputSize, const char *input); //don't use
    void queueRead(); //Failed attempt. Don't call
    void openPipe();
    void closePipe();
    
    HANDLE pipe = nullptr;
    HANDLE waitForDataEvent = nullptr;
    std::array<char,4096> recvBuffer;
};

