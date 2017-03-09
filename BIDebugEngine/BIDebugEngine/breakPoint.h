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
class breakPoint;
class IBreakPointCondition {
public:
    virtual ~IBreakPointCondition() {};
    virtual bool isMatching(Debugger* ,breakPoint* , const DebuggerInstructionInfo&) = 0;
};

class IBreakPointAction {
public:
    virtual ~IBreakPointAction() {};
    virtual void execute(Debugger*, breakPoint*, const DebuggerInstructionInfo&) = 0;
};

class BPAction_ExecCode : public IBreakPointAction {
public:
    BPAction_ExecCode(std::string _code) : code(std::move(_code)) {}
    virtual ~BPAction_ExecCode() {};
    virtual void execute(Debugger*, breakPoint*, const DebuggerInstructionInfo&);
private:
    std::string code;
};




class breakPoint {
public:
    breakPoint(uint16_t line);
    breakPoint(const breakPoint& bp) : line(bp.line) {}


    breakPoint(breakPoint&& other) : condition(std::move(other.condition)), action(std::move(other.action)), line(other.line) {}

    breakPoint& operator=(breakPoint&& other) {
        condition = std::move(other.condition);
        action = std::move(other.action);
        line = std::move(other.line);
        return *this;
    }

    ~breakPoint();
    RString filename;
    uint16_t line;
    uint16_t hitcount{ 0 };
    std::unique_ptr<IBreakPointCondition> condition;
    std::unique_ptr<IBreakPointAction> action;
};

