#include "BreakPoint.h"
#include "Debugger.h"
#include "Serialize.h"
#include <windows.h>
#include <fstream>

BreakPoint::BreakPoint(uint16_t _line) :line(_line) {}


BreakPoint::~BreakPoint() {}

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
                    condition = std::make_unique<BPCondition_Code>();
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
                    action = std::make_unique<BPAction_ExecCode>();
                    JsonArchive actJsonAr(actJson);
                    action->Serialize(actJsonAr);
                } break;
                default: break;
                case BPAction_types::Halt: {
                    action = std::make_unique<BPAction_Halt>();
                    JsonArchive actJsonAr(actJson);
                    action->Serialize(actJsonAr);
                } break;
                case BPAction_types::LogCallstack: {
                    action = std::make_unique<BPAction_LogCallstack>();
                    JsonArchive actJsonAr(actJson);
                    action->Serialize(actJsonAr);
                } break;
            }
        }
        label = pJson->value("label", "");
    }





}

bool BPCondition_Code::isMatching(Debugger*, BreakPoint*, const DebuggerInstructionInfo& info) {
    auto rtn = info.context->callStack.back()->EvaluateExpression(code.c_str(), 10);
    if (rtn.isNull())  return false; //#TODO this is code error.
    //We get a ptr to the IDebugValue of GameData. But we wan't the GameData vtable.    
    auto gdRtn = reinterpret_cast<GameData*>(rtn.get() - 2);
    return gdRtn->getBool();
}

void BPCondition_Code::Serialize(JsonArchive& ar) {
    if (!ar.reading()) {
        ar.Serialize("type", static_cast<int>(BPCondition_types::Code));
    }
    ar.Serialize("code", code);
}

void BPAction_ExecCode::execute(Debugger* dbg, BreakPoint* bp, const DebuggerInstructionInfo& info) {
    info.context->callStack.back()->EvaluateExpression((code + " " + std::to_string(bp->hitcount) + ";" +
        info.instruction->GetDebugName().data() +
        "\"").c_str(), 10);
}

void BPAction_ExecCode::Serialize(JsonArchive& ar) {
    if (!ar.reading()) {
        ar.Serialize("type", static_cast<int>(BPAction_types::ExecCode));
    }
    ar.Serialize("code", code);
}

extern EngineAlive* EngineAliveFnc;
void BPAction_Halt::execute(Debugger* dbg, BreakPoint* bp, const DebuggerInstructionInfo& info) {
    SECURITY_DESCRIPTOR SD;
    InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&SD, TRUE, NULL, FALSE);
    SECURITY_ATTRIBUTES SA;
    SA.nLength = sizeof(SA);
    SA.lpSecurityDescriptor = &SD;
    SA.bInheritHandle = TRUE;
    auto waitEvent = CreateEvent(
        &SA,    // default security attribute 
        TRUE,    // manual-reset event 
        FALSE,    // initial state = signaled 
        NULL);   // unnamed event object 

    dbg->onHalt(waitEvent, bp, info);
    bool halting = true;
    while (halting) {
        DWORD waitResult = WaitForSingleObject(waitEvent, 3000);

        EngineAliveFnc();
        if (waitResult != WAIT_TIMEOUT) {
            halting = false;
            dbg->onContinue();
        }
    }

    CloseHandle(waitEvent);
}

void BPAction_Halt::Serialize(JsonArchive& ar) {
    if (!ar.reading()) {
        ar.Serialize("type", static_cast<int>(BPAction_types::Halt));
    }
}

void BPAction_LogCallstack::execute(Debugger*, BreakPoint* bp, const DebuggerInstructionInfo& instructionInfo) {
    JsonArchive ar;
    instructionInfo.context->Serialize(ar);
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
