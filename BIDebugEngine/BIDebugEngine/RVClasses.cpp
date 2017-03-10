#include "RVClasses.h"
#include "Serialize.h"

void RV_GameInstruction::Serialize(JsonArchive& ar) {


    
    ar.Serialize("type", typeid(*this).name());
    ar.Serialize("name", GetDebugName());
    ar.Serialize("filename", _scriptPos._sourceFile);

    //Get offset from start of the line
    const char* curOffs = _scriptPos._content.data() + _scriptPos._pos;
    int lineOffset = 0;
    while (*curOffs != '\n') {
        curOffs--;
        lineOffset++;
    }
       

    ar.Serialize("fileOffset", { _scriptPos._sourceLine, _scriptPos._pos, lineOffset -1});

}