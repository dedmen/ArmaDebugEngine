#pragma once
#include <cstdint>
#include <map>
#include <memory>

struct RV_VMContext;
class Script;
class VMContext;

class Debugger {
public:
    Debugger();
    ~Debugger();


    void clear();
    std::shared_ptr<VMContext> getVMContext(RV_VMContext* vm);
    void writeFrameToFile(uint32_t frameCounter);

    std::map<uintptr_t, std::shared_ptr<VMContext>> VMPtrToScript;

};

