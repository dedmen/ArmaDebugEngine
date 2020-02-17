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
        Serialize(const char* key, const auto_array<Type>& value) {
        auto &_array = (*pJson)[key];
        _array.array({});
        if (isReading) {
            if (!_array.is_array()) __debugbreak();
            for (auto& it : _array) {
                __debugbreak(); //#TODO AutoArray pushback
            }
        } else {
            value.for_each([&_array](const Type& value) {
                JsonArchive element;
                value.Serialize(element);
                _array.push_back(*element.pJson);
            });
        }
    }

    template <class Type>
    typename std::enable_if<!has_Serialize<Type>::value>::type
        Serialize(const char* key, const auto_array<Type>& value) {
        auto &_array = (*pJson)[key];
        _array.array({});
        if (isReading) {
            if (!_array.is_array()) __debugbreak();
            for (auto& it : _array) {
                __debugbreak(); //#TODO AutoArray pushback
            }
        }
        else {
            value.for_each([&_array](const Type& value) {
                JsonArchive element;
                ::Serialize(value, element);
                _array.push_back(*element.pJson);
            });
        }
    }



    template <class Type>
    typename std::enable_if<has_Serialize<Type>::value || has_Serialize<typename Type::baseType>::value>::type
        Serialize(const char* key, auto_array<Type>& value) {
        auto &_array = (*pJson)[key];
        if (isReading) {
            if (!_array.is_array()) __debugbreak();
            for (auto& it : _array) {
                __debugbreak(); //#TODO AutoArray pushback
            }
        } else {
            value.for_each([&_array](Type& value) {
                JsonArchive element;
                value.Serialize(element);
                _array.push_back(*element.pJson);
            });
        }
    }



    template <class Type>
    typename std::enable_if<!has_Serialize<Type>::value && !has_Serialize<typename Type::baseType>::value>::type
        Serialize(const char* key, auto_array<Type>& value) {
        auto &_array = (*pJson)[key];
        if (isReading) {
            if (!_array.is_array()) __debugbreak();
            for (auto& it : _array) {
                __debugbreak(); //#TODO AutoArray pushback
            }
        } else {
            for (auto&& it : value) {
                JsonArchive element;
                ::Serialize(*it, element);
                _array.push_back(*element.pJson);
            }
        }
    }

    template <class Type>
    typename std::enable_if<!has_Serialize<Type>::value && !has_Serialize<typename Type::baseType>::value>::type
        Serialize(const char* key, compact_array<Type>& value) {
        auto &_array = (*pJson)[key];
        if (isReading) {
            if (!_array.is_array()) __debugbreak();
            for (auto& it : _array) {
                __debugbreak(); //#TODO AutoArray pushback
            }
        } else {
            for (auto&& it : value) {
                JsonArchive element;
                ::Serialize(*it, element);
                _array.push_back(*element.pJson);
            }
        }
    }



    template <class Type>
    typename std::enable_if<!has_Serialize<Type>::value>::type
        Serialize(const char* key, std::vector<Type>& value) {
        auto &_array = (*pJson)[key];
        if (isReading) {
            if (_array.is_array()) {
                for (auto& it : _array) {
                    if constexpr (std::is_convertible_v<Type, r_string>)
                        value.emplace_back(r_string(it.get<std::string>()));
                    else
                        value.push_back(it);
                }
            }
        } else {
            for (Type& it : value) {
                if constexpr (std::is_convertible_v<Type, r_string>)
                    _array.push_back(it.data());
                else
                    _array.push_back(it);
            }
                
        }
    }

    template <class Type>
    typename std::enable_if<has_Serialize<Type>::value>::type
        Serialize(const char* key, std::vector<Type>& value) {
        auto &_array = (*pJson)[key];
        if (isReading) {
            if (_array.is_array()) {
                for (auto& it : _array) {
                    auto iterator = value.emplace(value.end(), Type());
                    JsonArchive tmpAr(it);
                    iterator->Serialize(tmpAr);
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
    typename std::enable_if<has_Serialize<Type>::value && !std::is_convertible<Type, rv_arraytype>::value>::type
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


    //Generic serialization. Calls Type::Serialize
    template <class Type>
    typename std::enable_if<has_Serialize<Type>::value && !std::is_convertible<Type, rv_arraytype>::value>::type
        Serialize(const char* key, std::shared_ptr<Type>& value) {
        if (isReading) {
            __debugbreak(); //not implemented
        }
        else {
            JsonArchive element;
            if (value)
                value->Serialize(element);
            (*pJson)[key] = *element.pJson;
        }
    }

    template <class Type>
    typename std::enable_if<!has_Serialize<Type>::value && !std::is_convertible<Type, rv_arraytype>::value>::type
        Serialize(const char* key, Type& value) {
        if (isReading) {
            value = (*pJson)[key].get<Type>();
        } else {
            (*pJson)[key] = value;
        }
    }

    template <class Type>
    void writeOnly(const char* key, Type& value) {
        if (isReading) {
            __debugbreak();
        } else {
            (*pJson)[key] = value;
        }
    }


    template <class Type>
    typename std::enable_if<has_Serialize<Type>::value && !std::is_convertible<Type, rv_arraytype>::value>::type
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
    typename std::enable_if<!has_Serialize<Type>::value && !std::is_convertible<Type, rv_arraytype>::value>::type
        Serialize(const char* key, const Type& value) {
        if (isReading) {
            __debugbreak(); //not possible
        } else {
            (*pJson)[key] = value;
        }
    }

    void Serialize(const char* key, r_string& value);
    void Serialize(const char* key, const r_string& value);

    //************************************
    //serializeFunction - Function that is called for every element in the Array
    //************************************
    template <class Type, class Func>
    void Serialize(const char* key, auto_array<Type>& value, Func& serializeFunction) {
        auto &_array = (*pJson)[key];
        if (isReading) {
            if (!_array.is_array()) __debugbreak();
            for (auto& it : _array) {
                __debugbreak(); //#TODO AutoArray pushback
            }
        } else {
            value.forEach([&_array, &serializeFunction](auto&& value) {
                JsonArchive element;
                serializeFunction(element, value);
                _array.push_back(*element.pJson);
            });
        }
    }

    //************************************
    //serializeFunction - Function that is called for every element in the Array
    //************************************
    template <class Type, class Func>
    void Serialize(const char* key, const rv_array<Type>& value, Func&& serializeFunction) {
        if (isReading) {
            __debugbreak(); //not possible
        } else {
            auto &_array = (*pJson)[key];
            value.for_each([&_array, &serializeFunction](auto && value) {
                JsonArchive element;
                serializeFunction(element, value);
                _array.push_back(*element.pJson);
            });
        }
    }

    template <class Type>
    void Serialize(const char* key, const std::initializer_list<Type>& value) {
        auto &_array = (*pJson)[key];
        for (auto& it : value) {
            _array.push_back(it);
        }
    }

private:
    json* pJson;
    bool isReading;
};

class Serialize {
public:
    Serialize();
    ~Serialize();
};


static void from_json(const json& j, game_instruction& in) { }
