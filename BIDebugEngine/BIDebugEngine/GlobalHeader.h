#pragma once

// SFINAE test
template <typename T>
class has_Serialize2 {
    typedef char one;
    typedef long two;

    template <typename C> static one test(char[sizeof(&C::Serialize)]);
    template <typename C> static two test(...);

public:
    enum { value = sizeof(test<T>(0)) == sizeof(char) };
};


template<typename Class>
class has_Serialize {
    typedef char(&yes)[2];

    template<typename> struct Exists; // <--- changed

    template<typename V>
    static yes CheckMember(Exists<decltype(&V::Serialize)>*); // <--- changed (c++11)
    template<typename>
    static char CheckMember(...);

public:
    static const bool value = (sizeof(CheckMember<Class>(0)) == sizeof(yes));
};



class test1 {
public:
    void Serialize() {};
};

class test2 {
public:
    void Serialize2() {};
};

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

enum class haltType {
    breakpoint,
    step,
    error,
    assert,
    halt
};

#ifdef _DEBUG
#define WAIT_FOR_DEBUGGER_ATTACHED while (!IsDebuggerPresent()) Sleep(1);
#else
#define WAIT_FOR_DEBUGGER_ATTACHED 
#endif


typedef void EngineAlive(); //Call this periodically while in breakState
#ifdef X64
typedef void EngineEnableMouse(char enabled);
#else
typedef void EngineEnableMouse(bool enabled);
#endif
