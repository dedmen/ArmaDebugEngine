#include "NetworkController.h"
#include "json.hpp"
#include <thread>

using nlohmann::json;

NetworkController::NetworkController() {
    //server.messageRead.connect([this](const std::string& msg) {incomingMessage(msg); });
    pipeThread = std::move(std::thread([this]() {
        server.open();

        while (true) {
            auto msg = server.readMessageBlocking();
           
            if (!msg.empty())
                incomingMessage(msg);
                //server.messageRead(msg);
            //pServer->writeMessage(/*msg+*/"teeest");
        }

    }));
}


NetworkController::~NetworkController() {}

void NetworkController::incomingMessage(const std::string& message) {


    auto packet = json::parse(message.c_str());
    NC_CommandType command = static_cast<NC_CommandType>(packet["command"].get<int>());

}
