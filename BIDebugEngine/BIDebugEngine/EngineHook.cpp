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
    uintptr_t onScriptErrorJmpBack;

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

HookManager::Pattern pat_instructionBreakpoint{
    "xxxx?xxxx?xxxxxxxx????xxxxxxxxxxxx????xxxxxxxxxxxxxxx",
    "\x49\x8B\x44\x24\x00\x49\x8D\x54\x24\x00\x48\x85\xC0\x74\x22\x48\x8D\x0D\x00\x00\x00\x00\x48\x83\xC0\x10\x48\x3B\xC1\x74\x12\x48\x8B\x8F\x00\x00\x00\x00\x48\x85\xC9\x74\x06\x48\x8B\x01\xFF\x50\x08\x49\x8B\x04\x24",
    -0x3C
};

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
#endif

//#TODO onVariableChanged event
//#TODO hook https://community.bistudio.com/wiki/halt https://community.bistudio.com/wiki/echo https://community.bistudio.com/wiki/assert

scriptExecutionContext currentContext = scriptExecutionContext::Invalid;
MissionEventType currentEventHandler = MissionEventType::Ended; //#TODO create some invalid handler type

void EngineHook::placeHooks() {
    WAIT_FOR_DEBUGGER_ATTACHED;

    char* productType = reinterpret_cast<char*>(GlobalHookManager.findPattern(pat_productType));
    char* productVersion = reinterpret_cast<char*>(GlobalHookManager.findPattern(pat_productVersion));
    OutputDebugStringA("Product Type: ");
    OutputDebugStringA(productType);
    OutputDebugStringA("\t\tVersion: ");
    OutputDebugStringA(productVersion);
    OutputDebugStringA("\n");
    productType;

    bool* isDebuggerAttached = reinterpret_cast<bool*>(GlobalHookManager.findPattern(pat_IsDebuggerAttached));
    *isDebuggerAttached = false; //Small hack to keep RPT logging while Debugger is attached
                                 //Could also patternFind and patch (profv3 0107144F) to unconditional jmp

#ifdef X64
    GlobalHookManager.placeHook(hookTypes::scriptVMConstructor, pat_scriptVMConstructor, reinterpret_cast<uintptr_t>(scriptVMConstructor), scriptVMConstructorJmpBack, 3);
    GlobalHookManager.placeHook(hookTypes::scriptVMSimulateStart, pat_scriptVMSimulateStart, reinterpret_cast<uintptr_t>(scriptVMSimulateStart), scriptVMSimulateStartJmpBack, 2);
    GlobalHookManager.placeHook(hookTypes::scriptVMSimulateEnd, pat_scriptVMSimulateEnd, reinterpret_cast<uintptr_t>(scriptVMSimulateEnd));
    GlobalHookManager.placeHook(hookTypes::instructionBreakpoint, pat_instructionBreakpoint, reinterpret_cast<uintptr_t>(instructionBreakpoint), instructionBreakpointJmpBack, 0);
    //has to jmpback 13CF0B6 wants 0x00000000013cf0af
    GlobalHookManager.placeHook(hookTypes::worldSimulate, pat_worldSimulate, reinterpret_cast<uintptr_t>(worldSimulate), worldSimulateJmpBack, 1);
    //GlobalHookManager.placeHook(hookTypes::worldMissionEventStart, pat_worldMissionEventStart, reinterpret_cast<uintptr_t>(worldMissionEventStart), worldMissionEventStartJmpBack, 2);
    //GlobalHookManager.placeHook(hookTypes::worldMissionEventEnd, pat_worldMissionEventEnd, reinterpret_cast<uintptr_t>(worldMissionEventEnd), worldMissionEventEndJmpBack, 1);

#else
    GlobalHookManager.placeHook(hookTypes::scriptVMConstructor, pat_scriptVMConstructor, reinterpret_cast<uintptr_t>(scriptVMConstructor), scriptVMConstructorJmpBack, 3);
    GlobalHookManager.placeHook(hookTypes::scriptVMSimulateStart, pat_scriptVMSimulateStart, reinterpret_cast<uintptr_t>(scriptVMSimulateStart), scriptVMSimulateStartJmpBack, 1);
    GlobalHookManager.placeHook(hookTypes::scriptVMSimulateEnd, pat_scriptVMSimulateEnd, reinterpret_cast<uintptr_t>(scriptVMSimulateEnd));
    GlobalHookManager.placeHook(hookTypes::instructionBreakpoint, pat_instructionBreakpoint, reinterpret_cast<uintptr_t>(instructionBreakpoint), instructionBreakpointJmpBack, 1);
    GlobalHookManager.placeHook(hookTypes::worldSimulate, pat_worldSimulate, reinterpret_cast<uintptr_t>(worldSimulate), worldSimulateJmpBack, 1);
    GlobalHookManager.placeHook(hookTypes::worldMissionEventStart, pat_worldMissionEventStart, reinterpret_cast<uintptr_t>(worldMissionEventStart), worldMissionEventStartJmpBack, 2);
    GlobalHookManager.placeHook(hookTypes::worldMissionEventEnd, pat_worldMissionEventEnd, reinterpret_cast<uintptr_t>(worldMissionEventEnd), worldMissionEventEndJmpBack, 1);
    GlobalHookManager.placeHook(hookTypes::onScriptError, pat_onScriptError, reinterpret_cast<uintptr_t>(onScriptError), onScriptErrorJmpBack, 0);
#endif

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

void EngineHook::_onScriptError(uintptr_t gameSate) {
    auto gs = reinterpret_cast<GameState *>(gameSate);
    GlobalDebugger.onScriptError(gs);
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


    /*
    32bit
        jmp 0x123122
        0:  e9 1e 31 12 00          jmp    123123 <_main+0x123123>
    64bit
        FF 25 64bit relative
    */
#ifdef X64
    //auto distance = std::max(totalOffset, jmpTo) - std::min(totalOffset, jmpTo);
    // if distance < 2GB (2147483648) we could use the 32bit relative jmp
    VirtualProtect(reinterpret_cast<LPVOID>(totalOffset), 14u, 0x40u, &dwVirtualProtectBackup);
    auto jmpInstr = reinterpret_cast<unsigned char*>(totalOffset);
    auto addrOffs = reinterpret_cast<uint32_t*>(totalOffset + 1);
    *jmpInstr = 0x68; //push DWORD
    *addrOffs = jmpTo /*- totalOffset - 6*/;//offset
    *reinterpret_cast<uint32_t*>(totalOffset + 5) = 0x042444C7; //MOV [RSP+4],
    *reinterpret_cast<uint32_t*>(totalOffset + 9) = ((uint64_t) jmpTo) >> 32;//DWORD
    *reinterpret_cast<unsigned char*>(totalOffset + 13) = 0xc3;//ret
    VirtualProtect(reinterpret_cast<LPVOID>(totalOffset), 14u, dwVirtualProtectBackup, &dwVirtualProtectBackup);
    return totalOffset + 14;
#else
    VirtualProtect(reinterpret_cast<LPVOID>(totalOffset), 5u, 0x40u, &dwVirtualProtectBackup);
    auto jmpInstr = reinterpret_cast<unsigned char *>(totalOffset);
    auto addrOffs = reinterpret_cast<unsigned int *>(totalOffset + 1);
    *jmpInstr = 0xE9;
    *addrOffs = jmpTo - totalOffset - 5;
    VirtualProtect(reinterpret_cast<LPVOID>(totalOffset), 5u, dwVirtualProtectBackup, &dwVirtualProtectBackup);
    return totalOffset + 5;
#endif


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
    if (pat.offsetFunc) {
        auto found = findPattern(pat.pattern, pat.mask, pat.offset + offset);
        if (found)
            return pat.offsetFunc(found);
        return found;
    }

    return findPattern(pat.pattern, pat.mask, pat.offset + offset);
}
