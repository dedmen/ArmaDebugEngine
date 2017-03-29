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
    SourceDocPos tryGetFilenameAndCode();
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


//Intercept  https://github.com/intercept/intercept/pull/108

class RVScriptTypeInfo {
public:
    RString _name;
    void* _createFunction{ nullptr };
    RString _typeName;
    RString _readableName;
    RString _description;
    RString _category;
    RString _javaTypeShort;
    RString _javaTypeLong;

    void SerializeFull(JsonArchive &ar) const;
    void Serialize(JsonArchive &ar) const;
};


class RVScriptType : public DummyVClass {
public:
    const RVScriptTypeInfo* _type{ nullptr };
    class CompoundGameType : public AutoArray<const RVScriptTypeInfo *>, public DummyVClass {

    };
    const CompoundGameType* _compoundType{ nullptr };

    RVScriptType() {}
    void Serialize(JsonArchive& ar);
    std::vector<std::string> getTypeNames() const;
};

class NularOperator : public RefCount {
public:
    NularOperator() {}
    void* _funcPtr{ nullptr };
    RVScriptType _returnType;
    void Serialize(JsonArchive &ar);
};

class UnaryOperator : public RefCount {
public:
    UnaryOperator() {}
    void* _funcPtr{ nullptr };
    RVScriptType _returnType;
    RVScriptType _rightArgumentType;
    void Serialize(JsonArchive &ar);
};

class BinaryOperator : public RefCount {
public:
    BinaryOperator() {}
    void* _funcPtr{ nullptr };
    RVScriptType _returnType;
    RVScriptType _leftArgumentType;
    RVScriptType _rightArgumentType;
    void Serialize(JsonArchive &ar);
};

struct GameOperatorNameBase {
public:
    RString _operatorName;
private:
    uint32_t placeholder1;//0x4
    uint32_t placeholder2;//0x8 actually a pointer to empty memory
    uint32_t placeholder3;//0xC
    uint32_t placeholder4;//0x10
    uint32_t placeholder5;//0x14
    uint32_t placeholder6;//0x18  
    uint32_t placeholder7;//0x1C
    uint32_t placeholder8;//0x20
};

class ScriptCmdInfo {
public:
    RString _description;
    RString _example;
    RString _exampleResult;
    RString _addedInVersion;
    RString _changed;
    RString _category;
    void Serialize(JsonArchive &ar) const;
};

struct GameNular : public GameOperatorNameBase {
public:
    RString _name; //0x24
    Ref<NularOperator> _operator;
    ScriptCmdInfo _info;
    void *_nullptr{ nullptr };

    GameNular() {}

    const char *getMapKey() const { return _name; }
    void Serialize(JsonArchive &ar) const;
};

struct GameFunction : public GameOperatorNameBase {
    uint32_t placeholder9;//0x24
public:
    RString _name;
    Ref<UnaryOperator> _operator;
    RString _rightOperatorDescription;
    ScriptCmdInfo _info;

    GameFunction() {}
    void Serialize(JsonArchive &ar) const;
};

enum GamePriority {
    priority, testy
};

struct GameOperator : public GameOperatorNameBase {
    uint32_t placeholder9;//0x24
public:
    RString _name;
    GamePriority _priority; //Don't use Enum the higher the number the higher the prio//#TODO remove enum
    Ref<BinaryOperator> _operator;
    RString _leftOperatorDescription;
    RString _rightOperatorDescription;
    ScriptCmdInfo _info;

    GameOperator() {}
    void Serialize(JsonArchive &ar) const;
};


struct GameFunctions : public AutoArray<GameFunction>, public GameOperatorNameBase {
public:
    RString _name;
    GameFunctions() {}
    const char *getMapKey() const { return _name; }
};

struct GameOperators : public AutoArray<GameOperator>, public GameOperatorNameBase {
public:
    RString _name;
    GamePriority _priority; //Don't use Enum the higher the number the higher the prio//#TODO remove enum
    GameOperators() {}
    const char *getMapKey() const { return _name; }
};

#ifdef X64
class GameState {
public:
    //virtual void _NOU() {} //GameState has no vtable since 1.68 (it had one in 1.66)
    AutoArray<const RVScriptTypeInfo *> _scriptTypes;
    MapStringToClass<GameFunctions, AutoArray<GameFunctions>> _scriptFunctions; //functions  Should consult Intercept on these. 
    MapStringToClass<GameOperators, AutoArray<GameOperators>> _scriptOperators; //operators
    MapStringToClass<GameNular, AutoArray<GameNular>> _scriptNulars; //nulars  //#TODO add DebugBreak script nular. Check Visor
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
    AutoArray<const RVScriptTypeInfo *> _scriptTypes;
    MapStringToClass<GameFunctions, AutoArray<GameFunctions>> _scriptFunctions; //functions  Should consult Intercept on these. 
    MapStringToClass<GameOperators, AutoArray<GameOperators>> _scriptOperators; //operators
    MapStringToClass<GameNular, AutoArray<GameNular>> _scriptNulars; //nulars  //#TODO add DebugBreak script nular. Check Visor
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