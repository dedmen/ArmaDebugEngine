#pragma once
#include <string>
#include "RVClasses.h"
#include "json.hpp"
#include "VMContext.h"
using nlohmann::json;
#include "GlobalHeader.h"

class JsonArchive {
public:
    JsonArchive() : pJson(new json()), isReading(false) {}
    JsonArchive(json& js) : pJson(&js), isReading(true) {}
    ~JsonArchive() { if (!isReading) delete pJson; }
    bool reading() const { return isReading; }
    json* getRaw() const { return pJson; }
    std::string to_string();

    //typename std::enable_if<has_Serialize<Type>::value || has_Serialize<typename Type::baseType>::value>::type

    void Serialize(const char* key, JsonArchive& ar) {
        if (isReading) __debugbreak(); //not implemented
        (*pJson)[key] = *ar.pJson;
    }

    template <class Type>
    typename std::enable_if<has_Serialize<Type>::value>::type
        Serialize(const char* key, const AutoArray<Type>& value) {
        auto &_array = (*pJson)[key];
        _array.array({});
        if (isReading) {
            if (!_array.is_array()) __debugbreak();
            for (auto& it : _array) {
                __debugbreak(); //#TODO AutoArray pushback
            }
        } else {
            value.forEach([&_array](const Type& value) {
                JsonArchive element;
                value.Serialize(element);
                _array.push_back(*element.pJson);
            });
        }
    }

    template <class Type>
    typename std::enable_if<has_Serialize<Type>::value || has_Serialize<typename Type::baseType>::value>::type
        Serialize(const char* key, AutoArray<Type>& value) {
        auto &_array = (*pJson)[key];
        if (isReading) {
            if (!_array.is_array()) __debugbreak();
            for (auto& it : _array) {
                __debugbreak(); //#TODO AutoArray pushback
            }
        } else {
            value.forEach([&_array](Type& value) {
                JsonArchive element;
                value.Serialize(element);
                _array.push_back(*element.pJson);
            });
        }
    }



    template <class Type>
    typename std::enable_if<!has_Serialize<Type>::value && !has_Serialize<typename Type::baseType>::value>::type
        Serialize(const char* key, AutoArray<Type>& value) {
        auto &_array = (*pJson)[key];
        if (isReading) {
            if (!_array.is_array()) __debugbreak();
            for (auto& it : _array) {
                __debugbreak(); //#TODO AutoArray pushback
            }
        } else {
            value.forEach([&_array](Type& value) {
                _array.push_back(value);
            });
        }
    }


    template <class Type>
    typename std::enable_if<!has_Serialize<Type>::value>::type
        Serialize(const char* key, std::vector<Type>& value) {
        auto &_array = (*pJson)[key];
        if (isReading) {
            if (_array.is_array()) {
                for (auto& it : _array) {
                    value.push_back(it);
                }
            }
        } else {
            for (Type& it : value)
                _array.push_back(it);
        }
    }

    template <class Type>
    typename std::enable_if<has_Serialize<Type>::value>::type
        Serialize(const char* key, std::vector<Type>& value) {
        auto &_array = (*pJson)[key];
        if (isReading) {
            if (_array.is_array()) {
                for (auto& it : _array) {
                    auto iterator = value.emplace(value.end(),Type());
                    iterator->Serialize(JsonArchive(it));
                }
            }
        } else {
            for (Type& it : value) {
                JsonArchive element;
                it.Serialize(element);
                _array.push_back(*element.pJson);
            }
        }
    }





    //Generic serialization. Calls Type::Serialize
    template <class Type>
    typename std::enable_if<has_Serialize<Type>::value>::type
        Serialize(const char* key, Type& value) {
        if (isReading) {
            __debugbreak(); //not implemented
        } else {
            JsonArchive element;
            value.Serialize(element);
            (*pJson)[key] = *element.pJson;
        }
    }

    //Generic serialization. Calls Type::Serialize
    template <class Type>
    typename std::enable_if<has_Serialize<Type>::value>::type
        Serialize(const char* key, std::unique_ptr<Type>& value) {
        if (isReading) {
            __debugbreak(); //not implemented
        } else {
            JsonArchive element;
            if (value)
                value->Serialize(element);
            (*pJson)[key] = *element.pJson;
        }
    }



    template <class Type>
    typename std::enable_if<!has_Serialize<Type>::value>::type
        Serialize(const char* key, Type& value) {
        if (isReading) {
            value = (*pJson)[key].get<Type>();
        } else {
            (*pJson)[key] = value;
        }
    }


    template <class Type>
    typename std::enable_if<has_Serialize<Type>::value>::type
        Serialize(const char* key, const Type& value) {
        if (isReading) {
            __debugbreak(); //not possible
        } else {
            JsonArchive element;
            value.Serialize(element);
            (*pJson)[key] = *element.pJson;
        }
    }

    template <class Type>
    typename std::enable_if<!has_Serialize<Type>::value>::type
        Serialize(const char* key, const Type& value) {
        if (isReading) {
            __debugbreak(); //not possible
        } else {
            (*pJson)[key] = value;
        }
    }

    void Serialize(const char* key, RString& value);
    void Serialize(const char* key, const RString& value);

    //************************************
    //serializeFunction - Function that is called for every element in the Array
    //************************************
    template <class Type, class Func>
    void Serialize(const char* key, AutoArray<Type>& value, Func& serializeFunction);

    //************************************
    //serializeFunction - Function that is called for every element in the Array
    //************************************
    template <class Type, class Func>           //#TODO make these templates better :/
    void Serialize(const char* key, const Array<Type>& value, Func& serializeFunction);

    template <class Type>           //#TODO make these templates better :/
    void Serialize(const char* key, const std::initializer_list<Type>& value);

private:
    json* pJson;
    bool isReading;
};



template <class Type, class Func>
void JsonArchive::Serialize(const char* key, AutoArray<Type>& value, Func& serializeFunction) {
    auto &_array = (*pJson)[key];
    if (isReading) {
        if (!_array.is_array()) __debugbreak();
        for (auto& it : _array) {
            __debugbreak(); //#TODO AutoArray pushback
        }
    } else {
        value.forEach([&_array, serializeFunction](Type& value) {
            JsonArchive element;
            serializeFunction(element, value);
            _array.push_back(*element.pJson);
        });
    }
}

template <class Type, class Func>
void JsonArchive::Serialize(const char* key, const Array<Type>& value, Func& serializeFunction) {
    if (isReading) {
        __debugbreak(); //not possible
    } else {
        auto &_array = (*pJson)[key];
        value.forEach([&](const Type& value) {
            JsonArchive element;
            serializeFunction(element, value);
            _array.push_back(*element.pJson);
        });
    }
}

template <class Type>
void JsonArchive::Serialize(const char* key, const std::initializer_list<Type>& value) {
    auto &_array = (*pJson)[key];
    for (auto& it : value) {
        _array.push_back(it);
    }
}

class Serialize {
public:
    Serialize();
    ~Serialize();













};

