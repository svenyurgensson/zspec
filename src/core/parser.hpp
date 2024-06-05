#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <string_view>
#include "../../libs/Colors.hpp"
#include "../../libs/toml.hpp"
#include "../../libs/utils.hpp"
#include "test_suits_config.hpp"
#include "labels_parser.hpp"
#include "test_spec_executor.hpp"

void run_tests(const char* path);

constexpr int MAX_LOAD_ADDRESS = 0xFFFF;
constexpr int MAX_BIN_SIZE = 0x10000;
constexpr int MAX_CPU_TICKS_PER_TEST = 500000;
constexpr int MAX_LABELS_FILE_SIZE = 200000;