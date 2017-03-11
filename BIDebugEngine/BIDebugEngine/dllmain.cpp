// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include <string>
#include <Psapi.h>
#include "GlobalHeader.h"
#pragma comment (lib, "Psapi.lib")//GetModuleInformation
extern uintptr_t engineAlloc;
extern uintptr_t globalAlivePtr;
EngineAlive* EngineAliveFnc;

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
) {
    WAIT_FOR_DEBUGGER_ATTACHED


        switch (ul_reason_for_call) {
            case DLL_PROCESS_ATTACH:
            case DLL_THREAD_ATTACH:
            case DLL_THREAD_DETACH:
            case DLL_PROCESS_DETACH:
                break;
        }

    return TRUE;
}

extern "C" BOOL WINAPI _DllMainCRTStartup(
    HINSTANCE const instance,
    DWORD     const reason,
    LPVOID    const reserved
);

BOOL APIENTRY _RawDllMain(HMODULE, DWORD reason, LPVOID) {
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    //Post entry point and pre DllMain

    WAIT_FOR_DEBUGGER_ATTACHED


        //Get engine allocator - From my Intercept fork
        //Find the allocator base
        //This has to happen pre CRTInit because static/global Variables may need to alloc in Engine
        MODULEINFO modInfo = { 0 };
    HMODULE hModule = GetModuleHandleA(nullptr);
    GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));

    auto findInMemory = [&modInfo](char* pattern, size_t patternLength) ->uintptr_t {
        uintptr_t base = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
        uintptr_t size = static_cast<uintptr_t>(modInfo.SizeOfImage);
        for (DWORD i = 0; i < size - patternLength; i++) {
            bool found = true;
            for (DWORD j = 0; j < patternLength; j++) {
                found &= pattern[j] == *reinterpret_cast<char*>(base + i + j);
                if (!found)
                    break;
            }
            if (found)
                return base + i;
        }
        return 0;
    };

    auto getRTTIName = [](uintptr_t vtable) -> const char* {
        uintptr_t typeBase = *((uintptr_t*) (vtable - 4));
        uintptr_t type = *((uintptr_t*) (typeBase + 0xC));
        return (char*) (type + 9);
    };



    uintptr_t stringOffset = findInMemory("tbb4malloc_bi", 13);

    uintptr_t allocatorVtablePtr = (findInMemory((char*) &stringOffset, 4) - 4);
    const char* test = getRTTIName(*reinterpret_cast<uintptr_t*>(allocatorVtablePtr));
    engineAlloc = allocatorVtablePtr;

    EngineAliveFnc = reinterpret_cast<EngineAlive*>(reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll) + 0x10454B0);
    //Find by searching for.  "XML parsing error: cannot read the source file". function call right after start of while loop


    return TRUE;
}

extern "C" BOOL(WINAPI * const _pRawDllMain)(HINSTANCE, DWORD, LPVOID) = &_RawDllMain;


//Alternative entry point that can be specified in Linker settings that will be called before _RawDllMain
extern "C" BOOL WINAPI _DllPreAttach(
    HINSTANCE const instance,
    DWORD     const reason,
    LPVOID    const reserved
) {
    //Entry point
    WAIT_FOR_DEBUGGER_ATTACHED

        return _DllMainCRTStartup(instance, reason, reserved);
};

//kju asked for this in discord Feb, 18 2017 #tools_makers
bool shouldCreate(std::string inFile, std::string outfile) {
    HANDLE inputFileHandle = CreateFileA(inFile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!inputFileHandle) return false; //If I can't open the input File I also can't convert it.
    HANDLE outputFileHandle = CreateFileA(outfile.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!outputFileHandle) return true; //If output file doesn't exist I'll create it.
    FILETIME LastWriteTimeInputFile;
    FILETIME LastWriteTimeOutputFile;
    if (!GetFileTime(inputFileHandle, NULL, NULL, &LastWriteTimeInputFile)) return true; //If we can't get time just convert it anyway
    if (!GetFileTime(outputFileHandle, NULL, NULL, &LastWriteTimeOutputFile)) return true;
    //If input file was lastEdited after output file
    if (
        _ULARGE_INTEGER{ LastWriteTimeInputFile.dwLowDateTime, LastWriteTimeInputFile.dwHighDateTime }.QuadPart
            >
        _ULARGE_INTEGER{ LastWriteTimeOutputFile.dwLowDateTime, LastWriteTimeOutputFile.dwHighDateTime }.QuadPart
        ) return true;

    return false;
}