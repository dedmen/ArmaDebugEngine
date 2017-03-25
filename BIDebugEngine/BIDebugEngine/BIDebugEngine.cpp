//BI Debug Engine Interface

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN 
struct IUnknown; //Clang compiler error in windows.h

#include "BIDebugEngine.h"

#include "EngineHook.h"
static DllInterface dllIface;
static EngineInterface* engineIface;
extern "C" EngineHook GlobalEngineHook;
uintptr_t engineAlloc;

void DllInterface::Startup() {
    GlobalEngineHook.onStartup();
}

void DllInterface::Shutdown() {
    GlobalEngineHook.onShutdown();
}

void DllInterface::ScriptLoaded(IDebugScript *script, const char *name) {

}

void DllInterface::ScriptEntered(IDebugScript *script) {

}

void DllInterface::ScriptTerminated(IDebugScript *script) {

}

void DllInterface::FireBreakpoint(IDebugScript *script, unsigned int bp) {

}

void DllInterface::Breaked(IDebugScript *script) {

}

void DllInterface::DebugEngineLog(const char *str) {

}

DllInterface* Connect(EngineInterface *engineInt) {
    engineIface = engineInt;
    return &dllIface;
}