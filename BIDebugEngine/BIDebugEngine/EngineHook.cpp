#include "EngineHook.h"
#include <windows.h>
#include "BIDebugEngine.h"
#include "Debugger.h"
#include "Script.h"
#include "VMContext.h"

bool inScriptVM;
EngineHook GlobalEngineHook;
Debugger GlobalDebugger;
std::chrono::high_resolution_clock::time_point globalTime; //This is the total time NOT spent inside the debugger
std::chrono::high_resolution_clock::time_point frameStart; //Time at framestart
std::chrono::high_resolution_clock::time_point lastContextExit;

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
    
uintptr_t scriptVMConstructorJmpBack;
uintptr_t scriptVMSimulateStartJmpBack;
uintptr_t instructionBreakpointJmpBack;
uintptr_t worldSimulateJmpBack;
uintptr_t worldMissionEventStartJmpBack;
uintptr_t worldMissionEventEndJmpBack;

uintptr_t hookEnabled_Instruction{ 0 };
uintptr_t hookEnabled_Simulate{ 0 };
uintptr_t scriptVM;
_declspec(naked) void scriptVMConstructor() {
    __asm {
        push edi; //scriptVM Pointer
        mov ecx, offset GlobalEngineHook;
        call EngineHook::_scriptLoaded;
        //_return:
        push    1; //Fixup
        lea eax, [edi + 0x298];
        jmp scriptVMConstructorJmpBack;
    }
}
uintptr_t currentScriptVM;
//#define passSimulateScriptVMPtr  // This is too crashy right now. Don't know why. Registers look alright
#ifdef  passSimulateScriptVMPtr
#error "hookEnabled_Simulate may kill engine if it's disabled after simulateStart and before simulateEnd"
#endif
_declspec(naked) void scriptVMSimulateStart() {
    __asm {
#ifndef passSimulateScriptVMPtr
        mov currentScriptVM, ecx; // use this in case of scriptVM ptr not being easilly accessible in SimEnd
#endif
        push    eax;
        push    ecx;

        mov     eax, hookEnabled_Simulate;//Skip if hook is disabled
        test    eax, eax;
        jz      _return;

        push    ecx; //_scriptEntered arg
        mov     ecx, offset GlobalEngineHook;
        call    EngineHook::_scriptEntered;
    _return:
        pop     ecx;
        pop     eax;
        sub     esp, 34h; //Fixup
        push    edi;
        mov     edi, ecx;
#ifdef passSimulateScriptVMPtr
        cmp     byte ptr[edi + 0x2A0], 0;//if !Loaded we exit right away and never hit scriptVMSimulateEnd
        jz _skipVMPush;
        push edi; //scriptVM to receive again in scriptVMSimulateEnd
    _skipVMPush:
#endif
        jmp scriptVMSimulateStartJmpBack;
    }
}
_declspec(naked) void scriptVMSimulateEnd() {
    __asm {
        push    eax;
        push    ecx;
        push    edx;

        mov     ecx, hookEnabled_Simulate;//Skip if hook is disabled
        test    ecx, ecx;
        jz      _return;

        //prepare arguments for func call
#ifdef passSimulateScriptVMPtr
        mov     edi, [esp + 0xC + 0x4/*I added push edx*/]; //Retrieve out pushed scriptVM ptr
#else
        mov     edi, currentScriptVM; //use this in case of scriptVM ptr not being easilly accessible 
#endif
        push    edi; //scriptVM
        mov     ecx, offset GlobalEngineHook;
        test    al, al;//al == done
        jz      short _notDone;//script is not Done  
        call    EngineHook::_scriptTerminated;//script is Done
        jmp     short _return;
    _notDone:
        call    EngineHook::_scriptLeft;
    _return:
        pop     edx;
        pop     ecx; //These are probably not needed. But I can't guarantee that the compiler didn't expect these to stay unchanged
        pop     eax;
#ifdef passSimulateScriptVMPtr
        pop     edi; //Remove our pushed scriptVM ptr
#endif
        pop     ebp;//Fixup
        pop     edi;
        add     esp, 34h;
        retn    8;
    }
}

_declspec(naked) void instructionBreakpoint() {
    __asm {
        //mov instructionBP_gameState, ebp;
        //mov instructionBP_VMContext, edi;
        //mov instructionBP_Instruction, ebx;
        //push    eax; don't need to keep because get's overwritten by fixup
        push    ecx;
        mov     ecx, hookEnabled_Instruction;//Skip if hook is disabled
        test    ecx, ecx;
        jz      _return;
        mov     eax, [esp + 0x14C]; //instructionBP_IDebugScript
        push    eax;//instructionBP_IDebugScript
        push    ebp; //instructionBP_gameState
        push    edi; //instructionBP_VMContext
        push    ebx; //instructionBP_Instruction
        mov     ecx, offset GlobalEngineHook;
        call    EngineHook::_scriptInstruction;
    _return:
        pop     ecx;
        //pop     eax;
        mov     eax, [ebx + 14h];//fixup
        lea     edx, [ebx + 14h];
        jmp instructionBreakpointJmpBack;
    }
}

_declspec(naked) void worldSimulate() {
    __asm {
        push ecx;
        push eax;
        mov     ecx, offset GlobalEngineHook;
        call    EngineHook::_worldSimulate;
        pop     eax; //Don't know if eax will be modified but it's likely
        pop     ecx;
        sub     esp, 0x3D8;//fixup
        jmp worldSimulateJmpBack;
    }
}

_declspec(naked) void worldMissionEventStart() {
    __asm {
        push ecx;
        push eax;

        push eax; //_world_OnMissionEventStart argument
        mov     ecx, offset GlobalEngineHook;
        call    EngineHook::_world_OnMissionEventStart;
        pop     eax; //Don't know if eax will be modified but it's likely
        pop     ecx;

        push    ebx;  //fixup
        mov     ebx, ecx;
        push    esi;
        lea     esi, [eax + eax * 4];
        jmp worldMissionEventStartJmpBack;
    }
}

_declspec(naked) void worldMissionEventEnd() {
    __asm {
        push ecx;
        push eax;
        mov     ecx, offset GlobalEngineHook;
        call    EngineHook::_world_OnMissionEventEnd;
        pop     eax; //Don't know if eax will be modified but it's likely
        pop     ecx;

        pop     edi;   //fixup
        pop     esi;
        pop     ebx;
        mov     esp, ebp;
        pop     ebp;
        jmp worldMissionEventEndJmpBack;
    }
}

//#TODO remember to call GlobalAlive while in break state

enum class MissionEventType {
    Ended = 0,
    SelectedActionPerformed = 1,
    SelectedRotorLibActionPerformed = 2,
    SelectedActionChanged = 3,
    SelectedRotorLibActionChanged = 4,
    ControlsShifted = 5,
    Draw3D = 6,
    Loaded = 7,
    HandleDisconnect = 8,
    EntityRespawned = 9,
    EntityKilled = 10,
    MapSingleClick = 11,
    HCGroupSelectionChanged = 12,
    CommandModeChanged = 13,
    PlayerConnected = 14,
    PlayerDisconnected = 15,
    TeamSwitch = 16,
    GroupIconClick = 17,
    GroupIconOverEnter = 18,
    GroupIconOverLeave = 19,
    EachFrame = 20,
    PreloadStarted = 21,
    PreloadFinished = 22,
    Map = 23,
    PlayerViewChanged = 24
};

enum class scriptExecutionContext {
    Invalid = 0,
    scriptVM = 1,
    EventHandler = 2
};
scriptExecutionContext currentContext = scriptExecutionContext::Invalid;



void EngineHook::placeHooks() {
    while (!IsDebuggerPresent()) Sleep(1u);
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


}

void EngineHook::removeHooks(bool leavePFrameHook) {


}

void EngineHook::_worldSimulate() {
    static uint32_t frameCounter = 0;
    frameCounter++;
    OutputDebugStringA(("#Frame " + std::to_string(frameCounter) + "\n").c_str());
    for (auto& it : GlobalDebugger.VMPtrToScript) {
        OutputDebugStringA("\t");
        OutputDebugStringA((std::to_string(it.second->totalRuntime.count()) + "ns " + std::to_string(it.second->isScriptVM) + "\n").c_str());
        for (auto& it2 : it.second->contentPtrToScript) {
            if (it2.second->_fileName.empty()) continue;
            OutputDebugStringA("\t\t");
            OutputDebugStringA((it2.second->_fileName + " " + std::to_string(it2.second->instructionCount)).c_str());
            OutputDebugStringA("\n");
        }
    }
    OutputDebugStringA("#EndFrame\n");
    bool logFrame = false;
    if (logFrame || frameCounter % 1000 == 0)
        GlobalDebugger.writeFrameToFile(frameCounter);


    GlobalDebugger.clear();
    globalTime = std::chrono::high_resolution_clock::now();
    frameStart = globalTime;
}

void EngineHook::_scriptLoaded(uintptr_t scrVMPtr) {
    globalTimeKeeper _tc;
    auto scVM = reinterpret_cast<RV_ScriptVM *>(scrVMPtr);
    scVM->debugPrint("Load");
    auto myCtx = GlobalDebugger.getVMContext(&scVM->_context);
    myCtx->isScriptVM = true;
    //myCtx->canBeDeleted = false; //Should reimplement that again sometime. This causes scriptVM's to be deleted and loose their upper callstack every frame
}

void EngineHook::_scriptEntered(uintptr_t scrVMPtr) {
    globalTimeKeeper _tc;
    auto scVM = reinterpret_cast<RV_ScriptVM *>(scrVMPtr);
    scVM->debugPrint("Enter");
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
    scVM->debugPrint("Term " + std::to_string(myCtx->totalRuntime.count()));
    if (scVM->_context.callStacksCount - 1 > 0) {
        auto scope = scVM->_context.callStacks[scVM->_context.callStacksCount - 1];
        scope->printAllVariables();
    }
    myCtx->canBeDeleted = true;
    currentContext = scriptExecutionContext::Invalid;
}

void EngineHook::_scriptLeft(uintptr_t scrVMPtr) {
    globalTimeKeeper _tc;
    auto scVM = reinterpret_cast<RV_ScriptVM *>(scrVMPtr);
    GlobalDebugger.getVMContext(&scVM->_context)->dbg_LeaveContext();
    scVM->debugPrint("Left");
    //if (scVM->_context.callStacksCount - 1 > 0) {
    //    auto scope = scVM->_context.callStacks[scVM->_context.callStacksCount - 1];
    //    scope->printAllVariables();
    //}
    currentContext = scriptExecutionContext::Invalid;
}
uintptr_t lastCallstackIndex = 0;
void EngineHook::_scriptInstruction(uintptr_t instructionBP_Instruction, uintptr_t instructionBP_VMContext, uintptr_t instructionBP_gameState, uintptr_t instructionBP_IDebugScript) {
    globalTimeKeeper _tc;
    auto start = std::chrono::high_resolution_clock::now();

    auto instruction = reinterpret_cast<RV_GameInstruction *>(instructionBP_Instruction);
    auto ctx = reinterpret_cast<RV_VMContext *>(instructionBP_VMContext);
    auto context = GlobalDebugger.getVMContext(ctx);
    auto script = context->getScriptByContent(instruction->_scriptPos._content);
    script->dbg_instructionExec();
    //auto dbg = instruction->GetDebugName();
    //
    //
    if (!instruction->_scriptPos._sourceFile.isNull())
        script->_fileName = instruction->_scriptPos._sourceFile;
    //auto callStackIndex = ctx->callStacksCount;
    //bool stackChangeImminent = dbg == "operator call" || dbg == "function call"; //This is a call instruction. This means next instruction will go into lower scope

    context->addInstruction(ctx, instruction);




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
}

void EngineHook::_world_OnMissionEventEnd() {
    currentContext = scriptExecutionContext::Invalid;
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