#include "RVBaseTypes.h"

extern uintptr_t engineAlloc;
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
    const char* arr[6]{ "tbb4malloc_bi","tbb3malloc_bi","jemalloc_bi","tcmalloc_bi","nedmalloc_bi","custommalloc_bi" };
};

template<class Type>
void rv_allocator<Type>::deallocate(Type* _Ptr, size_t /*= 0*/) {
    //TODO assert when _ptr is not 32/64bit aligned
    // deallocate object at _Ptr
    uintptr_t allocatorBase = engineAlloc;
    MemTableFunctions* alloc = reinterpret_cast<MemTableFunctions*>(allocatorBase);
    alloc->Delete(_Ptr);
}

template<class Type>
Type* rv_allocator<Type>::allocate(size_t _count) {	// allocate array of _Count elements
    uintptr_t allocatorBase = engineAlloc;
    //uintptr_t allocatorBase = GET_ENGINE_ALLOCATOR;    
    MemTableFunctions* alloc = (MemTableFunctions*) allocatorBase;
    Type* newData = reinterpret_cast<Type*>(alloc->New(sizeof(Type)*_count));
    return newData;
}

template <class Type>
template <class ... _Types>
Type* rv_allocator<Type>::createSingle(_Types&&... _Args) {
    auto ptr = allocate(1);
    ::new (ptr) Type(std::forward<_Types>(_Args)...);
    //Have to do a little more unfancy stuff for people that want to overload the new operator
    return ptr;
}

template<class Type>
Type* rv_allocator<Type>::createArray(size_t _count) {
    auto ptr = allocate(_count);

    for (size_t i = 0; i < _count; ++i) {
        ::new (ptr + i) Type();
    }

    return ptr;
}

template <class Type, class Allocator>
compact_array<Type, Allocator>* compact_array<Type, Allocator>::create(size_t number_of_elements_) {
    size_t size = sizeof(compact_array) + sizeof(Type) * (number_of_elements_ - 1);//-1 because we already have one element in compact_array
    compact_array* buffer = reinterpret_cast<compact_array*>(Allocator::allocate(size));
    new(buffer) compact_array(number_of_elements_);
    return buffer;
}

template<class Type, class Allocator /*= rv_allocator<char>*/>
void compact_array<Type, Allocator>::del() const {
    this->~compact_array();
    const void* _thisptr = this;
    void* _thisptr2 = const_cast<void*>(_thisptr);
    Allocator::deallocate(reinterpret_cast<char*>(_thisptr2), _size - 1 + sizeof(compact_array));
}

template<class Type, class Allocator /*= rv_allocator<char>*/>
int compact_array<Type, Allocator>::release() const {
    int ret = decRef();
    if (!ret) del();
    return ret;
}

template class compact_array<char>;

size_t RString::find(char ch, size_t start) const {
    if (length() == 0) return -1;
    const char* pos = strchr(_ref->data() + start, ch);
    if (pos == nullptr) return -1;
    return pos - _ref->data();
}

size_t RString::find(const char* sub, int nStart) const {
    if (_ref == nullptr || length() == 0) return -1;
    const char* pos = strstr(_ref->data() + nStart, sub);
    if (pos == nullptr) return -1;
    return pos - _ref->data();
}

compact_array<char> * RString::create(const char *str, size_t len) {
    if (len == 0 || *str == 0) return nullptr;
    compact_array<char> *string = compact_array<char>::create(len + 1);
    strncpy_s(string->data(), string->size(), str, len);
    string->data()[len] = 0;
    return string;
}

compact_array<char> * RString::create(size_t len) {
    if (len == 0) return nullptr;
    compact_array<char> *string = compact_array<char>::create(len + 1);
    string->data()[0] = 0;
    return string;
}

compact_array<char> * RString::create(const char *str) {
    if (*str == 0) return nullptr;
    return create(str, strlen(str));
}
