#include "Script.h"
#include "RVClasses.h"

Script::Script(RString content) {//#TODO change to taking docPos as constructor
    _content = content;
    _fileName = "";
}

Script::~Script() {}

void Script::dbg_instructionExec() {
    ++instructionCount;
}
