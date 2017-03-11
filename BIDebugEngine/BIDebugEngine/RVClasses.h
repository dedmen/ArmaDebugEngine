#pragma once

#include <cstdint>
#include <memory>
#include <algorithm>
#include <string>
#include "GlobalHeader.h"
class JsonArchive;
//From my Intercept fork
extern uintptr_t engineAlloc;
template<class Type>
class rv_allocator : std::allocator<Type> {
    class MemTableFunctions {
    public:
        virtual void *New(size_t size) = 0;
        virtual void *New(size_t size, const char *file, int line) = 0;
        virtual void Delete(void *mem) = 0;
        virtual void Delete(void *mem, const char *file, int line) = 0;
        virtual void *Realloc(void *mem, size_t size) = 0;
        virtual void *Realloc(void *mem, size_t size, const char *file, int line) = 0;
        virtual void *Resize(void *mem, size_t size) = 0; //This actually doesn't do anything.

        virtual void *NewPage(size_t size, size_t align) = 0;
        virtual void DeletePage(void *page, size_t size) = 0;

        virtual void *HeapAlloc(void *mem, size_t size) = 0;
        virtual void *HeapAlloc(void *mem, size_t size, const char *file, int line) = 0;//HeapAlloc

        virtual void *HeapDelete(void *mem, size_t size) = 0;
        virtual void *HeapDelete(void *mem, size_t size, const char *file, int line) = 0;//HeapFree

        virtual int something(void* mem, size_t unknown) = 0; //Returns HeapSize(mem) - (unknown<=4 ? 4 : unknown) -(-0 & 3) -3

        virtual size_t GetPageRecommendedSize() = 0;

        virtual void *HeapBase() = 0;
        virtual size_t HeapUsed() = 0;

        virtual size_t HeapUsedByNew() = 0;
        virtual size_t HeapCommited() = 0;
        virtual int FreeBlocks() = 0;
        virtual int MemoryAllocatedBlocks() = 0;
        virtual void Report() = 0;//Does nothing on release build. Maybe on Profiling build
        virtual bool CheckIntegrity() = 0;//Does nothing on release build. Maybe on Profiling build returns true.
        virtual bool IsOutOfMemory() = 0;//If true we are so full we are already moving memory to disk.

        virtual void CleanUp() = 0;//Does nothing? I guess.
                                   //Synchronization for Multithreaded access
        virtual void Lock() = 0;
        virtual void Unlock() = 0;
        char* arr[6]{ "tbb4malloc_bi","tbb3malloc_bi","jemalloc_bi","tcmalloc_bi","nedmalloc_bi","custommalloc_bi" };
    };
public:
    static void deallocate(Type* _Ptr, size_t) {
        //#TODO assert when _ptr is not 32/64bit aligned
        // deallocate object at _Ptr
        uintptr_t allocatorBase = engineAlloc;
        MemTableFunctions* alloc = (MemTableFunctions*) allocatorBase;
        alloc->Delete(_Ptr);
    }
    //This only allocates the memory! This will not be initialized to 0 and the allocated object will not have it's constructor called! 
    //use the create* Methods instead
    static Type* allocate(size_t _count) {	// allocate array of _Count elements

        uintptr_t allocatorBase = engineAlloc;
        //uintptr_t allocatorBase = GET_ENGINE_ALLOCATOR;    
        MemTableFunctions* alloc = (MemTableFunctions*) allocatorBase;
        Type* newData = reinterpret_cast<Type*>(alloc->New(sizeof(Type)*_count));
        return newData;
    }

    //Allocates and Initializes one Object
    template<class... _Types>
    static Type* createSingle(_Types&&... _Args) {
        auto ptr = allocate(1);
        ::new (ptr) Type(std::forward<_Types>(_Args)...);
        //Have to do a little more unfancy stuff for people that want to overload the new operator
        return ptr;
    }

    //Allocates and initializes array of Elements using the default constructor.
    static Type* createArray(size_t _count) {
        auto ptr = allocate(_count);

        for (size_t i = 0; i < _count; ++i) {
            ::new (ptr + i) Type();
        }

        return ptr;
    }


    //#TODO implement game_data_pool and string pool here
};

class RefCountBaseT {
private:
    mutable uint32_t _count;
public:
    RefCountBaseT() { _count = 0; }
    RefCountBaseT(const RefCountBaseT &src) { _count = 0; }
    void operator =(const RefCountBaseT &src) {}
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

//From my Fork of Intercept //#TODO convert back to use Arma alloc
template<class Type, class Allocator = rv_allocator<char>> //Has to be allocator of type char
class compact_array : public RefCountBaseT {
    static_assert(std::is_literal_type<Type>::value, "Type must be a literal type");
public:

    int size() const { return _size; }
    Type *data() { return &_data; }
    const Type *data() const { return &_data; }


    //We delete ourselves! After release no one should have a pointer to us anymore!
    int release() const {
        int ret = decRef();
        if (!ret) del();
        return ret;
    }

    static compact_array* create(int number_of_elements_) {
        size_t size = sizeof(compact_array) + sizeof(Type)*(number_of_elements_ - 1);//-1 because we already have one element in compact_array
        compact_array* buffer = reinterpret_cast<compact_array*>(Allocator::allocate(size));
        new (buffer) compact_array(number_of_elements_);
        return buffer;
    }

    void del() const {
        this->~compact_array();
        const void* _thisptr = this;
        void* _thisptr2 = const_cast<void*>(_thisptr);
        Allocator::deallocate(reinterpret_cast<char*>(_thisptr2), _size - 1 + sizeof(compact_array));
    }
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
    __forceinline Type *get() const { return _ref; }

    __forceinline Type *operator -> () const { return _ref; }
    __forceinline operator Type *() const { return _ref; }

    template < typename = typename std::enable_if<has_Serialize<Type>::value>::type>
    void Serialize(JsonArchive& ar) {
        _ref->Serialize(ar);
    }

    template < typename = typename std::enable_if<has_Serialize<Type>::value>::type>
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

    template < typename = typename std::enable_if<has_Serialize<Type>::value>::type>
    void Serialize(JsonArchive& ar) {
        _ref->Serialize(ar);
    }

    template < typename = typename std::enable_if<has_Serialize<Type>::value>::type>
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
    //#TODO complete this. And return ref for RString::substr
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
    size_t find(char ch, size_t start = 0) const {
        if (length() == 0) return -1;
        const char *pos = strchr(_ref->data() + start, ch);
        if (pos == nullptr) return -1;
        return pos - _ref->data();
    }

    size_t find(const char *sub, int nStart = 0) const {
        if (_ref == nullptr || length() == 0) return -1;
        const char *pos = strstr(_ref->data() + nStart, sub);
        if (pos == nullptr) return -1;
        return pos - _ref->data();
    }
    bool isNull() const {
        return _ref.isNull();
    }




public://#TODO make private after all rv_strings were replaced
    Ref<compact_array<char>> _ref;

    static compact_array<char> *create(const char *str, int len) {
        if (len == 0 || *str == 0) return nullptr;
        compact_array<char> *string = compact_array<char>::create(len + 1);
        strncpy_s(string->data(), string->size(), str, len);
        string->data()[len] = 0;
        return string;
    }

    static compact_array<char> *create(int len) {
        if (len == 0) return nullptr;
        compact_array<char> *string = compact_array<char>::create(len + 1);
        string->data()[0] = 0;
        return string;
    }

    static compact_array<char> *create(const char *str) {
        if (*str == 0) return nullptr;
        return create(str, strlen(str));
    }
    void make_mutable() {
        if (_ref && _ref->refCount() > 1) {//If there is only one reference we can safely modify the string
            _ref = create(_ref->data());
        }
    }
};

//class RString {  //Quick and dirty Rstring impl
//public:
//    uintptr_t refcount;
//    uintptr_t stringlen;
//    char text[128];
//};

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

class RV_GameInstruction : public RefCount {
public:
    virtual ~RV_GameInstruction() {}
    virtual bool Execute(uintptr_t gameStatePtr, uintptr_t VMContextPtr) { return false; };
    virtual int GetStackChange(uintptr_t _ptr) { return 0; };
    virtual bool IsNewExpression() { return false; }
    virtual RString GetDebugName() { return RString(); };

    void Serialize(JsonArchive& ar);

    SourceDocPos _scriptPos;
};

template<class Type>
class Array {
protected:
    Type *_data;
    int _n;
public:
    Array() {};
    Type &get(int i) {
        return _data[i];
    }
    const Type &get(int i) const {
        return _data[i];
    }
    Type &operator [] (int i) { return get(i); }
    const Type &operator [] (int i) const { return get(i); }
    Type *data() { return _data; }
    int count() const { return _n; }

    Type &front() { return get(0); }
    Type &back() { return get(_n - 1); }

    const Type &front() const { return get(0); }
    const Type &back() const { return get(_n - 1); }

    bool isEmpty() const { return _n == 0; }

    template <class Func>
    void forEach(const Func &f) const {
        for (int i = 0; i < count(); i++) {
            f(get(i));
        }
    }

    template <class Func>
    void forEachBackwards(const Func &f) const { //This returns if Func returns true
        for (int i = count() - 1; i >= 0; i--) {
            if (f(get(i))) return;
        }
    }

    template <class Func>
    void forEach(const Func &f) {
        for (int i = 0; i < count(); i++) {
            f(get(i));
        }
    }
};

template<class Type>
class AutoArray : public Array<Type> {
protected:
    int _maxItems;
public:
    AutoArray() : _maxItems(0), Array() {};
};

static inline unsigned int CalculateStringHashValue(const char *key, int hashValue = 0) {
    while (*key) {
        hashValue = hashValue * 33 + static_cast<unsigned char>(*key++);
    }
    return hashValue;
}
static inline unsigned int CalculateStringHashValueCI(const char *key, int hashValue = 0) {
    while (*key) {
        hashValue = hashValue * 33 + static_cast<unsigned char>(tolower(*key++));
    }
    return hashValue;
}


template <class Type>
struct MapStringToClassTrait {
    static unsigned int hashKey(const char * key) {
        return CalculateStringHashValue(key);
    }
    static int compareKeys(const char * k1, const char * k2) {
        return strcmp(k1, k2);
    }
};

template <class Type>
struct MapStringToClassTraitCaseInsensitive : public MapStringToClassTrait<Type> {
    static unsigned int hashKey(const char * key) {
        return CalculateStringHashValueCI(key);
    }

    static int compareKeys(const char * k1, const char * k2) {
        return strcmpi(k1, k2);
    }
};

template <class Type, class Container, class Traits = MapStringToClassTrait<Type>>
class MapStringToClass {
protected:
    Container *_table;
    int _tableCount{ 0 };
    int _count{ 0 };
    static Type _nullEntry;
public:
    MapStringToClass() {}
    ~MapStringToClass() {}

    template <class Func>
    void forEach(Func func) const;

    template <class Func>
    void forEachBackwards(Func func) const;

    const Type &get(const char* key) const;

    static bool isNull(const Type &value) { return &value == &_nullEntry; }

    bool hasKey(const char* key) const;

public:
    int count() const { return _count; }
protected:
    int hashKey(const char* key) const;
};

template<class Type, class Container, class Traits>
Type MapStringToClass<Type, Container, Traits>::_nullEntry;


template<class Type, class Container, class Traits>
int MapStringToClass<Type, Container, Traits>::hashKey(const char* key) const {
    return Traits::hashKey(key) % _tableCount;
}

template<class Type, class Container, class Traits>
template <class Func>
void MapStringToClass<Type, Container, Traits>::forEach(Func func) const {
    if (!_table || !_count) return;
    for (int i = 0; i < _tableCount; i++) {
        const Container &container = _table[i];
        for (int j = 0; j < container.count(); j++) {
            func(container[j]);
        }
    }
}

template<class Type, class Container, class Traits>
template <class Func>
void MapStringToClass<Type, Container, Traits>::forEachBackwards(Func func) const {
    if (!_table || !_count) return;
    for (int i = _tableCount - 1; i >= 0; i--) {
        const Container &container = _table[i];
        for (int j = container.count() - 1; j >= 0; j--) {
            func(container[j]);
        }
    }
}

template<class Type, class Container, class Traits>
const Type &MapStringToClass<Type, Container, Traits>::get(const char* key) const {
    if (!_table || !_count) return _nullEntry;
    int hashedKey = hashKey(key);
    for (int i = 0; i < _table[hashedKey].count(); i++) {
        const Type &item = _table[hashedKey][i];
        if (Traits::compareKeys(item.getMapKey(), key) == 0)
            return item;
    }
    return _nullEntry;
}

template <class Type, class Container, class Traits>
bool MapStringToClass<Type, Container, Traits>::hasKey(const char* key) const {
    return !isNull(get(key));
}

