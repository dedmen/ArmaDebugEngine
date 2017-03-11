#pragma once
#include <cstdint>
#include <utility>
#include <map>
#include <vector>
#include <array>

enum class hookTypes {
    worldSimulate,   //FrameEnd/FrameStart
    instructionBreakpoint,
    scriptVMConstructor,    //ScriptLoaded
    scriptVMSimulateStart, //ScriptEntered
    scriptVMSimulateEnd,     //ScriptTerminated/ScriptLeft
    worldMissionEventStart,
    worldMissionEventEnd,
    End
};

//This manages all Hooks into Engine code
class EngineHook {
public:
    EngineHook();
    ~EngineHook();
    void placeHooks();
    void removeHooks(bool leavePFrameHook = true);



    //callbacks
    void _worldSimulate();
    void _scriptLoaded(uintptr_t scrVMPtr);
    void _scriptEntered(uintptr_t scrVMPtr);
    void _scriptTerminated(uintptr_t scrVMPtr);
    void _scriptLeft(uintptr_t scrVMPtr);
    void _scriptInstruction(uintptr_t instructionBP_Instruction, uintptr_t instructionBP_VMContext, uintptr_t instructionBP_gameState, uintptr_t instructionBP_IDebugScript);
    void _world_OnMissionEventStart(uintptr_t eventType);
    void _world_OnMissionEventEnd();
    void onShutdown();
    void onStartup();
private:



    uintptr_t placeHook(uintptr_t offset, uintptr_t jmpTo) const;
    std::map<hookTypes, std::pair<uintptr_t, std::vector<char>>> _unhookData; //Storing binary data to undo the hook later on
    uintptr_t engineBase;
    std::array<char, static_cast<int>(hookTypes::End)> _hooks; //#TODO find a more elegant way.. I don't like to static cast everytime
};


