#pragma once
#include "RVClasses.h"
#include <string>

class JsonArchive;
class IDebugValue;

typedef IRef<IDebugValue> IDebugValueRef;

class IDebugValue {
public:
    IDebugValue() {}
    virtual ~IDebugValue() {}

    // reference counting
    virtual int IaddRef() = 0;
    virtual int Irelease() = 0;

    virtual void getTypeString(char *buffer, int len) const = 0;
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
class GameValue;
class GameData : public RefCount, public IDebugValue {   //#TODO move to right headerfile
public:

    virtual const void* getGameType() const { return nullptr; } //Don't call
    virtual ~GameData() {}

    virtual bool getBool() const { return false; }
    virtual float getNumber() const { return 0; }
    virtual RString getString() const { return ""; } //Don't use this! use getAsString instead!
    virtual const AutoArray<GameValue> &getArray() const { return AutoArray<GameValue>(); }
    virtual void _1() const { }//Don't call this
    virtual void _2() const {}//Don't call thiss
    virtual void setReadOnly(bool val) {}
    //! check read only status
    virtual bool isReadOnly() const { return false; }

    virtual bool isFinal() const { return false; }
    virtual void setFinal(bool val) {};

    virtual RString getAsString() const { return ""; }
    virtual bool isEqualTo(const GameData *data) const { return false; };

    virtual const char *getTypeString() const { return ""; }
    virtual bool isNil() const { return false; }

    void Serialize(JsonArchive& ar) const;


};

class GameValue {
    uintptr_t vtable {0};
public:
    Ref<GameData> _data;
    GameValue() {};
    void Serialize(JsonArchive& ar) const;
    bool isNull() const { return _data.isNull(); }
};


class GameVariable : public IDebugVariable {
public:
    RString _name;
    GameValue _value;
    bool _readOnly{ false };

    GameVariable() {};
    const char *getMapKey() const { return _name; }
};

class CallStackItem : public RefCount, public IDebugScope {
public:
    CallStackItem* _parent;
    struct GameVarSpace {
        uintptr_t vtable;
        MapStringToClass<GameVariable, AutoArray<GameVariable> > _variables;
        GameVarSpace *_parent;
        bool _1;
    } _varSpace;
    /// number of values on the data stack when item was created
    int _stackBottom;
    /// number of values on the data stack before the current instruction processing
    int _stackLast;
    RString _scopeName;
};

class CallStackItemSimple : public CallStackItem {
public:
    AutoArray<Ref<RV_GameInstruction>> _instructions;
    int _currentInstruction;
    SourceDoc _content;
    bool _multipleInstructions;
};


struct GameCodeType { //#TODO can I inline this?
    RString _string;
    AutoArray<Ref<RV_GameInstruction>> _code;
    bool _compiled;
};

class GameDataCode : public GameData {
public:
    GameCodeType _instructions;
    bool _final;
};

class CallStackItemData : public CallStackItem {
public:
    Ref<GameDataCode> _code; // compiled code
    int _ip; // instruction pointer
};

struct RV_VMContext {
    uintptr_t vtable;
    Array<Ref<CallStackItem>> callStack; //max 64 items

    void Serialize(JsonArchive& ar);
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