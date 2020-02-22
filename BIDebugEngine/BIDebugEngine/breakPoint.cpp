#include "BreakPoint.h"
#include "Debugger.h"
#include "Serialize.h"
#include <condition_variable>
#include <memory>
#include <fstream>
#include <chrono>
#include <core.hpp>
using namespace std::chrono_literals;

void BreakPoint::Serialize(JsonArchive& ar) {
    ar.Serialize("filename", filename);
    ar.Serialize("line", line);

    if (!ar.reading()) {
        ar.Serialize("condition", condition);
        ar.Serialize("action", action);
    } else {
        auto pJson = ar.getRaw();

        if (!(*pJson)["condition"].is_null()) {
            auto condJson = (*pJson)["condition"];
            auto type = static_cast<BPCondition_types>(condJson.value<int>("type", 0));
            switch (type) {

                case BPCondition_types::invalid: break;
                case BPCondition_types::Code: {
                    condition = std::make_shared<BPCondition_Code>();
                    JsonArchive condJsonAr(condJson);
                    condition->Serialize(condJsonAr);
                } break;
                 case BPCondition_types::HitCount: {
                    condition = std::make_shared<BPCondition_HitCount>();
                    JsonArchive condJsonAr(condJson);
                    condition->Serialize(condJsonAr);
                } break;
                default: break;
            }
        }
        if (!(*pJson)["action"].is_null()) {
            auto actJson = (*pJson)["action"];
            auto type = static_cast<BPAction_types>(actJson.value<int>("type", 0));
            switch (type) {

                case BPAction_types::invalid: break;
                case BPAction_types::ExecCode: {
                    action = std::make_shared<BPAction_ExecCode>();
                    JsonArchive actJsonAr(actJson);
                    action->Serialize(actJsonAr);
                } break;
                default: break;
                case BPAction_types::Halt: {
                    action = std::make_shared<BPAction_Halt>(haltType::breakpoint);
                    JsonArchive actJsonAr(actJson);
                    action->Serialize(actJsonAr);
                } break;
                case BPAction_types::LogCallstack: {
                    action = std::make_shared<BPAction_LogCallstack>();
                    JsonArchive actJsonAr(actJson);
                    action->Serialize(actJsonAr);
                } break;
            }
        }
        label = pJson->value("label", "");
    }





}

bool BreakPoint::trigger(Debugger* dbg, const DebuggerInstructionInfo& instructionInfo) {
    if (condition && !condition->isMatching(dbg, this, instructionInfo)) return false;
    hitcount++;



    /*
    JsonArchive varArchive;
    JsonArchive nsArchive[4];
    auto func = [](JsonArchive& nsArchive, const GameDataNamespace* var) {
    var->_variables.forEach([&nsArchive](const GameVariable& var) {

    JsonArchive variableArchive;

    auto name = var._name;
    if (var._value.isNull()) {
    variableArchive.Serialize("type", "nil");
    nsArchive.Serialize(name.data(), variableArchive);
    return;
    }
    auto value = var._value._data;
    const auto type = value->getTypeString();

    variableArchive.Serialize("type", type);
    if (strcmp(type, "array") == 0) {
    variableArchive.Serialize("value", value->getArray());
    } else {
    variableArchive.Serialize("value", value->getAsString());
    }
    nsArchive.Serialize(name.data(), variableArchive);

    });
    };

    std::thread _1([&]() {func(nsArchive[0], instructionInfo.gs->_namespaces.get(0)); });
    std::thread _2([&]() {func(nsArchive[1], instructionInfo.gs->_namespaces.get(1)); });
    std::thread _3([&]() {func(nsArchive[2], instructionInfo.gs->_namespaces.get(2)); });
    std::thread _4([&]() {func(nsArchive[3], instructionInfo.gs->_namespaces.get(3)); });


    if (_1.joinable()) _1.join();
    varArchive.Serialize(instructionInfo.gs->_namespaces.get(0)->_name.data(), nsArchive[0]);
    if (_2.joinable()) _2.join();
    varArchive.Serialize(instructionInfo.gs->_namespaces.get(1)->_name.data(), nsArchive[1]);
    if (_3.joinable()) _3.join();
    varArchive.Serialize(instructionInfo.gs->_namespaces.get(2)->_name.data(), nsArchive[2]);
    if (_4.joinable()) _4.join();
    varArchive.Serialize(instructionInfo.gs->_namespaces.get(3)->_name.data(), nsArchive[3]);

    auto text = varArchive.to_string();
    std::ofstream f("P:\\AllVars.json", std::ios::out | std::ios::binary);
    f.write(text.c_str(), text.length());
    f.close();



    */




    executeActions(dbg, instructionInfo);

    //JsonArchive ar;
    //instructionInfo.context->Serialize(ar);
    //auto text = ar.to_string();
    //std::ofstream f("P:\\break.json", std::ios::out | std::ios::binary);
    //f.write(text.c_str(), text.length());
    //f.close();
    //std::ofstream f2("P:\\breakScript.json", std::ios::out | std::ios::binary);
    //f2.write(instructionInfo.instruction->_scriptPos._content.data(), instructionInfo.instruction->_scriptPos._content.length());
    //f2.close();
    return true;
}

void BreakPoint::executeActions(Debugger* dbg, const DebuggerInstructionInfo& instructionInfo) {
    if (action) action->execute(dbg, this, instructionInfo);
}

bool BPCondition_Code::isMatching(Debugger*, BreakPoint*, const DebuggerInstructionInfo& info) {
    auto allo = intercept::client::host::functions.get_engine_allocator();
    auto ef = allo->evaluate_func;
    if (!ef) return false;
    auto gs = allo->gameState;

    auto data = intercept::sqf::compile(code).get_as<game_data_code>();

    auto ns = gs->get_global_namespace(game_state::namespace_type::mission);
    static r_string fname = "interceptCall"sv;

    auto rtn = ef(*data, ns, fname);

    return rtn;
}

void BPCondition_Code::Serialize(JsonArchive& ar) {
    if (!ar.reading()) {
        ar.Serialize("type", static_cast<int>(BPCondition_types::Code));
    }
    ar.Serialize("code", code);
}

bool BPCondition_HitCount::isMatching(Debugger*, BreakPoint*, const DebuggerInstructionInfo&) {
    currentCount++;

    if (count == currentCount) {
        currentCount = 0;
        return true;
    }

    return false;
}

void BPCondition_HitCount::Serialize(JsonArchive& ar) {
    if (!ar.reading()) {
        ar.Serialize("type", static_cast<int>(BPCondition_types::HitCount));
    }
    ar.Serialize("count", count);
}

void BPAction_ExecCode::execute(Debugger* dbg, BreakPoint* bp, const DebuggerInstructionInfo& info) {
    auto allo = intercept::client::host::functions.get_engine_allocator();
    auto ef = allo->evaluate_func;
    if (!ef) return;
    auto gs = allo->gameState;

    auto data = intercept::sqf::compile(code + " " + std::to_string(bp->hitcount) + ";" +
        info.instruction->get_name().data() +
        "\"").get_as<game_data_code>();

    auto ns = gs->get_global_namespace(game_state::namespace_type::mission);
    static r_string fname = "interceptCall"sv;

    auto rtn = ef(*data, ns, fname);
}

void BPAction_ExecCode::Serialize(JsonArchive& ar) {
    if (!ar.reading()) {
        ar.Serialize("type", static_cast<int>(BPAction_types::ExecCode));
    }
    ar.Serialize("code", code);
}

extern EngineAlive* EngineAliveFnc;
extern EngineEnableMouse* EngineEnableMouseFnc;

void BPAction_Halt::execute(Debugger* dbg, BreakPoint* bp, const DebuggerInstructionInfo& info) {
    auto waitEvent = std::make_shared<std::pair<std::condition_variable, bool>>();
    waitEvent->second = false;
    std::mutex waitMutex;

    //#TODO catch crashes in these engine funcs by using https://msdn.microsoft.com/en-us/library/1deeycx5(v=vs.80).aspx http://stackoverflow.com/questions/457577/catching-access-violation-exceptions
    //#TODO get information from exception https://www.codeproject.com/Articles/422/SEH-and-C-Exceptions-catch-all-in-one https://msdn.microsoft.com/en-us/library/5z4bw5h5.aspx 
//https://msdn.microsoft.com/en-us/library/s58ftw19(v=vs.80).aspx This is maybe a better method instead of enabling SEH. Plus here I also get what exception happened without complicated handling
    static bool EngStable_EnableMouse = true;
    static bool EngStable_Alive = true;

    try {
        //#TODO also want to SkipKeysInThisFrame See 1.68.140.940
        if (EngineEnableMouseFnc && EngStable_EnableMouse) EngineEnableMouseFnc(false); //Free mouse from Arma
    } catch (...) {
        EngStable_EnableMouse = false;
    }

    dbg->onHalt(waitEvent, bp, info, type);
    bool halting = true;
    while (halting) {
        std::unique_lock<std::mutex> lk(waitMutex);
        waitEvent->first.wait_for(lk, 1s, [waitEvent]() {return waitEvent->second; });

        try {
            if (EngineAliveFnc && EngStable_Alive) EngineAliveFnc();
        } catch (...) {
            EngStable_Alive = false;
        }
        if (waitEvent->second) {
            halting = false;
            //EngineEnableMouseFnc(true); Causes mouse to jump to bottom right screen corner
            dbg->onContinue();
        }
    }

}

void BPAction_Halt::Serialize(JsonArchive& ar) {
    if (!ar.reading()) {
        ar.Serialize("type", static_cast<int>(BPAction_types::Halt));
    }
}

void BPAction_LogCallstack::execute(Debugger* dbg, BreakPoint* bp, const DebuggerInstructionInfo& instructionInfo) {
    JsonArchive ar;
    instructionInfo.context->Serialize(ar);
    if (basePath == "send") {
        JsonArchive arComplete;
        arComplete.Serialize("command", static_cast<int>(NC_OutgoingCommandType::BreakpointLog));
        arComplete.Serialize("data", ar);

        auto text = ar.to_string();
        dbg->nController.sendMessage(text);
        return;
    }

    auto text = ar.to_string();
    std::ofstream f(basePath + bp->label + std::to_string(bp->hitcount) + ".json", std::ios::out | std::ios::binary);
    f.write(text.c_str(), text.length());
    f.close();
}

void BPAction_LogCallstack::Serialize(JsonArchive& ar) {
    if (!ar.reading()) {
        ar.Serialize("type", static_cast<int>(BPAction_types::LogCallstack));
    }
    ar.Serialize("basePath", basePath);
}
