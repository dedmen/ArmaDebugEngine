#include "VMContext.h"
#include "RVClasses.h"
#include "Script.h"
#include <fstream>

VMContext::VMContext() {}


VMContext::~VMContext() {}

std::shared_ptr<Script> VMContext::getScriptByContent(r_string content) {
    uintptr_t contentPtr = reinterpret_cast<uintptr_t>(content.data());
    auto iter(contentPtrToScript.lower_bound(contentPtr));
    //Thank you SO! 
    if (iter == contentPtrToScript.end() || contentPtr < iter->first) {    // not found
        auto newEntry = std::make_shared<Script>(content);
        contentPtrToScript.insert(iter, { contentPtr, newEntry });// hinted insertion
        return newEntry;
    }
    return iter->second;
}

void VMContext::dbg_EnterContext() {
    runningSince = std::chrono::high_resolution_clock::now();
}

void VMContext::dbg_LeaveContext() {
    totalRuntime += std::chrono::high_resolution_clock::now() - runningSince;
}

void VMContext::dbg_instructionTimeDiff(std::chrono::high_resolution_clock::duration diff) {
    totalRuntime -= diff;
}
extern std::chrono::high_resolution_clock::time_point globalTime;
extern std::chrono::high_resolution_clock::time_point frameStart;

//RString str_newScope("NewScope");
//RString str_dummyPlaceholder("dummyPlaceholder");
//RString str_DbgEngine("DbgEngine");

void VMContext::addInstruction(RV_VMContext* ctx, game_instruction* instruction) {
    /*
    static uint32_t instructionCounter{ 0 };
    std::chrono::high_resolution_clock::time_point time(globalTime - frameStart);
    instructionCounter++;
    RString filename;
    if (!instruction->_scriptPos._sourceFile.isNull())
        filename = instruction->_scriptPos._sourceFile;
    uint16_t line = instruction->_scriptPos._sourceLine;
    uint16_t offset = instruction->_scriptPos._pos;
    auto dbg = instruction->GetDebugName();
    RString dbgName = dbg;
    if (dbgName.startsWith("const"))
        dbgName = dbgName.substr(0, 7);
    if (dbg.length() < 2) return;
    if (ctx->callStack.count() > 1 && instructions.empty())
        instructions.push_back(Instruction{ str_dummyPlaceholder,str_DbgEngine,0,0,time });
    if (ctx->callStack.count() == 1) {
        instructions.push_back(Instruction{ dbgName,filename,line,offset,time });
    } else if (ctx->callStack.count() > 1) {
        Instruction* lastInstruction = &instructions.back();
        for (int i = 0; i < ctx->callStack.count() - 2; i++) {
            if (lastInstruction->lowerScope.empty()) {
                lastInstruction->lowerScope.push_back(Instruction{ str_dummyPlaceholder,str_DbgEngine,0,0,time });
            }
            lastInstruction = &lastInstruction->lowerScope.back();
        }
        if (lastInstruction->lowerScope.capacity() - lastInstruction->lowerScope.size() < 2)
            lastInstruction->lowerScope.reserve(lastInstruction->lowerScope.capacity() * 2);
        if (lastInstruction->lowerScope.empty()) { //New scope
            lastInstruction->lowerScope.push_back(Instruction{ ctx->callStack.back()->allVariablesToString(), str_newScope ,0,0,time });
        }
        lastInstruction->lowerScope.push_back(Instruction{ dbgName,filename,line,offset,time });
    } else {
        __debugbreak();
    } */
}

void Instruction::writeToFile(std::ofstream& f, uint16_t tabs) {
    for (int i = 0; i < tabs; i++)
        f << "\t";
    f << "instr " << file << " " << line << ":" << offset << " T:" << execTime.time_since_epoch().count() / 1000 << " " << debugName << "\n";
    for (auto& it : lowerScope) {
        //if (!it.lowerScope.empty() || it.file == "NewScope")
        it.writeToFile(f, tabs + 1);
    }
}
