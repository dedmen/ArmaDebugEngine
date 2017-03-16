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
HookManager GlobalHookManager;
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


HookManager::Pattern pat_productType{//01827340
    "x????xxx?x????xx????xxxx?xxx????xxxx?xxxxx?xxxxxxxxxxx?xxxxxxxx?xx????xxxxx?xxx?xxx?xxxxxxxx?",
    "\x68\x00\x00\x00\x00\x8d\x44\x24\x00\x68\x00\x00\x00\x00\x50\xe8\x00\x00\x00\x00\x50\x8d\x44\x24\x00\x57\x50\xe8\x00\x00\x00\x00\x8b\x17\x83\xc4\x00\x8b\x08\x85\xc9\x74\x00\x8b\xc3\xf0\x0f\xc1\x01\x89\x0f\x85\xd2\x74\x00\x8b\xc6\xf0\x0f\xc1\x02\x48\x75\x00\x8b\x0d\x00\x00\x00\x00\x52\x8b\x01\xff\x50\x00\x8b\x54\x24\x00\x85\xd2\x74\x00\x8b\xc6\xf0\x0f\xc1\x02\x48\x75\x00"
};

HookManager::Pattern pat_productVersion{//01827340
    "x????x?xxxx????xxxxx????xx?xxx?x????x????xx?xxxx?x",
    "\x68\x00\x00\x00\x00\x6a\x00\x50\x0f\xb7\x87\x00\x00\x00\x00\x51\x50\x56\xff\x15\x00\x00\x00\x00\x83\xc4\x00\x84\xc0\x75\x00\x68\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x83\xc4\x00\x5e\x5f\x83\xc4\x00\xc3"
};

HookManager::Pattern pat_IsDebuggerAttached{//0206F310
    "x????x????xxxxx????xxxxx?xxxxxxx?xx?xxx?xx??x?xx????xxxx????xxx?xx??x",
    "\xb9\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x5e\x5b\x5f\x81\xc4\x00\x00\x00\x00\xc3\xf3\x0f\x10\x47\x00\x0f\x57\xc9\x0f\x2f\xc1\x76\x00\x8b\x47\x00\x85\xc0\x74\x00\x80\x78\x00\x00\x74\x00\x8b\x0d\x00\x00\x00\x00\x8b\x01\xff\x90\x00\x00\x00\x00\x85\xc0\x75\x00\x83\x7f\x00\x00\x74"
};

HookManager::Pattern pat_EngineAliveFnc{//010454B0
    "xx????xxxxxxxxxxxx????xxx?xxxx?x",
    //\xCC 's are custom
    "\x8B\x0D\x00\x00\x00\x00\x8B\x01\xFF\x20\xCC\xCC\xCC\xCC\xCC\xCC\x8b\x0d\x00\x00\x00\x00\xff\x74\x24\x00\x8b\x01\xff\x50\x00\xc3" 

    /*
    
    
    
    */

};

HookManager::Pattern pat_EngineEnableMouseFnc{//1159250 PROF only!!! This is 2 functions in nonProf. See 1.66.139.586 EnableMouseMovement DisableMosueMovement
    "xx?xxxx?xx????xx????x????x?x?xx?xx????????x????xx????????xx?????x?x????xx????x????x????xx????????xx????????xx????????x????x????x????xx?????x????x????",
    "\x83\xec\x00\x53\x8a\x5c\x24\x00\x38\x1d\x00\x00\x00\x00\x0f\x84\x00\x00\x00\x00\xa1\x00\x00\x00\x00\xa8\x00\x75\x00\x83\xc8\x00\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xa3\x00\x00\x00\x00\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x80\x3d\x00\x00\x00\x00\x00\x75\x00\xe8\x00\x00\x00\x00\x50\x68\x00\x00\x00\x00\x68\x00\x00\x00\x00\xb9\x00\x00\x00\x00\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xa3\x00\x00\x00\x00\xe8\x00\x00\x00\x00\xa3\x00\x00\x00\x00\xc6\x05\x00\x00\x00\x00\x00\xb9\x00\x00\x00\x00\xe8\x00\x00\x00\x00"
};

HookManager::Pattern pat_scriptVMConstructor{//10448BE
    "xxx?x????x????xx?x????x?xx????xx????xx?x????xxxxxxxx",
    "\x56\x8d\x4f\x00\xe8\x00\x00\x00\x00\x68\x00\x00\x00\x00\x8d\x4e\x00\xe8\x00\x00\x00\x00\x6a\x00\x8d\x87\x00\x00\x00\x00\x50\x68\x00\x00\x00\x00\x8d\x4e\x00\xe8\x00\x00\x00\x00\x8b\xc7\x5f\x5e\x5d\x5b\x59\xc2",
    0x010448BE - 0x010448A8
};

HookManager::Pattern pat_scriptVMSimulateStart{//1044E80
    "xx?xxxxx?????x?xxxxx?x",
    "\x83\xec\x00\x57\x8b\xf9\x80\xbf\x00\x00\x00\x00\x00\x75\x00\x32\xc0\x5f\x83\xc4\x00\xc2"
};

HookManager::Pattern pat_scriptVMSimulateEnd{//10451A3
    "xxx?xxx?x????xx?xxx?????xxx?xxx????xxx?xxxx?x",
    "\xff\x74\x24\x00\xff\x74\x24\x00\xe8\x00\x00\x00\x00\x83\xc4\x00\xc7\x44\x24\x00\x00\x00\x00\x00\x8d\x4c\x24\x00\xdd\xd8\xe8\x00\x00\x00\x00\x8a\x44\x24\x00\x5d\x5f\x83\xc4\x00\xc2",
    0x10451A3 - 0x0104517C
};

HookManager::Pattern pat_instructionBreakpoint{//103C610
    "xx?xx?xxx?xx?x????x?xx????xxx?xxxxx?xxxxxxxx",
    "\x8b\x43\x00\x8d\x53\x00\x85\xc0\x74\x00\x83\xc0\x00\x3d\x00\x00\x00\x00\x74\x00\x8b\x8f\x00\x00\x00\x00\x85\xc9\x74\x00\x8b\x01\x52\xff\x50\x00\x8b\x03\x8b\xcb\x57\x55\x8b\x40"
};

HookManager::Pattern pat_worldSimulate{//00B5ED90 PROF ONLY
    "xx????x????xxx????????xxxxxxx????x?x?xx?xx????????x????xx????????xx?????x?x????xx????x????x????xx????????xx????????xx????????x????x????x????xx",
    "\x81\xec\x00\x00\x00\x00\xa1\x00\x00\x00\x00\xc7\x84\x24\x00\x00\x00\x00\x00\x00\x00\x00\x56\x57\x8b\xf9\x89\xbc\x24\x00\x00\x00\x00\xa8\x00\x75\x00\x83\xc8\x00\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xa3\x00\x00\x00\x00\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x80\x3d\x00\x00\x00\x00\x00\x75\x00\xe8\x00\x00\x00\x00\x50\x68\x00\x00\x00\x00\x68\x00\x00\x00\x00\xb9\x00\x00\x00\x00\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xa3\x00\x00\x00\x00\xe8\x00\x00\x00\x00\xa3\x00\x00\x00\x00\xc6\x05"
};

HookManager::Pattern pat_worldMissionEventStart{//00B19E5C PROF ONLY
    "xxxxx?xx?xx?xxxxxxxxxxx??xxx?xx????x????x?x?xx?xx????????x????xx",
    "\x55\x8b\xec\x83\xe4\x00\x83\xec\x00\x8b\x45\x00\x53\x8b\xd9\x56\x8d\x34\x80\x57\x83\x7c\xb3\x00\x00\x89\x74\x24\x00\x0f\x8e\x00\x00\x00\x00\xa1\x00\x00\x00\x00\xa8\x00\x75\x00\x83\xc8\x00\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xa3\x00\x00\x00\x00\xc7\x05",
    0x00B19E5C - 0x00B19E50
};

HookManager::Pattern pat_worldMissionEventEnd{//00B1A0AB
    "xxx?xxx?xxxxxx?xx????xxxxx?xxxxxxx",
    "\x8b\x54\x24\x00\x85\xd2\x74\x00\xf0\x0f\xc1\x3a\x4f\x75\x00\x8b\x0d\x00\x00\x00\x00\x52\x8b\x01\xff\x50\x00\x5f\x5e\x5b\x8b\xe5\x5d\xc2",
    0x00B1A0AB  - 0x00B1A090
};

scriptExecutionContext currentContext = scriptExecutionContext::Invalid;
MissionEventType currentEventHandler = MissionEventType::Ended; //#TODO create some invalid handler type

void EngineHook::placeHooks() {
    WAIT_FOR_DEBUGGER_ATTACHED;

    char** productType = (char**) (GlobalHookManager.findPattern(pat_productType) + 1);
    char** productVersion = (char**) (GlobalHookManager.findPattern(pat_productVersion) + 1);
    OutputDebugStringA("Product Type: ");
    OutputDebugStringA(*productType);
    OutputDebugStringA("\t\tVersion: ");
    OutputDebugStringA(*productVersion);
    OutputDebugStringA("\n");
    productType;

    bool* isDebuggerAttached = *reinterpret_cast<bool**>((GlobalHookManager.findPattern(pat_IsDebuggerAttached) + 1));
    *isDebuggerAttached = false; //Small hack to keep RPT logging while Debugger is attached
                                 //Could also patternFind and patch (profv3 0107144F) to unconditional jmp


    GlobalHookManager.placeHook(hookTypes::scriptVMConstructor, pat_scriptVMConstructor, reinterpret_cast<uintptr_t>(scriptVMConstructor), scriptVMConstructorJmpBack, 3);
    GlobalHookManager.placeHook(hookTypes::scriptVMSimulateStart, pat_scriptVMSimulateStart, reinterpret_cast<uintptr_t>(scriptVMSimulateStart), scriptVMSimulateStartJmpBack, 1);
    GlobalHookManager.placeHook(hookTypes::scriptVMSimulateEnd, pat_scriptVMSimulateEnd, reinterpret_cast<uintptr_t>(scriptVMSimulateEnd));
    GlobalHookManager.placeHook(hookTypes::instructionBreakpoint, pat_instructionBreakpoint, reinterpret_cast<uintptr_t>(instructionBreakpoint), instructionBreakpointJmpBack, 1);
    GlobalHookManager.placeHook(hookTypes::worldSimulate, pat_worldSimulate, reinterpret_cast<uintptr_t>(worldSimulate), worldSimulateJmpBack, 1);
    GlobalHookManager.placeHook(hookTypes::worldMissionEventStart, pat_worldMissionEventStart, reinterpret_cast<uintptr_t>(worldMissionEventStart), worldMissionEventStartJmpBack, 2);
    GlobalHookManager.placeHook(hookTypes::worldMissionEventEnd, pat_worldMissionEventEnd, reinterpret_cast<uintptr_t>(worldMissionEventEnd), worldMissionEventEndJmpBack, 1);





    EngineAliveFnc = reinterpret_cast<EngineAlive*>(GlobalHookManager.findPattern(pat_EngineAliveFnc));
    //Find by searching for.  "XML parsing error: cannot read the source file". function call right after start of while loop

    //#TODO don't call EngineEnableMouseFnc if nullptr
    EngineEnableMouseFnc = reinterpret_cast<EngineEnableMouse*>(GlobalHookManager.findPattern(pat_EngineEnableMouseFnc));

    //To yield scriptVM and let engine run while breakPoint hit. 0103C5BB overwrite eax to Yield

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

uintptr_t HookManager::placeHook(uintptr_t offset, uintptr_t jmpTo, uint8_t jmpBackOffset) {
    auto totalOffset = offset + engineBase;
    return placeHookTotalOffs(totalOffset, jmpTo) + jmpBackOffset;
}

uintptr_t HookManager::placeHookTotalOffs(uintptr_t totalOffset, uintptr_t jmpTo) {
    DWORD dwVirtualProtectBackup;
    VirtualProtect(reinterpret_cast<LPVOID>(totalOffset), 5u, 0x40u, &dwVirtualProtectBackup);
    auto jmpInstr = reinterpret_cast<unsigned char *>(totalOffset);
    auto addrOffs = reinterpret_cast<unsigned int *>(totalOffset + 1);
    *jmpInstr = 0xE9;
    *addrOffs = jmpTo - totalOffset - 5;
    VirtualProtect(reinterpret_cast<LPVOID>(totalOffset), 5u, dwVirtualProtectBackup, &dwVirtualProtectBackup);

    return totalOffset + 5;
}


#include <Psapi.h>
HookManager::HookManager() {
    MODULEINFO modInfo = { 0 };
    HMODULE hModule = GetModuleHandle(NULL);
    GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));
    engineBase = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
    engineSize = static_cast<uintptr_t>(modInfo.SizeOfImage);
}

bool HookManager::placeHook(hookTypes type, const Pattern& pat, uintptr_t jmpTo, uintptr_t & jmpBackRef, uint8_t jmpBackOffset) {

    auto found = findPattern(pat);
    if (found == 0) {
        __debugbreak(); //#TODO report somehow
        return false;
    }
    jmpBackRef = placeHookTotalOffs(found, jmpTo) + jmpBackOffset;
    return true;
}

bool HookManager::placeHook(hookTypes, const Pattern & pat, uintptr_t jmpTo) {
    auto found = findPattern(pat);
    if (found == 0) {
        __debugbreak(); //#TODO report somehow
        return false;
    }
    placeHookTotalOffs(found, jmpTo);
    return true;
}

bool HookManager::MatchPattern(uintptr_t addr, const char* pattern, const char* mask) {
    DWORD size = strlen(mask);
    if (IsBadReadPtr((void*) addr, size))
        return false;
    bool found = true;
    for (DWORD j = 0; j < size; j++) {
        found &= mask[j] == '?' || pattern[j] == *(char*) (addr + j);
    }
    if (found)
        return true;
    return false;
}

uintptr_t HookManager::findPattern(const char* pattern, const char* mask, uintptr_t offset /*= 0*/) {
    DWORD base = (DWORD) engineBase;
    DWORD size = (DWORD) engineSize;

    DWORD patternLength = (DWORD) strlen(mask);

    for (DWORD i = 0; i < size - patternLength; i++) {
        bool found = true;
        for (DWORD j = 0; j < patternLength; j++) {
            found &= mask[j] == '?' || pattern[j] == *(char*) (base + i + j);
            if (!found)
                break;
        }
        if (found)
            return base + i + offset;
    }
    return 0x0;
}

uintptr_t HookManager::findPattern(const Pattern & pat, uintptr_t offset) {
    return findPattern(pat.pattern, pat.mask, pat.offset + offset);
}
