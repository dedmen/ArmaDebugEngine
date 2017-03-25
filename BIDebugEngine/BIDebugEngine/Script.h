#pragma once
#include <cstdint>
#include <string>
#include "RVBaseTypes.h"
class SourceDocPos;

class Script {
public:
    Script(RString content);
    ~Script();
    void dbg_instructionExec();
    uint32_t instructionCount{ 0 };
    RString _content;
    RString _fileName;

	static int getScriptLineOffset(SourceDocPos& pos);
	static std::string getScriptFromFirstLine(SourceDocPos& pos, bool compact = false);

};



