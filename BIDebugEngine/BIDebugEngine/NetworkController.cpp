#include "NetworkController.h"
#include "json.hpp"
#include <thread>
#include "Serialize.h"
#include "BreakPoint.h"
#include "Debugger.h"
#include "version.h"
#include <client.hpp>
#include <core.hpp>


using nlohmann::json;
extern Debugger GlobalDebugger;
NetworkController::NetworkController() {

}


NetworkController::~NetworkController() {
    if (pipeThread) {
        pipeThreadShouldRun = false;
        if (pipeThread->joinable())
            pipeThread->join();
        delete pipeThread;
    }
}

void NetworkController::init() {
    //server.messageRead.connect([this](const std::string& msg) {incomingMessage(msg); });
    pipeThread = new std::thread([this]() {

        server.onClientConnectedStateChanged.connect([this](bool state) {
            clientConnected = state;
            if (!clientConnected) {
                GlobalDebugger.commandContinue(StepType::STContinue);
            }
        });
        server.messageRead.connect([this](std::string message) { incomingMessage(message); });

        server.open();
    });
}
extern "C" uintptr_t hookEnabled_Instruction;
extern "C" uintptr_t hookEnabled_Simulate;
void NetworkController::incomingMessage(const std::string& message) {

    try {
        auto packet = json::parse(message.c_str());
        NC_CommandType command = static_cast<NC_CommandType>(packet.value<int>("command", 0));

        switch (command) {

            case NC_CommandType::invalid: break;
            case NC_CommandType::addBreakpoint: {
                JsonArchive ar(packet["data"]);
                BreakPoint bp;
                bp.Serialize(ar);
                bp.filename.to_lower();
                std::unique_lock<std::shared_mutex> lk(GlobalDebugger.breakPointsLock);
                //auto &bpVec = GlobalDebugger.breakPoints.get(bp.filename.c_str());
                //if (!GlobalDebugger.breakPoints.is_null(bpVec)) {
                auto foundItr = GlobalDebugger.breakPoints.find(bp.filename);
                if (foundItr != GlobalDebugger.breakPoints.end()) {
                    auto& bpVec = foundItr->second;
                    auto vecFound = std::find_if(bpVec.begin(), bpVec.end(), [lineNumber = bp.line](const BreakPoint& bp) {
                        return lineNumber == bp.line;
                    });
                    if (vecFound != bpVec.end()) {
                        //Breakpoint already exists. Delete old one
                        bpVec.erase(vecFound);
                    }
                    bpVec.push_back(std::move(bp));
                } else {
                    // GlobalDebugger.breakPoints.insert(Debugger::breakPointList(std::move(bp)));
                    GlobalDebugger.breakPoints.emplace(bp.filename, std::move(bp));
                }
            } break;
            case NC_CommandType::clearAllBreakpoints: {
                std::unique_lock<std::shared_mutex> lk(GlobalDebugger.breakPointsLock);
                GlobalDebugger.breakPoints.clear();
            }   break;
            case NC_CommandType::clearFileBreakpoints: { //clearFileBreakpoints
                JsonArchive ar(packet["data"]);
                r_string filename;
                ar.Serialize("filename", filename);
                std::unique_lock<std::shared_mutex> lk(GlobalDebugger.breakPointsLock);
                GlobalDebugger.breakPoints.erase(filename);
            }   break;
            case NC_CommandType::delBreakpoint: {
                JsonArchive ar(packet["data"]);
                uint16_t lineNumber;
                r_string fileName;
                ar.Serialize("line", lineNumber);
                ar.Serialize("filename", fileName);
                fileName.to_lower();
                //std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);
                std::unique_lock<std::shared_mutex> lk(GlobalDebugger.breakPointsLock);
                //auto &found = GlobalDebugger.breakPoints.get(fileName.c_str());
                //if (!GlobalDebugger.breakPoints.is_null(found)) {
                auto foundItr = GlobalDebugger.breakPoints.find(fileName);
                if (foundItr != GlobalDebugger.breakPoints.end()) {
                    auto& found = foundItr->second;
                    auto vecFound = std::find_if(found.begin(), found.end(), [lineNumber](const BreakPoint& bp) {
                        return lineNumber == bp.line;
                    });
                    if (vecFound != found.end()) {
                        found.erase(vecFound);
                    }
                    if (found.empty())
                        GlobalDebugger.breakPoints.erase(fileName);
                        //GlobalDebugger.breakPoints.remove(fileName.c_str());
                }
            } break;
            case NC_CommandType::BPContinue: {
                GlobalDebugger.commandContinue(static_cast<StepType>(packet.value<int>("data", 0)));
            } break;

            default: break;
            case NC_CommandType::MonitorDump: for (auto& it : GlobalDebugger.monitors) it->dump(); break;
            case NC_CommandType::setHookEnable: {
                hookEnabled_Simulate = packet.value<int>("state", 1);
                hookEnabled_Instruction = packet.value<int>("state", 1);
            } break;
            case NC_CommandType::getVariable: {
                JsonArchive ar(packet["data"]);
                uint16_t scope;
                std::vector<std::string> variableNames;
                ar.Serialize("scope", scope);
                ar.Serialize("name", variableNames);
                auto var = GlobalDebugger.getVariables(static_cast<VariableScope>(scope), variableNames);

                JsonArchive answer;
                answer.Serialize("handle", packet.value<std::string>("handle", {}));
                answer.Serialize("data", var);
                answer.Serialize("command", static_cast<int>(NC_OutgoingCommandType::VariableReturn));

                sendMessage(answer.to_string());
            } break;
            case NC_CommandType::getCurrentCode: {
                JsonArchive answer;
                answer.Serialize("handle", packet.value<std::string>("handle", {}));
                GlobalDebugger.grabCurrentCode(answer, packet.value<std::string>("file", ""));
                sendMessage(answer.to_string());
            } break;
            case NC_CommandType::getVersionInfo: {
                JsonArchive answer;
                
                answer.Serialize("handle", packet.value<std::string>("handle", {}));
                answer.Serialize("command", static_cast<int>(NC_OutgoingCommandType::versionInfo));
                answer.Serialize("build", DBG_BUILD);
                answer.Serialize("version", DBG_VERSION);
#ifdef X64
                answer.Serialize("arch", "X64");
#else
                answer.Serialize("arch", "X86");
#endif
                answer.Serialize("state", GlobalDebugger.state);

                GlobalDebugger.productInfo.Serialize(answer);
                JsonArchive HI;

                GlobalDebugger.SerializeHookIntegrity(HI);
                answer.Serialize("HI", HI);

                sendMessage(answer.to_string());
            } break;
            case NC_CommandType::getAllScriptCommands: {
                JsonArchive answer;
                answer.Serialize("handle", packet.value<std::string>("handle", {}));
                GlobalDebugger.serializeScriptCommands(answer);
                sendMessage(answer.to_string());
                } break;
            case NC_CommandType::getAvailableVariables: {
                JsonArchive ar(packet["data"]);
                uint16_t scope;
                ar.Serialize("scope", scope);//VariableScope::callstack not supported
                auto var = GlobalDebugger.getAvailableVariables(static_cast<VariableScope>(scope));

                JsonArchive dataAr;

                if (static_cast<VariableScope>(scope) & VariableScope::local)
                    dataAr.Serialize("2", var[VariableScope::local]);
                if (static_cast<VariableScope>(scope) & VariableScope::missionNamespace)
                    dataAr.Serialize("4", var[VariableScope::missionNamespace]);
                if (static_cast<VariableScope>(scope) & VariableScope::uiNamespace)
                    dataAr.Serialize("8", var[VariableScope::uiNamespace]);
                if (static_cast<VariableScope>(scope) & VariableScope::profileNamespace)
                    dataAr.Serialize("16", var[VariableScope::profileNamespace]);
                if (static_cast<VariableScope>(scope) & VariableScope::parsingNamespace)
                    dataAr.Serialize("32", var[VariableScope::parsingNamespace]);

                JsonArchive answer;
                answer.Serialize("handle", packet.value<std::string>("handle", {}));
                answer.Serialize("data", dataAr);
                answer.Serialize("command", static_cast<int>(NC_OutgoingCommandType::AvailableVariablesReturn));

                sendMessage(answer.to_string());
            } break;

            case NC_CommandType::haltNow: {
                GlobalDebugger.state = DebuggerState::waitForHalt;
            } break;

            case NC_CommandType::ExecuteCode: {
                //I don't verify that debugger is halted... please be nice.
                //#TODO ^
                JsonArchive ar(packet["data"]);

                r_string script;
                ar.Serialize("script", script);


                GlobalDebugger.executeScriptInHalt(script, static_cast<r_string>(packet.value<std::string>("handle", {})));
            } break;

            case NC_CommandType::LoadFile: {
                JsonArchive answer;
                answer.Serialize("handle", packet.value<std::string>("handle", {}));
                answer.Serialize("command", static_cast<int>(NC_OutgoingCommandType::LoadFileResult));

                intercept::client::invoker_lock lock(GlobalDebugger.state == DebuggerState::breakState); // need to run script command

                {
                    JsonArchive ar(packet["data"]);
                    r_string path;
                    ar.Serialize("path", path);
                    JsonArchive answerAr;
                    answerAr.Serialize("path", path);
//#ifdef SerializeScriptContent

                    if (path.find("/temp/bin/a3/") == 0) // Invalid paths written into vanilla SQFC files by ArmaScriptCompiler
                        path = path.substr(9, -1);

                    std::string path2 = path.c_str();
                    //path2.startsWith c++20
                    for (char& path1 : path2)
                    {
                        if (path1 == '/')
                            path1 = '\\';
                    }

                    answerAr.Serialize("content", intercept::sqf::load_file(path2));
//#endif

                    answer.Serialize("data", answerAr);
                }
                sendMessage(answer.to_string());
            } break;

            case NC_CommandType::SetExceptionFilter: {
                GlobalDebugger.exceptionFilter = 0;

                for (const auto& filterEntry : packet["data"]["scriptErrFilters"])
                {
                    uint32_t num = filterEntry.get<uint32_t>();
                    GlobalDebugger.exceptionFilter |= (1ull << num);
                }

            } break;

            case NC_CommandType::FetchAllFunctionsInNamespace: {
                JsonArchive ar(packet["data"]);
                uint16_t scope;
                ar.Serialize("scope", scope);

                intercept::client::invoker_lock lock(GlobalDebugger.state == DebuggerState::breakState);

                auto functions = GlobalDebugger.getCodeVariables(static_cast<VariableScope>(scope));

                JsonArchive answer;
                answer.Serialize("handle", packet.value<std::string>("handle", {}));
                answer.Serialize("command", static_cast<int>(NC_OutgoingCommandType::AllFunctionsInNamespaceResult));

                JsonArchive answerAr;
                auto& _array = (*answerAr.getRaw())["modules"];

                for (const auto& function : functions)
                {
                    // We need to get a filename
                    if (auto code = function.var->value.get_as<game_data_code>())
                    {
                        if (code->instructions.is_empty())
                            continue;
                        if (!code->instructions.front()->sdp)
                            continue;
                        if (code->instructions.front()->sdp->sourcefile.empty())
                            continue;

                        JsonArchive element;
                        element.Serialize("name", function.var->name);
                        element.Serialize("ns", static_cast<int>(function.ns));

                        std::string path = code->instructions.front()->sdp->sourcefile.c_str();
                        for (char& path1 : path)
                        {
                            if (path1 == '/')
                                path1 = '\\';
                        }

                        //if (path.front() == '\\')
                        //    path.erase(0, 1);

                        if (path.find("temp\\bin\\A3\\") == 0) // Invalid paths written into vanilla SQFC files by ArmaScriptCompiler
                            path = path.substr(9, -1);

                        //if (path.find("fn_3den_init.sqf") != -1)
                        //    __debugbreak();

                        element.Serialize("path", path);

                        _array.push_back(*element.getRaw());
                    }
                }

                answer.Serialize("data", answerAr);
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
    server.writeMessage(message + "\n");
}

void NetworkController::onShutdown() {
    server.close();
}
