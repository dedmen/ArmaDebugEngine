#pragma once

// SFINAE test
template <typename T>
class has_Serialize2 {
    typedef char one;
    typedef long two;

    template <typename C> static one test(char[sizeof(&C::Serialize)]);
    template <typename C> static two test(...);

public:
    enum { value = sizeof(test<T>(0)) == sizeof(char) };
};


template<typename Class>
class has_Serialize {
    typedef char(&yes)[2];

    template<typename> struct Exists; // <--- changed

    template<typename V>
    static yes CheckMember(Exists<decltype(&V::Serialize)>*); // <--- changed (c++11)
    template<typename>
    static char CheckMember(...);

public:
    static const bool value = (sizeof(CheckMember<Class>(0)) == sizeof(yes));
};



class test1 {
public:
    void Serialize() {};
};

class test2 {
public:
    void Serialize2() {};
};