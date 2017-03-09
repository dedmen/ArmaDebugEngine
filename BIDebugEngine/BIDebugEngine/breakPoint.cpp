#include "BreakPoint.h"
#include "Debugger.h"


BreakPoint::BreakPoint(uint16_t _line):line(_line) {}


BreakPoint::~BreakPoint() {}

void BPAction_ExecCode::execute(Debugger* dbg, BreakPoint* bp, const DebuggerInstructionInfo& info) {
    info.context->callStack.back()->EvaluateExpression((code + " " + std::to_string(bp->hitcount) + ";"+
        info.instruction->GetDebugName().data()+        
        "\"").c_str(), 10);
}
