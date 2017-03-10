#include "Debugger.h"
#include "VMContext.h"
#include <fstream>
#include "Script.h"
#include "Serialize.h"
#include <unordered_set>
#include <windows.h>

Debugger::Debugger() {
    BreakPoint bp(8);

    bp.action = std::make_unique<BPAction_ExecCode>("systemChat \"hello guys\"");
    JsonArchive test;
    bp.Serialize(test);
    auto text = test.to_string();
    breakPoints["z\\ace\\addons\\explosives\\functions\\fnc_setupExplosive.sqf"].push_back(std::move(bp));
    monitors.push_back(std::make_shared<Monitor_knownScriptFiles>());
}


Debugger::~Debugger() {}

void Debugger::clear() {
    if (VMPtrToScript.empty()) return;
    std::vector<uintptr_t> to_delete;
    to_delete.reserve(VMPtrToScript.size());
    for (auto&it : VMPtrToScript) {
        if (it.second->canBeDeleted)
            to_delete.push_back(it.first);
    }
    if (to_delete.size() == VMPtrToScript.size()) {
        VMPtrToScript.clear();
        return;
    }
    for (auto it : to_delete)
        VMPtrToScript.erase(it);


}

std::shared_ptr<VMContext> Debugger::getVMContext(RV_VMContext* vm) {
    uintptr_t contentPtr = reinterpret_cast<uintptr_t>(vm);
    auto iter(VMPtrToScript.lower_bound(contentPtr));
    //Thank you SO! 
    if (iter == VMPtrToScript.end() || contentPtr < iter->first) {    // not found
        auto newEntry = std::make_shared<VMContext>();
        VMPtrToScript.insert(iter, { contentPtr, newEntry });// hinted insertion
        return newEntry;
    }
    return iter->second;
}

void Debugger::writeFrameToFile(uint32_t frameCounter) {

    std::ofstream file("P:\\" + std::to_string(frameCounter) + ".txt");

    if (file.is_open()) {

        for (auto& it : VMPtrToScript) {
            file << "VM " << it.second->totalRuntime.count() / 1000 << "us isVM:" << it.second->isScriptVM << "\n";

            for (auto& instruction : it.second->instructions) {
                uint16_t tabs = 1;
                instruction.writeToFile(file, tabs);
            }
        }
    }
    file.flush();
}

void Debugger::onInstruction(DebuggerInstructionInfo& instructionInfo) {
    checkForBreakpoint(instructionInfo);

    for (auto& it : monitors)
        it->onInstruction(this,instructionInfo);



    auto instruction = instructionInfo.instruction;
    //auto context = getVMContext(instructionInfo.context);
    //context->addInstruction(instructionInfo.context, instruction);
    //auto script = context->getScriptByContent(instruction->_scriptPos._content);
    //script->dbg_instructionExec();
    //auto dbg = instruction->GetDebugName();
    //
    //
    //if (!instruction->_scriptPos._sourceFile.isNull())
    //    script->_fileName = instruction->_scriptPos._sourceFile;
    //auto callStackIndex = ctx->callStacksCount;
    //bool stackChangeImminent = dbg == "operator call" || dbg == "function call"; //This is a call instruction. This means next instruction will go into lower scope
    if (false && currentContext == scriptExecutionContext::EventHandler && instructionInfo.context->callStack.count() > 3) {
        JsonArchive ar;
        instructionInfo.context->Serialize(ar);
        auto text = ar.to_string();


    }



}

void Debugger::checkForBreakpoint(DebuggerInstructionInfo& instructionInfo) {

    if (breakPoints.empty()) return;
    auto found = breakPoints.find(instructionInfo.instruction->_scriptPos._sourceFile.data());
    if (found == breakPoints.end() || found->second.empty()) return;
    auto &bps = breakPoints[instructionInfo.instruction->_scriptPos._sourceFile.data()];
    for (auto& bp : bps) {
        if (bp.line == instructionInfo.instruction->_scriptPos._sourceLine) {//#TODO move into breakPoint::trigger that returns bool if it triggered
            if (bp.condition && !bp.condition->isMatching(this, &bp, instructionInfo)) continue;
            bp.hitcount++;
            if (bp.action) bp.action->execute(this, &bp, instructionInfo); //#TODO move into breakPoint::executeActions 

            //JsonArchive ar;
            //instructionInfo.context->Serialize(ar);
            //auto text = ar.to_string();
            //std::ofstream f("P:\\break.json", std::ios::out | std::ios::binary);
            //f.write(text.c_str(), text.length());
            //f.close();
            //std::ofstream f2("P:\\breakScript.json", std::ios::out | std::ios::binary);
            //f2.write(instructionInfo.instruction->_scriptPos._content.data(), instructionInfo.instruction->_scriptPos._content.length());
            //f2.close();
        }
    }
}

void Debugger::onShutdown() {
    for (auto& it : monitors)
        it->onShutdown();
}

void Debugger::onStartup() {
    nController.init();
}

void Debugger::onHalt(HANDLE waitEvent, BreakPoint* bp, const DebuggerInstructionInfo& instructionInfo) {
    breakStateContinueEvent = waitEvent;
    state = DebuggerState::breakState;


    JsonArchive ar;
    ar.Serialize("command", 1); //#TODO make enum for this
    instructionInfo.context->Serialize(ar); //Set's callstack
    
    //#TODO add GameState variables

    JsonArchive instructionAr;
    instructionInfo.instruction->Serialize(instructionAr);
    ar.Serialize("instruction", instructionAr);


    auto text = ar.to_string();
    nController.sendMessage(text);


    std::ofstream f("P:\\break.json", std::ios::out | std::ios::binary);
    f.write(text.c_str(), text.length());
    f.close();
    std::ofstream f2("P:\\breakScript.json", std::ios::out | std::ios::binary);
    f2.write(instructionInfo.instruction->_scriptPos._content.data(), instructionInfo.instruction->_scriptPos._content.length());
    f2.close();






}

void Debugger::onContinue() {
    state = DebuggerState::running;
    breakStateContinueEvent = 0;
}

void Debugger::commandContinue() {
    if (state != DebuggerState::breakState) return;
    ResetEvent(breakStateContinueEvent);
    SetEvent(breakStateContinueEvent);
}
