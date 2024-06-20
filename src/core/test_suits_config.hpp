#pragma once

struct PredefinedConstant {
    std::string name;
    int64_t value;
};

struct MemoryImage {
    int load_addr = 0;
    std::string bin_file;
    int size = 0xFFFF;
    char mem[0x10000];

    std::vector<PredefinedConstant> constants;
};

struct TestClear {
    int start = -1;
    int end = -1;
    int fill = -1;
};

struct InitMemory {
    int start = -1;
    std::vector<int> fill;
};

struct FileInitMemory {
    int start = -1;
    int size = -1;
    int offset = -1;
    std::string filename;
    char content[0x10000];
};

struct TestInit {
    TestClear test_clear;
    std::vector<InitMemory> memory_initializers;
    std::vector<FileInitMemory> file_initializers;
};

struct TestRun {
    std::string name;
    int call = -1;
    int max_ticks = -1;
};

struct TestExpectRegisters {
    int fl_z = -1, fl_p = -1, fl_c = -1;
    int reg_a = -1, reg_a_ = -1, reg_h = -1, reg_h_ = -1, reg_l = -1, reg_l_ = -1;
    int reg_b = -1, reg_b_ = -1, reg_c = -1, reg_c_ = -1, reg_d = -1, reg_d_ = -1,  reg_e = -1, reg_e_ = -1;
    int reg_ix = -1, reg_iy = -1, reg_sp = -1;
};

struct TestExpectTiming {
    int max_tick = -1;
    int exact_tick = -1;
};

struct TestMemoryState {
    int address = -1;
    int value = -1;
};

struct TestExpectMemory {
    bool is_word = false;
    int address = -1;
    int value = -1;
    int value_not = -1;
};

struct TestExpectPort {
    int address = -1;
    int value = -1;
    std::vector<uint8_t> series;
};

struct TestPreconditions {
    int fl_z = -1, fl_p = -1, fl_c = -1;
    int reg_a = -1, reg_a_ = -1, reg_h = -1, reg_h_ = -1, reg_l = -1, reg_l_ = -1;
    int reg_b = -1, reg_b_ = -1, reg_c = -1, reg_c_ = -1, reg_d = -1, reg_d_ = -1,  reg_e = -1, reg_e_ = -1;
    int reg_sp = -1;
    int reg_ix = -1;
    int reg_iy = -1;

    std::vector<TestMemoryState> memory_states;
};

enum TRESULT { PASS, FAIL, SKIP };

struct TestResult {
    TRESULT code;
    std::vector<std::string> results;
};

struct SingleTest {
    std::string name;
    bool is_skipped = false;
    TestRun test_run;
    int repeat = 1;
    TestPreconditions preconditions;
    TestInit initializer;

    TestExpectRegisters expect_registers;
    std::vector<TestExpectMemory> memory_expectations;
    std::vector<TestExpectPort> ports_expectations;
    TestExpectTiming expect_timing;

    TestResult result;
};

struct ZTest {
    MemoryImage memory_image;
    std::vector<SingleTest> tests_list;
    int total_count = 0;
    int skipped_count = 0;
    int failed_count = 0;
};

const std::string REGISTERS[] = { "a", "h", "l", "b", "c", "d", "e",
                                "a_", "h_", "l_", "b_", "c_", "d_", "e_",
                                "hl", "hl_", "bc", "bc_", "de", "de_", "sp",
                                "ix", "iy",
                                "fl_z", "fl_p", "fl_c" };

// For keeping pristine memory image between tests
static MemoryImage original_memory_image;
// labels filepath
static std::string labels_file;
// here tests lives
static ZTest zspec_suit;