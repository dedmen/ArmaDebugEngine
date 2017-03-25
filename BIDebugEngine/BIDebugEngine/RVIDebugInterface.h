#pragma once
#include <string>
#include "RVBaseTypes.h"
class IDebugValue;
typedef IRef<IDebugValue> IDebugValueRef;

class IDebugValue {
public:
    IDebugValue() {}
    virtual ~IDebugValue() {}

    // reference counting
    virtual int IaddRef() = 0;
    virtual int Irelease() = 0;

    virtual void getTypeStr(char *buffer, int len) const = 0;
    //base is number base. So base10 by default probably
    virtual void getValue(unsigned int base, char *buffer, int len) const = 0;
    virtual bool isArray() const = 0;
    virtual int itemCount() const = 0;
    virtual IDebugValueRef getItem(int i) const = 0;
};

class IDebugVariable {
public:
    virtual ~IDebugVariable() {}
    //Use _name variable directly instead! getName copies the buffer
    virtual void getName(char *buffer, int len) const {};
    virtual IDebugValueRef getValue() const { return IDebugValueRef(); };
    //RString _name;
};

class IDebugScope {
public:
    virtual ~IDebugScope() {}
    virtual const char *getName() const = 0;
    virtual int varCount() const = 0;
    virtual int getVariables(const IDebugVariable **storage, int count) const = 0;
    virtual IDebugValueRef EvaluateExpression(const char *code, unsigned int rad) = 0;
    virtual void getSourceDocPosition(char *file, int fileSize, int &line) = 0;
    virtual IDebugScope *getParent() = 0;

    void printAllVariables();
    std::string allVariablesToString();
};

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

    virtual void GetScriptContext(IDebugScope * &scope) = 0;
    virtual void SetBreakpoint(const char *file, int line, unsigned long bp, bool enable) = 0;
    virtual void RemoveBreakpoint(unsigned long bp) = 0;
    virtual void EnableBreakpoint(unsigned long bp, bool enable) = 0;
};

typedef Ref<IDebugScript> IDebugScriptRef;
