#ifndef D_ARRAY_H
#define D_ARRAY_H

#include "../../src/core/logger.h"
#include "../../src/core/defines.h"

// clang-format off
template <class T>
class Array
{
    private:
    T *_data;
    size_t _size;
    size_t _capacity;

    public:
    // constructors
    Array(size_t capacity = 16)
        : _data((T *)malloc(capacity * sizeof(T))), 
          _size(0), _capacity(capacity)
    {
        if(_data == nullptr)
        {
            LERROR("could not allocate memory for array");
            return;
        }
    }

    Array(const T* data, size_t size)
        : _data((T *)malloc(size * sizeof(T))),
          _size(size), _capacity(size)
    {
        if(_data == nullptr)
        {
            LERROR("could not allocate memory for array");
            return;
        }
        memcpy(_data, data, size * sizeof(T));
    }

    // clang-format on

    // getters
    size_t size() const { return _size; }
    size_t capacity() const { return _capacity; }
    T *data() const { return _data; }
    bool empty() const { return _size == 0; }

    // functions
    void append(const T &value)
    {
        if(_size == _capacity)
        {
            // vector doubling technique
            _capacity *= 2;
            _data = (T *)realloc(_data, _capacity * sizeof(T));
            if(_data == nullptr)
            {
                LERROR("could not allocate memory for array");
                return;
            }
        }
        _data[_size++] = value;
    }

    T &operator[](size_t index) { return _data[index]; }

    // iterator support
    T *begin() { return _data; }
    T *end() { return _data + _size; }

    // operators
    // copy assignment
    Array &operator=(const Array &other)
    {
        if(this == &other)
            return *this;

        _size     = other._size;
        _capacity = other._capacity;
        _data     = (T *)realloc(_data, _capacity * sizeof(T));
        if(_data == nullptr)
        {
            LERROR("could not allocate memory for array");
            return *this;
        }

        memcpy(_data, other._data, _size * sizeof(T));
        return *this;
    }

    // destructor
    ~Array() { free(_data); }
};

#endif