#include "EngineHook.h"
#include <windows.h>
#include "BIDebugEngine.h"
#include "Debugger.h"
#include "Script.h"
#include "VMContext.h"
#include <sstream>
#include "Serialize.h"

bool inScriptVM;
extern "C" EngineHook GlobalEngineHook;
EngineHook GlobalEngineHook;
Debugger GlobalDebugger;
std::chrono::high_resolution_clock::time_point globalTime; //This is the total time NOT spent inside the debugger
std::chrono::high_resolution_clock::time_point frameStart; //Time at framestart
std::chrono::high_resolution_clock::time_point lastContextExit;

#define OnlyOneInstructionPerLine

class globalTimeKeeper {
public:
    globalTimeKeeper() {
        globalTime += std::chrono::high_resolution_clock::now() - lastContextExit;
    }
    ~globalTimeKeeper() {
        lastContextExit = std::chrono::high_resolution_clock::now();
    }
};

EngineHook::EngineHook() {
    engineBase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));

}

EngineHook::~EngineHook() {}

extern "C" {
uintptr_t scriptVMConstructorJmpBack;
uintptr_t scriptVMSimulateStartJmpBack;
uintptr_t instructionBreakpointJmpBack;
uintptr_t worldSimulateJmpBack;
uintptr_t worldMissionEventStartJmpBack;
uintptr_t worldMissionEventEndJmpBack;

uintptr_t hookEnabled_Instruction{ 1 };
uintptr_t hookEnabled_Simulate{ 1 };
uintptr_t scriptVM;
uintptr_t currentScriptVM;
}



EngineAlive* EngineAliveFnc;
EngineEnableMouse* EngineEnableMouseFnc;

//x64 assembly http://lallouslab.net/2016/01/11/introduction-to-writing-x64-assembly-in-visual-studio/

//#define passSimulateScriptVMPtr  // This is too crashy right now. Don't know why. Registers look alright ### Need to define this in masm preproc
#ifdef  passSimulateScriptVMPtr
#error "hookEnabled_Simulate may kill engine if it's disabled after simulateStart and before simulateEnd"
#endif

extern "C" void scriptVMConstructor();
extern "C" void scriptVMSimulateStart();
extern "C" void scriptVMSimulateEnd();
extern "C" void instructionBreakpoint();
extern "C" void worldSimulate();
extern "C" void worldMissionEventStart();
extern "C" void worldMissionEventEnd();


scriptExecutionContext currentContext = scriptExecutionContext::Invalid;
MissionEventType currentEventHandler = MissionEventType::Ended; //#TODO create some invalid handler type


void EngineHook::placeHooks() {
    WAIT_FOR_DEBUGGER_ATTACHED;
    if (!_hooks[static_cast<std::size_t>(hookTypes::scriptVMConstructor)]) {
        scriptVMConstructorJmpBack = placeHook(0x10448BE, reinterpret_cast<uintptr_t>(scriptVMConstructor)) + 3;
        _hooks[static_cast<std::size_t>(hookTypes::scriptVMConstructor)] = true;
    }
    if (!_hooks[static_cast<std::size_t>(hookTypes::scriptVMSimulateStart)]) {
        scriptVMSimulateStartJmpBack = placeHook(0x1044E80, reinterpret_cast<uintptr_t>(scriptVMSimulateStart)) + 1;
        _hooks[static_cast<std::size_t>(hookTypes::scriptVMSimulateStart)] = true;
    }
    if (!_hooks[static_cast<std::size_t>(hookTypes::scriptVMSimulateEnd)]) {
        placeHook(0x10451A3, reinterpret_cast<uintptr_t>(scriptVMSimulateEnd));
        _hooks[static_cast<std::size_t>(hookTypes::scriptVMSimulateEnd)] = true;
    }
    if (!_hooks[static_cast<std::size_t>(hookTypes::instructionBreakpoint)]) {
        instructionBreakpointJmpBack = placeHook(0x103C610, reinterpret_cast<uintptr_t>(instructionBreakpoint)) + 1;
        _hooks[static_cast<std::size_t>(hookTypes::instructionBreakpoint)] = true;
    }
    if (!_hooks[static_cast<std::size_t>(hookTypes::worldSimulate)]) {
        worldSimulateJmpBack = placeHook(0x00B5ED90, reinterpret_cast<uintptr_t>(worldSimulate)) + 1;
        _hooks[static_cast<std::size_t>(hookTypes::worldSimulate)] = true;
    }
    if (!_hooks[static_cast<std::size_t>(hookTypes::worldMissionEventStart)]) {
        worldMissionEventStartJmpBack = placeHook(0x00B19E5C, reinterpret_cast<uintptr_t>(worldMissionEventStart)) + 2;
        _hooks[static_cast<std::size_t>(hookTypes::worldMissionEventStart)] = true;
    }
    if (!_hooks[static_cast<std::size_t>(hookTypes::worldMissionEventEnd)]) {
        worldMissionEventEndJmpBack = placeHook(0x00B1A0AB, reinterpret_cast<uintptr_t>(worldMissionEventEnd)) + 1;
        _hooks[static_cast<std::size_t>(hookTypes::worldMissionEventEnd)] = true;
    }
    bool* isDebuggerAttached = reinterpret_cast<bool*>(engineBase + 0x206F310);
    *isDebuggerAttached = false; //Small hack to keep RPT logging while Debugger is attached
    //Could also patternFind and patch (profv3 0107144F) to unconditional jmp

    EngineAliveFnc = reinterpret_cast<EngineAlive*>(engineBase + 0x10454B0);
    //Find by searching for.  "XML parsing error: cannot read the source file". function call right after start of while loop
   
    EngineEnableMouseFnc = reinterpret_cast<EngineEnableMouse*>(engineBase + 0x1159250);

    //To yield scriptVM and let engine run while breakPoint hit. 0103C5BB overwrite eax to Yield









	/*
	
	 char** to product version string
	x????xxx?x????xx????xxxx?xxx????xxxx?xxxxx?xxxxxxxxxxx?xxxxxxxx?xx????xxxxx?
\x68\x00\x00\x00\x00\x8d\x44\x24\x00\x68\x00\x00\x00\x00\x50\xe8\x00\x00\x00\x00\x50\x8d\x44\x24\x00\x57\x50\xe8\x00\x00\x00\x00\x8b\x17\x83\xc4\x00\x8b\x08\x85\xc9\x74\x00\x8b\xc3\xf0\x0f\xc1\x01\x89\x0f\x85\xd2\x74\x00\x8b\xc6\xf0\x0f\xc1\x02\x48\x75\x00\x8b\x0d\x00\x00\x00\x00\x52\x8b\x01\xff\x50\x00
	*/


}

void EngineHook::removeHooks(bool leavePFrameHook) {


}

void EngineHook::_worldSimulate() {
    static uint32_t frameCounter = 0;
    frameCounter++;
    //OutputDebugStringA(("#Frame " + std::to_string(frameCounter) + "\n").c_str());
    //for (auto& it : GlobalDebugger.VMPtrToScript) {
    //    OutputDebugStringA("\t");
    //    OutputDebugStringA((std::to_string(it.second->totalRuntime.count()) + "ns " + std::to_string(it.second->isScriptVM) + "\n").c_str());
    //    for (auto& it2 : it.second->contentPtrToScript) {
    //        if (it2.second->_fileName.empty()) continue;
    //        OutputDebugStringA("\t\t");
    //        OutputDebugStringA((it2.second->_fileName + " " + std::to_string(it2.second->instructionCount)).c_str());
    //        OutputDebugStringA("\n");
    //    }
    //}
    //OutputDebugStringA("#EndFrame\n");
    //bool logFrame = false;
    //if (logFrame || frameCounter % 1000 == 0)
    //    GlobalDebugger.writeFrameToFile(frameCounter);


    GlobalDebugger.clear();
    globalTime = std::chrono::high_resolution_clock::now();
    frameStart = globalTime;
}

void EngineHook::_scriptLoaded(uintptr_t scrVMPtr) {
    globalTimeKeeper _tc;
    auto scVM = reinterpret_cast<RV_ScriptVM *>(scrVMPtr);
    //scVM->debugPrint("Load");
    auto myCtx = GlobalDebugger.getVMContext(&scVM->_context);
    myCtx->isScriptVM = true;
    //myCtx->canBeDeleted = false; //Should reimplement that again sometime. This causes scriptVM's to be deleted and loose their upper callstack every frame
}

void EngineHook::_scriptEntered(uintptr_t scrVMPtr) {
    globalTimeKeeper _tc;
    auto scVM = reinterpret_cast<RV_ScriptVM *>(scrVMPtr);
    //scVM->debugPrint("Enter");
    currentContext = scriptExecutionContext::scriptVM;

    auto context = GlobalDebugger.getVMContext(&scVM->_context);
    auto script = context->getScriptByContent(scVM->_doc._content);
    if (!scVM->_doc._fileName.isNull() || !scVM->_docpos._sourceFile.isNull())
        script->_fileName = scVM->_doc._fileName.isNull() ? scVM->_docpos._sourceFile : scVM->_doc._fileName;
    context->dbg_EnterContext();
}

void EngineHook::_scriptTerminated(uintptr_t scrVMPtr) {
    globalTimeKeeper _tc;
    auto scVM = reinterpret_cast<RV_ScriptVM *>(scrVMPtr);
    GlobalDebugger.getVMContext(&scVM->_context)->dbg_LeaveContext();
    auto myCtx = GlobalDebugger.getVMContext(&scVM->_context);
    //scVM->debugPrint("Term " + std::to_string(myCtx->totalRuntime.count()));
    if (scVM->_context.callStack.count() - 1 > 0) {
        auto scope = scVM->_context.callStack.back();
        scope->printAllVariables();
    }
    myCtx->canBeDeleted = true;
    currentContext = scriptExecutionContext::Invalid;
}

void EngineHook::_scriptLeft(uintptr_t scrVMPtr) {
    globalTimeKeeper _tc;
    auto scVM = reinterpret_cast<RV_ScriptVM *>(scrVMPtr);
    GlobalDebugger.getVMContext(&scVM->_context)->dbg_LeaveContext();
    //scVM->debugPrint("Left");
    //if (scVM->_context.callStacksCount - 1 > 0) {
    //    auto scope = scVM->_context.callStacks[scVM->_context.callStacksCount - 1];
    //    scope->printAllVariables();
    //}
    currentContext = scriptExecutionContext::Invalid;
}
uintptr_t lastCallstackIndex = 0;

#ifdef OnlyOneInstructionPerLine
uint16_t lastInstructionLine;
const char* lastInstructionFile;
#endif

void EngineHook::_scriptInstruction(uintptr_t instructionBP_Instruction, uintptr_t instructionBP_VMContext, uintptr_t instructionBP_gameState, uintptr_t instructionBP_IDebugScript) {
    globalTimeKeeper _tc;
    auto start = std::chrono::high_resolution_clock::now();

    auto instruction = reinterpret_cast<RV_GameInstruction *>(instructionBP_Instruction);
    auto ctx = reinterpret_cast<RV_VMContext *>(instructionBP_VMContext);
    auto gs = reinterpret_cast<GameState *>(instructionBP_gameState);
#ifdef OnlyOneInstructionPerLine
    if (instruction->_scriptPos._sourceLine != lastInstructionLine || instruction->_scriptPos._content.data() != lastInstructionFile) {
#endif
        GlobalDebugger.onInstruction(DebuggerInstructionInfo{ instruction, ctx, gs });
#ifdef OnlyOneInstructionPerLine
        lastInstructionLine = instruction->_scriptPos._sourceLine;
        lastInstructionFile = instruction->_scriptPos._content.data();
    }
#endif       


    //if (dbg == "const \"cba_help_VerScript\"") __debugbreak();
    //bool isNew = instruction->IsNewExpression();
    //if (script->_fileName.find("tfar") != std::string::npos) {
    //    
    //    auto line = instruction->_scriptPos._sourceLine;
    //    auto offset = instruction->_scriptPos._pos;   
    //    OutputDebugStringA((std::string("instruction L") + std::to_string(line) + " O" + std::to_string(offset) + " " + dbg.data() + " " + std::to_string(isNew) + "\n").c_str());
    //    if (lastCallstackIndex != callStackIndex)
    //        OutputDebugStringA(("stack " + script->_fileName + "\n").c_str());
    //}
    //lastCallstackIndex = callStackIndex;
    //
    //if (inScriptVM) {
    //    auto dbg = instruction->GetDebugName();
    //    auto line = instruction->_scriptPos._sourceLine;
    //    auto offset = instruction->_scriptPos._pos;
    //    OutputDebugStringA((std::string("instruction L") + std::to_string(line) + " O" + std::to_string(offset) + " " + dbg.data() + "\n").c_str());
    //    context->dbg_instructionTimeDiff(std::chrono::high_resolution_clock::now() - start);
    //}
}

void EngineHook::_world_OnMissionEventStart(uintptr_t eventType) {
    currentContext = scriptExecutionContext::EventHandler;
    currentEventHandler = static_cast<MissionEventType>(eventType);
}

void EngineHook::_world_OnMissionEventEnd() {
    currentContext = scriptExecutionContext::Invalid;
}

void EngineHook::onShutdown() {
    GlobalDebugger.onShutdown();
}

void EngineHook::onStartup() {
    placeHooks();
    GlobalDebugger.onStartup();
}

uintptr_t EngineHook::placeHook(uintptr_t offset, uintptr_t jmpTo) const {
    auto totalOffset = offset + engineBase;

    DWORD dwVirtualProtectBackup;
    VirtualProtect(reinterpret_cast<LPVOID>(totalOffset), 5u, 0x40u, &dwVirtualProtectBackup);
    auto jmpInstr = reinterpret_cast<unsigned char *>(totalOffset);
    auto addrOffs = reinterpret_cast<unsigned int *>(totalOffset + 1);
    *jmpInstr = 0xE9;
    *addrOffs = jmpTo - totalOffset - 5;
    VirtualProtect(reinterpret_cast<LPVOID>(totalOffset), 5u, dwVirtualProtectBackup, &dwVirtualProtectBackup);

    return totalOffset + 5;
}