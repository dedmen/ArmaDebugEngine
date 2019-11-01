#pragma once

#include <cstdint>
#include "RVBaseTypes.h"
#include "RVIDebugInterface.h"
#include <array>
#include <types.hpp>
class JsonArchive;

void Serialize(const game_instruction&, JsonArchive& ar);

//inline char* rv_strdup(const char* in) {
//    auto len = strlen(in);
//    char* newMem = rv_allocator<char>::allocate(len + 1);
//    memcpy_s(newMem, len + 1, in, len);
//    newMem[len] = 0;
//    return newMem;
//}

class GameData : public intercept::types::game_data {
public:
    virtual bool get_as_bool() const { return false; }
    virtual float get_as_number() const { return 0.f; }
    virtual const r_string& get_as_string() const { static r_string dummy; dummy.clear(); return dummy; } ///Only usable on String and Code! Use to_string instead!
    virtual const auto_array<game_value>& get_as_const_array() const { static auto_array<game_value> dummy; dummy.clear(); return dummy; } //Why would you ever need this?
    virtual auto_array<game_value> &get_as_array() { static auto_array<game_value> dummy; dummy.clear(); return dummy; }
    virtual game_data *copy() const { return nullptr; }
    virtual void set_readonly(bool) {} //No clue what this does...
    virtual bool get_readonly() const { return false; }
    virtual bool get_final() const { return false; }
    virtual void set_final(bool) {} //Only on GameDataCode AFAIK
};

void Serialize(const GameData&, JsonArchive& ar);


class GameValue {
    uintptr_t vtable{ 0 };
public:
    intercept::types::ref<GameData> _data;
    GameValue() {};

    bool isNull() const { return _data.is_null(); }
};

void Serialize(const game_value&, JsonArchive& ar);

class CallStackItemSimple : public vm_context::callstack_item {
public:
    ref<compact_array<ref<game_instruction>>> _instructions;
    int _currentInstruction;
    sourcedoc _content;
    bool _multipleInstructions;
};

class DummyVClass {
public:
    virtual void Dummy() {};
};

class CallStackItemData : public vm_context::callstack_item {
public:
    intercept::types::ref<game_data_code> _code; // compiled code
    int _ip; // instruction pointer
};

class CallStackItemArrayForEach : public vm_context::callstack_item {
public:
    intercept::types::ref<game_data_code> _code; // compiled code
    intercept::types::ref<game_data_array> _array;
    int _forEachIndex; 
};

class CallStackItemForBASIC : public vm_context::callstack_item {
public:
    r_string _varName;
    float _varValue;
    float _to;
    float _step;
    intercept::types::ref<game_data_code> _code; // compiled code
};

class CallStackItemApply : public vm_context::callstack_item {
public:
    intercept::types::ref<game_data_code> _code; // compiled code
    intercept::types::ref<game_data_array> _array;
    intercept::types::ref<game_data_array> _array2; //output array maybe
    int _forEachIndex;
};

class CallStackItemArrayFindCond : public vm_context::callstack_item {
public:
    intercept::types::ref<game_data_code> _code; // compiled code
    intercept::types::ref<game_data_array> _array;
    int _forEachIndex;
};







struct RV_VMContext : public vm_context {

    const game_variable* getVariable(std::string varName);
    void Serialize(JsonArchive& ar);
};


class RV_ScriptVM : public IDebugScript {
public:
    void debugPrint(std::string prefix);



    char pad_0x0000[3 * sizeof(uintptr_t)]; //0x0000
    RV_VMContext _context;
    //#TODO X64 for vars below here
    char pad_0x0150[280]; //0x0150
    r_string _displayName;
    bool _canSuspend;
    bool _1;
    bool _undefinedIsNil;
    bool _2;
    bool _3;
    bool _4;
};

sourcedocpos tryGetFilenameAndCode(const intercept::types::vm_context::callstack_item& it);
void Serialize(const GameData& gd, JsonArchive& ar);
void Serialize(const game_value& gv, JsonArchive& ar);
void SerializeError(const intercept::types::game_state::game_evaluator&, JsonArchive& ar);

//Intercept  https://github.com/intercept/intercept/pull/108

void SerializeFull(const script_type_info& t, JsonArchive &ar);
void Serialize(const script_type_info& t, JsonArchive &ar);

class RVScriptType : public DummyVClass {
public:
    const script_type_info* _type{ nullptr };
    class CompoundGameType : public auto_array<const script_type_info *>, public DummyVClass {

    };
    const CompoundGameType* _compoundType{ nullptr };

    RVScriptType() {}
    void Serialize(JsonArchive& ar);
    std::vector<std::string> getTypeNames() const;
};

class NularOperator : public intercept::types::refcount {
public:
    NularOperator() {}
    void* _funcPtr{ nullptr };
    RVScriptType _returnType;
    void Serialize(JsonArchive &ar);
};

class UnaryOperator : public intercept::types::refcount {
public:
    UnaryOperator() {}
    void* _funcPtr{ nullptr };
    RVScriptType _returnType;
    RVScriptType _rightArgumentType;
    void Serialize(JsonArchive &ar);
};

class BinaryOperator : public intercept::types::refcount {
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
    r_string _operatorName;
private:
    std::array<size_t,
#if _WIN64 || __X86_64__
        10
#else
#ifdef __linux__
        8
#else
        11
#endif
#endif
    > securityStuff{};  //Will scale with x64
                        //size_t securityStuff[11];
};

class ScriptCmdInfo {
public:
    r_string _description;
    r_string _example;
    r_string _exampleResult;
    r_string _addedInVersion;
    r_string _changed;
    r_string _category;
    void Serialize(JsonArchive &ar) const;
};

struct GameNular : public GameOperatorNameBase {
public:
    r_string _name; //0x24
    intercept::types::ref<NularOperator> _operator;
    ScriptCmdInfo _info;
    void *_nullptr{ nullptr };

    GameNular() {}

    const char *getMapKey() const { return _name.c_str(); }
    void Serialize(JsonArchive &ar) const;
};

struct GameFunction : public GameOperatorNameBase {
    uint32_t placeholder9;//0x24
public:
    r_string _name;
    intercept::types::ref<UnaryOperator> _operator;
    r_string _rightOperatorDescription;
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
    r_string _name;
    GamePriority _priority; //Don't use Enum the higher the number the higher the prio//#TODO remove enum
    intercept::types::ref<BinaryOperator> _operator;
    r_string _leftOperatorDescription;
    r_string _rightOperatorDescription;
    ScriptCmdInfo _info;

    GameOperator() {}
    void Serialize(JsonArchive &ar) const;
};

namespace intercept::__internal {
    class gsNular : public GameNular {};

    class game_functions : public auto_array<GameFunction>, public GameOperatorNameBase {
    public:
        game_functions(std::string name) : _name(name.c_str()) {}
        r_string _name;
        game_functions() noexcept {}
        const char *get_map_key() const noexcept { return _name.data(); }
    };

    class game_operators : public auto_array<GameFunction>, public GameOperatorNameBase {
    public:
        game_operators(std::string name) : _name(name.c_str()) {}
        r_string _name;
        int32_t placeholder10{ 4 }; //0x2C Small int 0-5  priority
        game_operators() noexcept {}
        const char *get_map_key() const noexcept { return _name.data(); }
    };
}









using GameState = intercept::types::game_state;
