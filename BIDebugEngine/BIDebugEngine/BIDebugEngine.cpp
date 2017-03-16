//BI Debug Engine Interface


#include "BIDebugEngine.h"
#include <windows.h>
#include "EngineHook.h"
#include <thread>
static DllInterface dllIface;
static EngineInterface* engineIface;
extern "C" EngineHook GlobalEngineHook;

uintptr_t engineAlloc;

#include "NamedPipeServer.h"


void DllInterface::Startup() {
    GlobalEngineHook.onStartup();
}

void DllInterface::Shutdown() {
    GlobalEngineHook.onShutdown();
}

void DllInterface::ScriptLoaded(IDebugScript *script, const char *name) {

}

void DllInterface::ScriptEntered(IDebugScript *script) {

}

void DllInterface::ScriptTerminated(IDebugScript *script) {

}

void DllInterface::FireBreakpoint(IDebugScript *script, unsigned int bp) {

}

void DllInterface::Breaked(IDebugScript *script) {

}

void DllInterface::DebugEngineLog(const char *str) {

}

DllInterface* Connect(EngineInterface *engineInt) {
    WAIT_FOR_DEBUGGER_ATTACHED;
    engineIface = engineInt;
    return &dllIface;
}
char VarBuffer[1024];
void IDebugScope::printAllVariables() {


    auto varC = varCount();
    std::vector<IDebugVariable*> vars;
    vars.resize(varC);
    IDebugVariable** vbase = vars.data();
    getVariables(const_cast<const IDebugVariable**>(vbase), varC);

    for (auto& var : vars) {
        var->getName(VarBuffer, 1023);
        std::string name(VarBuffer);
        auto value = var->getValue();
        if (!value) continue;
        value->getTypeString(VarBuffer, 1023);
        std::string valueType(VarBuffer);
        value->getValue(10, VarBuffer, 1023);
        OutputDebugStringA((name + " " + valueType + ": " + std::string(VarBuffer) + "\n").c_str());
    }
}

std::string IDebugScope::allVariablesToString() {
    std::string output;
    auto varC = varCount();
    std::vector<IDebugVariable*> vars;
    vars.resize(varC);
    IDebugVariable** vbase = vars.data();
    getVariables(const_cast<const IDebugVariable**>(vbase), varC);

    for (auto& var : vars) {
        var->getName(VarBuffer, 1023);
        std::string name(VarBuffer);
        auto value = var->getValue();
        if (!value) continue;
        value->getTypeString(VarBuffer, 1023);
        std::string valueType(VarBuffer);
        if (valueType == "code" || valueType == "Array") {
            output += (name + " : " + valueType + "; ");
            continue;
        }
        value->getValue(10, VarBuffer, 1023);
        output += (name + "=" + std::string(VarBuffer) + " : " + valueType + "; ");
    }
    return output;
}

void RV_ScriptVM::debugPrint(std::string prefix) {
    std::string title = _displayName;
    std::string filename = _doc._fileName.isNull() ? _docpos._sourceFile : _doc._fileName;
    std::string data;// = _doc._content.data();
    OutputDebugStringA((prefix + " " + title + "F " + filename + " " + data + "\n").c_str());
}
#include "Serialize.h"

const GameVariable* RV_VMContext::getVariable(std::string varName) {
    const GameVariable* value = nullptr;
    callStack.forEachBackwards([&value, &varName](const Ref<CallStackItem>& item) {
        auto var = item->_varSpace.getVariable(varName);
        if (var) {
            value = var;
            return true;
        }
        return false;
    });
    return value;
}

void RV_VMContext::Serialize(JsonArchive& ar) {


    ar.Serialize("callstack", callStack, [](JsonArchive& ar, const Ref<CallStackItem>& item) {
        auto& type = typeid(*item.get());
        const auto typeName = type.name();
        ar.Serialize("type", typeName);
        auto hash = type.hash_code();
        {
            JsonArchive varArchive;

            item->_varSpace._variables.forEach([&varArchive](const GameVariable& var) {

                JsonArchive variableArchive;

                auto name = var._name;
                if (var._value.isNull()) {
                    variableArchive.Serialize("type", "nil");
                    varArchive.Serialize(name.data(), variableArchive);
                    return;
                }
                auto value = var._value._data;
                const auto type = value->getTypeString();

                variableArchive.Serialize("type", type);
                if (strcmp(type, "array") == 0) {
                    variableArchive.Serialize("value", value->getArray());
                } else {
                    variableArchive.Serialize("value", value->getAsString());
                }
                varArchive.Serialize(name.data(), variableArchive);

            });
            ar.Serialize("variables", varArchive);
        }

        switch (hash) {

            case 0xed08ac32: { //CallStackItemSimple
                auto stackItem = static_cast<const CallStackItemSimple*>(item.get());
                ar.Serialize("fileName", stackItem->_content._fileName);
                ar.Serialize("contentSample", stackItem->_content._content.substr(0, 100));
                ar.Serialize("ip", stackItem->_currentInstruction);
                ar.Serialize("lastInstruction", *(stackItem->_instructions.get(stackItem->_currentInstruction - 1)));
                //ar.Serialize("instructions", stackItem->_instructions);
            }   break;

            case 0x224543d0: { //CallStackItemData
                auto stackItem = static_cast<const CallStackItemData*>(item.get());

                ar.Serialize("ip", stackItem->_ip);
                ar.Serialize("final", stackItem->_code->_final);
                ar.Serialize("compiled", stackItem->_code->_instructions._compiled);
                ar.Serialize("contentSample", stackItem->_code->_instructions._string.substr(0, 100)); //#TODO send whole code over

                ar.Serialize("lastInstruction", *(stackItem->_code->_instructions._code.get(stackItem->_ip - 1)));
                //ar.Serialize("instructions", stackItem->_code->_instructions._code);
            }   break;
            case 0x254c4241: { //CallStackItemArrayForEach

            } break;
            default:
                //__debugbreak(); 
                return;
        }
    });
}

void GameData::Serialize(JsonArchive& ar) const {
    const auto type = getTypeString();
    ar.Serialize("type", type);
    if (strcmp(type, "array") == 0) {
        ar.Serialize("value", getArray());
    } else {
        ar.Serialize("value", getAsString());
    }
}

void GameValue::Serialize(JsonArchive& ar) const {
    _data->Serialize(ar);
}

const GameVariable* GameVarSpace::getVariable(const std::string& varName) const {
    auto & var = _variables.get(varName.c_str());
    if (!_variables.isNull(var)) {
        return &var;
    }
    if (_parent) {
        return _parent->getVariable(varName);
    }
    return nullptr;
}
