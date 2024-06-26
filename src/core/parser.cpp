#include "parser.hpp"

// int result = tbl[section][entry].as_integer().operator const int64_t & ();

using namespace std::string_view_literals;

void build_test_init(toml::node_view<toml::node> section, SingleTest * test) {
    auto clear_start = section.at_path("clear")["start"].value<int>();
    auto clear_end = section.at_path("clear")["end"].value<int>();
    auto clear_fill = section.at_path("clear")["fill"].value<int>();

    if (clear_start.has_value()) {
        if (clear_end.has_value()) {
            int fill = 0;
            if (clear_fill.has_value()) fill = *clear_fill;
            if (*clear_start >= *clear_end)
                fail(string_format("Clearing start address: %d should be less end address: %d", *clear_start, *clear_end));
            test->initializer.test_clear.start = check_16bit(*clear_start);
            test->initializer.test_clear.end   = check_16bit(*clear_end);
            test->initializer.test_clear.fill  = check_8bit(*clear_fill);
        } else warn("You provided 'clear_start' without 'clear_end', skipping clearing memory!\n");
    } else if (clear_end.has_value()) {
        warn("You provided 'clear_end' without 'clear_start', skipping clearing memory!\n");
    }

    // init memory
    auto memory_inits = section.at_path("memory");
    if (!memory_inits.is_array()) return;

    memory_inits.as_array()->for_each([&test](auto& el) {
        if (!el.is_table()) return;

        auto cur_mem_init = el.as_table();
        auto start_def = cur_mem_init->at_path("start");
        int start;
        std::string fname;

        if (!start_def.is_string() && !start_def.is_integer())
            fail(string_format("Error in test: '%s'. Memory start address for initializing must be provided!", test->name.c_str()), el.source());

        if (start_def.is_string()) {
            fname = start_def.as_string()->operator const std::string & ();
            auto iter = std::find_if(original_memory_image.constants.begin(), original_memory_image.constants.end(), [&fname] (const PredefinedConstant& v) { return v.name == fname; });
            if (iter == original_memory_image.constants.end())
                fail(string_format("Cannot found definition for fname or label: '%s' in test: '%s'", fname.c_str(), test->name.c_str()), el.source());
            int index = std::distance(original_memory_image.constants.begin(), iter);
            start = original_memory_image.constants[index].value;
        } else {
            start = start_def.as_integer()->operator const int64_t & ();
        }
        check_16bit(start);

        auto fill = el.at_path("fill");
        if (fill.is_value() && !fill.is_array())
            fail(string_format("Error in test: '%s'. Directive 'fill' must be array of bytes!", test->name.c_str()), el.source());

        auto file = el.at_path("file");

        if (fill.is_value() && file.is_value())
            fail(string_format("Error in test: '%s'. Cannot use 'fill' and 'file' directives simultaneously!", test->name.c_str()), el.source());

        if (fill.is_array()) { // fill memory with array of bytes
            auto fill_arr = fill.as_array();
            std::cout << "    fill at: " << ssprintf0x0000x(start) << " with: " << fill << "\n";
            InitMemory imem = InitMemory{ .name = fname, .start = start };
            for (auto&& elm : *fill_arr) {
                if (elm.is_integer()) {
                    int v = * elm.template value<int>();
                    check_8bit(v);
                    imem.fill.push_back(v);
                } else {
                    fail(string_format("Error in test: '%s'. Element 'fill' should contains only integer values!", test->name.c_str()), el.source());
                }
            }
            test->initializer.memory_initializers.push_back(imem);

        } else if (file.is_value()) { // fill memory from binary file
            auto image_file = file.as_string()->operator const std::string & ();
            std::cout << "fill memory at: " << start << " from file: " << image_file << "\n";
            std::optional<int> load_size = el.at_path("size").template value<int>();

            if (load_size.has_value()) check_16bit(*load_size);

            int offset = 0;
            std::optional<int> load_offs = el.at_path("offset").template value<int>();

            if (load_offs.has_value()) offset = check_16bit(*load_offs);

            // check file
            std::ifstream ifd(image_file, std::ios::binary | std::ios::ate);

            if (!ifd)
                fail(string_format("Cannot open memory image file '%s'!", image_file.c_str()), el.source());

            int size = ifd.tellg();
            if (size + offset > MAX_BIN_SIZE)
                fail(string_format("File file '%s' is too big (%d). Maximum is: %d", image_file.c_str(), size, MAX_BIN_SIZE), el.source());

            int last_addr = offset + size;
            int diff = size - (last_addr - MAX_BIN_SIZE);

            if (size > MAX_BIN_SIZE)
                fail(string_format("File file '%s' is too big (%d). Maximum is: %d", image_file.c_str(), size, diff), el.source());

            ifd.seekg(0, std::ios::beg);
            FileInitMemory fmem = { .start = (int)start, .size = size, .offset = offset, .filename = image_file };
            // load file
            ifd.read(&fmem.content[0], size);
            // copy binary to array
            test->initializer.file_initializers.push_back(fmem);
            ifd.close();
        } else {
            fail(string_format("Error in test: '%s'. Expect either 'fill' or 'file' memory initialize directive for init address: %d!", test->name.c_str(), start), el.source());
        }
    });
}

void build_test_run(toml::node_view<toml::node> section, SingleTest * test) {
    auto max_ticks = section.at_path("max_ticks").template value<int>();

    if (max_ticks.has_value()) {
        int max_ticks_val = *max_ticks;

        if (max_ticks_val > MAX_CPU_TICKS_PER_TEST || max_ticks_val < 0)
            fail(string_format("Wrong 'max_ticks' value:  in section [test.run]!", max_ticks_val), section.at_path("max_ticks").node()->source());

        test->test_run.max_ticks = max_ticks_val;
    }

    std::string constant_function_name = "";
    auto f_name = section.at_path("fname").template value<std::string>();

    if (f_name.has_value()) {
        const std::string fname = *f_name;
        auto iter = std::find_if(original_memory_image.constants.begin(), original_memory_image.constants.end(), [&fname] (const PredefinedConstant& v) { return v.name == fname; });

        if (iter == original_memory_image.constants.end())
             fail(string_format("Cannot found definition for fname or label: '%s' in test: '%s'", fname.c_str(), test->name.c_str()),  section.node()->source());
        int index = std::distance(original_memory_image.constants.begin(), iter);
        constant_function_name = fname;
        test->test_run.call = original_memory_image.constants[index].value;

    } else {
        auto call = section.at_path("call").template value<int>();

        if (call.has_value()) {
            int call_addr = *call;
            if (call_addr >= MAX_BIN_SIZE || call_addr < 0)
                fail(string_format("Start address %d is wrong in test: %s", call_addr, test->name.c_str()), section.node()->source());

            test->test_run.call = call_addr;

        } else {
            fail(string_format("Start address or function to run is not found in test: %s", test->name.c_str()), section.node()->source());
        }
    }
    std::cout << "    run " << constant_function_name << Colors::CYAN << "@" << Colors::RESET << ssprintf0x0000x(test->test_run.call) << "\n";

    auto run_count = section.at_path("repeat").template value<int>();

    if (run_count.has_value()) {
        int repeat = *run_count;

        if (repeat > MAX_RUN_COUNT || repeat < 1)
            fail(string_format("Wrong 'repeat' value: %d in section [test.run], maximum is: %d!", repeat, MAX_RUN_COUNT), section.at_path("run_count").node()->source());

        test->repeat = repeat;
    } else test->repeat = 1;
}

void build_test_expect_registers(toml::node_view<toml::node> exp_regs, SingleTest * test) {
    if (!exp_regs.is_table())
        return;
    exp_regs.as_table()->for_each([&test](auto& key, auto& val) {
        std::string current_reg = string_tolower((std::string)key.str());

        if (!(val.is_integer() || val.is_boolean()))
            fail(string_format("Error in test: '%s'. Expect value for register: '%s' to be integer!", test->name.c_str(), current_reg.c_str()), key.source());

        int value;
        if (val.is_integer()) value = (int64_t)(*(val.as_integer()));
        if (val.is_boolean()) value = (*(val.as_boolean())) ? 1 : 0;

        SWITCH(current_reg) {
            CASE("a"):  { test->expect_registers.reg_a = check_8bit(value); break; }
            CASE("h"):  { test->expect_registers.reg_h = check_8bit(value); break; }
            CASE("l"):  { test->expect_registers.reg_l = check_8bit(value); break; }
            CASE("b"):  { test->expect_registers.reg_b = check_8bit(value); break; }
            CASE("c"):  { test->expect_registers.reg_c = check_8bit(value); break; }
            CASE("d"):  { test->expect_registers.reg_d = check_8bit(value); break; }
            CASE("e"):  { test->expect_registers.reg_e = check_8bit(value); break; }
            CASE("a_"): { test->expect_registers.reg_a_ = check_8bit(value); break; }
            CASE("h_"): { test->expect_registers.reg_h_ = check_8bit(value); break; }
            CASE("l_"): { test->expect_registers.reg_l_ = check_8bit(value); break; }
            CASE("b_"): { test->expect_registers.reg_b_ = check_8bit(value); break; }
            CASE("c_"): { test->expect_registers.reg_c_ = check_8bit(value); break; }
            CASE("d_"): { test->expect_registers.reg_d_ = check_8bit(value); break; }
            CASE("e_"): { test->expect_registers.reg_e_ = check_8bit(value); break; }

            CASE("fl_z"): { test->expect_registers.fl_z = check_bit(value); break; }
            CASE("fl_p"): { test->expect_registers.fl_p = check_bit(value); break; }
            CASE("fl_c"): { test->expect_registers.fl_c = check_bit(value); break; }

            CASE("sp"):  { test->expect_registers.reg_sp = check_16bit(value); break; }
            CASE("hl"):  { check_16bit(value); test->expect_registers.reg_h = (value >> 8) & 0xff; test->expect_registers.reg_l = value & 0xff; break; }
            CASE("de"):  { check_16bit(value); test->expect_registers.reg_d = (value >> 8) & 0xff; test->expect_registers.reg_e = value & 0xff; break; }
            CASE("bc"):  { check_16bit(value); test->expect_registers.reg_b = (value >> 8) & 0xff; test->expect_registers.reg_c = value & 0xff; break; }
            CASE("hl_"): { check_16bit(value); test->expect_registers.reg_h_ = (value >> 8) & 0xff; test->expect_registers.reg_l_ = value & 0xff; break; }
            CASE("de_"): { check_16bit(value); test->expect_registers.reg_d_ = (value >> 8) & 0xff; test->expect_registers.reg_e_ = value & 0xff; break; }
            CASE("bc_"): { check_16bit(value); test->expect_registers.reg_b_ = (value >> 8) & 0xff; test->expect_registers.reg_c_ = value & 0xff; break; }

            CASE("ix"): { test->expect_registers.reg_ix = check_16bit(value); break; }
            CASE("iy"): { test->expect_registers.reg_iy = check_16bit(value); break; }

            default: {
                fail(string_format("Error in test: '%s'. Unknown register: '%s' in expectations!", test->name.c_str(), current_reg.c_str()), key.source());
            }
        }
    });
}

void build_test_expect_memory(toml::node_view<toml::node> section, SingleTest * test) {
    if (!section.is_array_of_tables()) return;

    section.as_array()->for_each([&test](auto& el) {
        auto address   = el.at_path("address");
        auto is_word   = el.at_path("word").template value<bool>();
        auto value     = el.at_path("value").template value<int64_t>();
        auto value_not = el.at_path("value_not").template value<int64_t>();
        auto series    = el.at_path("series");

        int addr = -1;
        TestExpectMemory expm;
        std::string fname;

        if (address.is_integer()) {
            auto long_addr = * address.template value<int64_t>();
            addr = (int)long_addr;
        } else if (address.is_string()) {
            fname = address.as_string()->operator const std::string & ();

            auto iter = std::find_if(original_memory_image.constants.begin(), original_memory_image.constants.end(), [&fname] (const PredefinedConstant& v) { return v.name == fname; });

            if (iter == original_memory_image.constants.end())
                fail(string_format("Cannot found definition for fname or label: '%s' in test: '%s'", fname.c_str(), test->name.c_str()), address.node()->source());

            int index = std::distance(original_memory_image.constants.begin(), iter);
            addr = original_memory_image.constants[index].value;
        } else {
            fail(string_format("Error in test: '%s'. Memory expectation must have 'address' set.", test->name.c_str()), el.source());
        }

        expm.name = fname;
        expm.address = addr;

        int max_load_addr =  MAX_LOAD_ADDRESS;
        int max_int = 0xff;

        if (* is_word) {
            max_load_addr =  MAX_LOAD_ADDRESS - 1;
            max_int = 0xffff;
        }

        if (addr > max_load_addr || addr < 0)
            fail(string_format("Error in test: '%s'. Memory expectation 'address'=%d exceeds max address: %d!", test->name.c_str(), addr, max_load_addr), address.node()->source());

        if (series.is_array() && series.is_homogeneous(toml::node_type::integer)) {
            int idx = addr;
            series.as_array()->for_each([&test, &idx, &is_word, &max_load_addr, &max_int](auto& el) {
                auto val = el.as_integer()->operator const int64_t & ();

                if (val > max_int || val < 0)
                    fail(string_format("Error in test: '%s'. Memory expectation 'value'=%d exceeds %d!", test->name.c_str(), val, max_int), el.source());

                if (idx > max_load_addr)
                    fail(string_format("Error in test: '%s'. Memory expectation 'address'=%d exceeds max address: %d!", test->name.c_str(), idx, max_load_addr), el.source());

                TestExpectMemory expm;
                expm.is_word = (* is_word);
                expm.address = idx;
                expm.value = static_cast<int>(val);
                test->memory_expectations.push_back(expm);
                idx += 1;
            });

        } else {

            if (value.has_value() && value_not.has_value())
                fail(string_format("Error in test: '%s'. Memory expectation 'value' and 'value_not' cannot be defined simultaneously!", test->name.c_str()),  el.at_path("value").node()->source());

            if (!value.has_value() && !value_not.has_value())
                fail(string_format("Error in test: '%s'. Memory expectation 'value' or 'value_not' have to be defined!", test->name.c_str()),  el.source());

            if (value.has_value()) {
                int val = * value;

                if (val > max_int || val < 0)
                    fail(string_format("Error in test: '%s'. Memory expectation 'value'=%d exceeds %d!", test->name.c_str(), val, max_int),  el.at_path("value").node()->source());

                expm.is_word = (* is_word);
                expm.address = addr;
                expm.value = val;
            }
            if (value_not.has_value()) {
                int val = * value_not;

                if (val > max_int || val < 0)
                    fail(string_format("Error in test: '%s'. Memory expectation 'value_not'=%d exceeds %d!", test->name.c_str(), val, max_int),  el.at_path("value_not").node()->source());

                expm.is_word = (* is_word);
                expm.address = addr;
                expm.value_not = val;
            }

            test->memory_expectations.push_back(expm);
        }
    });
}

void build_test_expect_ports(toml::node_view<toml::node> section, SingleTest * test) {


} // TODO test.expect_port

void build_test_expect_timing(toml::node_view<toml::node> section, SingleTest * test) {
    auto max_ticks = section["max_ticks"].value<int64_t>();
    auto exact_ticks = section["exact_ticks"].value<int64_t>();

    if (max_ticks.has_value()) {
        if (exact_ticks.has_value())
            fail(string_format("Error in test: '%s'. Cannot have set both 'max_ticks' and 'exact_ticks'!", test->name.c_str()), section.at_path("max_ticks").node()->source());

        if (*max_ticks >= MAX_CPU_TICKS_PER_TEST || *max_ticks < 4)
            fail(string_format("Error in test: '%s'. 'max_ticks' set to: %d, but maximum allowed is: %d and minimum is: 4!", test->name.c_str(), *max_ticks, MAX_CPU_TICKS_PER_TEST), section.at_path("max_ticks").node()->source());

        test->expect_timing.max_tick = *max_ticks;
    } else if (exact_ticks.has_value()) {

        if (*exact_ticks >= MAX_CPU_TICKS_PER_TEST || *exact_ticks < 4)
            fail(string_format("Error in test: '%s'. 'exact_ticks' set to: %d, but maximum allowed is: %d and minimum is: 4!", test->name.c_str(), *exact_ticks, MAX_CPU_TICKS_PER_TEST), section.at_path("exact_ticks").node()->source());

        test->expect_timing.exact_tick = *exact_ticks;
    }
}

void set_precondition_reg(std::string current_reg, int value, SingleTest* test) {
    std::cout << "    " << string_toupper(current_reg) << " = ";
    SWITCH(current_reg) {
        CASE("a"):  { std::cout << ssprintf0x00x(value); test->preconditions.reg_a  = check_8bit(value); return; }
        CASE("h"):  { std::cout << ssprintf0x00x(value); test->preconditions.reg_h  = check_8bit(value); return; }
        CASE("l"):  { std::cout << ssprintf0x00x(value); test->preconditions.reg_l  = check_8bit(value); return; }
        CASE("b"):  { std::cout << ssprintf0x00x(value); test->preconditions.reg_b  = check_8bit(value); return; }
        CASE("c"):  { std::cout << ssprintf0x00x(value); test->preconditions.reg_c  = check_8bit(value); return; }
        CASE("d"):  { std::cout << ssprintf0x00x(value); test->preconditions.reg_d  = check_8bit(value); return; }
        CASE("e"):  { std::cout << ssprintf0x00x(value); test->preconditions.reg_e  = check_8bit(value); return; }
        CASE("a_"): { std::cout << ssprintf0x00x(value); test->preconditions.reg_a_ = check_8bit(value); return; }
        CASE("h_"): { std::cout << ssprintf0x00x(value); test->preconditions.reg_h_ = check_8bit(value); return; }
        CASE("l_"): { std::cout << ssprintf0x00x(value); test->preconditions.reg_l_ = check_8bit(value); return; }
        CASE("b_"): { std::cout << ssprintf0x00x(value); test->preconditions.reg_b_ = check_8bit(value); return; }
        CASE("c_"): { std::cout << ssprintf0x00x(value); test->preconditions.reg_c_ = check_8bit(value); return; }
        CASE("d_"): { std::cout << ssprintf0x00x(value); test->preconditions.reg_d_ = check_8bit(value); return; }
        CASE("e_"): { std::cout << ssprintf0x00x(value); test->preconditions.reg_e_ = check_8bit(value); return; }

        CASE("fl_z"): { std::cout << ssprintf_bool(value); test->preconditions.fl_z = check_bit(value); return; }
        CASE("fl_p"): { std::cout << ssprintf_bool(value); test->preconditions.fl_p = check_bit(value); return; }
        CASE("fl_c"): { std::cout << ssprintf_bool(value); test->preconditions.fl_c = check_bit(value); return; }

        CASE("sp"):  { std::cout << ssprintf0x0000x(value); test->preconditions.reg_sp = check_16bit(value); return; }
        CASE("hl"):  { std::cout << ssprintf0x0000x(value); check_16bit(value); test->preconditions.reg_h  = (value >> 8) & 0xff; test->preconditions.reg_l = value & 0xff; return; }
        CASE("de"):  { std::cout << ssprintf0x0000x(value); check_16bit(value); test->preconditions.reg_d  = (value >> 8) & 0xff; test->preconditions.reg_e = value & 0xff; return; }
        CASE("bc"):  { std::cout << ssprintf0x0000x(value); check_16bit(value); test->preconditions.reg_b  = (value >> 8) & 0xff; test->preconditions.reg_c = value & 0xff; return; }
        CASE("hl_"): { std::cout << ssprintf0x0000x(value); check_16bit(value); test->preconditions.reg_h_ = (value >> 8) & 0xff; test->preconditions.reg_l_ = value & 0xff; return; }
        CASE("de_"): { std::cout << ssprintf0x0000x(value); check_16bit(value); test->preconditions.reg_d_ = (value >> 8) & 0xff; test->preconditions.reg_e_ = value & 0xff; return; }
        CASE("bc_"): { std::cout << ssprintf0x0000x(value); check_16bit(value); test->preconditions.reg_b_ = (value >> 8) & 0xff; test->preconditions.reg_c_ = value & 0xff; return; }

        CASE("ix"): { std::cout << ssprintf0x0000x(value); test->preconditions.reg_ix = check_16bit(value); return; }
        CASE("iy"): { std::cout << ssprintf0x0000x(value); test->preconditions.reg_iy = check_16bit(value); return; }
    }
}


void handle_tests(toml::node_view<toml::node> test_section, int test_to_run) {
    if (!test_section.is_array_of_tables())
        fail("'[[test]]' section(s) not found!");

    std::cout << "\n";

    test_section.as_array()->for_each([&test_to_run](auto& section, int test_idx) {
        SingleTest test = { .is_skipped = false };

        const toml::table& sec = *section.as_table();

        auto test_name = sec["name"].value_or(string_format("test_%d", test_idx + 1));
        test.name = test_name.c_str();

        std::printf("[test][%.3i]: ", test_idx+1);

        auto skipped_name = sec["xname"].value_or("#"sv);

        bool need_skip = ((test_to_run !=0) &&
                          (test_to_run != (test_idx + 1))) ||
                          sec["xname"].is_string();

        zspec_suit.total_count += 1;

        if (need_skip) {
            test.is_skipped = true;
            test.name = skipped_name;
            std::cout << Colors::BLUE << test.name  << " (skipped)" << Colors::RESET << "\n";
            test.result.code = SKIP;
            zspec_suit.skipped_count += 1;
            return;
        } else {
            std::cout << test.name  << "\n";
        }

        // set registers
        // TODO: fix setting precondition registers
        for(const std::string &current_reg : REGISTERS) {
            auto reg = section.at_path(current_reg).template value<int>();

            if (reg.has_value()) {
                set_precondition_reg(current_reg, *reg, &test);
                std::cout << "\n";
            } else {
                reg = section.at_path(string_toupper(current_reg)).template value<int>();

                if (reg.has_value()) {
                    set_precondition_reg(current_reg, *reg, &test);
                    std::cout << "\n";
                }
            }
        };

        auto init_section = section.at_path("init");

        if (init_section.is_table() && !init_section.as_table()->empty()) {
            build_test_init(init_section, &test);
        }

        auto run_section = section.at_path("run");
        build_test_run(run_section, &test);

        auto expect_section = section.at_path("expect");

        auto expect_registers_section = expect_section.at_path("registers");
        build_test_expect_registers(expect_registers_section, &test);

        auto expect_memory_section = expect_section.at_path("memory");

        build_test_expect_memory(expect_memory_section, &test);

        auto expect_ports_section = expect_section.at_path("port");
        build_test_expect_ports(expect_ports_section, &test);

        auto expect_timing_section = expect_section.at_path("timing");
        build_test_expect_timing(expect_timing_section, &test);

        zspec_suit.tests_list.push_back(test);

        execute_test_spec(&test, &original_memory_image, &zspec_suit);

        test_idx++;
        std::cout << "\n";
    });
}

void handle_init_section(toml::node_view<toml::node> init_section) {
    // set initial vars and files
    auto load_addr = init_section["load_addr"].value<int>();

    if (!load_addr.has_value())
        fail("'load_address' not found in [init] section!", init_section["load_addr"].node()->source());

    if (load_addr > MAX_LOAD_ADDRESS)
        fail(string_format("'load_address' exeeds maximum memory (%d) address!", MAX_LOAD_ADDRESS), init_section["load_addr"].node()->source());

    if (load_addr < 0)
        fail("'load_address' cannot be less then 0!", init_section["load_addr"].node()->source());

    original_memory_image.load_addr = *load_addr;

    // maybe build content
    auto build = init_section["build"];

    if (build.is_array()) {

        if (build.as_array()->size() < 1)
            fail("Error! Parameter 'build' must be array of strings: filename of external program and it`s arguments, but it is empty!", build.node()->source());

        std::vector<std::string> argv;

        build.as_array()->for_each([&build, &argv](auto& el) {
            if constexpr (toml::is_string<decltype(el)>) {
                argv.push_back(*el);
            } else {
                std::cout << Colors::RED << build << Colors::RESET << "\n";
                fail("Error! Parameter 'build' must be array of strings, example:\nbuild = [\"external_program\", \"--arg1\", \"value\", \"-o\", \"outfile\" ]\nbut you provided wrong value!", build.node()->source());
            }
        });

        auto cmd = new CommandLine(argv[0]);
        for (int i = 1; i < argv.size(); i++) cmd->arg(argv[i]);
        std::cout << Colors::YELLOW << "Run external program: " << cmd->getCommandlineString() << "\n";
        int result = cmd->executeAndWait(0, 0, 0);

        if (result !=0)
            fail("Error running external program!", build.node()->source());
        std::cout << Colors::RESET;
    } else
        if (build.is_value()) {
            fail("Error! Parameter 'build' must be array of strings: filename of external program and it`s arguments, but it is not array!", build.node()->source());
        }

    // load content
    auto bin_file = init_section["bin_file"].value<std::string>();

    if (!bin_file.has_value())
        fail("'bin_file' not found in [init] section!", init_section["bin_file"].node()->source());

    // try to check file exists, its size and load it
    std::string memory_image_file = *bin_file;
    std::ifstream ifd(memory_image_file, std::ios::binary | std::ios::ate);

    if (!ifd) {
        fail(string_format("Cannot open 'bin_file'=%s", memory_image_file.c_str()), init_section["bin_file"].node()->source());
    }

    int size = ifd.tellg();

    if (size > MAX_BIN_SIZE) {
        fail(string_format("File 'bin_file' %s is too big (%d). Maximum is: %d", memory_image_file.c_str(), size, MAX_BIN_SIZE), init_section["bin_file"].node()->source());
    }

    int last_addr = original_memory_image.load_addr + size;
    int diff = size - (last_addr - MAX_BIN_SIZE);

    if (size > MAX_BIN_SIZE) {
        fail(string_format("File 'bin_file' %s is too big (%d). Maximum is: %d", memory_image_file.c_str(), size, diff), init_section["bin_file"].node()->source());
    }

    ifd.seekg(0, std::ios::beg);
    ifd.read(&original_memory_image.mem[original_memory_image.load_addr], size);
    ifd.close();

    // defining consts
    auto defined_constants = init_section["const"];
    if (defined_constants.is_table()) {
        defined_constants.as_table()->for_each([](auto& key, toml::value<int64_t>& value) {
            if constexpr (toml::is_integer<decltype(value)>) {

                if (value > MAX_LOAD_ADDRESS || value < 0) {
                    std::cerr << Colors::RED << "Defined constant: '" << key << "' = "sv << value << " exeeds maximum memory ("sv << MAX_LOAD_ADDRESS << ") address!" << std::endl << key.source() << std::endl;
                    exit(1);
                }

                PredefinedConstant pconst = { (std::string)key, (int64_t)value };
                original_memory_image.constants.push_back(pconst);

            } else {
                std::cerr << Colors::RED << "Constants can be only integers! Found: " << key << " = "sv << value << std::endl << key.source() << std::endl;
                exit(1);
            }
        });
    }

    // load labels
    auto lf = init_section["labels_file"].value<std::string>();

    if (lf.has_value()) {
        // try to check file exists, its size and load it
        std::string labels_file = *lf;
        std::ifstream ifd(labels_file, std::ios::binary | std::ios::ate);

        if (!ifd)
            fail(string_format("Cannot open 'labels_file' = %s", labels_file.c_str()), init_section["labels_file"].node()->source());

        int size = ifd.tellg();

        if (size > MAX_LABELS_FILE_SIZE)
            fail(string_format("Labels file: %s is too big!", labels_file.c_str()), init_section["labels_file"].node()->source());

        ifd.seekg(0, std::ios::beg);

        // parse labels
        std::filesystem::path file_path = labels_file;
        std::cout << "Parsing labels: " << file_path << Colors::MAGENTA << "\n";
        labels_parser(&ifd, file_path.extension().generic_string());

        ifd.close();
    }
}

void run_tests(const char* path, int test_to_run) {
    toml::parse_result config;

    try {
        config = toml::parse_file( path );
    } catch (toml::parse_error &ex) {
        fail_parse_error(ex);
    }

    toml::node_view<toml::node> init_section = config["init"];
    handle_init_section(init_section);

    toml::node_view<toml::node> test_sections = config["test"];
    handle_tests(test_sections, test_to_run);

    std::cout << "------------------------------------------------------------------------\n" << std::dec;

    int total_tests = zspec_suit.total_count;
    int skipped = zspec_suit.skipped_count;
    int failed = zspec_suit.failed_count;
    int passed = total_tests - (skipped + failed);

    std::cout << "Total tests: " << total_tests << "  ";
    std::cout << Colors::BLUE  << "Skipped tests: " << skipped << "  ";
    std::cout << Colors::RED   << "Failed tests: "  << failed << "  ";
    std::cout << Colors::GREEN << "Passed tests: "  << passed << "\n";
}