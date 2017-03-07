#pragma once
#include <map>
#include <memory>
#include <chrono>
#include <vector>
#include "BIDebugEngine.h"

class RString;
class Script;

class Instruction {
public:
    std::string debugName;
    std::string file;
    uint16_t line;
    uint16_t offset; //#TODO make 32bit
    std::chrono::high_resolution_clock::time_point execTime;
    std::vector<Instruction> lowerScope;
    void writeToFile(std::ofstream& file, uint16_t tabs);
};



class VMContext {
public:
    VMContext();
    ~VMContext();

    std::shared_ptr<Script> getScriptByContent(RString content);
    void dbg_EnterContext();
    void dbg_LeaveContext();
    void dbg_instructionTimeDiff(std::chrono::high_resolution_clock::duration diff);
    void addInstruction(RV_VMContext* ctx, RV_GameInstruction* instruction);
    bool isScriptVM{ false }; //is false for unscheduled scripts
    bool canBeDeleted{ true }; //scriptVM's are only deleted when they are finished
    std::map<uintptr_t, std::shared_ptr<Script>> contentPtrToScript;
    std::chrono::high_resolution_clock::duration totalRuntime {0};
    std::chrono::high_resolution_clock::time_point runningSince; //Used to track elapsed time on scriptStop

    std::vector<Instruction> instructions;
};

