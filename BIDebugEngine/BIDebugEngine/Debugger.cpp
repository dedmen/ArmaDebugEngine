#include "Debugger.h"
#include "VMContext.h"
#include <fstream>
#include "Script.h"
#include "Serialize.h"
#include <condition_variable>
#include "version.h"
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
    lastKnownGameState = instructionInfo.gs;
    JsonArchive ar;
    serializeScriptCommands(ar);
    std::ofstream f("P:\\funcs.json", std::ios::out | std::ios::binary);
    auto text = ar.to_string();
    f.write(text.c_str(), text.length());
    f.close();
    if (monitors.empty() && breakPoints.empty()) return;
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

void Debugger::onScriptError(GameState * gs) {
    BPAction_Halt hAction(haltType::error);
    hAction.execute(this, nullptr, DebuggerInstructionInfo{ nullptr,gs->_context ,gs });
}

void Debugger::onScriptAssert(GameState* gs) {
    BPAction_Halt hAction(haltType::assert);
    hAction.execute(this, nullptr, DebuggerInstructionInfo{ nullptr,gs->_context ,gs });
}

void Debugger::onScriptHalt(GameState* gs) {
    BPAction_Halt hAction(haltType::halt);
    hAction.execute(this, nullptr, DebuggerInstructionInfo{ nullptr,gs->_context ,gs });
}

void Debugger::checkForBreakpoint(DebuggerInstructionInfo& instructionInfo) {

    if (state == DebuggerState::stepState) {
        if (instructionInfo.context != stepInfo.context) { //Lost context. Can't step anymore
            //#TODO don't care about this if scriptVM stepping
            commandContinue(StepType::STContinue);
        }
        auto level = instructionInfo.context->callStack.count();
        if (level <= stepInfo.stepLevel &&
            //Prevent stepOver from triggering in the same Line
            (stepInfo.stepType == StepType::STOver ? stepInfo.stepLine != instructionInfo.instruction->_scriptPos._sourceLine : true)
            ) {
            BPAction_Halt hAction(haltType::step);
            hAction.execute(this, nullptr, instructionInfo);
            return; //We already halted here. Don't care if there are more breakpoints here.
        }
    }
    if (breakPoints.empty()) return;
    //if (_strcmpi(instructionInfo.instruction->_scriptPos._sourceFile.data(), "z\\ace\\addons\\explosives\\functions\\fnc_setupExplosive.sqf") == 0)
    //    __debugbreak();
    std::shared_lock<std::shared_mutex> lk(breakPointsLock);
    auto &found = breakPoints.get(instructionInfo.instruction->_scriptPos._sourceFile.data());
    if (breakPoints.isNull(found) || found.empty()) return;
    for (auto& bp : found) {
        if (bp.line == instructionInfo.instruction->_scriptPos._sourceLine) {
            lk.unlock();//if this BP halt's adding/deleting breakpoints should still be possible
            bp.trigger(this, instructionInfo);
            lk.lock();
        }

    }
}

void Debugger::onShutdown() {
    for (auto& it : monitors)
        it->onShutdown();
    nController.onShutdown();
}

void Debugger::onStartup() {
    nController.init();
    state = DebuggerState::running;
}

void Debugger::onHalt(std::shared_ptr<std::pair<std::condition_variable, bool>> waitEvent, BreakPoint* bp, const DebuggerInstructionInfo& instructionInfo, haltType type) {
    breakStateContinueEvent = waitEvent;
    state = DebuggerState::breakState;
    breakStateInfo.bp = bp;
    breakStateInfo.instruction = &instructionInfo;

    JsonArchive ar;
    switch (type) {
        case haltType::breakpoint:
            ar.Serialize("command", static_cast<int>(NC_OutgoingCommandType::halt_breakpoint));
            break;
        case haltType::step:
            ar.Serialize("command", static_cast<int>(NC_OutgoingCommandType::halt_step));
            break;
        case haltType::error:
            ar.Serialize("command", static_cast<int>(NC_OutgoingCommandType::halt_error));
            break;
        case haltType::assert:
            ar.Serialize("command", static_cast<int>(NC_OutgoingCommandType::halt_scriptAssert));
            break;
        case haltType::halt:
            ar.Serialize("command", static_cast<int>(NC_OutgoingCommandType::halt_scriptHalt));
            break;
        default: break;
    }
    if (instructionInfo.context)
        instructionInfo.context->Serialize(ar); //Set's callstack


    if (instructionInfo.instruction) {
        JsonArchive instructionAr;
        instructionInfo.instruction->Serialize(instructionAr);
        ar.Serialize("instruction", instructionAr);
    } else if (type == haltType::error) {
        JsonArchive errorAr;
        instructionInfo.gs->GEval->SerializeError(errorAr);
        ar.Serialize("error", errorAr);
    } else if (type == haltType::assert || type == haltType::halt) {
        JsonArchive assertAr;

        auto& _errorPosition = instructionInfo.context->_lastInstructionPos; //#BUG context could be nullptr

        assertAr.Serialize("fileOffset", { _errorPosition._sourceLine, _errorPosition._pos, Script::getScriptLineOffset(_errorPosition) });
        assertAr.Serialize("filename", _errorPosition._sourceFile);
        assertAr.Serialize("content", Script::getScriptFromFirstLine(_errorPosition));


        ar.Serialize("halt", assertAr);
    }





    auto text = ar.to_string();
    nController.sendMessage(text);
    if (!nController.isClientConnected()) commandContinue(StepType::STContinue);

#ifndef isCI
    std::ofstream f("P:\\break.json", std::ios::out | std::ios::binary);
    f.write(text.c_str(), text.length());
    f.close();
    std::ofstream f2("P:\\breakScript.json", std::ios::out | std::ios::binary);
    if (instructionInfo.instruction) {
        f2.write(instructionInfo.instruction->_scriptPos._content.data(), instructionInfo.instruction->_scriptPos._content.length());
    } else {
        f2.write(instructionInfo.gs->GEval->_errorPosition._content.data(), instructionInfo.gs->GEval->_errorPosition._content.length());
    }
    f2.close();
#endif





}

void Debugger::onContinue() {
    breakStateContinueEvent = 0;
    breakStateInfo.bp = nullptr;
    breakStateInfo.instruction = nullptr;
}

void Debugger::commandContinue(StepType stepType) {
    if (state == DebuggerState::running || state == DebuggerState::Uninitialized) return nController.sendMessage("{\"command\":8}");

    if (stepType == StepType::STContinue) {
        state = DebuggerState::running;
    } else {
        state = DebuggerState::stepState;
        stepInfo.stepType = stepType;
        if (!breakStateInfo.instruction || !breakStateInfo.instruction->instruction) {
            state = DebuggerState::running;
            goto jumpOut;
        }
        stepInfo.stepLine = breakStateInfo.instruction->instruction ? breakStateInfo.instruction->instruction->_scriptPos._sourceLine : breakStateInfo.instruction->context->_lastInstructionPos._sourceLine;
        stepInfo.context = breakStateInfo.instruction->context;

        switch (stepType) {
            case StepType::STInto:
                stepInfo.stepLevel = std::numeric_limits<decltype(stepInfo.stepLevel)>::max();
                break;
            case StepType::STOver:
                stepInfo.stepLevel = breakStateInfo.instruction->context->callStack.count();
                break;
            case StepType::STOut:
                stepInfo.stepLevel = breakStateInfo.instruction->context->callStack.count() - 1;
                break;
            default: break;
        }
    }
    jumpOut:
    if (breakStateContinueEvent) {
        breakStateContinueEvent->first.notify_all();
        breakStateContinueEvent->second = true;
    }
    nController.sendMessage("{\"command\":8}");//#TODO this breaks if Continue commands number changes
}

void Debugger::setGameVersion(const char* productType, const char* productVersion) {
    productInfo.gameType = productType;
    productInfo.gameVersion = productVersion;
}

void Debugger::SerializeHookIntegrity(JsonArchive& answer) {
    answer.Serialize("SvmCon", HI.__scriptVMConstructor);
    answer.Serialize("SvmSimSt", HI.__scriptVMSimulateStart);
    answer.Serialize("SvmSimEn", HI.__scriptVMSimulateEnd);
    answer.Serialize("InstrBP", HI.__instructionBreakpoint);
    answer.Serialize("WSim", HI.__worldSimulate);
    answer.Serialize("WMEVS", HI.__worldMissionEventStart);
    answer.Serialize("WMEVE", HI.__worldMissionEventEnd);
    answer.Serialize("ScrErr", HI.__onScriptError);
    answer.Serialize("PreDef", HI.scriptPreprocDefine);
    answer.Serialize("PreCon", HI.scriptPreprocConstr);
    answer.Serialize("ScrAass", HI.scriptAssert);
    answer.Serialize("ScrHalt", HI.scriptHalt);
    answer.Serialize("Alive", HI.engineAlive);
    answer.Serialize("EnMouse", HI.enableMouse);
}

void Debugger::onScriptEcho(RString msg) {
    OutputDebugString("echo: ");
    OutputDebugString(msg.data());
    OutputDebugString("\n");
}

void Debugger::serializeScriptCommands(JsonArchive& answer) {
    if (!lastKnownGameState) return;

    JsonArchive types;
    for (auto& type : lastKnownGameState->_scriptTypes) {
        JsonArchive entry;
        type->SerializeFull(entry);
        types.Serialize(type->_name, entry);
    }
    JsonArchive nulars;
    lastKnownGameState->_scriptNulars.forEach([&](const GameNular& gn) {
        JsonArchive entry;
        gn.Serialize(entry);
        nulars.Serialize(gn._operatorName, entry);
    });
    JsonArchive funcs;
    lastKnownGameState->_scriptFunctions.forEach([&](const GameFunctions& gn) {
        funcs.Serialize(gn._operatorName, gn);
    });
    JsonArchive ops;
    lastKnownGameState->_scriptOperators.forEach([&](const GameOperators& gn) {
        std::vector<JsonArchive> entries;
        ops.Serialize(gn._operatorName, gn);
    });
    answer.Serialize("nulars", nulars);
    answer.Serialize("functions", funcs);
    answer.Serialize("operators", ops);
    answer.Serialize("types", types);

}

std::vector<Debugger::VariableInfo> Debugger::getVariables(VariableScope scope, std::vector<std::string>& varNames) const {
    std::vector<Debugger::VariableInfo> output;
    if (state != DebuggerState::breakState) return output; //#TODO make global NS getVar work without breakstate

    for (auto& varName : varNames) {
        std::transform(varName.begin(), varName.end(), varName.begin(), ::tolower); //Has to be tolowered before
        bool found = false;
        if (varName.front() == '_') {
            if (false && scope & VariableScope::callstack) {
                //Only get's variable from last available scope
                auto var = breakStateInfo.instruction->context->getVariable(varName);
                if (var) {
                    output.emplace_back(var, VariableScope::callstack);
                    found = true;
                }

                //Use this to get the variable from all scopes
                //const GameVariable* value = nullptr;
                //breakStateInfo.instruction->context->callStack.forEachBackwards([&](const Ref<CallStackItem>& item) {
                //    auto var = item->_varSpace.getVariable(varName);
                //    if (var) {
                //        output.emplace_back(var, VariableScope::callstack);
                //        found = true;
                //        return false;
                //    }
                //    return false;
                //});
            }
            if (scope & VariableScope::local || scope & VariableScope::callstack) {
                if (breakStateInfo.instruction->gs->GEval->local) {
                    auto var = breakStateInfo.instruction->gs->GEval->local->getVariable(varName);
                    if (var) {
                        output.emplace_back(var, VariableScope::local);
                        found = true;
                    }
                }
            }
        } else {
            if (scope & VariableScope::missionNamespace) {
                auto& varSpace = breakStateInfo.instruction->gs->_namespaces[3]->_variables;
                //std::ofstream v("P:\\vars");
                //varSpace.forEach([&varName, &v](const GameVariable& it) {
                //    if (it._name == varName.c_str()) __debugbreak();
                //    v << it._name.data() << "\n";
                //});
                //v.close();
                auto &var = varSpace.get(varName.c_str());
                if (!varSpace.isNull(var)) {
                    output.emplace_back(&var, VariableScope::missionNamespace);
                    found = true;
                }

            }
            if (scope & VariableScope::uiNamespace) {
                auto& varSpace = breakStateInfo.instruction->gs->_namespaces[1]->_variables;
                auto &var = varSpace.get(varName.c_str());
                if (!varSpace.isNull(var)) {
                    output.emplace_back(&var, VariableScope::uiNamespace);
                    found = true;
                }
            }

            if (scope & VariableScope::profileNamespace) {

            }

            if (scope & VariableScope::parsingNamespace) {
                auto& varSpace = breakStateInfo.instruction->gs->_namespaces[2]->_variables;
                auto &var = varSpace.get(varName.c_str());
                if (!varSpace.isNull(var)) {
                    output.emplace_back(&var, VariableScope::parsingNamespace);
                    found = true;
                }
            }

        }
        //if (!found) {
        //    output.emplace_back(varName);
        //}
    }

    return output;
}

void Debugger::grabCurrentCode(JsonArchive& answer, const std::string& file) const {
    if (state != DebuggerState::breakState) {
        answer.Serialize("exception", "getCurrentCode: Not in breakState!");
        return;
    }
    if (breakStateInfo.instruction->instruction->_scriptPos._sourceFile != file.c_str()) {

        breakStateInfo.instruction->context->callStack.forEachBackwards([&](const Ref<CallStackItem>& item) {
            auto fileCode = item->tryGetFilenameAndCode();
            if (!fileCode._content.isNull() && fileCode._sourceFile == file.c_str()) {
                answer.Serialize("code", Script::getScriptFromFirstLine(fileCode));
                answer.Serialize("fileName", fileCode._sourceFile);
                return true;
            }
            return false;
        });

    }               
    answer.Serialize("code", Script::getScriptFromFirstLine(breakStateInfo.instruction->instruction->_scriptPos));
    answer.Serialize("fileName", breakStateInfo.instruction->instruction->_scriptPos._sourceFile);
}

void Debugger::VariableInfo::Serialize(JsonArchive& ar) const {
    if (!var) {
        ar.Serialize("type", "void");
        ar.Serialize("name", notFoundName);
    } else {
        ar.Serialize("name", var->_name);
        var->_value.Serialize(ar);
        ar.Serialize("ns", static_cast<int>(ns));
    }
}
extern std::string gameVersionResource;

void Debugger::productInfoStruct::Serialize(JsonArchive &ar) {
    ar.Serialize("gameType", gameType);

    if (gameVersion.isNull()) {
        gameVersion = gameVersionResource;
    }

    ar.Serialize("gameVersion", gameVersion);
}
