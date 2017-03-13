#include "Debugger.h"
#include "VMContext.h"
#include <fstream>
#include "Script.h"
#include "Serialize.h"
#include <unordered_set>
#include <windows.h>
#include <unordered_map>
//std::array<const char*, 710> files{};
Debugger::Debugger() {
    //BreakPoint bp(8);
    //bp.filename = "z\\ace\\addons\\explosives\\functions\\fnc_setupExplosive.sqf";
    //bp.action = std::make_unique<BPAction_ExecCode>("systemChat \"hello guys\"");
    //JsonArchive test;
    //bp.Serialize(test);
    //auto text = test.to_string();
    //breakPoints.insert(std::move(bp));

    //monitors.push_back(std::make_shared<Monitor_knownScriptFiles>());




    /*
    while (!IsDebuggerPresent()) Sleep(1);
    struct keyedBP {
        RString _name;
        const char* getMapKey() const { return _name; }
    };
    std::vector<std::string> files2;
    for (auto& f : files) {
        files2.push_back(f);
    }
    for (int x = 0; x < 5; ++x) {
        uint64_t addT{ 0 };
        uint64_t resolve1T{ 0 };
        uint64_t resolve2T{ 0 };
        uint64_t resolveUnknownT{ 0 };
        for (int i = 0; i < 1000; ++i) {
            //MapStringToClassNonRV<keyedBP, std::vector<keyedBP>, MapStringToClassTraitCaseInsensitive> map;
            //MapStringToClassNonRV<keyedBP, std::vector<keyedBP>> map;
            std::unordered_map<RString, int> map;
            std::chrono::high_resolution_clock::time_point preAdd = std::chrono::high_resolution_clock::now();
            for (auto& f : files2) {
                //std::transform(f.begin(), f.end(), f.begin(), ::tolower);
                //map.insert(keyedBP{ f.c_str() });
                map[f] = 1;
            }
            std::chrono::high_resolution_clock::time_point postAdd = std::chrono::high_resolution_clock::now();
            for (auto& f : files2) {
                //std::transform(f.begin(), f.end(), f.begin(), ::tolower);
                //auto &found = map.get(f.c_str());
                //if (map.isNull(found))  __debugbreak();
                auto found = map.find(f);
                if (!(found != map.end())) __debugbreak();
            }
            std::chrono::high_resolution_clock::time_point resolve1 = std::chrono::high_resolution_clock::now();
            std::for_each(files2.rbegin(), files2.rend(), [&](std::string& f) {
                //std::transform(f.begin(), f.end(), f.begin(), ::tolower);
                //auto &found = map.get(f.c_str());
                //if (map.isNull(found))  __debugbreak();
                auto found = map.find(f);
                if (!(found != map.end())) __debugbreak();
            });
            std::chrono::high_resolution_clock::time_point resolve2 = std::chrono::high_resolution_clock::now();

            //auto &found = map.get("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
            //if (!map.isNull(found))  __debugbreak();
            auto found = map.find("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
            if (found != map.end()) __debugbreak();
            std::chrono::high_resolution_clock::time_point resolveUnknown = std::chrono::high_resolution_clock::now();

            addT += std::chrono::duration_cast<std::chrono::microseconds>(postAdd - preAdd).count();
            resolve1T += std::chrono::duration_cast<std::chrono::microseconds>(resolve1 - postAdd).count();
            resolve2T += std::chrono::duration_cast<std::chrono::microseconds>(resolve2 - resolve1).count();
            resolveUnknownT += std::chrono::duration_cast<std::chrono::microseconds>(resolveUnknown - resolve2).count();
        }
        OutputDebugStringA((std::string("addT ") + std::to_string(addT) + "\n").c_str());
        OutputDebugStringA((std::string("resolve1T ") + std::to_string(resolve1T) + "\n").c_str());
        OutputDebugStringA((std::string("resolve2T ") + std::to_string(resolve2T) + "\n").c_str());
        OutputDebugStringA((std::string("resolveUnknownT ") + std::to_string(resolveUnknownT) + "\n").c_str());
    }
  */ 
 /*
results:
    std::map<std::string, int> map; case sensitive
        addT 327.888
        resolve1T 431.802
        resolve2T 426.456
        resolveUnknownT 693

        addT 328.354
        resolve1T 464.537
        resolve2T 459.745
        resolveUnknownT 341

    std::map<RString, int> map; case insensitive

        addT 755.388
        resolve1T 697.654
        resolve2T 707.371
        resolveUnknownT 108

        addT 749.522
        resolve1T 690.555
        resolve2T 702.566
        resolveUnknownT 106

    std::map<RString, int> map; case sensitive

        addT 471.092
        resolve1T 395.438
        resolve2T 394.476
        resolveUnknownT 46

    MapStringToClassNonRV<keyedBP, std::vector<keyedBP>> map; case sensitive

        addT 515.686
        resolve1T 154.526
        resolve2T 156.115
        resolveUnknownT 9

    MapStringToClassNonRV<keyedBP, std::vector<keyedBP>, MapStringToClassTraitCaseInsensitive> map; Case insensitive

        addT 916.630
        resolve1T 363.788
        resolve2T 362.788
        resolveUnknownT 59

    std::unordered_map<std::string, int> map; case sensitive

        addT 386.028
        resolve1T 218.414
        resolve2T 220.328
        resolveUnknownT 570

    MapStringToClassNonRV<keyedBP, std::vector<keyedBP>> map; string tolower Case insensitive

        addT 694.363
        resolve1T 264.319
        resolve2T 265.026
        resolveUnknownT 13

    std::unordered_map<RString, int> map;
        addT 687.556
        resolve1T 443.118
        resolve2T 447.275
        resolveUnknownT 148


    Best resolve time while case insensitive MapClassToStringNonRV with tolowering all inputs
    Best resolve time while case sensitive MapClassToStringNonRV

*/


}


Debugger::~Debugger() {}

void Debugger::clear() {
    if (VMPtrToScript.empty()) return;
    std::vector<uintptr_t> to_delete;
    to_delete.reserve(VMPtrToScript.size());
    for (auto&it : VMPtrToScript) {
        if (it.second->canBeDeleted)
            to_delete.push_back(it.first);
    }
    if (to_delete.size() == VMPtrToScript.size()) {
        VMPtrToScript.clear();
        return;
    }
    for (auto it : to_delete)
        VMPtrToScript.erase(it);


}

std::shared_ptr<VMContext> Debugger::getVMContext(RV_VMContext* vm) {
    uintptr_t contentPtr = reinterpret_cast<uintptr_t>(vm);
    auto iter(VMPtrToScript.lower_bound(contentPtr));
    //Thank you SO! 
    if (iter == VMPtrToScript.end() || contentPtr < iter->first) {    // not found
        auto newEntry = std::make_shared<VMContext>();
        VMPtrToScript.insert(iter, { contentPtr, newEntry });// hinted insertion
        return newEntry;
    }
    return iter->second;
}

void Debugger::writeFrameToFile(uint32_t frameCounter) {

    std::ofstream file("P:\\" + std::to_string(frameCounter) + ".txt");

    if (file.is_open()) {

        for (auto& it : VMPtrToScript) {
            file << "VM " << it.second->totalRuntime.count() / 1000 << "us isVM:" << it.second->isScriptVM << "\n";

            for (auto& instruction : it.second->instructions) {
                uint16_t tabs = 1;
                instruction.writeToFile(file, tabs);
            }
        }
    }
    file.flush();
}

void Debugger::onInstruction(DebuggerInstructionInfo& instructionInfo) {
    instructionInfo.instruction->_scriptPos._sourceFile.lower();
    checkForBreakpoint(instructionInfo);

    for (auto& it : monitors)
        it->onInstruction(this, instructionInfo);



    auto instruction = instructionInfo.instruction;
    //auto context = getVMContext(instructionInfo.context);
    //context->addInstruction(instructionInfo.context, instruction);
    //auto script = context->getScriptByContent(instruction->_scriptPos._content);
    //script->dbg_instructionExec();
    //auto dbg = instruction->GetDebugName();
    //
    //
    //if (!instruction->_scriptPos._sourceFile.isNull())
    //    script->_fileName = instruction->_scriptPos._sourceFile;
    //auto callStackIndex = ctx->callStacksCount;
    //bool stackChangeImminent = dbg == "operator call" || dbg == "function call"; //This is a call instruction. This means next instruction will go into lower scope
    if (false && currentContext == scriptExecutionContext::EventHandler && instructionInfo.context->callStack.count() > 3) {
        JsonArchive ar;
        instructionInfo.context->Serialize(ar);
        auto text = ar.to_string();


    }



}

void Debugger::checkForBreakpoint(DebuggerInstructionInfo& instructionInfo) {

    if (breakPoints.empty()) return;
    //if (_strcmpi(instructionInfo.instruction->_scriptPos._sourceFile.data(), "z\\ace\\addons\\explosives\\functions\\fnc_setupExplosive.sqf") == 0)
    //    __debugbreak();
    auto &found = breakPoints.get(instructionInfo.instruction->_scriptPos._sourceFile.data());
    if (breakPoints.isNull(found) || found.empty()) return;
    for (auto& bp : found) {
        if (bp.line == instructionInfo.instruction->_scriptPos._sourceLine) {//#TODO move into breakPoint::trigger that returns bool if it triggered
            if (bp.condition && !bp.condition->isMatching(this, &bp, instructionInfo)) continue;
            bp.hitcount++;



            /*
                 JsonArchive varArchive;
                 JsonArchive nsArchive[4];
                 auto func = [](JsonArchive& nsArchive, const GameDataNamespace* var) {
                     var->_variables.forEach([&nsArchive](const GameVariable& var) {

                         JsonArchive variableArchive;

                         auto name = var._name;
                         if (var._value.isNull()) {
                             variableArchive.Serialize("type", "nil");
                             nsArchive.Serialize(name.data(), variableArchive);
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
                         nsArchive.Serialize(name.data(), variableArchive);

                     });
                 };

                 std::thread _1([&]() {func(nsArchive[0], instructionInfo.gs->_namespaces.get(0)); });
                 std::thread _2([&]() {func(nsArchive[1], instructionInfo.gs->_namespaces.get(1)); });
                 std::thread _3([&]() {func(nsArchive[2], instructionInfo.gs->_namespaces.get(2)); });
                 std::thread _4([&]() {func(nsArchive[3], instructionInfo.gs->_namespaces.get(3)); });


                 if (_1.joinable()) _1.join();
                 varArchive.Serialize(instructionInfo.gs->_namespaces.get(0)->_name.data(), nsArchive[0]);
                 if (_2.joinable()) _2.join();
                 varArchive.Serialize(instructionInfo.gs->_namespaces.get(1)->_name.data(), nsArchive[1]);
                 if (_3.joinable()) _3.join();
                 varArchive.Serialize(instructionInfo.gs->_namespaces.get(2)->_name.data(), nsArchive[2]);
                 if (_4.joinable()) _4.join();
                 varArchive.Serialize(instructionInfo.gs->_namespaces.get(3)->_name.data(), nsArchive[3]);

                 auto text = varArchive.to_string();
                 std::ofstream f("P:\\AllVars.json", std::ios::out | std::ios::binary);
                 f.write(text.c_str(), text.length());
                 f.close();



                 */




            if (bp.action) bp.action->execute(this, &bp, instructionInfo); //#TODO move into breakPoint::executeActions 

            //JsonArchive ar;
            //instructionInfo.context->Serialize(ar);
            //auto text = ar.to_string();
            //std::ofstream f("P:\\break.json", std::ios::out | std::ios::binary);
            //f.write(text.c_str(), text.length());
            //f.close();
            //std::ofstream f2("P:\\breakScript.json", std::ios::out | std::ios::binary);
            //f2.write(instructionInfo.instruction->_scriptPos._content.data(), instructionInfo.instruction->_scriptPos._content.length());
            //f2.close();
        }
    }
}

void Debugger::onShutdown() {
    for (auto& it : monitors)
        it->onShutdown();
}

void Debugger::onStartup() {
    nController.init();
    state = DebuggerState::running;
}

void Debugger::onHalt(HANDLE waitEvent, BreakPoint* bp, const DebuggerInstructionInfo& instructionInfo) {
    breakStateContinueEvent = waitEvent;
    state = DebuggerState::breakState;
    breakStateInfo.bp = bp;
    breakStateInfo.instruction = &instructionInfo;

    JsonArchive ar;
    ar.Serialize("command", 1); //#TODO make enum for this
    instructionInfo.context->Serialize(ar); //Set's callstack

    //#TODO add GameState variables

    JsonArchive instructionAr;
    instructionInfo.instruction->Serialize(instructionAr);
    ar.Serialize("instruction", instructionAr);


    auto text = ar.to_string();
    nController.sendMessage(text);


    std::ofstream f("P:\\break.json", std::ios::out | std::ios::binary);
    f.write(text.c_str(), text.length());
    f.close();
    std::ofstream f2("P:\\breakScript.json", std::ios::out | std::ios::binary);
    f2.write(instructionInfo.instruction->_scriptPos._content.data(), instructionInfo.instruction->_scriptPos._content.length());
    f2.close();






}

void Debugger::onContinue() {
    state = DebuggerState::running;
    breakStateContinueEvent = 0;
    breakStateInfo.bp = nullptr;
    breakStateInfo.instruction = nullptr;
}

void Debugger::commandContinue() {
    if (state != DebuggerState::breakState) return;
    ResetEvent(breakStateContinueEvent);
    SetEvent(breakStateContinueEvent);
}

const GameVariable * Debugger::getVariable(VariableScope scope, std::string varName) {
    if (state != DebuggerState::breakState) return nullptr; //#TODO make global NS getVar work without breakstate
    std::transform(varName.begin(), varName.end(), varName.begin(), ::tolower); //Has to be tolowered before
    if (scope & VariableScope::callstack) {
        auto var = breakStateInfo.instruction->context->getVariable(varName);
        if (var) return var;
    }
    if (scope & VariableScope::local) {
        auto var = breakStateInfo.instruction->gs;
    }
    if (scope & VariableScope::missionNamespace) {
        auto& varSpace = breakStateInfo.instruction->gs->_namespaces[3]->_variables;
        std::ofstream v("P:\\vars");
        varSpace.forEach([&varName, &v](const GameVariable& it) {
            if (it._name == varName.c_str()) __debugbreak();
            v << it._name.data() << "\n";
        });
        v.close();
        auto &var = varSpace.get(varName.c_str());
        if (!varSpace.isNull(var))
            return &var;
    }
    if (scope & VariableScope::uiNamespace) {
        auto& varSpace = breakStateInfo.instruction->gs->_namespaces[1]->_variables;
        auto &var = varSpace.get(varName.c_str());
        if (!varSpace.isNull(var))
            return &var;
    }





    return nullptr;
}
