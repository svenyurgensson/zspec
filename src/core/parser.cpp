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
        if constexpr (toml::is_table<decltype(el)>) {
            const toml::table& cur_mem_init = *el.as_table();
            std::optional<int64_t> start = cur_mem_init["start"].value<int>();
            if (!start.has_value()) fail(string_format("Error in test: '%s'. Memory start address for initializing must be provided!", test->name.c_str()));
            check_16bit(*start);

            auto fill = el.at_path("fill");
            if (fill.is_value() && !fill.is_array())            
                fail(string_format("Error in test: '%s'. Directive 'fill' must be array of bytes!", test->name.c_str()));            

            std::optional<std::string> file = el["file"].template value<std::string>();

            if (fill.is_value() && file.has_value()) 
                fail(string_format("Error in test: '%s'. Cannot use 'fill' and 'file' directives simultaneously!", test->name.c_str()));            
        
            if (fill.is_array()) { // fill memory with array of bytes
                auto fill_arr = fill.as_array();
                std::cout << "fill at: " << *start << " with: " << fill << "\n";
                InitMemory imem;
                imem.start = *start;
                auto iter = std::find_if(fill_arr->begin(), fill_arr->end(), [&imem] (const auto& v) { 
                    return v.is_integer() && 
                            check_8bit(* v.template value<int>()) &&
                            (([&imem](const auto& vv) { imem.fill.push_back(* vv.template value<int>()); return true; })(v)); 
                });
                test->initializer.memory_initializers.push_back(imem);

            } else if (file.has_value()) { // fill memory from binary file
                std::cout << "fill memory at: " << *start << " from file: " << *file << "\n";
                std::optional<int> load_size = el["size"].template value<int>();
                if (load_size.has_value()) check_16bit(*load_size);
                int offset = 0;
                std::optional<int> load_offs = el["offset"].template value<int>();
                if (load_offs.has_value()) offset = check_16bit(*load_offs);

                // check file
                std::string image_file = *file;
                std::ifstream ifd(image_file, std::ios::binary | std::ios::ate);

                if (!ifd)
                    fail(string_format("Cannot open 'file' %s!", image_file.c_str()));
                int size = ifd.tellg();
                if (size + offset > MAX_BIN_SIZE)
                    fail(string_format("File 'file' %s is too big (%d). Maximum is: %d", image_file.c_str(), size, MAX_BIN_SIZE));
                int last_addr = offset + size;
                int diff = size - (last_addr - MAX_BIN_SIZE);
                if (size > MAX_BIN_SIZE)
                    fail(string_format("File 'file' %s is too big (%d). Maximum is: %d", image_file.c_str(), size, diff));

                ifd.seekg(0, std::ios::beg);
                FileInitMemory fmem = { .start = (int)*start, .size = size, .offset = offset, .filename = image_file };
                // load file
                ifd.read(&fmem.content[0], size);
                // copy binary to array
                test->initializer.file_initializers.push_back(fmem);
                ifd.close();
            } else {
                fail(string_format("Error in test: '%s'. Expect either 'fill' or 'file' memory initialize directive for init address: %d!", test->name.c_str(), *start));
            }
        }
    });
}

void build_test_run(toml::node_view<toml::node> section, SingleTest * test) {
    auto max_ticks = section.at_path("max_ticks").template value<int>();
    if (max_ticks.has_value()) {
        int max_ticks_val = *max_ticks;
        if (max_ticks_val > MAX_BIN_SIZE || max_ticks_val < 0) fail("Wrong 'max_ticks' value in section [test.run]!");
        test->test_run.max_ticks = max_ticks_val;
    }

    std::string constant_function_name = "";
    auto f_name = section.at_path("fname").template value<std::string>();
    if (f_name.has_value()) {
        const std::string fname = *f_name;
        auto iter = std::find_if(original_memory_image.constants.begin(), original_memory_image.constants.end(), [&fname] (const PredefinedConstant& v) { return v.name == fname; });
        
        if (iter == original_memory_image.constants.end())
             fail(string_format("Cannot found definition for fname: '%s' for test: '%s'", fname.c_str(), test->name.c_str())); 
        int index = std::distance(original_memory_image.constants.begin(), iter);                   
        constant_function_name = "(function: '" + fname + "')";
        test->test_run.call = original_memory_image.constants[index].value;

    } else {
        auto call = section.at_path("call").template value<int>();

        if (call.has_value()) {
            int call_addr = *call;
            if (call_addr >= MAX_BIN_SIZE || call_addr < 0) 
                fail(string_format("Start address %d is wrong for test: %s", call_addr, test->name.c_str()));  

            test->test_run.call = call_addr;     
            
        } else {
            fail(string_format("Start address or function to run is not found for test: %s", test->name.c_str()));
        }
    }
    std::cout << "calling address: 0x" << test->test_run.call << std::dec << " " << constant_function_name << "\n";
}

void build_test_expect_registers(toml::node_view<toml::node> exp_regs, SingleTest * test) {
    if (!exp_regs.is_table())        
        std::cout << "exp regs: " << exp_regs << "\n";
    exp_regs.as_table()->for_each([&test](auto& key, auto& val) {
        std::string current_reg = string_tolower((std::string)key.str());
        if (!(val.is_integer() || val.is_boolean()))
            fail(string_format("Error in test: '%s'. Expect value for register: '%s' to be integer!", test->name.c_str(), current_reg.c_str()));

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
            default: { fail(string_format("Error in test: '%s'. Unknown register: '%s' in expectations!", test->name.c_str(), current_reg.c_str())); }
        }
    });
}

void build_test_expect_memory(toml::node_view<toml::node> section, SingleTest * test) {
    if (!section.is_array_of_tables()) return;

    section.as_array()->for_each([&test](auto& el) {
        auto address   = el.at_path("address").template value<int64_t>();
        auto value     = el.at_path("value").template value<int64_t>();
        auto value_not = el.at_path("value_not").template value<int64_t>();
        
        if (!address.has_value())
            fail(string_format("Error in test: '%s'. Memory expectation must have 'address' to check!", test->name.c_str())); 
        int addr = * address;
        if (addr > MAX_LOAD_ADDRESS || addr < 0)
            fail(string_format("Error in test: '%s'. Memory expectation 'address'=%d exceeds max address: %d!", test->name.c_str(), addr, MAX_LOAD_ADDRESS)); 

        if (value.has_value() && value_not.has_value())
            fail(string_format("Error in test: '%s'. Memory expectation 'value' and 'value_not' cannot be defined simultaneously!", test->name.c_str()));     
        if (!value.has_value() && !value_not.has_value())
            fail(string_format("Error in test: '%s'. Memory expectation 'value' or 'value_not' have to be defined!", test->name.c_str()));     

        TestExpectMemory expm;

        if (value.has_value()) {
            int val = * value;
            if (val > 0xFF || addr < 0)
                fail(string_format("Error in test: '%s'. Memory expectation 'value'=%d exceeds 0xFF!", test->name.c_str(), val)); 
            expm = TestExpectMemory{ .address = addr, .value = val };
        }
        if (value_not.has_value()) {
            int val = * value_not;
            if (val > 0xFF || addr < 0)
                fail(string_format("Error in test: '%s'. Memory expectation 'value_not'=%d exceeds 0xFF!", test->name.c_str(), val)); 
            expm = TestExpectMemory{ .address = addr, .value_not = val };
        }

        test->memory_expectations.push_back(expm);
    });
} 

void build_test_expect_ports(toml::node_view<toml::node> section, SingleTest * test) {
    

} // TODO test.expect_port

void build_test_expect_timing(toml::node_view<toml::node> section, SingleTest * test) {
    auto max_ticks = section["max_ticks"].value<int64_t>();
    auto exact_ticks = section["exact_ticks"].value<int64_t>();

    if (max_ticks.has_value()) {
        if (exact_ticks.has_value())
            fail(string_format("Error in test: '%s'. Cannot have set both 'max_ticks' and 'exact_ticks'!", test->name.c_str())); 
        
        if (*max_ticks >= MAX_CPU_TICKS_PER_TEST || *max_ticks < 4)
            fail(string_format("Error in test: '%s'. 'max_ticks' set to: %d, but maximum allowed is: %d and minimum is: 4!", test->name.c_str(), *max_ticks, MAX_CPU_TICKS_PER_TEST));         

        test->expect_timing.max_tick = *max_ticks;
    } else if (exact_ticks.has_value()) {

        if (*exact_ticks >= MAX_CPU_TICKS_PER_TEST || *exact_ticks < 4)
            fail(string_format("Error in test: '%s'. 'exact_ticks' set to: %d, but maximum allowed is: %d and minimum is: 4!", test->name.c_str(), *exact_ticks, MAX_CPU_TICKS_PER_TEST)); 

        test->expect_timing.exact_tick = *exact_ticks;
    }
}

void set_precondition_reg(std::string current_reg, int value, SingleTest * test) {
    std::cout << "set reg: " << current_reg << " = 0x" << std::hex << value << "\n";
    SWITCH(current_reg) {
        CASE("a"):  { test->expect_registers.reg_a  = check_8bit(value); return; }
        CASE("h"):  { test->expect_registers.reg_h  = check_8bit(value); return; }
        CASE("l"):  { test->expect_registers.reg_l  = check_8bit(value); return; }
        CASE("b"):  { test->expect_registers.reg_b  = check_8bit(value); return; }
        CASE("c"):  { test->expect_registers.reg_c  = check_8bit(value); return; }
        CASE("d"):  { test->expect_registers.reg_d  = check_8bit(value); return; }
        CASE("e"):  { test->expect_registers.reg_e  = check_8bit(value); return; }        
        CASE("a_"): { test->expect_registers.reg_a_ = check_8bit(value); return; }
        CASE("h_"): { test->expect_registers.reg_h_ = check_8bit(value); return; }
        CASE("l_"): { test->expect_registers.reg_l_ = check_8bit(value); return; }
        CASE("b_"): { test->expect_registers.reg_b_ = check_8bit(value); return; }
        CASE("c_"): { test->expect_registers.reg_c_ = check_8bit(value); return; }
        CASE("d_"): { test->expect_registers.reg_d_ = check_8bit(value); return; }
        CASE("e_"): { test->expect_registers.reg_e_ = check_8bit(value); return; }        

        CASE("fl_z"): { test->expect_registers.fl_z = check_bit(value); return; }
        CASE("fl_p"): { test->expect_registers.fl_p = check_bit(value); return; }
        CASE("fl_c"): { test->expect_registers.fl_c = check_bit(value); return; }

        CASE("sp"):  { test->expect_registers.reg_sp = check_16bit(value); return; }
        CASE("hl"):  { check_16bit(value); test->expect_registers.reg_h  = (value >> 8) & 0xff; test->expect_registers.reg_l = value & 0xff; return; }
        CASE("de"):  { check_16bit(value); test->expect_registers.reg_d  = (value >> 8) & 0xff; test->expect_registers.reg_e = value & 0xff; return; }
        CASE("bc"):  { check_16bit(value); test->expect_registers.reg_b  = (value >> 8) & 0xff; test->expect_registers.reg_c = value & 0xff; return; }
        CASE("hl_"): { check_16bit(value); test->expect_registers.reg_h_ = (value >> 8) & 0xff; test->expect_registers.reg_l_ = value & 0xff; return; }
        CASE("de_"): { check_16bit(value); test->expect_registers.reg_d_ = (value >> 8) & 0xff; test->expect_registers.reg_e_ = value & 0xff; return; }
        CASE("bc_"): { check_16bit(value); test->expect_registers.reg_b_ = (value >> 8) & 0xff; test->expect_registers.reg_c_ = value & 0xff; return; }
    }
}


void handle_tests(toml::node_view<toml::node> test_section) {
    if (!test_section.is_array_of_tables()) fail("'[[test]]' section(s) not found!");

    test_section.as_array()->for_each([](auto& section, int test_idx) {
        SingleTest test = { .is_skipped = false };
        const toml::table& sec = *section.as_table();

        auto test_name = sec["name"].value_or(string_format("test_%d", test_idx + 1));
        test.name = test_name.c_str();

        auto skipped_name = sec["xname"].value_or("#"sv);
        if (sec["xname"].is_string()) {
            test.is_skipped = true;
            test.name = skipped_name;
            std::cout << "Current test: " << Colors::BLUE << test.name  << " (skipped)" << Colors::RESET << "\n";
        } else {
            std::cout << "Current test: " << test.name  << "\n";
        }

        // set registers 
        // TODO: fix setting precondition registers
        for(const std::string &current_reg : REGISTERS) {
            auto reg = section.at_path(current_reg).template value<int>();

            if (reg.has_value()) { 
                set_precondition_reg(current_reg, *reg, &test); 
            } else {
                reg = section.at_path(string_toupper(current_reg)).template value<int>();
                if (reg.has_value()) { 
                    set_precondition_reg(current_reg, *reg, &test);
                } 
            }
        };

        auto init_section = section.at_path("init");        
        if (init_section.is_table() && !init_section.as_table()->empty()) {
            build_test_init(init_section, &test);
        } else {
            std::cout << "No init section for test: '" << test.name << "'\n";
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

        execute_test_spec(test_idx);

        test_idx++; 
        std::cout << "\n";
    });
}

void handle_init_section(toml::node_view<toml::node> init_section) {
    // set initial vars and files
    auto load_addr = init_section["load_addr"].value<int>();
    if (!load_addr.has_value()) fail("'load_address' not found in [init] section!");
    if (load_addr > MAX_LOAD_ADDRESS) fail(string_format("'load_address' exeeds maximum memory (%d) address!", MAX_LOAD_ADDRESS));
    if (load_addr < 0) fail("'load_address' cannot be less then 0!");
    original_memory_image.load_addr = *load_addr;

    // load content
    auto bin_file = init_section["bin_file"].value<std::string>();
    if (!bin_file.has_value()) fail("'bin_file' not found in [init] section!");

    // try to check file exists, its size and load it
    std::string memory_image_file = *bin_file;
    std::ifstream ifd(memory_image_file, std::ios::binary | std::ios::ate);
    if (!ifd) {
        fail(string_format("Cannot open 'bin_file'=%s", memory_image_file.c_str()));
    }
    int size = ifd.tellg();
    if (size > MAX_BIN_SIZE) {
        fail(string_format("File 'bin_file' %s is too big (%d). Maximum is: %d", memory_image_file.c_str(), size, MAX_BIN_SIZE));
    }
    int last_addr = original_memory_image.load_addr + size;
    int diff = size - (last_addr - MAX_BIN_SIZE);
    if (size > MAX_BIN_SIZE) {
        fail(string_format("File 'bin_file' %s is too big (%d). Maximum is: %d", memory_image_file.c_str(), size, diff));
    }
    ifd.seekg(0, std::ios::beg);
    ifd.read(&original_memory_image.mem[0], size);
    ifd.close();

    // defining consts
    auto defined_constants = init_section["const"];
    defined_constants.as_table()->for_each([](auto& key, toml::value<int64_t>& value) {
        if constexpr (toml::is_integer<decltype(value)>) {
            if (value > MAX_LOAD_ADDRESS || value < 0) {
                std::cerr << Colors::RED << "Defined constant: '" << key << "' = "sv << value << " exeeds maximum memory ("sv << MAX_LOAD_ADDRESS << ") address!" << std::endl;
                exit(1);
            }
            PredefinedConstant pconst = { (std::string)key, (int64_t)value };
            original_memory_image.constants.push_back(pconst);
        } else {
            std::cerr << Colors::RED << "Constants can be only integers! Found: " << key << " = "sv << value << std::endl;
            exit(1);
        }        
    });

    // load labels
    auto lf = init_section["labels_file"].value<std::string>();
    if (lf.has_value()) {
        // try to check file exists, its size and load it
        std::string labels_file = *lf;
        std::ifstream ifd(labels_file, std::ios::binary | std::ios::ate);
        if (!ifd) {
            fail(string_format("Cannot open 'labels_file' = %s", labels_file.c_str()));
        }
        int size = ifd.tellg();
        if (size > MAX_LABELS_FILE_SIZE) 
            fail(string_format("Labels file: %s is too big!", labels_file.c_str()));

        ifd.seekg(0, std::ios::beg);

        // parse labels
        std::filesystem::path file_path = labels_file;   

        labels_parser(&ifd, file_path.extension());
        
        ifd.close();
    }
}

void config_parser(const char* path) {
    toml::parse_result config;

    try {
        config = toml::parse_file( path );
    } catch (toml::parse_error &ex) {   
        fail_parse_error(ex); 
    }

    toml::node_view<toml::node> init_section = config["init"];
    handle_init_section(init_section);

    toml::node_view<toml::node> test_sections = config["test"];
    handle_tests(test_sections);
}