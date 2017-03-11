#pragma once
#include <string>

class RString;

class Script {
public:
    Script(RString content);
    ~Script();
    void dbg_instructionExec();
    uint32_t instructionCount{ 0 };
    std::string _content;
    std::string _fileName;
};

