#include "Serialize.h"



Serialize::Serialize() {

}


Serialize::~Serialize() {}

std::string JsonArchive::to_string() {
    return pJson->dump(); // keep it all on one line, no prettifying or there will be no way for clients to separate the objects
}

void JsonArchive::Serialize(const char* key, const r_string& value) {
    if (isReading) __debugbreak(); //can't read this
    (*pJson)[key] = value.data();
}

void JsonArchive::Serialize(const char* key, r_string& value) {
    if (isReading) {
        value = (*pJson)[key].get<std::string>();
    } else {
        (*pJson)[key] = value.data();
    }
}
