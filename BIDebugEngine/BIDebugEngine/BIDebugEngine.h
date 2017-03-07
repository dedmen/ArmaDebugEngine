#pragma once
#include "RVClasses.h"
#include <string>

class IDebugValue;
typedef Ref<IDebugValue> IDebugValueRef;

class IDebugValue {
public:
    IDebugValue() {}
    virtual ~IDebugValue() {}

    // reference counting
    virtual int addRef() = 0;
    virtual int release() = 0;

    virtual void getType(char *buffer, int len) const = 0;
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
    RString _name;
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
    SKInto,
    SKOver,
    SKOut
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
                        
class EngineInterface {
public:
    EngineInterface() {}
    virtual ~EngineInterface() {}

    virtual void LogF(const char *str) = 0;
};

class DllInterface {
public:
    DllInterface() {}
    virtual ~DllInterface() {}

    virtual void Startup();
    virtual void Shutdown();

    virtual void ScriptLoaded(IDebugScript *script, const char *name);
    virtual void ScriptEntered(IDebugScript *script);
    virtual void ScriptTerminated(IDebugScript *script);

    virtual void FireBreakpoint(IDebugScript *script, unsigned int bp);
    virtual void Breaked(IDebugScript *script);

    virtual void DebugEngineLog(const char *str);
};

class CallStackItem : public RefCount, public IDebugScope {
    CallStackItem* _parent;
    struct GameVarSpace {
        uintptr_t vtable;
        uintptr_t _1;
        uintptr_t _2;
        uintptr_t _3;
        uintptr_t _4;
        uintptr_t _5;
    } _varSpace;
    /// number of values on the data stack when item was created
    int _stackBottom;
    /// number of values on the data stack before the current instruction processing
    int _stackLast;
    RString _scopeName;
};

struct RV_VMContext {
    uintptr_t vtable;
    CallStackItem** callStacks;
    int callStacksCount;
};

class RV_ScriptVM : public IDebugScript {
public:
    void debugPrint(std::string prefix);



    char pad_0x0000[0xC]; //0x0000
    RV_VMContext _context;
    char pad_0x0018[0x114]; //0x0018
    SourceDoc _doc; //0x0130 _doc
    SourceDocPos _docpos; //0x013C 
    char pad_0x0150[280]; //0x0150
    RString _displayName;
    bool _canSuspend;
    bool _localOnly;
    bool _undefinedIsNil;
    bool _exceptionThrown;
    bool _break;
    bool _breakOut;
};