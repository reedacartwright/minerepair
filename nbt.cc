/*
# Copyright (c) 2019 Reed A. Cartwright <reed@cartwright.ht>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
*/

#include <cassert>
#include <vector>
#include <cstring>

#include "nbt.hpp"

template<typename T>
inline
char* read_val(char *first, char *last, std::string_view name, std::vector<mcberepair::nbt_t> *v) {
    assert(first != nullptr);
    assert(last != nullptr);
    assert(first <= last);
    assert(v != nullptr);

    if(last-first < sizeof(T)) {
        return nullptr;
    }
    T val;
    memcpy(&val, first, sizeof(T));
    v->emplace_back(name, val);

    return first+sizeof(T);
}

template<typename T>
inline
char* read_val_array(char *first, char *last, std::string_view name, std::vector<mcberepair::nbt_t> *v) {
    assert(first != nullptr);
    assert(last != nullptr);
    assert(first <= last);
    assert(v != nullptr);

    using array_size_t = decltype(T::size);

    if(last-first < sizeof(array_size_t)) {
        return nullptr;
    }
    array_size_t array_size;
    memcpy(&array_size, first, sizeof(array_size_t));
    first += sizeof(int32_t);
    if(last-first < sizeof(std::remove_pointer_t<decltype(T::data)>)*array_size) {
        return nullptr;
    }
    v->emplace_back(name, T{array_size, reinterpret_cast<decltype(T::data)>(first)});

    return first + sizeof(T)*array_size;
}

inline
char* read_list(char *first, char *last, std::string_view name, std::vector<mcberepair::nbt_t> *v) {
    assert(first != nullptr);
    assert(last != nullptr);
    assert(first <= last);
    assert(v != nullptr);

    if(last-first < sizeof(int8_t)) {
        return nullptr;
    }
    int8_t list_type;
    memcpy(&list_type, first, sizeof(int8_t));

    first += sizeof(int8_t);

    if(last-first < sizeof(int32_t)) {
        return nullptr;
    }
    int32_t list_size;
    memcpy(&list_size, first, sizeof(int32_t));

    v->emplace_back(name, mcberepair::nbt_list_t{list_size, list_type});

    return first + sizeof(int32_t);
}

bool parse_nbt(char *first, char *last) {
    assert(first != nullptr);
    assert(last != nullptr);
    assert(first <= last);

    using nbt_type = mcberepair::nbt_type;

    std::vector<mcberepair::nbt_t> nbt_data;

    if(first == last) {
        return true;
    }

    char *p = first;

    while(p < last) {
        // read the type
        auto type = nbt_type{*p};
        p += 1;
        if(type == nbt_type::END) {
            nbt_data.emplace_back(nullptr, mcberepair::nbt_end_t{});
            continue;
        }
        // read the size of the name
        if(last-p < 2) {
            return false; // malformed
        }
        uint16_t name_len;
        memcpy(&name_len, p, sizeof(name_len));
        p += 2;
        if(last-p < name_len) {
            return false;
        }
        std::string_view name{p,name_len};

        switch(nbt_type{type}) {
            case nbt_type::BYTE:
                p = read_val<int8_t>(p, last, name, &nbt_data);
                break;
            case nbt_type::SHORT:
                p = read_val<int16_t>(p, last, name, &nbt_data);
                break;
            case nbt_type::INT:
                p = read_val<int32_t>(p, last, name, &nbt_data);
                break;
            case nbt_type::LONG:
                p = read_val<int64_t>(p, last, name, &nbt_data);
                break;
            case nbt_type::FLOAT:
                p = read_val<float>(p, last, name, &nbt_data);
                break;
            case nbt_type::DOUBLE:
                p = read_val<double>(p, last, name, &nbt_data);
                break;
            case nbt_type::BYTE_ARRAY:
                p = read_val_array<mcberepair::nbt_byte_array_t>(p, last, name, &nbt_data);
                break;
            case nbt_type::STRING:
                p = read_val_array<mcberepair::nbt_string_t>(p, last, name, &nbt_data);
                break;
            case nbt_type::INT_ARRAY:
                p = read_val_array<mcberepair::nbt_int_array_t>(p, last, name, &nbt_data);
                break;
            case nbt_type::LONG_ARRAY:
                p = read_val_array<mcberepair::nbt_long_array_t>(p, last, name, &nbt_data);
                break;
            case nbt_type::COMPOUND:
                nbt_data.emplace_back(nullptr, mcberepair::nbt_compound_t{});
                break;
            case nbt_type::LIST:
                p = read_list(p, last, name, &nbt_data);
                if(p == nullptr) {
                    return false;
                }
                todo;
                break;
            default:
                return false;
        }
        if(p == nullptr) {
            return false;
        }

    }
    return true;
}