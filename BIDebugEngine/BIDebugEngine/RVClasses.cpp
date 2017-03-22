#include "RVClasses.h"
#include "Serialize.h"
#include "Script.h"

void RV_GameInstruction::Serialize(JsonArchive& ar) {



    ar.Serialize("type", typeid(*this).name());
    ar.Serialize("name", GetDebugName());
    ar.Serialize("filename", _scriptPos._sourceFile);

    ar.Serialize("fileOffset", { _scriptPos._sourceLine, _scriptPos._pos, Script::getScriptLineOffset(_scriptPos) });

}
