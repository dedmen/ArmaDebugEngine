#include "NetworkController.h"
#include "json.hpp"
#include <thread>


NetworkController::NetworkController() {
    server.messageRead.connect([this](const std::string& msg) {incomingMessage(msg); });
    pipeThread = std::move(std::thread([this]() {
        server.open();

        while (true) {
            auto msg = server.readMessageBlocking();
            if (!msg.empty())
                server.messageRead(msg);
            //pServer->writeMessage(/*msg+*/"teeest");
        }

    }));
}


NetworkController::~NetworkController() {}

void NetworkController::incomingMessage(const std::string& message) {

}
