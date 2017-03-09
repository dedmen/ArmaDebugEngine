#include "breakPoint.h"
#include "Debugger.h"


breakPoint::breakPoint(uint16_t _line):line(_line) {}


breakPoint::~breakPoint() {}

void BPAction_ExecCode::execute(Debugger* dbg, breakPoint* bp, const DebuggerInstructionInfo& info) {
    info.context->callStack.back()->EvaluateExpression((code + " " + std::to_string(bp->hitcount) + ";"+
        info.instruction->GetDebugName().data()+        
        "\"").c_str(), 10);
}
