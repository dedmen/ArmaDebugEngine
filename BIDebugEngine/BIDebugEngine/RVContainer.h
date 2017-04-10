#pragma once
#include "RVBaseTypes.h"
#include <vector>

class RVArrayType {};

template<class Type>
class Array : RVArrayType {
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

    Type* begin() { return &get(0); }
    Type* end() { return &get(_n); }

    const Type* begin() const { return &get(0); }
    const Type* end() const { return &get(_n); }

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
	typename std::enable_if<std::is_same<decltype(&Func::operator()), bool>::value,void>::type
		forEachBackwards(const Func &f) const { //This returns if Func returns true
        for (int i = count() - 1; i >= 0; i--) {
            if (f(get(i))) return;
        }
    }

	template <class Func>
	typename std::enable_if<!std::is_same<decltype(&Func::operator()), bool>::value, void>::type
		forEachBackwards(const Func &f) const { //This returns if Func returns true
		for (int i = count() - 1; i >= 0; i--) {
			f(get(i));
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
    AutoArray() : _maxItems(0), Array<Type>() {};
};


static inline unsigned int hashStringCaseSensitive(const char *key, int hashValue = 0) {
    while (*key) {
        hashValue = hashValue * 33 + static_cast<unsigned char>(*key++);
    }
    return hashValue;
}
static inline unsigned int hashStringCaseInsensitive(const char *key, int hashValue = 0) {
    while (*key) {
        hashValue = hashValue * 33 + static_cast<unsigned char>(tolower(*key++));
    }
    return hashValue;
}

template<>
struct std::hash<RString> {
    size_t operator()(const RString& _Keyval) const {
        return hashStringCaseInsensitive(_Keyval);
    }
};

struct MapStringToClassTrait {
    static unsigned int hashKey(const char * key) {
        return hashStringCaseSensitive(key);
    }
    static int compareKeys(const char * k1, const char * k2) {
        return strcmp(k1, k2);
    }
};

struct MapStringToClassTraitCaseInsensitive : public MapStringToClassTrait {
    static unsigned int hashKey(const char * key) {
        return hashStringCaseInsensitive(key);
    }

    static int compareKeys(const char * k1, const char * k2) {
        return _strcmpi(k1, k2);
    }
};

template <class Type, class Container, class Traits = MapStringToClassTrait>
class MapStringToClass {
protected:
    Container* _table;
    int _tableCount{ 0 };
    int _count{ 0 };
    static Type _nullEntry;
public:
    MapStringToClass() {}

    template <class Func>
    void forEach(Func func) const {
        if (!_table || !_count) return;
        for (int i = 0; i < _tableCount; i++) {
            const Container &container = _table[i];
            for (int j = 0; j < container.count(); j++) {
                func(container[j]);
            }
        }
    }

    template <class Func>
    void forEachBackwards(Func func) const {
        if (!_table || !_count) return;
        for (int i = _tableCount - 1; i >= 0; i--) {
            const Container &container = _table[i];
            for (int j = container.count() - 1; j >= 0; j--) {
                func(container[j]);
            }
        }
    }

    const Type &get(const char* key) const {
        if (!_table || !_count) return _nullEntry;
        int hashedKey = hashKey(key);
        for (int i = 0; i < _table[hashedKey].count(); i++) {
            const Type &item = _table[hashedKey][i];
            if (Traits::compareKeys(item.getMapKey(), key) == 0)
                return item;
        }
        return _nullEntry;
    }

    static bool isNull(const Type &value) { return &value == &_nullEntry; }

    bool hasKey(const char* key) const {
        return !isNull(get(key));
    }

public:
    int count() const { return _count; }
protected:
    int hashKey(const char* key) const {
        return Traits::hashKey(key) % _tableCount;
    }
};

template<class Type, class Container, class Traits>
Type MapStringToClass<Type, Container, Traits>::_nullEntry;

template <class Type, class Container, class Traits = MapStringToClassTrait>
class MapStringToClassNonRV {
protected:
    std::vector<Container> _table;
    int _tableCount{ 0 };
    int _count{ 0 };
    static Type _nullEntry;
public:
    MapStringToClassNonRV() { init(); }
    ~MapStringToClassNonRV() {}

    void init() {
        clear();
    }

    void clear() {
        _table.clear();
        _table.resize(15);
        _count = 0;
        _tableCount = 15;
    }

    void rebuild(int tableSize) {
        _tableCount = tableSize;
        std::vector<Container> newTable;
        newTable.resize(tableSize);
        for (auto& container : _table) {
            for (Type& item : container) {
                auto hashedKey = hashKey(item.getMapKey(), tableSize);
                auto it = newTable[hashedKey].emplace(newTable[hashedKey].end(), Type());
                *it = std::move(item);
            }
        }
        std::swap(_table, newTable);
    }


    Type& insert(const Type &value) {
        //Check if key already exists
        auto key = value.getMapKey();
        Type &old = get(key);
        if (!isNull(old)) {
            return old;
        }

        //Are we full?
        if (_count + 1 > (16 * _tableCount)) {
            int tableSize = _tableCount + 1;
            do {
                tableSize *= 2;
            } while (_count + 1 > (16 * (tableSize - 1)));
            rebuild(tableSize - 1);
        }
        auto hashedkey = hashKey(key);
        auto &x = *(_table[hashedkey].insert(_table[hashedkey].end(), value));
        _count++;
        return x;
    }

    Type& insert(Type &&value) {
        //Check if key already exists
        auto key = value.getMapKey();
        Type &old = get(key);
        if (!isNull(old)) {
            return old;
        }

        //Are we full?
        if (_count + 1 > (16 * _tableCount)) {
            int tableSize = _tableCount + 1;
            do {
                tableSize *= 2;
            } while (_count + 1 > (16 * (tableSize - 1)));
            rebuild(tableSize - 1);
        }
        auto hashedkey = hashKey(key);
        auto &x = *(_table[hashedkey].emplace(_table[hashedkey].end(), Type()));
        x = std::move(value);
        _count++;
        return x;
    }

    bool remove(const char* key) {
        if (_count <= 0) return false;

        int hashedKey = hashKey(key);
        for (size_t i = 0; i < _table[hashedKey].size(); i++) {
            Type &item = _table[hashedKey][i];
            if (Traits::compareKeys(item.getMapKey(), key) == 0) {
                _table[hashedKey].erase(_table[hashedKey].begin() + i);
                _count--;
                return true;
            }
        }
        return false;
    }



    template <class Func>
    void forEach(Func func) const {
        if (_table.empty() || !_count) return;
        for (int i = 0; i < _tableCount; i++) {
            const Container &container = _table[i];
            for (int j = 0; j < container.size(); j++) {
                func(container[j]);
            }
        }
    }

    template <class Func>
    void forEachBackwards(Func func) const {
        if (_table.empty() || !_count) return;
        for (int i = _tableCount - 1; i >= 0; i--) {
            const Container &container = _table[i];
            for (int j = container.size() - 1; j >= 0; j--) {
                func(container[j]);
            }
        }
    }

    const Type &get(const char* key) const {
        if (_table.empty() || !_count) return _nullEntry;
        int hashedKey = hashKey(key);
        for (int i = 0; i < _table[hashedKey].size(); i++) {
            const Type &item = _table[hashedKey][i];
            if (Traits::compareKeys(item.getMapKey(), key) == 0)
                return item;
        }
        return _nullEntry;
    }

    Type &get(const char* key) {
        if (_table.empty() || !_count) return _nullEntry;
        int hashedKey = hashKey(key);
        for (auto& item : _table[hashedKey]) {
            if (Traits::compareKeys(item.getMapKey(), key) == 0)
                return item;
        }
        return _nullEntry;
    }

    static bool isNull(const Type &value) { return &value == &_nullEntry; }

    bool hasKey(const char* key) const {
        return !isNull(get(key));
    }
    bool empty() const { return _count == 0; }

public:
    int count() const { return _count; }
protected:
    int hashKey(const char* key, int modul = -1) const {
        return Traits::hashKey(key) % (modul == -1 ? _tableCount : modul);
    }
};

template<class Type, class Container, class Traits>
Type MapStringToClassNonRV<Type, Container, Traits>::_nullEntry;

