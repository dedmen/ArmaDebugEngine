#include "Script.h"

Script::Script(RString content) {//#TODO change to taking docPos as constructor
    _content = content;
    _fileName = "";
}

Script::~Script() {}

void Script::dbg_instructionExec() {
    ++instructionCount;
}

int Script::getScriptLineOffset(SourceDocPos& pos) {
    if (pos._content.isNull()) return 0;
    const char* curOffs = pos._content.data() + pos._pos;
    int lineOffset = 0;
    while (*curOffs != '\n' && curOffs > pos._content.data()) {
        curOffs--;
        lineOffset++;
    }
    return std::max(lineOffset - 1, 0);
}

std::string Script::getScriptFromFirstLine(SourceDocPos& pos, bool compact) {
    if (pos._content.isNull()) return pos._content.data();
    auto needSourceFile = pos._sourceFile.isNull();
    int line = pos._sourceLine + 1;
    auto start = pos._content.cbegin();
    auto end = pos._content.cend();
    std::string filename(needSourceFile ? "" : pos._sourceFile.data());
    auto curPos = start;
    std::string output;
    bool inWantedFile = needSourceFile;
    output.reserve(end - start);
    auto readLineMacro = [&]() {
        curPos += 6;
        auto numberEnd = std::find(curPos, end, ' ');
        auto number = std::stoi(std::string(curPos, 0, numberEnd - curPos));
        curPos = numberEnd + 2;
        auto nameEnd = std::find(curPos, end, '"');
        std::string name(curPos, 0, nameEnd - curPos);
        if (needSourceFile) {
            needSourceFile = false;
            filename = name;
        }
        bool wasInWantedFile = inWantedFile;
        inWantedFile = (name == filename);
        curPos = nameEnd + 2;
        //if (inWantedFile && *curPos == '\n') {
        //	curPos++;
        //}//after each #include there is a newline which we also don't want


        if (wasInWantedFile) {
            output.append("#include \"");
            output.append(name);
            output.append("\"");
        }


        if (curPos > end) return false;
        return true;
    };
    auto readLine = [&]() {
        if (curPos > end) return false;
        if (*curPos == '#') return readLineMacro();
        auto lineEnd = std::find(curPos, end, '\n') + 1;
        if (inWantedFile) {
            output.append(curPos, lineEnd - curPos);
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
