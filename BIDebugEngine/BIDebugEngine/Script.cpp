#include "Script.h"
#include <types.hpp>

Script::Script(r_string content) {//#TODO change to taking docPos as constructor
    _content = content;
    _fileName = "";
}

Script::~Script() {}

void Script::dbg_instructionExec() {
    ++instructionCount;
}

uint32_t Script::getScriptLineOffset(const intercept::types::sourcedocpos& pos) {
    if (pos.content.empty()) return 0;
    const char* curOffs = pos.content.data() + pos.pos;
    int lineOffset = 0;
    while (*curOffs != '\n' && curOffs > pos.content.data()) {
        curOffs--;
        lineOffset++;
    }
    return std::max(lineOffset - 1, 0);
}

std::string Script::getScriptFromFirstLine(const intercept::types::sourcedocpos& pos, bool compact) {
    if (pos.content.empty()) return pos.content.data();
    auto needSourceFile = pos.sourcefile.empty();
    int line = pos.sourceline + 1;
    auto start = pos.content.begin();
    auto end = pos.content.end();
    std::string filename(needSourceFile ? "" : pos.sourcefile.data());
    std::transform(filename.begin(), filename.end(), filename.begin(), tolower);
    auto curPos = start;
    auto curLine = 1U;
    std::string output;
    bool inWantedFile = needSourceFile;
    output.reserve(end - start);

    auto removeEmptyLines = [&](int count) {
        for (size_t i = 0; i < count; i++) {
            auto found = output.find("\n\n");
            if (found != std::string::npos)
                output.replace(found, 2, 1, '\n');
            else if (output.front() == '\n') {
                    output.erase(0,1);
            } else {
                output.replace(0, output.find("\n"), 1, '\n');
            }
        }
    };

    auto readLineMacro = [&]() {
        curPos += 6;
        auto numberEnd = std::find(curPos, end, ' ');
        auto number = std::stoi(std::string(*curPos, numberEnd - curPos));
        curPos = numberEnd + 2;
        auto nameEnd = std::find(curPos, end, '"');
        std::string name(*curPos, nameEnd - curPos);
        std::transform(name.begin(), name.end(), name.begin(), tolower);
        if (needSourceFile) {
            needSourceFile = false;
            filename = name;
        }
        bool wasInWantedFile = inWantedFile;
        inWantedFile = (name == filename);
        if (inWantedFile) {
            if (number < curLine) removeEmptyLines((curLine - number));
            curLine = number;
        }
        if (*(nameEnd + 1) == '\r') nameEnd++;
        curPos = nameEnd + 2;
        //if (inWantedFile && *curPos == '\n') {
        //	curPos++;
        //}//after each #include there is a newline which we also don't want


        if (wasInWantedFile) {
            output.append("#include \"");
            output.append(name);
            output.append("\"\n");
            curLine++;
        }


        if (curPos > end) return false;
        return true;
    };
    auto readLine = [&]() {
        if (curPos > end) return false;
        if (*curPos == '#') return readLineMacro();
        auto lineEnd = std::find(curPos, end, '\n') + 1;
        if (inWantedFile) {
            output.append(*curPos, lineEnd - curPos);
            curLine++;
        }
        //line is curPos -> lineEnd
        curPos = lineEnd;
        if (curPos > end) return false;
        return true;
    };
    while (readLine()) {};
    if (compact) {
        //http://stackoverflow.com/a/24315631
        size_t start_pos = 0;
        while ((start_pos = output.find("\n\n\n", 0)) != std::string::npos) {
            output.replace(start_pos, 3, "\n");
        }
    }
    return output;
}
