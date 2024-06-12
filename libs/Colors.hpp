#pragma once

namespace Colors {
    #ifdef __MINGW32__
        constexpr const char* RED     = "";
        constexpr const char* GREEN   = "";
        constexpr const char* YELLOW  = "";
        constexpr const char* BLUE    = "";
        constexpr const char* MAGENTA = "";
        constexpr const char* CYAN    = "";
        constexpr const char* RESET   = "";
    #else
        constexpr const char* RED     = "\x1b[31m";
        constexpr const char* GREEN   = "\x1b[32m";
        constexpr const char* YELLOW  = "\x1b[33m";
        constexpr const char* BLUE    = "\x1b[34m";
        constexpr const char* MAGENTA = "\x1b[35m";
        constexpr const char* CYAN    = "\x1b[36m";
        constexpr const char* RESET   = "\x1b[0m";
    #endif
};