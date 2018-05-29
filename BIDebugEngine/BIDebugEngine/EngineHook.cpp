#include "EngineHook.h"

#include "Debugger.h"
#include "Script.h"
#include "VMContext.h"
#include "Serialize.h"
#include "Tracker.h"
#include <intercept.hpp>
#include "SQF-Assembly-Iface.h"

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
#ifdef ScriptProfiling 
    globalTimeKeeper() {
        globalTime += std::chrono::high_resolution_clock::now() - lastContextExit;
    }
    ~globalTimeKeeper() {
        lastContextExit = std::chrono::high_resolution_clock::now();
    }
#endif
    globalTimeKeeper() {}
    ~globalTimeKeeper() {}
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
    uintptr_t onScriptErrorJmpBack;
    uintptr_t scriptPreprocessorConstructorJmpBack;
    uintptr_t scriptPreprocessorDefineDefine;
    uintptr_t scriptAssertJmpBack;
    uintptr_t scriptHaltJmpBack;
    uintptr_t scriptEchoJmpBack;

    const char* preprocMacroName = "DEBUG";
    const char* preprocMacroValue = "true";

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
extern "C" void onScriptError();
extern "C" void scriptPreprocessorConstructor();
extern "C" void onScriptAssert();
extern "C" void onScriptHalt();
extern "C" void onScriptEcho();

#ifdef X64

HookManager::Pattern pat_productType{
    "xxx????xxx????xxxxx????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxx",
    "\x4C\x8D\x05\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x4D\x98\xE8\x00\x00\x00\x00\x48\x8B\x10\x48\x85\xD2\x74\x06\x48\x83\xC2\x10\xEB\x03\x49\x8B\xD6\x48\x8B\x06\x48\x8D\x48\x10\x48\x85\xC0\x75\x03\x49\x8B\xCE\xE8\x00\x00\x00\x00\x48\x8B\xD8\x48\x85\xC0\x74\x03\xF0\xFF\x00",
     [](uintptr_t found) -> uintptr_t {
    uint32_t ptr = *reinterpret_cast<uint32_t*>(found + 0x3);
    uintptr_t xx = found + ptr + 7;
    return xx;
}

};

HookManager::Pattern pat_productVersion{
    "xxx????xxxx?xxxxxxxxx?????xx????xxxxxxx????x????xxx????x????x????xxx????xxxx????x????xxx????xx????????xxxxxxxx",
    "\x48\x8D\x0D\x00\x00\x00\x00\x48\x89\x4C\x24\x00\x44\x0F\xB7\xC0\x8B\xCF\xC7\x44\x24\x00\x00\x00\x00\x00\xFF\x15\x00\x00\x00\x00\x84\xC0\x75\x1D\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xE9\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x4C\x89\xB4\x24\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\x8E\x00\x00\x00\x00\xC7\x86\x00\x00\x00\x00\x00\x00\x00\x00\x48\x85\xC9\x74\x0B\x48\x8B\x01",
    [](uintptr_t found) -> uintptr_t {
    uint32_t ptr = *reinterpret_cast<uint32_t*>(found + 0x3);
    uintptr_t xx = found + ptr + 7;
    return xx;
}
};

HookManager::Pattern pat_IsDebuggerAttached{ //PROF ONLY
    "xxx????x????xxxx????xxxxxxx????xxxx?xxxx????xxxxxxxxxxxxxxxxxxx????xxxx?xxxxxxxx????xxxxxx",
    "\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\x8C\x24\x00\x00\x00\x00\x4D\x8B\x16\x48\x8D\x84\x24\x00\x00\x00\x00\x48\x89\x4C\x24\x00\x48\x8D\x94\x24\x00\x00\x00\x00\x83\xCE\xFF\x49\x8B\xCE\x4C\x8B\xCB\x45\x8B\xC4\x89\x7C\x24\x28\x89\xB4\x24\x00\x00\x00\x00\x48\x89\x44\x24\x00\x41\xFF\x52\x20\x4C\x8B\xBC\x24\x00\x00\x00\x00\x83\xCF\xFF\x4D\x85\xFF",
    [](uintptr_t found) -> uintptr_t {
    uint32_t ptr = *reinterpret_cast<uint32_t*>(found + 0x3);
    uintptr_t xx = found + ptr + 7;
    return xx;
}

};

//\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x01\x48\xFF\x20 xxx????xxxxxx
//\xCC\xCC\xCC	xxx								    
//\x48\x8B\xD1\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x01\x48\xFF\x60\x08 xxxxxx????xxxxxxx
HookManager::Pattern pat_EngineAliveFnc{
    "xxx????xxxxxxxxxxxxxxx????xxxxxxx",
    //\xCC 's are custom
    "\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x01\x48\xFF\x20\xCC\xCC\xCC\x48\x8B\xD1\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x01\x48\xFF\x60\x08"
};


HookManager::Pattern pat_EngineEnableMouseFnc{//PROF only!!!
    "xxxxxxxx????xxxxx????xx????xxxx?xxxxxxx?xxxx?xxxxxxxxxx????????xx????xx????xx?????xxx????xxx????xxx????xxx????",
    "\x40\x57\x48\x83\xEC\x50\x38\x0D\x00\x00\x00\x00\x0F\xB6\xF9\x0F\x84\x00\x00\x00\x00\x8B\x05\x00\x00\x00\x00\x48\x89\x5C\x24\x00\x83\xCB\xFF\x48\x89\x74\x24\x00\x4C\x89\x74\x24\x00\xA8\x01\x75\x1A\x83\xC8\x01\x48\xC7\x05\x00\x00\x00\x00\x00\x00\x00\x00\x89\x1D\x00\x00\x00\x00\x89\x05\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x4C\x8D\x35\x00\x00\x00\x00\x75\x46\xE8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x4C\x8D\x05\x00\x00\x00\x00"
};

HookManager::Pattern pat_scriptVMConstructor{
    "xxxxxxxx????xxx????xxxxx????xxx????xxx????xxxxxxxx????xxxxxxxxxxxxxxxxxx",
    "\x48\x8D\x4F\x20\x48\x8B\xD6\xE8\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x4E\x20\xE8\x00\x00\x00\x00\x4C\x8D\x87\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8D\x4E\x20\x41\xB1\x01\xE8\x00\x00\x00\x00\x48\x8B\xC7\x48\x83\xC4\x60\x41\x5F\x41\x5E\x41\x5D\x5F\x5E\x5D\x5B\xC3",
    0x00000000013AD34E - 0x00000000013AD318
};

HookManager::Pattern pat_scriptVMSimulateStart{//PROF ONLY
    "xxxxxxxx?xxx????xx?????xxxx?xxxxxxxxxxxxxx?xxx????xxxxxx????xxxx????xxxx????xxxx????xxxx????xxxx????xxxxxxxx",
    "\x40\x55\x41\x56\x48\x8D\x6C\x24\x00\x48\x81\xEC\x00\x00\x00\x00\x80\xB9\x00\x00\x00\x00\x00\x0F\x29\x74\x24\x00\x4C\x8B\xF1\x0F\x28\xF2\x75\x12\x32\xC0\x0F\x28\x74\x24\x00\x48\x81\xC4\x00\x00\x00\x00\x41\x5E\x5D\xC3\x8B\x05\x00\x00\x00\x00\x48\x89\x9C\x24\x00\x00\x00\x00\x48\x89\xB4\x24\x00\x00\x00\x00\x48\x89\xBC\x24\x00\x00\x00\x00\x4C\x89\xA4\x24\x00\x00\x00\x00\x4C\x89\xBC\x24\x00\x00\x00\x00\x45\x33\xFF\x83\xCF\xFF\xA8\x01"
};


HookManager::Pattern pat_scriptVMSimulateEnd{
    "xxxxxxxx????xxxxxxxxxxxxx????xxxxxxxxxx????xxxx?xxxxxxx????xxxx",
    "\x48\x8B\x55\x1F\x4C\x8B\xA4\x24\x00\x00\x00\x00\x48\x85\xD2\x74\x12\xF0\xFF\x0A\x75\x0D\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x01\xFF\x50\x18\x48\x8B\xBC\x24\x00\x00\x00\x00\x0F\x28\x74\x24\x00\x41\x0F\xB6\xC6\x48\x81\xC4\x00\x00\x00\x00\x41\x5E\x5D\xC3",
    0x00000000013ADF49 - 0x00000000013ADF1E
};

//HookManager::Pattern pat_instructionBreakpoint{
//    "xxxx?xxxx?xxxxxxxx????xxxxxxxxxxxx????xxxxxxxxxxxxxxx",
//    "\x49\x8B\x44\x24\x00\x49\x8D\x54\x24\x00\x48\x85\xC0\x74\x22\x48\x8D\x0D\x00\x00\x00\x00\x48\x83\xC0\x10\x48\x3B\xC1\x74\x12\x48\x8B\x8F\x00\x00\x00\x00\x48\x85\xC9\x74\x06\x48\x8B\x01\xFF\x50\x08\x49\x8B\x04\x24",
//    -0x3C
//};

HookManager::Pattern pat_worldSimulate{//PROF ONLY
    "xxxxxxxxxxxxxxxxxxxxxx????xxx????xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx?xxxx????xxxxxxxxxxxxxxxxxxxx????xxxx????xxxx????xxxx????xxxxxx????xxxx",
    "\x48\x8B\xC4\x4C\x89\x48\x20\x4C\x89\x40\x18\x48\x89\x48\x08\x55\x53\x41\x55\x48\x8D\xA8\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x89\x70\xE0\x48\x89\x78\xD8\x4C\x89\x60\xD0\x4C\x89\x70\xC8\x4C\x89\x78\xC0\x48\x8B\xF9\x0F\x29\x78\x98\x44\x0F\x29\x40\x00\x44\x0F\x29\x88\x00\x00\x00\x00\x33\xC9\x41\x83\xCD\xFF\x49\x8B\xF1\x49\x8B\xD8\x89\x4C\x24\x5C\x44\x0F\x29\x90\x00\x00\x00\x00\x44\x0F\x29\x98\x00\x00\x00\x00\x44\x0F\x29\xA0\x00\x00\x00\x00\x44\x0F\x29\xA8\x00\x00\x00\x00\x44\x0F\x28\xC1\x8B\x05\x00\x00\x00\x00\xA8\x01\x75\x17"
};

//#TODO this onMissionEvent func only works for events without args. Need the other ones

//HookManager::Pattern pat_worldMissionEventStart{//00B19E5C PROF ONLY
//	"xxxxx?xx?xx?xxxxxxxxxxx??xxx?xx????x????x?x?xx?xx????????x????xx",
//	"\x55\x8b\xec\x83\xe4\x00\x83\xec\x00\x8b\x45\x00\x53\x8b\xd9\x56\x8d\x34\x80\x57\x83\x7c\xb3\x00\x00\x89\x74\x24\x00\x0f\x8e\x00\x00\x00\x00\xa1\x00\x00\x00\x00\xa8\x00\x75\x00\x83\xc8\x00\xc7\x05\x00\x00\x00\x00\x00\x00\x00\x00\xa3\x00\x00\x00\x00\xc7\x05",
//	0x00B19E5C - 0x00B19E50
//};
//
//HookManager::Pattern pat_worldMissionEventEnd{//00B1A0AB
//	"xxx?xxx?xxxxxx?xx????xxxxx?xxxxxxx",
//	"\x8b\x54\x24\x00\x85\xd2\x74\x00\xf0\x0f\xc1\x3a\x4f\x75\x00\x8b\x0d\x00\x00\x00\x00\x52\x8b\x01\xff\x50\x00\x5f\x5e\x5b\x8b\xe5\x5d\xc2",
//	0x00B1A0AB - 0x00B1A090
//};



HookManager::Pattern pat_onScriptError{
    "xxxxxxxxxxxxxxx????xxx????xxxxxx?????xx????xxxxxxxxxxxxxxxxxxxxxxxxxxxx",
    "\x48\x8B\xC4\x48\x89\x48\x08\x55\x48\x8D\x68\xA1\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x91\x00\x00\x00\x00\x48\x89\x55\x07\xF7\x42\x00\x00\x00\x00\x00\x0F\x84\x00\x00\x00\x00\x48\x89\x58\xF0\x48\x89\x70\xE8\x48\x89\x78\xE0\x48\x8B\x7A\x48\x4C\x89\x60\xD8\x4C\x89\x68\xD0\x4C\x89\x70\xC8"
};

HookManager::Pattern pat_scriptPreprocessorConstructor{// this is Preproc not FilePreproc!
    "xxxx?xxxxxxxx????xxxxxxxxxxxxxxx?????xxx????xxx????x????xxx????xxxxxxxxxxx????xxxxxxxxxxxxx????xxxxxxxxxxxxxxxxxxx????????xxxxxxx?xxxxxx",
    "\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x48\x8D\x05\x00\x00\x00\x00\x33\xFF\x48\x8B\xD9\x48\x89\x01\x48\x89\x79\x18\x48\xC7\x41\x00\x00\x00\x00\x00\x48\x89\xB9\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\x93\x00\x00\x00\x00\x48\x85\xC0\x74\x03\xF0\xFF\x00\x48\x89\x83\x00\x00\x00\x00\x48\x85\xD2\x74\x12\xF0\xFF\x0A\x75\x0D\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x01\xFF\x50\x18\x48\x89\x7B\x08\x48\x89\x7B\x28\x89\x7B\x10\xC7\x83\x00\x00\x00\x00\x00\x00\x00\x00\x48\x8B\xC3\x48\x8B\x5C\x24\x00\x48\x83\xC4\x20\x5F\xC3",
    0x00000000013C39C5 - 0x00000000013C3960
};

HookManager::Pattern pat_scriptPreprocessorDefineDefine{
    "xxxx?xxxxxxxxxxxx?xxxx????xxxx?xxxxx????xxxxxxxx????xxxxxxxxxxxxxxxx?xxxxxxxxx?",
    "\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x50\x48\x8B\xD9\x48\x8D\x4C\x24\x00\x48\x8B\xFA\xE8\x00\x00\x00\x00\x48\x8D\x54\x24\x00\x48\x8D\x4B\x18\xE8\x00\x00\x00\x00\x48\x8D\x4B\x18\x48\x8B\xD7\xE8\x00\x00\x00\x00\x8B\x48\x28\x85\xC9\x7E\x05\xFF\xC9\x89\x48\x28\x48\x8B\x54\x24\x00\x48\x85\xD2\x74\x0F\x44\x8B\x44\x24\x00"
};

HookManager::Pattern pat_onScriptAssert{
    "xxxx?xxxx?xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx????xxxx????xxx????xxxxxx?????xxxxxxxxxxxxx",
    "\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xD9\x49\x8B\x48\x08\x49\x8B\xF8\x48\x8B\xF2\x48\x85\xC9\x74\x0A\x48\x8B\x01\xFF\x50\x20\x84\xC0\x75\x0D\xBA\x00\x00\x00\x00\x48\x8B\xCE\xE8\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x03\x48\xC7\x43\x00\x00\x00\x00\x00\x48\x8B\x47\x08\x48\x8B\x4B\x08\x48\x85\xC0\x74\x04"
};

HookManager::Pattern pat_onScriptHalt{
    "xxxxxxxxxxxxx????xxxx????xxxxxx????xxxxxxxxx",
    "\x40\x53\x48\x83\xEC\x20\x48\x8B\xC2\x48\x8B\xD9\xBA\x00\x00\x00\x00\x48\x8B\xC8\xE8\x00\x00\x00\x00\x33\xD2\x48\x8B\xCB\xE8\x00\x00\x00\x00\x48\x8B\xC3\x48\x83\xC4\x20\x5B\xC3"
};

HookManager::Pattern pat_onScriptEcho{
    "xxxx?xxxxxxxxxxxx?xxxx????xxxx?xxxxxxxxxxxxx????xxxxxxxxx????xxxxx",
    "\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xD9\x48\x8D\x54\x24\x00\x49\x8B\xC8\xE8\x00\x00\x00\x00\x48\x8B\x54\x24\x00\x48\x85\xD2\x74\x12\xF0\xFF\x0A\x75\x0D\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x01\xFF\x50\x18\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x03\x33\xC0"
};

#else
HookManager::Pattern pat_productType{//01827340
    "x????xxx?x????xx????xxxx?xxx????xxxx?xxxxx?xxxxxxxxxxx?xxxxxxxx?xx????xxxxx?xxx?xxx?xxxxxxxx?",
    "\x68\x00\x00\x00\x00\x8d\x44\x24\x00\x68\x00\x00\x00\x00\x50\xe8\x00\x00\x00\x00\x50\x8d\x44\x24\x00\x57\x50\xe8\x00\x00\x00\x00\x8b\x17\x83\xc4\x00\x8b\x08\x85\xc9\x74\x00\x8b\xc3\xf0\x0f\xc1\x01\x89\x0f\x85\xd2\x74\x00\x8b\xc6\xf0\x0f\xc1\x02\x48\x75\x00\x8b\x0d\x00\x00\x00\x00\x52\x8b\x01\xff\x50\x00\x8b\x54\x24\x00\x85\xd2\x74\x00\x8b\xc6\xf0\x0f\xc1\x02\x48\x75\x00",
    [](uintptr_t found) -> uintptr_t {
    return *reinterpret_cast<uintptr_t*>(found + 0x1);
}
};

HookManager::Pattern pat_productVersion{//01827340
    "x????x?xxxx????xxxxx????xx?xxx?x????x????x????x????xx?xxxx?x",
    "\x68\x00\x00\x00\x00\x6a\x00\x50\x0f\xb7\x87\x00\x00\x00\x00\x51\x50\x56\xff\x15\x00\x00\x00\x00\x83\xc4\x00\x84\xc0\x75\x00\x68\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x68\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x83\xc4\x00\x5e\x5f\x83\xc4\x00\xc3",
    [](uintptr_t found) -> uintptr_t {
    return *reinterpret_cast<uintptr_t*>(found + 0x1);
}
};

HookManager::Pattern pat_IsDebuggerAttached{//0206F310
    "x????x????xxxxx????xxxxx?xxxxxxx?xx?xxx?xx??x?xx????xxxx????xxx?xx??x",
    "\xb9\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x5e\x5b\x5f\x81\xc4\x00\x00\x00\x00\xc3\xf3\x0f\x10\x47\x00\x0f\x57\xc9\x0f\x2f\xc1\x76\x00\x8b\x47\x00\x85\xc0\x74\x00\x80\x78\x00\x00\x74\x00\x8b\x0d\x00\x00\x00\x00\x8b\x01\xff\x90\x00\x00\x00\x00\x85\xc0\x75\x00\x83\x7f\x00\x00\x74",
    [](uintptr_t found) -> uintptr_t {
    return *reinterpret_cast<uintptr_t*>(found + 0x1);
}
};

HookManager::Pattern pat_EngineAliveFnc{//010454B0
    "xx????xxxxxxxxxxxx????xxx?xxxx?x",
    //\xCC 's are custom
    "\x8B\x0D\x00\x00\x00\x00\x8B\x01\xFF\x20\xCC\xCC\xCC\xCC\xCC\xCC\x8b\x0d\x00\x00\x00\x00\xff\x74\x24\x00\x8b\x01\xff\x50\x00\xc3"
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

HookManager::Pattern pat_scriptVMSimulateStart{//1044E80 PROF ONLY
    "xx?xxxxx?????x?xxxxx?x",
    "\x83\xec\x00\x55\x8b\xe9\x80\xbd\x00\x00\x00\x00\x00\x75\x00\x32\xc0\x5d\x83\xc4\x00\xc2"
};

HookManager::Pattern pat_scriptVMSimulateEnd{//10451A3
    "xxx?xxx?x????xx?xxx?????xxx?xxx????xxx?xxxx?x??",
    "\xff\x74\x24\x00\xff\x74\x24\x00\xe8\x00\x00\x00\x00\x83\xc4\x00\xc7\x44\x24\x00\x00\x00\x00\x00\x8d\x4c\x24\x00\xdd\xd8\xe8\x00\x00\x00\x00\x8a\x44\x24\x00\x5b\x5d\x83\xc4\x00\xc2\x00\x00",
    0x0107EB67 - 0x0107EB40
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
    0x00B1A0AB - 0x00B1A090
};

HookManager::Pattern pat_onScriptError{//0106A590 1.68.140.940
    "xx?xxxxx?xx????xxx?xx?xxxx????xx?xx????xxxx?xxxx?xx?x",
    "\x83\xec\x00\x8b\xc1\x89\x44\x24\x00\x8b\x90\x00\x00\x00\x00\x89\x54\x24\x00\x8b\x42\x00\x85\xc0\x0f\x84\x00\x00\x00\x00\x83\xf8\x00\x0f\x84\x00\x00\x00\x00\x53\x56\x8b\x72\x00\x57\x85\xf6\x74\x00\x83\xc6\x00\xeb",
    0x0106A5B7 - 0x0106A590
};

HookManager::Pattern pat_scriptPreprocessorConstructor{//001DEE33 1.68.140.940
    "xx?xx????xx????????xx????????xx????????xxxxx?xxxxx",
    "\x8a\x42\x00\x88\x86\x00\x00\x00\x00\xc7\x86\x00\x00\x00\x00\x00\x00\x00\x00\xc7\x86\x00\x00\x00\x00\x00\x00\x00\x00\xc7\x86\x00\x00\x00\x00\x00\x00\x00\x00\x8b\x3a\x85\xff\x74\x00\x8b\x17\x85\xd2\x74"
};

HookManager::Pattern pat_scriptPreprocessorDefineDefine{//0108F780 1.68.140.940
    "xx?xxxx?xxxxx?xxx?x????xxx?xxx?x????xxx?xx?x????xx?xxx?xxx?xxx?xxx?xxx?xxx?xx",
    "\x83\xec\x00\x56\xff\x74\x24\x00\x8b\xf1\x8d\x4c\x24\x00\xff\x74\x24\x00\xe8\x00\x00\x00\x00\x8d\x44\x24\x00\x50\x8d\x4e\x00\xe8\x00\x00\x00\x00\xff\x74\x24\x00\x8d\x4e\x00\xe8\x00\x00\x00\x00\x8b\x48\x00\x85\xc9\x7e\x00\x49\x89\x48\x00\x8b\x44\x24\x00\x85\xc0\x74\x00\xff\x74\x24\x00\x8d\x4c\x24\x00\x50\xe8"
};

HookManager::Pattern pat_onScriptAssert{//01052790 1.68.140.940
    "xxxxx?xxx?xxx?xxxx?xxxxx?x?xxx?x????xx?xxx?xx????xx?????xx",
    "\x53\x56\x8b\x74\x24\x00\x57\x8b\x4e\x00\x85\xc9\x74\x00\x8b\x01\x8b\x40\x00\xff\xd0\x84\xc0\x75\x00\x6a\x00\xff\x74\x24\x00\xe8\x00\x00\x00\x00\x83\xc4\x00\x8b\x7c\x24\x00\xc7\x07\x00\x00\x00\x00\xc7\x47\x00\x00\x00\x00\x00\x8b\x56"
};

HookManager::Pattern pat_onScriptHalt{//01050B10 1.68.140.940
    "x?xxx?x????xxx?xx?x?x????xxx?x",
    "\x6a\x00\xff\x74\x24\x00\xe8\x00\x00\x00\x00\x8b\x4c\x24\x00\x83\xc4\x00\x6a\x00\xe8\x00\x00\x00\x00\x8b\x44\x24\x00\xc3"
};

HookManager::Pattern pat_onScriptEcho{//01052800 1.68.140.940
    "xxxxxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxx????xx?????xxxxx?????",
    "\x83\xEC\x08\x8B\x4C\x24\x14\x8D\x04\x24\x56\x57\x50\xE8\x00\x00\x00\x00\x8B\x54\x24\x08\x83\xCF\xFF\x85\xD2\x74\x15\x8B\xC7\xF0\x0F\xC1\x02\x48\x75\x0C\x8B\x0D\x00\x00\x00\x00\x52\x8B\x01\xFF\x50\x0C\x8B\x74\x24\x14\xC7\x06\x00\x00\x00\x00\xC7\x46\x00\x00\x00\x00\x00\x8B\x4E\x04\xC7\x46\x00\x00\x00\x00\x00"
};

#endif

//#TODO onVariableChanged event
//#TODO hook VariableAssignment. 1.68 profv1 Assignment::Execute has notes

scriptExecutionContext currentContext = scriptExecutionContext::Invalid;
MissionEventType currentEventHandler = MissionEventType::Ended; //#TODO create some invalid handler type

void EngineHook::placeHooks() {

    //char* productType = reinterpret_cast<char*>(GlobalHookManager.findPattern(pat_productType));
    //char* productVersion = reinterpret_cast<char*>(GlobalHookManager.findPattern(pat_productVersion));
    //OutputDebugStringA("Product Type: ");
    //OutputDebugStringA(productType ? productType : "");
    //OutputDebugStringA("\t\tVersion: ");
    //OutputDebugStringA(productVersion ? productVersion : "");
    //OutputDebugStringA("\n");
	//
    //if (!productType || !productVersion) {
    //    std::string error("Could not find gameVersion or gameType. This means this game version is likely incompatible! \n");
    //    error += "Version: " + std::string(productVersion ? productVersion : "NOT FOUND") + "\n";
    //    error += "Type: " + std::string(productType ? productType : "NOT FOUND") + "\n";
    //    if (productType && strncmp(productType, "", 2))
    //        error += "You are not running a Profiling version of Arma. This is needed for Arma Debug Engine to work!";
	//	
    //    //MessageBox(error.c_str(), ErrorMsgBoxType::error);
    //    return;
    //}
    //if (productType && strncmp(productType, "Arma3RetailProfile_DX11", 13)) {
    //    std::string error("You are not running a Profiling version of Arma. This is needed for Arma Debug Engine to work!\n\nFurther error messages might be caused by this.");
	//
    //    //MessageBox(error.c_str(), ErrorMsgBoxType::warning);
    //}


    //GlobalDebugger.setGameVersion(productType, productVersion);

    bool* isDebuggerAttached = reinterpret_cast<bool*>(GlobalHookManager.findPattern(pat_IsDebuggerAttached));
    if (isDebuggerAttached)
        *isDebuggerAttached = false; //Small hack to keep RPT logging while Debugger is attached
                                     //Could also patternFind and patch (profv3 0107144F) to unconditional jmp
    HookIntegrity HI;
#ifdef X64

	GASM.setHook(SQF_Assembly_Iface::InstructionType::GameInstructionNewExpression, [](intercept::types::game_instruction* instr, intercept::types::game_state& state, intercept::types::vm_context& ctx) -> void {
		GlobalEngineHook._scriptInstruction(instr, state, ctx);
	});
	GASM.setHook(SQF_Assembly_Iface::InstructionType::GameInstructionConst, [](intercept::types::game_instruction* instr, intercept::types::game_state& state, intercept::types::vm_context& ctx) -> void {
		GlobalEngineHook._scriptInstruction(instr, state, ctx);
	});
	GASM.setHook(SQF_Assembly_Iface::InstructionType::GameInstructionFunction, [](intercept::types::game_instruction* instr, intercept::types::game_state& state, intercept::types::vm_context& ctx) -> void {
		GlobalEngineHook._scriptInstruction(instr, state, ctx);
	});
	GASM.setHook(SQF_Assembly_Iface::InstructionType::GameInstructionOperator, [](intercept::types::game_instruction* instr, intercept::types::game_state& state, intercept::types::vm_context& ctx) -> void {
		GlobalEngineHook._scriptInstruction(instr, state, ctx);
	}); 
	GASM.setHook(SQF_Assembly_Iface::InstructionType::GameInstructionAssignment, [](intercept::types::game_instruction* instr, intercept::types::game_state& state, intercept::types::vm_context& ctx) -> void {
		GlobalEngineHook._scriptInstruction(instr, state, ctx);
	});
	GASM.setHook(SQF_Assembly_Iface::InstructionType::GameInstructionVariable, [](intercept::types::game_instruction* instr, intercept::types::game_state& state, intercept::types::vm_context& ctx) -> void {
		GlobalEngineHook._scriptInstruction(instr, state, ctx);
	});
	GASM.setHook(SQF_Assembly_Iface::InstructionType::GameInstructionArray, [](intercept::types::game_instruction* instr, intercept::types::game_state& state, intercept::types::vm_context& ctx) -> void {
		GlobalEngineHook._scriptInstruction(instr, state, ctx);
	});

    //HI.__instructionBreakpoint = GlobalHookManager.placeHook(hookTypes::instructionBreakpoint, pat_instructionBreakpoint, reinterpret_cast<uintptr_t>(instructionBreakpoint), instructionBreakpointJmpBack, 0);
    //has to jmpback 13CF0B6 wants 0x00000000013cf0af

    //HI.__onScriptError = GlobalHookManager.placeHook(hookTypes::onScriptError, pat_onScriptError, reinterpret_cast<uintptr_t>(onScriptError), onScriptErrorJmpBack, 5);
    scriptPreprocessorDefineDefine = GlobalHookManager.findPattern(pat_scriptPreprocessorDefineDefine);

    HI.scriptPreprocDefine = (scriptPreprocessorDefineDefine != 0);
    if (scriptPreprocessorDefineDefine) //else report error
        HI.scriptPreprocConstr = GlobalHookManager.placeHook(hookTypes::scriptPreprocessorConstructor, pat_scriptPreprocessorConstructor, reinterpret_cast<uintptr_t>(scriptPreprocessorConstructor), scriptPreprocessorConstructorJmpBack, 0xA);

	static auto assertHook = intercept::client::host::register_sqf_command("assert"sv, "", [](uintptr_t, game_value_parameter par) -> game_value {
		return {};
	}, game_data_type::NOTHING, game_data_type::BOOL);
	static auto haltHook = intercept::client::host::register_sqf_command("halt"sv, "", [](uintptr_t, game_value_parameter par) -> game_value {
		return {};
	}, game_data_type::NOTHING, game_data_type::BOOL);
	static auto echoHook = intercept::client::host::register_sqf_command("echo"sv, "", [](uintptr_t, game_value_parameter par) -> game_value {
		return {};
	}, game_data_type::NOTHING, game_data_type::BOOL);
	
	
	HI.scriptAssert = assertHook.has_function();
	HI.scriptHalt = haltHook.has_function();
	HI.scriptEcho = echoHook.has_function();



#else

    HI.__instructionBreakpoint = GlobalHookManager.placeHook(hookTypes::instructionBreakpoint, pat_instructionBreakpoint, reinterpret_cast<uintptr_t>(instructionBreakpoint), instructionBreakpointJmpBack, 1);
    HI.__onScriptError = GlobalHookManager.placeHook(hookTypes::onScriptError, pat_onScriptError, reinterpret_cast<uintptr_t>(onScriptError), onScriptErrorJmpBack, 0);

    scriptPreprocessorDefineDefine = GlobalHookManager.findPattern(pat_scriptPreprocessorDefineDefine);

    HI.scriptPreprocDefine = (scriptPreprocessorDefineDefine != 0);
    if (scriptPreprocessorDefineDefine) //else report error
        HI.scriptPreprocConstr = GlobalHookManager.placeHook(hookTypes::scriptPreprocessorConstructor, pat_scriptPreprocessorConstructor, reinterpret_cast<uintptr_t>(scriptPreprocessorConstructor), scriptPreprocessorConstructorJmpBack, 4);
    HI.scriptAssert = GlobalHookManager.placeHook(hookTypes::onScriptAssert, pat_onScriptAssert, reinterpret_cast<uintptr_t>(onScriptAssert), scriptAssertJmpBack, 0xB7 - 0x95);
    HI.scriptHalt = GlobalHookManager.placeHook(hookTypes::onScriptHalt, pat_onScriptHalt, reinterpret_cast<uintptr_t>(onScriptHalt), scriptHaltJmpBack, 1 + 0xE);
    HI.scriptEcho = GlobalHookManager.placeHook(hookTypes::onScriptEcho, pat_onScriptEcho, reinterpret_cast<uintptr_t>(onScriptEcho), scriptEchoJmpBack, 2);
#endif

    EngineAliveFnc = reinterpret_cast<EngineAlive*>(GlobalHookManager.findPattern(pat_EngineAliveFnc));
    //Find by searching for.  "XML parsing error: cannot read the source file". function call right after start of while loop
    HI.engineAlive = (EngineAliveFnc != nullptr);

    EngineEnableMouseFnc = reinterpret_cast<EngineEnableMouse*>(GlobalHookManager.findPattern(pat_EngineEnableMouseFnc));
    HI.enableMouse = (EngineEnableMouseFnc != nullptr);
    //To yield scriptVM and let engine run while breakPoint hit. 0103C5BB overwrite eax to Yield




#ifndef ScriptProfiling
    HI.__scriptVMConstructor = true;
    HI.__scriptVMSimulateStart = true;
    HI.__scriptVMSimulateEnd = true;
    HI.__worldSimulate = true;
    HI.__worldMissionEventStart = true;
    HI.__worldMissionEventEnd = true;
#endif

    GlobalDebugger.setHookIntegrity(HI);

    if (!
        (	//HI.__scriptVMConstructor
            //&& HI.__scriptVMSimulateStart
            //&& HI.__scriptVMSimulateEnd
            //&& 
            HI.__instructionBreakpoint
            //&& HI.__worldSimulate
            //&& HI.__worldMissionEventStart
            //&& HI.__worldMissionEventEnd
            && HI.__onScriptError
            && HI.scriptPreprocDefine
            && HI.scriptPreprocConstr
            && HI.scriptAssert
            && HI.scriptHalt
            && HI.scriptEcho
            && HI.engineAlive
            && HI.enableMouse)) {
        std::string error("Some hooks have failed. Certain functionality might not be available.\n\n");

        bool fatal = false;
        if (!HI.__scriptVMConstructor) 	  error += "SMVCON	\tFAILED MINOR	\n\tEffect: Not important\n\n";
        if (!HI.__scriptVMSimulateStart)  error += "SVMSIMST\tFAILED MINOR	\n\tEffect: Not important\n\n";
        if (!HI.__scriptVMSimulateEnd)	  error += "SVMSIMEN\tFAILED MINOR	\n\tEffect: Not important\n\n";
        if (!HI.__instructionBreakpoint) { error += "INSTRBP	\tFAILED FATAL	\n\tEffect: No Breakpoints possible.\n\n"; fatal = true; }
        if (!HI.__worldSimulate)		  error += "WSIM	\tFAILED MINOR	\n\tEffect: Not important\n\n";
        if (!HI.__worldMissionEventStart) error += "WMEVS	\tFAILED MINOR	\n\tEffect: Not important\n\n";
        if (!HI.__worldMissionEventEnd)	  error += "WMEVE	\tFAILED MINOR	\n\tEffect: Not important\n\n";
        if (!HI.__onScriptError)		  error += "SCRERR	\tFAILED WARNING	\n\tEffect: script Error will not trigger a Breakpoint\n\n";
        if (!HI.scriptPreprocDefine)	  error += "PREDEF	\tFAILED WARNING	\n\tEffect: Preprocessor Macro \"DEBUG\" will not be available\n\n";
        if (!HI.scriptPreprocConstr)	  error += "PRECON	\tFAILED WARNING	\n\tEffect: Preprocessor Macro \"DEBUG\" will not be available\n\n";
        if (!HI.scriptAssert)			  error += "SCRASS	\tFAILED WARNING	\n\tEffect: Script Command \"assert\" will not trigger a break\n\n";
        if (!HI.scriptHalt)				  error += "SCRHALT	\tFAILED WARNING	\n\tEffect: Script Command \"halt\" will not trigger a break\n\n";
        if (!HI.scriptEcho)				  error += "SCRECHO	\tFAILED WARNING	\n\tEffect: Script Command \"echo\" will not echo anything\n\n";
        if (!HI.engineAlive)			  error += "ALIVE	\tFAILED WARNING	\n\tEffect: Game might think it froze if a breakpoint is triggered\n\n";
        if (!HI.enableMouse)			  error += "ENMOUSE	\tFAILED WARNING	\n\tEffect: Mouse might be stuck in Game and has to be Freed by opening Task-Manager via CTRL+ALT+DEL\n\n";
        if (fatal) error += "\n A Fatal error occured. Your Game version is not compatible with ArmaDebugEngine. Please tell a dev.";

#ifdef X64
        if (!fatal) error += "\n You are running a 64-Bit version. The 64Bit ArmaDebugEngine might be a little behind the 32Bit version in development so non-fatal errors might actually be normal.";
#endif


        //MessageBox(error.c_str(), fatal ? ErrorMsgBoxType::error : ErrorMsgBoxType::warning);
    }

    //Tracker::trackPiwik();
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
    auto script = context->getScriptByContent(scVM->_context._doc._content);
    if (!scVM->_context._doc._fileName.isNull() || !scVM->_context._lastInstructionPos._sourceFile.isNull())
        script->_fileName = scVM->_context._doc._fileName.isNull() ? scVM->_context._lastInstructionPos._sourceFile : scVM->_context._doc._fileName;
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

void EngineHook::_scriptInstruction(intercept::types::game_instruction* instr, intercept::types::game_state& state, intercept::types::vm_context& ctxi) {
    globalTimeKeeper _tc;
    //auto start = std::chrono::high_resolution_clock::now();

    auto instruction = reinterpret_cast<RV_GameInstruction *>(instr);
    auto ctx = reinterpret_cast<RV_VMContext *>(&ctxi);
    auto gs = reinterpret_cast<GameState *>(&state);
#ifdef OnlyOneInstructionPerLine
    if (instruction->_scriptPos._sourceLine != lastInstructionLine || instruction->_scriptPos._content.data() != lastInstructionFile) {
#endif
        DebuggerInstructionInfo info{ instruction, ctx, gs };
        GlobalDebugger.onInstruction(info);
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

void EngineHook::_onScriptError(uintptr_t gameSate) {
    auto gs = reinterpret_cast<GameState *>(gameSate);
    if (gs && gs->GEval->_errorType != 0)
        GlobalDebugger.onScriptError(gs);
}

void EngineHook::_onScriptAssert(uintptr_t gameSate) {
    auto gs = reinterpret_cast<GameState *>(gameSate);
    GlobalDebugger.onScriptAssert(gs);
}

void EngineHook::_onScriptHalt(uintptr_t gameSate) {
    auto gs = reinterpret_cast<GameState *>(gameSate);
    GlobalDebugger.onScriptHalt(gs);
}

void EngineHook::_onScriptEcho(uintptr_t gameValue) {
    auto gv = reinterpret_cast<GameValue *>(gameValue);
    if (!gv->_data) return;
    auto msg = gv->_data->getAsString();
    GlobalDebugger.onScriptEcho(msg);
}

void EngineHook::onShutdown() {
    GlobalDebugger.onShutdown();
}

void EngineHook::onStartup() {
    placeHooks();
    GlobalDebugger.onStartup();
}

void intercept::pre_start() {
	
	GASM.init();
	GlobalEngineHook.placeHooks();
	GlobalDebugger.onStartup();

}
