#include "NetworkController.h"
#include "json.hpp"
#include <thread>
#include "Serialize.h"
#include "BreakPoint.h"
#include "Debugger.h"


using nlohmann::json;
extern Debugger GlobalDebugger;
NetworkController::NetworkController() {

}


NetworkController::~NetworkController() {
    if (pipeThread) {
        pipeThread->join();
        delete pipeThread;
    }


}

void NetworkController::init() {
    //server.messageRead.connect([this](const std::string& msg) {incomingMessage(msg); });
    pipeThread = new std::thread([this]() {
        server.open();

        while (true) {//#TODO make exitable :u add onShutdown thingy
            auto msg = server.readMessageBlocking();

            if (!msg.empty())
                incomingMessage(msg);
            //server.messageRead(msg);
            //pServer->writeMessage(/*msg+*/"teeest");
        }

    });
}
extern uintptr_t hookEnabled_Instruction;
extern uintptr_t hookEnabled_Simulate;
void NetworkController::incomingMessage(const std::string& message) {

    try {
        auto packet = json::parse(message.c_str());
        NC_CommandType command = static_cast<NC_CommandType>(packet.value<int>("command", 0));
        command;

        switch (command) {

            case NC_CommandType::invalid: break;
            case NC_CommandType::addBreakpoint: {
                JsonArchive ar(packet["data"]);
                BreakPoint bp;
                bp.Serialize(ar);
                bp.filename.lower();
                auto &bpVec = GlobalDebugger.breakPoints.get(bp.filename);
                if (!GlobalDebugger.breakPoints.isNull(bpVec)) {
                    auto vecFound = std::find_if(bpVec.begin(), bpVec.end(), [lineNumber = bp.line](const BreakPoint& bp) {
                        return lineNumber == bp.line;
                    });
                    if (vecFound != bpVec.end()) {
                        //Breakpoint already exists. Delete old one
                        bpVec.erase(vecFound);
                    }
                    bpVec.push_back(std::move(bp));
                } else {
                    GlobalDebugger.breakPoints.insert(std::move(bp));
                }
            } break;
            case NC_CommandType::delBreakpoint: {
                JsonArchive ar(packet["data"]);
                uint16_t lineNumber;
                std::string fileName;
                ar.Serialize("line", lineNumber);
                ar.Serialize("filename", fileName);
                std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);
                auto &found = GlobalDebugger.breakPoints.get(fileName.c_str());
                if (!GlobalDebugger.breakPoints.isNull(found)) {
                    auto vecFound = std::find_if(found.begin(), found.end(), [lineNumber](const BreakPoint& bp) {
                        return lineNumber == bp.line;
                    });
                    if (vecFound != found.end()) {
                        found.erase(vecFound);
                    }
                    if (found.empty())
                        GlobalDebugger.breakPoints.remove(fileName.c_str());
                }
            } break;
            case NC_CommandType::BPContinue: {
                GlobalDebugger.commandContinue();
            } break;

            default: break;
            case NC_CommandType::MonitorDump: for (auto& it : GlobalDebugger.monitors) it->dump(); break;
            case NC_CommandType::setHookEnable: {
                hookEnabled_Simulate = packet.value<int>("state", 1);
                hookEnabled_Instruction = hookEnabled_Instruction;
            } break;
            case NC_CommandType::getVariable: {

                JsonArchive ar(packet["data"]);
                uint16_t scope;
                std::string variableName;
                ar.Serialize("scope", scope);
                ar.Serialize("name", variableName);
                auto var = GlobalDebugger.getVariable(static_cast<VariableScope>(scope), variableName);

                JsonArchive answer;

                if (!var) {
                    answer.Serialize("type", "nil");
                } else {
                    answer.Serialize("name", var->_name);
                    var->_value.Serialize(answer);
                }
                sendMessage(answer.to_string());
            } break;
        }
    }
    catch (std::exception &ex) {
        JsonArchive ar;
        ar.Serialize("exception", std::string(ex.what()));
        server.writeMessage(ar.to_string());
    }
}

void NetworkController::sendMessage(const std::string& message) {
    server.writeMessage(message);
}
