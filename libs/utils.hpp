#pragma once 
#include <cctype>
#include <string>
#include <stdexcept>
#include <cwchar>
#include <cstdio>
#include <type_traits>
#include "toml.hpp"


static std::vector<std::string> split_string(std::string str, char splitter) {
    std::vector<std::string> result;
    std::string current = ""; 
    for(int i = 0; i < str.size(); i++){
        if(str[i] == splitter){
            if(current != ""){
                result.push_back(current);
                current = "";
            } 
            continue;
        }
        current += str[i];
    }
    if(current.size() != 0)
        result.push_back(current);
    return result;
}

static inline std::string string_toupper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), 
                   [](unsigned char c){ return std::toupper(c); }
                  );
    return s;
}

static inline std::string string_tolower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), 
                   [](unsigned char c){ return std::tolower(c); }
                  );
    return s;
}

static inline void fail(std::string str) {
    std::cerr << Colors::RED << str << Colors::RESET;
    exit(1);
}

static inline void warn(std::string str) {
    std::cerr << Colors::YELLOW << str << Colors::RESET;
}

static inline void fail_parse_error(toml::parse_error& err) {
    std::cerr
        << Colors::RED
        << "Error parsing file '" << *err.source().path
        << "':\n" << err.description()
        << "\n (" << err.source().begin << ")\n"
        << Colors::RESET;
    exit(1);
}

template<typename T, typename ... Args>
static std::basic_string<T> string_format(T const* const format, Args ... args)
{
    int size_signed{ 0 };

    // 1) Determine size with error handling:    
    if constexpr (std::is_same_v<T, char>) { // C++17
        size_signed = std::snprintf(nullptr, 0, format, args ...);
    }
    else {
        size_signed = std::swprintf(nullptr, 0, format, args ...);
    }  
    if (size_signed <= 0) {
        throw std::runtime_error("error during formatting.");
    }
    const auto size = static_cast<size_t>(size_signed);

    // 2) Prepare formatted string:
    std::basic_string<T> formatted(size, T{});
    if constexpr (std::is_same_v<T, char>) { // C++17
        std::snprintf(formatted.data(), size + 1, format, args ...); // +1 for the '\0' (it will not be part of formatted).
    }
    else {
        std::swprintf(formatted.data(), size + 1, format, args ...); // +1 for the '\0' (it will not be part of formatted).
    }

    return formatted; // Named Return Value Optimization (NRVO), avoids an unnecessary copy. 
}

// string_format("string. number %d.", 10)
// TODO: add context!
static inline int check_8bit(int i) {
    if (i > 255 || i < 0) fail(string_format("%d is wrong 8-bit value!", i));
    return i;
}

// TODO: add context!
static inline int check_16bit(int i) {
    if (i > 65535 || i < 0) fail(string_format("%d is wrong 16-bit value!", i));
    return i;
}

// TODO: add context!
static inline int check_bit(int i) {
    switch (i) {
        case 1: return 1;
        case 0: return 0;
        default: fail(string_format("%d is wrong boolean value!", i));
    }
    return 0;
}


#define SWITCH(str)  switch(s_s::str_hash_for_switch(str))
#define CASE(str)    static_assert(s_s::str_is_correct(str) && (s_s::str_len(str) <= s_s::MAX_LEN),\
"CASE string contains wrong characters, or its length is greater than 9");\
case s_s::str_hash(str, s_s::str_len(str))
#define DEFAULT  default

namespace s_s
{
    typedef unsigned char uchar;
    typedef unsigned long long ullong;

    const uchar MAX_LEN = 9;
    const ullong N_HASH = static_cast<ullong>(-1);

    constexpr ullong raise_128_to(const uchar power)
    {
        return 1ULL << 7 * power;
    }

    constexpr bool str_is_correct(const char* const str)
    {
        return (static_cast<signed char>(*str) > 0) ? str_is_correct(str + 1) : (*str ? false : true);
    }

    constexpr uchar str_len(const char* const str)
    {
        return *str ? (1 + str_len(str + 1)) : 0;
    }

    constexpr ullong str_hash(const char* const str, const uchar current_len)
    {
        return *str ? (raise_128_to(current_len - 1) * static_cast<uchar>(*str) + str_hash(str + 1, current_len - 1)) : 0;
    }

    inline ullong str_hash_for_switch(const char* const str)
    {
        return (str_is_correct(str) && (str_len(str) <= MAX_LEN)) ? str_hash(str, str_len(str)) : N_HASH;
    }

    inline ullong str_hash_for_switch(const std::string& str)
    {
        return (str_is_correct(str.c_str()) && (str.length() <= MAX_LEN)) ? str_hash(str.c_str(), str.length()) : N_HASH;
    }
}