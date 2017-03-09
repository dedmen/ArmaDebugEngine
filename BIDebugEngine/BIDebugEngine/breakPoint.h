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
};

class IBreakPointAction {
public:
    virtual ~IBreakPointAction() {};
    virtual void execute(Debugger*, BreakPoint*, const DebuggerInstructionInfo&) = 0;
};

class BPAction_ExecCode : public IBreakPointAction {
public:
    BPAction_ExecCode(std::string _code) : code(std::move(_code)) {}
    virtual ~BPAction_ExecCode() {};

    void execute(Debugger*, BreakPoint*, const DebuggerInstructionInfo&) override;
private:
    std::string code;
};




class BreakPoint {
public:
    BreakPoint(uint16_t line);
    BreakPoint(const BreakPoint& bp) : line(bp.line) {}


    BreakPoint(BreakPoint&& other) : condition(std::move(other.condition)), action(std::move(other.action)), line(other.line) {}

    BreakPoint& operator=(BreakPoint&& other) {
        condition = std::move(other.condition);
        action = std::move(other.action);
        line = std::move(other.line);
        return *this;
    }

    ~BreakPoint();
    RString filename;
    uint16_t line;
    uint16_t hitcount{ 0 };
    std::unique_ptr<IBreakPointCondition> condition;
    std::unique_ptr<IBreakPointAction> action;
};

