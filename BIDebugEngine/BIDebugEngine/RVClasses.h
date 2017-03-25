#pragma once

#include <cstdint>
#include "RVContainer.h"
#include "RVIDebugInterface.h"
class JsonArchive;

class RV_GameInstruction : public RefCount {
public:
    virtual ~RV_GameInstruction() {}
    virtual bool Execute(uintptr_t gameStatePtr, uintptr_t VMContextPtr) { return false; };
    virtual int GetStackChange(uintptr_t _ptr) { return 0; };
    virtual bool IsNewExpression() { return false; }
    virtual RString GetDebugName() { return RString(); };

    void Serialize(JsonArchive& ar);

    SourceDocPos _scriptPos;
};


//inline char* rv_strdup(const char* in) {
//    auto len = strlen(in);
//    char* newMem = rv_allocator<char>::allocate(len + 1);
//    memcpy_s(newMem, len + 1, in, len);
//    newMem[len] = 0;
//    return newMem;
//}


class GameValue;
class GameData : public RefCount, public IDebugValue {   //#TODO move to right headerfile
public:
    static AutoArray<GameValue> emptyGVAutoArray;
    virtual const void* getGameType() const { return nullptr; } //Don't call
    virtual ~GameData() {}

    virtual bool getBool() const { return false; }
    virtual float getNumber() const { return 0; }
    virtual RString getString() const { return ""; } //Don't use this! use getAsString instead!
    virtual const AutoArray<GameValue> &getArray() const { return emptyGVAutoArray; }
    virtual void _1() const {}//Don't call this
    virtual void _2() const {}//Don't call this
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
    uintptr_t vtable{ 0 };
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

struct GameVarSpace {
    uintptr_t vtable;
    MapStringToClass<GameVariable, AutoArray<GameVariable> > _variables;
    GameVarSpace *_parent;
    const GameVariable* getVariable(const std::string& varName) const;
    bool _1;
};

class CallStackItem : public RefCount, public IDebugScope {
public:
    CallStackItem* _parent;
    GameVarSpace _varSpace;
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

class GameDataCode : public GameData {
public:
    struct {
        RString _string;
        AutoArray<Ref<RV_GameInstruction>> _code;
        bool _compiled;
    } _instructions;
    bool _final;
};

class DummyVClass {
public:
    virtual void Dummy() {};
};


class GameDataNamespace : public GameData, public DummyVClass {
public:
    MapStringToClass<GameVariable, AutoArray<GameVariable> > _variables;
    RString _name;
    bool _1;
};

class CallStackItemData : public CallStackItem {
public:
    Ref<GameDataCode> _code; // compiled code
    int _ip; // instruction pointer
};

struct RV_VMContext {
    uintptr_t vtable;
    Array<Ref<CallStackItem>> callStack; //max 64 items
#ifdef X64
    char pad_0x0018[0x220]; //0x0018
#else
    char pad_0x0018[0x114]; //0x0018
#endif
    SourceDoc _doc;
    SourceDocPos _lastInstructionPos;

    const GameVariable* getVariable(std::string varName);
    void Serialize(JsonArchive& ar);
};


class RV_ScriptVM : public IDebugScript {
public:
    void debugPrint(std::string prefix);



    char pad_0x0000[3 * sizeof(uintptr_t)]; //0x0000
    RV_VMContext _context;
    //#TODO X64 for vars below here
    char pad_0x0150[280]; //0x0150
    RString _displayName;
    bool _canSuspend;
    bool _1;
    bool _undefinedIsNil;
    bool _2;
    bool _3;
    bool _4;
};

class GameEvaluator : public RefCount {
public:
    GameVarSpace *local;  // local variables
    int           handle; // for debug purposes to test the Begin/EndContext matching pairs

    bool _1;
    bool _2;
    uint32_t _errorType;
    RString _errorMessage;
    SourceDocPos _errorPosition;
    void SerializeError(JsonArchive& ar);
};
#ifdef X64
class GameState {
public:
    //virtual void _NOU() {} //GameState has no vtable since 1.68 (it had one in 1.66)
    AutoArray<void*> _1;
    MapStringToClass<void*, AutoArray<void*>> _2; //functions  Should consult Intercept on these. 
    MapStringToClass<void*, AutoArray<void*>> _3; //operators
    MapStringToClass<void*, AutoArray<void*>> _4; //nulars  //#TODO add DebugBreak script nular. Check Visor
    char _5[0x220];
    Ref<GameEvaluator> GEval;
    Ref<GameDataNamespace> _globalNamespace; //Can change by https://community.bistudio.com/wiki/with
                                             /*
                                             default,
                                             ui,
                                             parsing,
                                             mission
                                             */
    AutoArray<Ref<GameDataNamespace>> _namespaces; //Contains missionNamespace and uiNamespace
    bool _6;
    bool _showScriptErrors; //-showScriptErrors start parameter

    RV_VMContext *_context;	//Current context
                            //AutoArray<callStackItemCreator> _callStackItems;
};
#else
class GameState;
//class callStackItemCreator {
//public:
//    callStackItemCreator() {}
//
//    CallStackItem*(*createFunction)(CallStackItem* parent, GameVarSpace* parentVariables, const GameState* gs, bool serialization) { nullptr };
//    RString type;
//};

class GameState {
public:
    //virtual void _NOU() {} //GameState has no vtable since 1.68 (it had one in 1.66)
    AutoArray<void*> _1;
    MapStringToClass<void*, AutoArray<void*>> _2; //functions  Should consult Intercept on these. 
    MapStringToClass<void*, AutoArray<void*>> _3; //operators
    MapStringToClass<void*, AutoArray<void*>> _4; //nulars  //#TODO add DebugBreak script nular. Check Visor
    char _5[0x114];
    Ref<GameEvaluator> GEval;
    Ref<GameDataNamespace> _globalNamespace; //Can change by https://community.bistudio.com/wiki/with
                                             /*
                                             default,
                                             ui,
                                             parsing,
                                             mission
                                             */
    AutoArray<Ref<GameDataNamespace>> _namespaces; //Contains missionNamespace and uiNamespace


    bool _6;
    bool _showScriptErrors; //-showScriptErrors start parameter

    RV_VMContext *_context;	//Current context
                            //AutoArray<callStackItemCreator> _callStackItems;
};
#endif