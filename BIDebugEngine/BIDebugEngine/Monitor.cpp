#include "Monitor.h"
#include "Debugger.h"
#include "Serialize.h"
#include <fstream>

void Monitor_knownScriptFiles::onInstruction(Debugger*, const DebuggerInstructionInfo& instructionInfo) {
    scriptLines[instructionInfo.instruction->sdp.sourcefile].insert(instructionInfo.instruction->sdp.sourceline);
}

void Monitor_knownScriptFiles::onShutdown() {
    dump();
}

void Monitor_knownScriptFiles::dump() {
    JsonArchive ar;
    for (auto& it : scriptLines) {
        std::vector<uint32_t> lines;
        for (auto& line : it.second)
            lines.push_back(line);
        std::sort(lines.begin(), lines.end());
        lines.erase(std::unique(lines.begin(), lines.end()), lines.end()); //Remove duplicates. std::unordered_set doesn't seem to obey my orders
        ar.Serialize(it.first.data(), lines);
    }
    auto text = ar.to_string();
    std::ofstream f("P:\\Monitor_knownScriptFiles.json");
    f.write(text.c_str(), text.length());
    f.close();
}
