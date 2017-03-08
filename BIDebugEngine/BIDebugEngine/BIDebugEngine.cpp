//BI Debug Engine Interface


#include "BIDebugEngine.h"
#include <windows.h>
#include "EngineHook.h"
static DllInterface dllIface;
static EngineInterface* engineIface;
extern EngineHook GlobalEngineHook;
uintptr_t engineAlloc;

void DllInterface::Startup() {  
    GlobalEngineHook.placeHooks();    
}

void DllInterface::Shutdown() {
    Sleep(10);
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
    while (!IsDebuggerPresent()) Sleep(1);
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
        if(valueType == "code" || valueType == "Array") {
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
    OutputDebugStringA((prefix + " " + title + "F " + filename +" "+ data + "\n").c_str());
}
#include "Serialize.h"
void RV_VMContext::Serialize(JsonArchive& ar) {


    ar.Serialize("callstack", callStack, [](JsonArchive& ar, const Ref<CallStackItem>& item) {
        auto& type = typeid(*item.get());
        auto typeName = type.name();
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
                auto type = value->getTypeString();
                
                variableArchive.Serialize("type", type);
                if (strcmp(type,"array") == 0) {
                    variableArchive.Serialize("value", value->getArray());
                } else {
                    variableArchive.Serialize("value", value->getAsString());
                }
                varArchive.Serialize(name.data(), variableArchive);

            });


             /*

            auto varC = item->varCount();
            std::vector<IDebugVariable*> vars;
            vars.resize(varC);
            IDebugVariable** vbase = vars.data();
            item->getVariables(const_cast<const IDebugVariable**>(vbase), varC);

            
            for (auto& var : vars) {
                auto value = var->getValue();
                if (!value) continue;
                value->getTypeString(VarBuffer, 1023);
                std::string valueType(VarBuffer);
                std::string valueText;
                JsonArchive variableArchive;
                variableArchive.Serialize("type", valueType);
                if (value->isArray()) {
                    std::vector<std::string> values;
                    for (int i = 0; i < value->itemCount(); ++i) {
                        value->getItem(i)->getValue(10, VarBuffer, 1023);
                        values.push_back(std::string(VarBuffer));
                    }
                    variableArchive.Serialize("value", values);
                } else {
                    value->getValue(10, VarBuffer, 1023);
                    variableArchive.Serialize("value", std::string(VarBuffer));
                }

                varArchive.Serialize(var->_name.data(), variableArchive);
   
            }  */
            ar.Serialize("variables", varArchive);
        }







        //#TODO serialize all variables

        switch (hash) {

            case 0xed08ac32: { //CallStackItemSimple
                auto stackItem = static_cast<const CallStackItemSimple*>(item.get());
                ar.Serialize("fileName", stackItem->_content._fileName);
                ar.Serialize("contentSample", stackItem->_content._content.substr(0, 100));
                ar.Serialize("ip", stackItem->_currentInstruction);
                ar.Serialize("lastInstruction", *(stackItem->_instructions.get(stackItem->_currentInstruction-1)));
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
    auto type = getTypeString();
    ar.Serialize("type", type);
    if (strcmp(type, "array") == 0) {
        ar.Serialize("value",getArray());           
    } else {
        ar.Serialize("value", getAsString());
    }
}

void GameValue::Serialize(JsonArchive& ar) const{
    _data->Serialize(ar);
}
