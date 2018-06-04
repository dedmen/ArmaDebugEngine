#pragma once
#include <cstdint>
#include <string>
#include "RVBaseTypes.h"

namespace intercept {
    namespace types {
        struct sourcedocpos;
    }
}

class Script {
public:
    Script(r_string content);
    ~Script();
    void dbg_instructionExec();
    uint32_t instructionCount{ 0 };
    r_string _content;
    r_string _fileName;

    static uint32_t getScriptLineOffset(const intercept::types::sourcedocpos& pos);
    static std::string getScriptFromFirstLine(const intercept::types::sourcedocpos& pos, bool compact = false);

};



