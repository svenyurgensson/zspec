#include <iostream>
#include <filesystem>
#include <string>
#include <string_view>
#include "../../libs/Colors.hpp"
#include "../../libs/toml.hpp"
#include "test_suits_config.hpp"

void config_parser(const char* path);


constexpr int MAX_LOAD_ADDRESS = 0xFFFF;
constexpr int MAX_BIN_SIZE = 0x10000;