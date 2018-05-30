#pragma once
#include <string>
#include "RVBaseTypes.h"
#include <debug.hpp>

using IDebugValueRef = intercept::types::__internal::I_debug_value::RefType;

class IDebugVariable {
public:
    virtual ~IDebugVariable() {}
    //Use _name variable directly instead! getName copies the buffer
    virtual void getName(char *buffer, int len) const {};
    virtual IDebugValueRef getValue() const { return IDebugValueRef(); };
    //RString _name;
};

void printAllVariables(const intercept::types::vm_context::IDebugScope& s);
std::string allVariablesToString(const intercept::types::vm_context::IDebugScope& s);



enum class StepType {
    STContinue,
    STInto,
    STOver,
    STOut
};

enum class StepSize {
    SUStatement,
    SULine,
    SUInstruction
};

class IDebugScript {
public:
    IDebugScript() {}
    virtual ~IDebugScript() {}

    // reference counting
    virtual int addRef() = 0;
    virtual int release() = 0;

    virtual bool IsAttached() const = 0;
    virtual bool IsEntered() const = 0;
    virtual bool IsRunning() const = 0;

    virtual void AttachScript() = 0;
    virtual void EnterScript() = 0;
    virtual void RunScript(bool run) = 0;
    virtual void Step(StepType kind, StepSize unit) = 0;

    virtual void GetScriptContext(vm_context::IDebugScope * &scope) = 0;
    virtual void SetBreakpoint(const char *file, int line, unsigned long bp, bool enable) = 0;
    virtual void RemoveBreakpoint(unsigned long bp) = 0;
    virtual void EnableBreakpoint(unsigned long bp, bool enable) = 0;
};

typedef ref<IDebugScript> IDebugScriptRef;
