// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>
#include <string>

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }





    return TRUE;
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