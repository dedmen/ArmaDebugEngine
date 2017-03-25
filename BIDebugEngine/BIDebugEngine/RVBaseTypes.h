#pragma once
#include <algorithm>
#include <string>
#include "GlobalHeader.h"
class JsonArchive;
template<class Type>
class rv_allocator : std::allocator<Type> {
public:
    static void deallocate(Type* _Ptr, size_t = 0);
    //This only allocates the memory! This will not be initialized to 0 and the allocated object will not have it's constructor called! 
    //use the create* Methods instead
    static Type* allocate(size_t _count);

    //Allocates and Initializes one Object
    template<class... _Types>
    static Type* createSingle(_Types&&... _Args);

    //Allocates and initializes array of Elements using the default constructor.
    static Type* createArray(size_t _count);
};

class RefCountBaseT {
private:
    mutable uint32_t _count;
public:
    RefCountBaseT() { _count = 0; }
    RefCountBaseT(const RefCountBaseT &src) { _count = 0; }
    void operator =(const RefCountBaseT &src) const {}
    __forceinline int addRef() const {
        return ++_count;
    }
    __forceinline int decRef() const {
        return --_count;
    }
    __forceinline int refCount() const { return _count; }
};


class RefCount : public RefCountBaseT {
    typedef RefCountBaseT base;
public:
    virtual ~RefCount() {}
    __forceinline int release() const {
        int ret = base::decRef();
        if (ret == 0) destroy();
        return ret;
    }
    virtual void destroy() const { delete const_cast<RefCount *>(this); }
    virtual double memUsed() const { return 0; }
};

//From my Fork of Intercept
template<class Type, class Allocator = rv_allocator<char>> //Has to be allocator of type char
class compact_array : public RefCountBaseT {
    static_assert(std::is_literal_type<Type>::value, "Type must be a literal type");
public:

    size_t size() const { return _size; }
    Type *data() { return &_data; }
    const Type *data() const { return &_data; }


    //We delete ourselves! After release no one should have a pointer to us anymore!
    int release() const;

    static compact_array* create(size_t number_of_elements_);

    void del() const;
    //#TODO copy functions

    size_t _size;
#ifdef _DEBUG
    union {
        Type _data;
        Type _dataDummy[128];
    };
#else
    Type _data;
#endif

private:
    explicit compact_array(size_t size_) {
        _size = size_;
    }
};

template<class Type>
class Ref {
protected:
    Type *_ref;
public:
    typedef Type baseType;
    __forceinline Ref() { _ref = NULL; }

    const Ref<Type> &operator = (Type *source) {
        Type *old = _ref;
        if (source) source->addRef();
        _ref = source;
        if (old) old->release();
        return *this;
    }
    Ref(const Ref &sRef) {
        Type *source = sRef._ref;
        if (source) source->addRef();
        _ref = source;
    }
    const Ref<Type> &operator = (const Ref &sRef) {
        Type *source = sRef._ref;
        Type *old = _ref;
        if (source) source->addRef();
        _ref = source;
        if (old) old->release();
        return *this;
    }
    __forceinline bool isNull() const { return _ref == nullptr; }
    __forceinline bool isNotNull() const { return !isNull(); }
    __forceinline ~Ref() { release(); }
    void release() { if (_ref) _ref->release(), _ref = nullptr; }
    __forceinline Type *get() { return _ref; }
    __forceinline const Type *get() const { return _ref; }

    __forceinline Type *operator -> () const { return _ref; }
    __forceinline operator Type *() const { return _ref; }

    template < typename = typename std::enable_if<has_Serialize<Type>::value>>
    void Serialize(JsonArchive& ar) {
        _ref->Serialize(ar);
    }

    template < typename = typename std::enable_if<has_Serialize<Type>::value>>
    void Serialize(JsonArchive& ar) const {
        _ref->Serialize(ar);
    }

};

template<class Type>
class IRef {
protected:
    Type *_ref;
public:
    typedef Type baseType;
    __forceinline IRef() { _ref = NULL; }

    const Ref<Type> &operator = (Type *source) {
        Type *old = _ref;
        if (source) source->IaddRef();
        _ref = source;
        if (old) old->Irelease();
        return *this;
    }
    IRef(const IRef &sRef) {
        Type *source = sRef._ref;
        if (source) source->IaddRef();
        _ref = source;
    }
    const IRef<Type> &operator = (const IRef &sRef) {
        Type *source = sRef._ref;
        Type *old = _ref;
        if (source) source->IaddRef();
        _ref = source;
        if (old) old->Irelease();
        return *this;
    }
    __forceinline bool isNull() const { return _ref == nullptr; }
    __forceinline bool isNotNull() const { return !isNull(); }
    __forceinline ~IRef() { release(); }
    void release() { if (_ref) _ref->Irelease(), _ref = nullptr; }
    __forceinline Type *get() const { return _ref; }

    __forceinline Type *operator -> () const { return _ref; }
    __forceinline operator Type *() const { return _ref; }

    template < typename = typename std::enable_if<has_Serialize<Type>::value>>
    void Serialize(JsonArchive& ar) {
        _ref->Serialize(ar);
    }

    template < typename = typename std::enable_if<has_Serialize<Type>::value>>
    void Serialize(JsonArchive& ar) const {
        _ref->Serialize(ar);
    }
};



class RStringRef {
    RStringRef(Ref<compact_array<char>>& str) {

    }
private:
    Ref<compact_array<char>> _ref;
    uint32_t offset;
    uint32_t length;
    //#TODO complete this. And return ref for RString::substr Problem is reading the string needs 0 terminator so calling data on it is forbidden
};


class RString {
public:
    RString() {}
    RString(const char* str, size_t len) {
        if (str) _ref = create(str, len);
        else _ref = create(len);
    }
    RString(const char* str) {
        if (str) _ref = create(str);
    }
    RString(const std::string str) {
        if (str.length()) _ref = create(str.c_str(), str.length());
    }
    RString(RString&& _move) {
        _ref = _move._ref;
        _move._ref = nullptr;
    }
    RString(const RString& _copy) {
        _ref = _copy._ref;
    }


    RString& operator = (RString&& _move) {
        if (this == &_move)
            return *this;
        _ref = _move._ref;
        _move._ref = nullptr;
        return *this;
    }
    RString& operator = (const RString& _copy) {
        _ref = _copy._ref;
        return *this;
    }
    RString& operator = (const std::string& _copy) {
        if (_copy.length()) _ref = create(_copy.c_str(), _copy.length());
        return *this;
    }
    RString& operator = (const char* _copy) {
        if (_copy) _ref = create(_copy, strlen(_copy));
        return *this;
    }
    bool operator < (const RString& _right) const {
        return std::strcmp(*this, _right) > 0;
    }

    const char* data() const {
        if (_ref) return _ref->data();
        static char empty[]{ 0 };
        return empty;
    }
    char* data_mutable() {
        if (!_ref) return nullptr;
        make_mutable();
        return _ref->data();
    }

    operator const char *() const { return data(); }
    operator std::string() const { return std::string(data()); }
    //This calls strlen so O(N) 
    size_t length() const {
        if (!_ref) return 0;
        return strlen(_ref->data());
    }

    //== is case insensitive just like scripting
    bool operator == (const char *other) {
        return _strcmpi(*this, other) == 0;
    }

    //!= is case insensitive just like scripting
    bool operator != (const char *other) {
        return _strcmpi(*this, other) != 0;
    }

    bool compare_case_sensitive(const char *other) {
        return _stricmp(*this, other) == 0;
    }
    bool startsWith(const char* other) {
        return _strnicmp(*this, other, std::min(length(), strlen(other))) == 0;
    }
    RString substr(size_t offset, size_t length) const {
        if (_ref)
            return RString(data() + offset, length);
        return RString();
    }

    size_t find(char ch, size_t start = 0) const;
    size_t find(const char* sub, int nStart = 0) const;

    bool isNull() const {
        return _ref.isNull();
    }

    void lower() {
        if (_ref)
            _strlwr_s(_ref->data(), _ref->_size);
    };
    const char* cbegin() {
        if (_ref) return _ref->data();
        return data();
    }
    const char* cend() {
        if (_ref) return _ref->data() + length();
        return data();
    }


private:
    Ref<compact_array<char>> _ref;

    static compact_array<char> *create(const char *str, size_t len);
    static compact_array<char> *create(size_t len);
    static compact_array<char> *create(const char *str);

    void make_mutable() {
        if (_ref && _ref->refCount() > 1) {//If there is only one reference we can safely modify the string
            _ref = create(_ref->data());
        }
    }
};

class SourceDoc {
public:
    uintptr_t vtable;
    RString _fileName;//String
    RString _content;//String
};

class SourceDocPos {
    uintptr_t vtable;
public:
    RString _sourceFile;
    int _sourceLine;

    RString _content;
    int _pos;
};