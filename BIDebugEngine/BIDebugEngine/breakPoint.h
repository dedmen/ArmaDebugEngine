#pragma once
#include <cstdint>
#include "RVClasses.h"
#include "VMContext.h"

struct breakPointBreakInfo {
    RV_VMContext* pVMContext;
    game_instruction* pInstruction;
    //#TODO add gameState ptr
};

struct DebuggerInstructionInfo;
class Debugger;
class BreakPoint;
class IBreakPointCondition {
public:
    virtual ~IBreakPointCondition() {};
    virtual bool isMatching(Debugger*, BreakPoint*, const DebuggerInstructionInfo&) = 0;
    virtual void Serialize(JsonArchive& ar) { return; };
};

class BPCondition_Code : public IBreakPointCondition { //Executes code and check if return value is true
public:
    virtual ~BPCondition_Code() {};

    bool isMatching(Debugger*, BreakPoint*, const DebuggerInstructionInfo&) override;
    void Serialize(JsonArchive& ar) override;
private:
    std::string code;
};

//Beware when adding hitcount condition. Hitcount is only incremented when condition is matching.

class IBreakPointAction {
public:
    virtual ~IBreakPointAction() {};
    virtual void execute(Debugger*, BreakPoint*, const DebuggerInstructionInfo&) = 0;
    virtual void Serialize(JsonArchive& ar) { return; };
};

class BPAction_ExecCode : public IBreakPointAction {
public:
    BPAction_ExecCode(std::string _code) : code(std::move(_code)) {}
    BPAction_ExecCode() {}
    virtual ~BPAction_ExecCode() {};

    void execute(Debugger*, BreakPoint*, const DebuggerInstructionInfo&) override;
    void Serialize(JsonArchive& ar) override;
private:
    std::string code;
};

class BPAction_Halt : public IBreakPointAction { //This freezes the Engine till further commands
public:
    BPAction_Halt(haltType _type) : type(_type) {}
    virtual ~BPAction_Halt() {};

    void execute(Debugger*, BreakPoint*, const DebuggerInstructionInfo&) override;
    void Serialize(JsonArchive& ar) override;
    haltType type;
};

class BPAction_LogCallstack : public IBreakPointAction { //Logs callstack to file
public:
    BPAction_LogCallstack(std::string _basePath) : basePath(_basePath) {}
    BPAction_LogCallstack() {}
    virtual ~BPAction_LogCallstack() {};

    void execute(Debugger*, BreakPoint*, const DebuggerInstructionInfo&) override;
    void Serialize(JsonArchive& ar) override;
private:
    std::string basePath;
};


enum class BPCondition_types {
    invalid,
    Code
};

enum class BPAction_types {
    invalid,
    ExecCode,
    Halt,
    LogCallstack
};





class BreakPoint {
public:
    BreakPoint(uint16_t line);
	BreakPoint(BreakPoint& bp) { *this = std::move(bp);}//should never happen
    BreakPoint() {};

    BreakPoint(BreakPoint&& other) noexcept : line(other.line), condition(std::move(other.condition)), action(std::move(other.action)) {}

    BreakPoint& operator=(BreakPoint&& other) {
        condition = std::move(other.condition);
        action = std::move(other.action);
        line = std::move(other.line);
        return *this;
    }
    BreakPoint& operator=(BreakPoint& other) {
        __debugbreak(); //This should not happen
        return *this;
    }
	const char* get_map_key() const { return filename.c_str(); }
    ~BreakPoint();
    void Serialize(JsonArchive& ar);
    bool trigger(Debugger*, const DebuggerInstructionInfo&);
    void executeActions(Debugger*, const DebuggerInstructionInfo&);
    r_string filename;
    uint16_t line{ 0 };
    uint16_t hitcount{ 0 };
    std::unique_ptr<IBreakPointCondition> condition;
    std::unique_ptr<IBreakPointAction> action;
    std::string label;
};

