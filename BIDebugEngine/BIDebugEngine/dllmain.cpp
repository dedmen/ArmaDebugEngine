// dllmain.cpp : Defines the entry point for the DLL application.
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN 
#include "EngineHook.h"
#include <string>
#include <future>
struct IUnknown; //Clang compiler error in windows.h
#include <windows.h>
#include <Psapi.h>
#include "GlobalHeader.h"
#pragma comment (lib, "Psapi.lib")//GetModuleInformation
#pragma comment (lib, "version.lib")
#include <intercept.hpp>
#undef OutputDebugString
#undef MessageBox


int intercept::api_version() {
    return 1;
}

extern uintptr_t engineAlloc;
extern uintptr_t globalAlivePtr;
std::string gameVersionResource;
BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
) {
    WAIT_FOR_DEBUGGER_ATTACHED;
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: {

            WCHAR fileName[_MAX_PATH];
            DWORD size = GetModuleFileName(nullptr, fileName, _MAX_PATH);
            fileName[size] = NULL;
            DWORD handle = 0;
            size = GetFileVersionInfoSize(fileName, &handle);
            BYTE* versionInfo = new BYTE[size];
            if (GetFileVersionInfo(fileName, handle, size, versionInfo)) {
                UINT    			len = 0;
                VS_FIXEDFILEINFO*   vsfi = nullptr;
                VerQueryValue(versionInfo, L"\\", reinterpret_cast<void**>(&vsfi), &len);

                short version = HIWORD(vsfi->dwFileVersionLS);
                short version2 = LOWORD(vsfi->dwFileVersionLS);
                short minor = HIWORD(vsfi->dwFileVersionMS);
                short minor2 = LOWORD(vsfi->dwFileVersionMS);
                gameVersionResource = std::to_string(minor) + "." + std::to_string(minor2) + (version < 100 ? ".0" : ".") + std::to_string(version) + (version2 < 100 ? "0" : "") + std::to_string(version2);
            }
            delete[] versionInfo;

        }
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

    WAIT_FOR_DEBUGGER_ATTACHED;


	//Intercept Loader - Written by me
	MODULEINFO modInfo = { nullptr };
	HMODULE hModule = GetModuleHandleA(nullptr);
	GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));
	const uintptr_t baseAddress = reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll);
	const uintptr_t moduleSize = static_cast<uintptr_t>(modInfo.SizeOfImage);


	auto findInMemory = [baseAddress, moduleSize](const char* pattern, size_t patternLength) ->uintptr_t {
		const uintptr_t base = baseAddress;
		const uintptr_t size = moduleSize;
		for (uintptr_t i = 0; i < size - patternLength; i++) {
			bool found = true;
			for (uintptr_t j = 0; j < patternLength; j++) {
				found &= pattern[j] == *reinterpret_cast<char*>(base + i + j);
				if (!found)
					break;
			}
			if (found)
				return base + i;
		}
		return 0;
	};

	auto findInMemoryPattern = [baseAddress, moduleSize](const char* pattern, const char* mask, uintptr_t offset = 0) {
		const uintptr_t base = baseAddress;
		const uintptr_t size = moduleSize;

		const uintptr_t patternLength = static_cast<uintptr_t>(strlen(mask));

		for (uintptr_t i = 0; i < size - patternLength; i++) {
			bool found = true;
			for (uintptr_t j = 0; j < patternLength; j++) {
				found &= mask[j] == '?' || pattern[j] == *reinterpret_cast<char*>(base + i + j);
				if (!found)
					break;
			}
			if (found)
				return base + i + offset;
		}
		return static_cast<uintptr_t>(0x0u);
	};

	auto getRTTIName = [](uintptr_t vtable) -> const char* {
		class v1 {
			virtual void doStuff() {}
		};
		class v2 : public v1 {
			virtual void doStuff() {}
		};
		v2* v = (v2*) vtable;
		auto& typex = typeid(*v);
#ifdef __GNUC__
		auto test = typex.name();
#else
		auto test = typex.raw_name();
#endif
		return test;
	};



	auto future_stringOffset =findInMemory("tbb4malloc_bi", 13);



	auto future_allocatorVtablePtr = [&]() {
		uintptr_t stringOffset = future_stringOffset;
#ifndef __linux__
		return (findInMemory(reinterpret_cast<char*>(&stringOffset), sizeof(uintptr_t)) - sizeof(uintptr_t));
#else
		uintptr_t vtableStart = stringOffset - (0x09D20C70 - 0x09D20BE8);
		return vtableStart;
		//return (findInMemory(reinterpret_cast<char*>(&vtableStart), 4));
#endif

	}();

	const uintptr_t allocatorVtablePtr = future_allocatorVtablePtr;


	engineAlloc = allocatorVtablePtr;

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
    WAIT_FOR_DEBUGGER_ATTACHED;

    return _DllMainCRTStartup(instance, reason, reserved);
};

void OutputDebugString(const char* lpOutputString) {
    OutputDebugStringA(lpOutputString);
}

int MessageBox(const char* text, ErrorMsgBoxType type) {
    return MessageBoxA(0, text, "ArmaDebugEngine", (type == ErrorMsgBoxType::error ? MB_ICONERROR : MB_ICONWARNING) | MB_OK | MB_SYSTEMMODAL | MB_TOPMOST);
}



//This is here because I want to keep includes of windows.h low
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
#ifdef _DEBUG
        __debugbreak(); //#TODO report somehow
#endif
        return false;
    }
    jmpBackRef = placeHookTotalOffs(found, jmpTo) + jmpBackOffset;
    return true;
}

bool HookManager::placeHook(hookTypes, const Pattern & pat, uintptr_t jmpTo) {
    auto found = findPattern(pat);
    if (found == 0) {
#ifdef _DEBUG
        __debugbreak(); //#TODO report somehow
#endif
        return false;
    }
    placeHookTotalOffs(found, jmpTo);
    return true;
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
    *addrOffs = static_cast<uint32_t>(jmpTo) /*- totalOffset - 6*/;//offset
    *reinterpret_cast<uint32_t*>(totalOffset + 5) = 0x042444C7; //MOV [RSP+4],
    *reinterpret_cast<uint32_t*>(totalOffset + 9) = static_cast<uint64_t>(jmpTo) >> 32;//DWORD
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


bool HookManager::MatchPattern(uintptr_t addr, const char* pattern, const char* mask) {
    size_t size = strlen(mask);
    if (IsBadReadPtr((void*) addr, size))
        return false;
    bool found = true;
    for (size_t j = 0; j < size; j++) {
        found &= mask[j] == '?' || pattern[j] == *(char*) (addr + j);
    }
    if (found)
        return true;
    return false;
}

uintptr_t HookManager::findPattern(const char* pattern, const char* mask, uintptr_t offset /*= 0*/) {
    uintptr_t base = engineBase;
    uint32_t size = engineSize;

    uintptr_t patternLength = (DWORD) strlen(mask);

    for (uintptr_t i = 0; i < size - patternLength; i++) {
        bool found = true;
        for (uintptr_t j = 0; j < patternLength; j++) {
            found &= mask[j] == '?' || pattern[j] == *reinterpret_cast<char*>(base + i + j);
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
