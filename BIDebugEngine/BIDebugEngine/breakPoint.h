#pragma once
#include <cstdint>
#include "RVClasses.h"
#include "VMContext.h"

struct breakPointBreakInfo {
    RV_VMContext* pVMContext;
    RV_GameInstruction* pInstruction;
    //#TODO add gameState ptr
};

struct DebuggerInstructionInfo;
class Debugger;
class BreakPoint;
class IBreakPointCondition {
public:
    virtual ~IBreakPointCondition() {};
    virtual bool isMatching(Debugger* ,BreakPoint* , const DebuggerInstructionInfo&) = 0;
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
    BPAction_Halt() {}
    virtual ~BPAction_Halt() {};

    void execute(Debugger*, BreakPoint*, const DebuggerInstructionInfo&) override;
    void Serialize(JsonArchive& ar) override;
};

enum class BPCondition_types {
    invalid,
    Code
};

enum class BPAction_types {
   invalid,
   ExecCode,
   Halt
};





class BreakPoint {
public:
    BreakPoint(uint16_t line);
    BreakPoint(const BreakPoint& bp) : line(bp.line) {}
    BreakPoint() {};

    BreakPoint(BreakPoint&& other) : condition(std::move(other.condition)), action(std::move(other.action)), line(other.line) {}

    BreakPoint& operator=(BreakPoint&& other) {
        condition = std::move(other.condition);
        action = std::move(other.action);
        line = std::move(other.line);
        return *this;
    }

    ~BreakPoint();
    void Serialize(JsonArchive& ar);
    RString filename;
    uint16_t line {0};
    uint16_t hitcount{ 0 };
    std::unique_ptr<IBreakPointCondition> condition;
    std::unique_ptr<IBreakPointAction> action;
};

