#include "Serialize.h"



Serialize::Serialize() {

}


Serialize::~Serialize() {}

std::string JsonArchive::to_string() {
    return pJson->dump(4);
}

void JsonArchive::Serialize(const char* key, const RString& value) {
    if (isReading) __debugbreak(); //can't read this
    (*pJson)[key] = value.data();
}

void JsonArchive::Serialize(const char* key, RString& value) {
    if (isReading) {
        value = (*pJson)[key].get<std::string>();
    } else {
        (*pJson)[key] = value.data();
    }
}
