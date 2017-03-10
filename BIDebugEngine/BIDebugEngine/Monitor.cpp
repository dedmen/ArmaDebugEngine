#include "Monitor.h"
#include "Debugger.h"
#include "Serialize.h"
#include <fstream>

void Monitor_knownScriptFiles::onInstruction(Debugger*, const DebuggerInstructionInfo& instructionInfo) {
    scriptLines[instructionInfo.instruction->_scriptPos._sourceFile].insert(instructionInfo.instruction->_scriptPos._sourceLine);
}

void Monitor_knownScriptFiles::onShutdown() {
    JsonArchive ar;
    for (auto& it : scriptLines) {
        std::vector<uint32_t> lines;
        for (auto& line : it.second)
            lines.push_back(line);
        std::sort(lines.begin(), lines.end());
        ar.Serialize(it.first.data(), lines);
    }
    auto text = ar.to_string();
    std::ofstream f("P:\\Monitor_knownScriptFiles.json");
    f.write(text.c_str(), text.length());
    f.close();
}
