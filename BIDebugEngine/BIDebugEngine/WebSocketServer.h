#pragma once

#include <string>
#include <array>
#include "SignalSlot.h"

class WebSocketServer {
public:
    WebSocketServer();
    ~WebSocketServer();
    void open();
    void close();
    void writeMessage(std::string message);
    Signal<void(std::string)> messageRead;
    Signal<void(bool)> onClientConnectedStateChanged;
};

