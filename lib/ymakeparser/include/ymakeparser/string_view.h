#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include "../../src/core/defines.h"

#include <cstdlib>
#include <cstring>

struct StringView
{
    private:
    const char *_data;
    usize _size;

    public:
    // constructors
    // clang-format off
    StringView(const char *data, usize size)
        : _data(data), _size(size) {}

    // clang-format on

    StringView(const char *data) : _data(data), _size(strlen(data)) {}
    StringView() : _data(nullptr), _size(0) {}

    // getters
    const char *data() const { return _data; }
    usize size() const { return _size; }

    // functions
    bool empty() const { return _size == 0 || _data == nullptr; }

    void trim()
    {
        // remove leading spaces
        while(*_data == ' ')
        {
            _data++;
            _size--;
        }

        // remove trailing spaces
        while(_data[_size] == ' ')
            _size--;
    }

    StringView substr(usize start, usize end) const
    {
        // get substring from start to end
        return StringView(_data + start, end - start);
    }

    bool find(const char *str) const
    {
        // find the first occurence of str in the string
        return strstr(_data, str) != nullptr;
    }

    // Add char access operator
    char operator[](usize pos) const { return pos < _size ? _data[pos] : '\0'; }

    // Add comparison operators
    bool operator==(const StringView &other) const
    {
        return _size == other._size && memcmp(_data, other._data, _size) == 0;
    }

    bool operator!=(const StringView &other) const { return !(*this == other); }

    // Add string comparison
    int compare(const StringView &other) const
    {
        usize min_size = _size < other._size ? _size : other._size;
        int result     = memcmp(_data, other._data, min_size);
        if(result == 0)
        {
            if(_size < other._size)
                return -1;
            if(_size > other._size)
                return 1;
        }
        return result;
    }

    // Add starts_with and ends_with
    bool starts_with(const StringView &sv) const { return _size >= sv._size && memcmp(_data, sv._data, sv._size) == 0; }

    bool ends_with(const StringView &sv) const
    {
        return _size >= sv._size && memcmp(_data + _size - sv._size, sv._data, sv._size) == 0;
    }

    // Add iterator support
    const char *begin() const { return _data; }
    const char *end() const { return _data + _size; }

    // split returns an array of StringView
    StringView *split(char delim, usize &count) const
    {
        // count the number of delimiters
        count = 0;
        for(usize i = 0; i < _size; i++)
            if(_data[i] == delim)
                count++;

        // allocate memory for the array of StringView
        StringView *result = (StringView *)malloc((count + 1) * sizeof(StringView));

        // split the string
        usize start = 0;
        usize index = 0;
        for(usize i = 0; i < _size; i++)
        {
            if(_data[i] == delim)
            {
                result[index++] = substr(start, i);
                start           = i + 1;
            }
        }
        result[index++] = substr(start, _size);

        return result;
    }

    ~StringView()
    {
        _data = nullptr;
        _size = 0;
    }
};

#endif // STRING_VIEW_H