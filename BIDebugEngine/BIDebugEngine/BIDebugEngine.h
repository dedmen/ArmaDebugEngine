#pragma once
class IDebugScript;

class EngineInterface {
public:
    EngineInterface() {}
    virtual ~EngineInterface() {}

    virtual void LogF(const char *str) = 0;
};

class DllInterface {
public:
    DllInterface() {}
    virtual ~DllInterface() {}

    virtual void Startup();
    virtual void Shutdown();

    virtual void ScriptLoaded(IDebugScript *script, const char *name);
    virtual void ScriptEntered(IDebugScript *script);
    virtual void ScriptTerminated(IDebugScript *script);

    virtual void FireBreakpoint(IDebugScript *script, unsigned int bp);
    virtual void Breaked(IDebugScript *script);

    virtual void DebugEngineLog(const char *str);
};

