#pragma once
#include <algorithm>
#include <string>
#include "GlobalHeader.h"
#include <containers.hpp>

class JsonArchive;
using intercept::types::r_string;
using intercept::types::ref;

template <typename Type>
typename std::enable_if<has_Serialize<Type>::value>::type
    Serialize(const intercept::types::i_ref<Type>& _ref, JsonArchive& ar) {
    Serialize(*_ref,ar);
}


template <class Type, class Container, class Traits = intercept::types::map_string_to_class_trait>
using MapStringToClass = intercept::types::map_string_to_class<Type, Container, Traits>;