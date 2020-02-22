#include "RVClasses.h"
#include "Serialize.h"
#include "Script.h"
#include <windows.h>

void Serialize(const game_instruction& in, JsonArchive& ar) {

    auto& type = typeid(in);
    //#TODO X64 may on this didn't try but I expect it to
    const auto typeName = ((&type) && ((__std_type_info_data*) &type)->_UndecoratedName) ? type.name() : "TypeFAIL";
    ar.Serialize("type", typeName);
    ar.Serialize("name", in.get_name());
    auto space = in.sdp.sourcefile.find("[");
    auto properPath = (space != std::string::npos) ? in.sdp.sourcefile.substr(0, space-1) : in.sdp.sourcefile;

    ar.Serialize("filename", (std::string)properPath);

    ar.Serialize("fileOffset", { in.sdp.sourceline, in.sdp.pos, Script::getScriptLineOffset(in.sdp) });

}


void RV_ScriptVM::debugPrint(std::string prefix) {
    std::string title = static_cast<std::string>(_displayName);
    std::string filename = static_cast<std::string>(_context.sdoc.sourcefile.empty() ? _context.sdocpos.sourcefile : _context.sdoc.sourcefile);
    std::string data;// = _doc._content.data();
    OutputDebugStringA((prefix + " " + title + "F " + filename + " " + data + "\n").c_str());
}
#include "Serialize.h"

const game_variable* RV_VMContext::getVariable(std::string varName) {
    const game_variable* value = nullptr;
    callstack.for_each_backwards([&value, &varName](const ref<callstack_item>& item) {
        auto var = item->_varSpace.get_variable(varName);
        if (var) {
            value = var;
            return true;
        }
        return false;
    });
    return value;
}

void RV_VMContext::Serialize(JsonArchive& ar) {


    ar.Serialize("callstack", callstack, [](JsonArchive& ar, const ref<callstack_item>& item) {
        auto& type = typeid(*item.get());
        //#TODO X64 fails on this
        const auto typeName = ((&type) && ((__std_type_info_data*) &type)->_UndecoratedName) ? type.name() : "TypeFAIL";

        ar.Serialize("type", typeName);
        auto hash = type.hash_code();
        {
            JsonArchive varArchive;

            item->_varSpace.variables.for_each([&varArchive](const game_variable& var) {

                JsonArchive variableArchive;

                auto name = var.name;
                if (var.value.is_null()) {
                    variableArchive.Serialize("type", "nil");
                    varArchive.Serialize(name.data(), variableArchive);
                    return;
                }
                auto value = var.value.data;
                const auto type = value->type_as_string();

                variableArchive.Serialize("type", type);
                if (strcmp(type, "array") == 0) {
                    variableArchive.Serialize("value", var.value.to_array());
                } else {
                    variableArchive.Serialize("value", static_cast<r_string>(var.value));
                }
                varArchive.Serialize(name.data(), variableArchive);

            });
            ar.Serialize("variables", varArchive);
        }

        switch (hash) {
            case CallStackItemSimple::type_hash: {
                auto stackItem = static_cast<const CallStackItemSimple*>(item.get());
                ar.Serialize("fileName", stackItem->_content.sourcefile);
                ar.Serialize("contentSample", (std::string)stackItem->_content.content.substr(0, 100));
                ar.Serialize("ip", stackItem->_currentInstruction);
                JsonArchive arInst;
                ::Serialize(*(stackItem->_instructions.get(stackItem->_currentInstruction - 1)), arInst);
                ar.Serialize("lastInstruction", arInst);
                //ar.Serialize("instructions", stackItem->_instructions);
            }   break;

            case CallStackItemData::type_hash: {
                auto stackItem = static_cast<const CallStackItemData*>(item.get());

                ar.Serialize("ip", stackItem->_ip);
                ar.Serialize("final", stackItem->_code->is_final);
                ar.Serialize("compiled", stackItem->_code->instructions);
                ar.Serialize("contentSample", (std::string)stackItem->_code->code_string.substr(0, 100)); //#TODO send whole code over
                JsonArchive arInst;
                ::Serialize(*(stackItem->_code->instructions.get(stackItem->_ip - 1)), arInst);
                ar.Serialize("lastInstruction", arInst);
                //ar.Serialize("instructions", stackItem->_code->_instructions._code);
            }   break;

            case CallStackItemArrayForEach::type_hash: {
                auto stackItem = static_cast<const CallStackItemArrayForEach*>(item.get());

                ar.Serialize("forEachIndex", stackItem->_forEachIndex);
            } break;

            case CallStackItemApply::type_hash: {
                auto stackItem = static_cast<const CallStackItemApply*>(item.get());
                ar.Serialize("forEachIndex", stackItem->_forEachIndex);
            } break;

            case CallStackRepeat::type_hash: {
            } break;

            case CallStackItemConditionSelect::type_hash: {
                auto stackItem = static_cast<const CallStackItemConditionSelect*>(item.get());
                //Yes, that's a different name, but both have the same layout.
                ar.Serialize("forEachIndex", stackItem->_forEachIndex);
            } break;

            case CallStackItemForBASIC::type_hash: {
                auto stackItem = static_cast<const CallStackItemForBASIC*>(item.get());
                ar.Serialize("varName", stackItem->_varName);
                ar.Serialize("varValue", stackItem->_varValue);
                ar.Serialize("to", stackItem->_to);
                ar.Serialize("step", stackItem->_step);

            } break;

            case CallStackItemFor::type_hash: {
            } break;

            case CallStackItemArrayFindCond::type_hash: {
                auto stackItem = static_cast<const CallStackItemArrayFindCond*>(item.get());
                ar.Serialize("forEachIndex", stackItem->_forEachIndex);
            } break;

            default:
                //__debugbreak(); 
                break;
        }
    });
}

void Serialize(const GameData& gd, JsonArchive& ar) {
    const auto type = std::string_view(gd.type_as_string());
    ar.Serialize("type", type.data());

    if (type == "array"sv) {
        ar.Serialize("value", gd.get_as_const_array());
    } else if (type == "code") {
        sourcedocpos x;
        x.content = gd.get_as_string();
        ar.Serialize("value", "<code>"); //Script::getScriptFromFirstLine(x, true)
    } else if (type == "float") {
        ar.Serialize("value", gd.get_as_number());
    } else if (type == "bool") {
        ar.Serialize("value", gd.get_as_bool());
    } else if (type == "string") {
        ar.Serialize("value", gd.get_as_string());
    } else {
        ar.Serialize("value", gd.to_string());
    }
}

void Serialize(const game_value& gv, JsonArchive& ar) {
    if (gv.data)
        Serialize(*static_cast<const GameData*>(gv.data.get()), ar);
    else {
        ar.Serialize("type", "void");
        ar.Serialize("value", "NIL");
    }
}
void SerializeError(const intercept::types::game_state::game_evaluator& e,JsonArchive& ar) {
    ar.Serialize("fileOffset", std::vector<uint32_t>{ e._errorPosition.sourceline, e._errorPosition.pos, Script::getScriptLineOffset(e._errorPosition) });

    ar.Serialize("type", e._errorType);
    //#TODO errorType enum
    /*
    "OK"
    .rdata:01853BD0 aGen            db 'GEN',0              ; DATA XREF: sub_D78D0:loc_D79D5o
    .rdata:01853BD4 aExpo           db 'EXPO',0
    .rdata:01853BDC aNum            db 'NUM',0              ; DATA XREF: sub_D78D0:loc_D7BB9o
    .rdata:01853BE0 aVar_0          db 'VAR',0              ; DATA XREF: sub_D78D0:loc_D7CADo
    .rdata:01853BE4 aBad_var        db 'BAD_VAR',0          ; DATA XREF: sub_D78D0:loc_D7DA1o
    .rdata:01853BEC aDiv_zero       db 'DIV_ZERO',0         ; DATA XREF: sub_D78D0+5C8o
    .rdata:01853BF8 aTg90           db 'TG90',0             ; DATA XREF: sub_D78D0+61Eo
    .rdata:01853C00 aOpenb          db 'OPENB',0            ; DATA XREF: sub_D78D0+674o
    .rdata:01853C08 aCloseb         db 'CLOSEB',0           ; DATA XREF: sub_D78D0+6CAo
    .rdata:01853C10 aOpen_brackets  db 'OPEN_BRACKETS',0    ; DATA XREF: sub_D78D0+720o
    .rdata:01853C20 aClose_brackets db 'CLOSE_BRACKETS',0   ; DATA XREF: sub_D78D0+776o
    .rdata:01853C30 aOpen_braces    db 'OPEN_BRACES',0      ; DATA XREF: sub_D78D0+7CCo
    .rdata:01853C3C aClose_braces   db 'CLOSE_BRACES',0     ; DATA XREF: sub_D78D0+822o
    .rdata:01853C4C aEqu            db 'EQU',0              ; DATA XREF: sub_D78D0+872o
    .rdata:01853C50 aSemicolon      db 'SEMICOLON',0        ; DATA XREF: sub_D78D0+8C8o
    .rdata:01853C5C aQuote          db 'QUOTE',0            ; DATA XREF: sub_D78D0+91Eo
    .rdata:01853C64 aSingle_quote   db 'SINGLE_QUOTE',0     ; DATA XREF: sub_D78D0+974o
    .rdata:01853C74 aLine_long      db 'LINE_LONG',0        ; DATA XREF: sub_D78D0+A20o
    .rdata:01853C80 aNamespace_0    db 'NAMESPACE',0        ; DATA XREF: sub_D78D0+AC9o
    .rdata:01853C8C aDim            db 'DIM',0              ; DATA XREF: sub_D78D0+B1Fo
    .rdata:01853C90 aUnexpected_clo db 'UNEXPECTED_CLOSEB',0 ; DATA XREF: sub_D78D0+B75o
    .rdata:01853CA4 aAssertation_fa db 'ASSERTATION_FAILED',0 ; DATA XREF: sub_D78D0+BCBo
    .rdata:01853CB8 aHalt_function  db 'HALT_FUNCTION',0    ; DATA XREF: sub_D78D0+C21o
    .rdata:01853CC8 aForeign        db 'FOREIGN',0          ; DATA XREF: sub_D78D0+C77o
    .rdata:01853CD0 aScope_name_def db 'SCOPE_NAME_DEFINED_TWICE',0 ; DATA XREF: sub_D78D0+CCCo
    .rdata:01853CEC aScope_not_foun db 'SCOPE_NOT_FOUND',0  ; DATA XREF: sub_D78D0+D1Fo
    .rdata:01853CFC aInvalid_try_bl db 'INVALID_TRY_BLOCK',0 ; DATA XREF: sub_D78D0+D72o
    .rdata:01853D10 aUnhandled_exce db 'UNHANDLED_EXCEPTION',0 ; DATA XREF: sub_D78D0+DC5o
    .rdata:01853D24 aStack_overflow db 'STACK_OVERFLOW',0   ; DATA XREF: sub_D78D0+E18o
    .rdata:01853D34 aHandled        db 'HANDLED',0
    */



    ar.Serialize("message", e._errorMessage);
    ar.Serialize("filename", e._errorPosition.sourcefile);
    ar.Serialize("content", Script::getScriptFromFirstLine(e._errorPosition));
}

void GameNular::Serialize(JsonArchive &ar) const {
    //ar.Serialize("name", _name);
    JsonArchive op;
    _operator->Serialize(op);
    ar.Serialize("op", op);
    _info.Serialize(ar);
}

void GameFunction::Serialize(JsonArchive &ar) const {
    //ar.Serialize("name", _name);
    JsonArchive op;
    _operator->Serialize(op);
    ar.Serialize("op", op);
    ar.Serialize("rname", _rightOperatorDescription);
    _info.Serialize(ar);
}

void Serialize(const intercept::__internal::gsNular& n,JsonArchive &ar) {
    //ar.Serialize("name", _name);
    JsonArchive op;
    n._operator->Serialize(op);
    ar.Serialize("op", op);
    n._info.Serialize(ar);
}

void GameOperator::Serialize(JsonArchive &ar) const {
    //ar.Serialize("name", _name);
    JsonArchive op;
    _operator->Serialize(op);
    ar.Serialize("op", op);
    ar.Serialize("rArgDesc", _rightOperatorDescription);
    ar.Serialize("lArgDesc", _leftOperatorDescription);
    ar.Serialize("priority", _priority);
    _info.Serialize(ar);
}

void SerializeFull(const script_type_info& t, JsonArchive &ar) {
    ar.Serialize("name", t._name);
    ar.Serialize("typeName", t._typeName);
    ar.Serialize("readableName", t._readableName);
    ar.Serialize("desc", t._description);
    ar.Serialize("cat", t._category);
    //ar.Serialize("sig", _javaTypeLong);
    //ar.Serialize("j", _javaTypeShort);
    ar.Serialize("name", t._name);
}

void Serialize(const script_type_info& t, JsonArchive &ar) {
    ar.Serialize("type", t._name);
}

void NularOperator::Serialize(JsonArchive &ar) {
    ar.Serialize("retT", _returnType.getTypeNames());
}

void BinaryOperator::Serialize(JsonArchive &ar) {
    ar.Serialize("retT", _returnType.getTypeNames());
    ar.Serialize("argL", _leftArgumentType.getTypeNames());
    ar.Serialize("argR", _rightArgumentType.getTypeNames());
}

void UnaryOperator::Serialize(JsonArchive &ar) {
    ar.Serialize("retT", _returnType.getTypeNames());
    ar.Serialize("argT", _rightArgumentType.getTypeNames());
}

void RVScriptType::Serialize(JsonArchive& ar) {
    ar.Serialize("types", getTypeNames());
}

std::vector<std::string> RVScriptType::getTypeNames() const {
    std::vector<std::string> ars;//#TODO should return r_string

    if (_type) {
        ars.push_back((std::string)_type->_name);
    }
    if (_compoundType) {
        for (auto& it : *_compoundType) {
            ars.push_back((std::string)it->_name);
        }
    }
    return ars;
}

void ScriptCmdInfo::Serialize(JsonArchive &ar) const {
    ar.Serialize("desc", _description);
    ar.Serialize("ex", _example);
    ar.Serialize("exres", _exampleResult);
    ar.Serialize("since", _addedInVersion);
    ar.Serialize("cat", _category);
}

sourcedocpos tryGetFilenameAndCode(const intercept::types::vm_context::callstack_item& it) {

    auto& type = typeid(*&it);
    //#TODO X64 fails on this
    __debugbreak(); //Some place already has these
    const auto typeName = ((&type) && ((__std_type_info_data*) &type)->_UndecoratedName) ? type.name() : "TypeFAIL";
    auto hash = type.hash_code();

    switch (hash) {
        case CallStackItemSimple::type_hash: {
            auto stackItem = static_cast<const CallStackItemSimple*>(&it);
            //#TODO test if this is the correct instruction or if i should -1 this
            const auto& lastInstruction = (stackItem->_instructions.get(stackItem->_currentInstruction))->sdp;
            return lastInstruction;
        }   break;

        case CallStackItemData::type_hash: { //CallStackItemData
            auto stackItem = static_cast<const CallStackItemData*>(&it);
            auto & lastInstruction = (stackItem->_code->instructions.get(stackItem->_ip - 1))->sdp;
            return lastInstruction;
        }   break;

        case CallStackItemArrayForEach::type_hash: { //CallStackItemArrayForEach

        } break;
    }

    return sourcedocpos();
}
