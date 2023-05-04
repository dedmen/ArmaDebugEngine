#pragma once
#include <shared/types.hpp>

class SQF_Assembly_Iface {
public:

    enum class InstructionType {
        GameInstructionNewExpression,
        GameInstructionConst,
        GameInstructionFunction,
        GameInstructionOperator,
        GameInstructionAssignment,
        GameInstructionVariable,
        GameInstructionArray,
        GameInstructionNular
    };

    void init();

    using instructionExecFnc = void(*) (intercept::types::game_instruction* instr, intercept::types::game_state& state, intercept::types::vm_context& ctx);
    void setHook(InstructionType type, instructionExecFnc func);

    bool ready;
};



static inline SQF_Assembly_Iface GASM;
