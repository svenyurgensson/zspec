#include "test_spec_executor.hpp"

SingleTest* curr_test; // shared b/w functs
MemoryImage* mem_img;
MMU* mmu_unit;
Z80* z80_cpu;
bool is_finished;
uint16_t top_sp;

unsigned char readByte(void* arg, unsigned short addr) {
    return ((MMU*)arg)->RAM[addr];
}

void writeByte(void* arg, unsigned short addr, unsigned char value) {
    if (addr > 0xFFFF) {
        return;
    }
    ((MMU*)arg)->RAM[addr] = value;
}

unsigned char inPort(void* arg, unsigned short port) {
    return ((MMU*)arg)->IO[port];
}

void outPort(void* arg, unsigned short port, unsigned char value) {
    ((MMU*)arg)->IO[port] = value;
}

void prefill_mem() {
    // copy mem image from loaded bin file
    int start_addr = mem_img->load_addr;
    char * src = &mem_img->mem[0];
    int size = mem_img->size;
    memcpy(&mmu_unit->RAM[0], src, sizeof(mem_img->mem));

    // clear mem segment if present
    TestClear cl = curr_test->initializer.test_clear;
    if (cl.start != -1 && cl.end != -1) {
        memset(&mmu_unit->RAM[cl.start], (char)cl.fill, cl.end - cl.start + 1);
    }
    // init from init.memory section
    for (auto mi : curr_test->initializer.memory_initializers) {
        if (mi.start == -1) continue;
        int i = 0;
        for (auto f : mi.fill) {
            mmu_unit->RAM[mi.start + i] = (uint8_t)f;
            i++;
        }
    }
    // init from init.file section
    for (auto mf : curr_test->initializer.file_initializers) {
        if (mf.start == -1 || mf.size == -1) continue;
        memcpy(&mmu_unit->RAM[mf.start], &mf.content[0], mf.size);
    }
}

void assign_registers() {
    z80_cpu->reg.pair.F = 0;
    TestPreconditions * p = &curr_test->preconditions;

    if (p->fl_z == 1) z80_cpu->reg.pair.F |= z80_cpu->flagZ();
    else if (p->fl_z == 0) z80_cpu->reg.pair.F &= ~z80_cpu->flagZ();
    if (p->fl_c == 1) z80_cpu->reg.pair.F |= z80_cpu->flagC();
    else if (p->fl_c == 0) z80_cpu->reg.pair.F &= ~z80_cpu->flagC();
    if (p->fl_p == 1) z80_cpu->reg.pair.F |= z80_cpu->flagPV();
    else if (p->fl_p == 0) z80_cpu->reg.pair.F &= ~z80_cpu->flagPV();

    if (p->reg_a != -1) z80_cpu->reg.pair.A = p->reg_a;
    if (p->reg_b != -1) z80_cpu->reg.pair.B = p->reg_b;
    if (p->reg_c != -1) z80_cpu->reg.pair.C = p->reg_c;
    if (p->reg_d != -1) z80_cpu->reg.pair.D = p->reg_d;
    if (p->reg_e != -1) z80_cpu->reg.pair.E = p->reg_e;
    if (p->reg_h != -1) z80_cpu->reg.pair.H = p->reg_h;
    if (p->reg_l != -1) z80_cpu->reg.pair.L = p->reg_l;

    if (p->reg_a_ != -1) z80_cpu->reg.back.A = p->reg_a_;
    if (p->reg_b_ != -1) z80_cpu->reg.back.B = p->reg_b_;
    if (p->reg_c_ != -1) z80_cpu->reg.back.C = p->reg_c_;
    if (p->reg_d_ != -1) z80_cpu->reg.back.D = p->reg_d_;
    if (p->reg_e_ != -1) z80_cpu->reg.back.E = p->reg_e_;
    if (p->reg_h_ != -1) z80_cpu->reg.back.H = p->reg_h_;
    if (p->reg_l_ != -1) z80_cpu->reg.back.L = p->reg_l_;

    if (p->reg_ix != -1) z80_cpu->reg.IX = p->reg_ix;
    if (p->reg_iy != -1) z80_cpu->reg.IY = p->reg_iy;

    // assign PC
    z80_cpu->reg.PC = curr_test->test_run.call;

    // skipping SP for now...
}

void assign_sp() {
    // find appropriate stack position to run
    top_sp = 0x3FF0; // TODO: temporary
    z80_cpu->reg.SP = top_sp;

    z80_cpu->addReturnHandler([](void* arg) -> void {
        uint16_t sp = z80_cpu->reg.SP;

        if (sp == top_sp) is_finished = true;
    });
}

int compare_print(std::string entity_name, int expected, uint8_t actual) {
    if (expected == -1) return 0;
    uint8_t expected_ch = (uint8_t)expected;

    if (expected_ch == actual) {
        std::cout << Colors::GREEN << "    expect " << entity_name << " == " << ssprintf0x00x(expected) << ", actual: " << ssprintf0x00x(actual) << Colors::RESET << "\n";
        return 0;
    } else {
        std::cout << Colors::RED << "    expect " << entity_name << " == " << ssprintf0x00x(expected) << ", but actual: " << ssprintf0x00x(actual) << Colors::RESET << "\n";
        return 1;
    }
}

int compare_print_flag(std::string entity_name, int expected, uint8_t actual) {
    if (expected == -1) return 0;
    uint8_t expected_ch = (uint8_t)expected;

    std::string exp = "false", act = "true";
    if (expected) exp = "true";
    if (actual) act = "true";

    if (expected_ch == actual) {
        std::cout << Colors::GREEN << "    expect " << entity_name << " == " << exp << ", actual: " << act << Colors::RESET << "\n";
        return 0;
    } else {
        std::cout << Colors::RED << "    expect " << entity_name << " == " << exp << ", but actual: " << act << Colors::RESET << "\n";
        return 1;
    }
}

int print_check_regs_expectations() {
    TestExpectRegisters * t = &curr_test->expect_registers;
    int total_failed = 0;

    total_failed += compare_print("A", t->reg_a, z80_cpu->reg.pair.A);
    total_failed += compare_print("B", t->reg_b, z80_cpu->reg.pair.B);
    total_failed += compare_print("C", t->reg_c, z80_cpu->reg.pair.C);
    total_failed += compare_print("D", t->reg_d, z80_cpu->reg.pair.D);
    total_failed += compare_print("E", t->reg_e, z80_cpu->reg.pair.E);
    total_failed += compare_print("H", t->reg_h, z80_cpu->reg.pair.H);
    total_failed += compare_print("L", t->reg_l, z80_cpu->reg.pair.L);

    total_failed += compare_print("A'", t->reg_a_, z80_cpu->reg.back.A);
    total_failed += compare_print("B'", t->reg_b_, z80_cpu->reg.back.B);
    total_failed += compare_print("C'", t->reg_c_, z80_cpu->reg.back.C);
    total_failed += compare_print("D'", t->reg_d_, z80_cpu->reg.back.D);
    total_failed += compare_print("E'", t->reg_e_, z80_cpu->reg.back.E);
    total_failed += compare_print("H'", t->reg_h_, z80_cpu->reg.back.H);
    total_failed += compare_print("L'", t->reg_l_, z80_cpu->reg.back.L);

    total_failed += compare_print("IX", t->reg_ix, z80_cpu->reg.IX);
    total_failed += compare_print("IY", t->reg_iy, z80_cpu->reg.IY);

    total_failed += compare_print_flag("flag Z", t->fl_z, z80_cpu->isFlagZ());
    total_failed += compare_print_flag("flag C", t->fl_c, z80_cpu->isFlagC());
    total_failed += compare_print_flag("flag PV", t->fl_p, z80_cpu->isFlagPV());

    return total_failed;
}

int print_check_mem_expectations() {
    int faults = 0;

    for (auto exp : curr_test->memory_expectations) {
        int addr = exp.address;
        int mem_value = mmu_unit->RAM[addr];
        if (exp.is_word) {
            mem_value += 256 * mmu_unit->RAM[addr+1];
        }

        if (exp.value != -1) {
            if (mem_value == exp.value) {
                std::cout << Colors::GREEN << "    RAM @ addr " << ssprintf0x0000x(addr) << " must be: ";
                if (exp.is_word) {
                    std::cout << ssprintf0x0000x(exp.value) << ", actual: " << ssprintf0x0000x(mem_value);
                } else {
                    std::cout << ssprintf0x00x(exp.value) << ", actual: " << ssprintf0x00x(mem_value);
                }
                std::cout << Colors::RESET << "\n";
            } else {
                std::cout << Colors::RED << "    RAM @ addr " << ssprintf0x0000x(addr) << " must be: ";
                 if (exp.is_word) {
                    std::cout << ssprintf0x0000x(exp.value) << ", but actual: " << ssprintf0x0000x(mem_value);
                } else {
                    std::cout << ssprintf0x00x(exp.value) << ", but actual: " << ssprintf0x00x(mem_value);
                }
                std::cout << Colors::RESET << "\n";
                faults += 1;
            }
        }
        if (exp.value_not != -1) {
            if (mem_value != exp.value_not) {
                std::cout << Colors::GREEN << "    RAM @ addr " << ssprintf0x0000x(addr) << " must not be equal: " << ssprintf0x00x(exp.value) << ", actual: " << ssprintf0x00x(mem_value) << Colors::RESET << "\n";
            } else {
                std::cout << Colors::RED << "    RAM @ addr " << ssprintf0x0000x(addr) << " must not be equal: " << ssprintf0x00x(exp.value) << ", but actual: " << ssprintf0x00x(mem_value) << Colors::RESET << "\n";
                faults += 1;
            }
        }
    }

    return faults;
}

int print_check_ports_expectations() {
    // TODO: ports
    return 0;
}

int print_check_timing_expectations(int curr_tick) {
    int max_tick = curr_test->expect_timing.max_tick;
    int exact_tick = curr_test->expect_timing.exact_tick;

    if (max_tick != -1) {
        if (max_tick >= curr_tick) {
            std::cout << Colors::GREEN << "    CPU max ticks expected to be =< " << max_tick << ", actual: " << curr_tick << Colors::RESET << "\n";
            return 0;
        } else {
            std::cout << Colors::RED << "    CPU max ticks expected to be =< " << max_tick << ", but actual: " << curr_tick << Colors::RESET << "\n";
            return 1;
        }
    }
    if (exact_tick != -1) {
        if (exact_tick == curr_tick) {
            std::cout << Colors::GREEN << "    CPU max ticks expected to be == " << exact_tick << ", actual: " << curr_tick << Colors::RESET << "\n";
            return 0;
        } else {
            std::cout << Colors::RED << "    CPU max ticks expected to be equal: " << exact_tick << ", but actual: " << curr_tick << Colors::RESET << "\n";
            return 1;
        }
    }
    std::cout << Colors::GREEN << "    CPU ticks: " << curr_tick << Colors::RESET << "\n";
    return 0;
}

void execute_test_spec(SingleTest* test, MemoryImage* m, ZTest* suit) {
    if (test->is_skipped) {
        std::cout << Colors::BLUE << "Skip test: " << test->name << Colors::RESET << "\n\n";
        return;
    }
    mem_img = m;
    std::cout << Colors::CYAN << "  running test..." << Colors::RESET << "\n";

    curr_test = test;
    mmu_unit = new MMU();
    z80_cpu = new Z80(readByte, writeByte, inPort, outPort, mmu_unit);
    mmu_unit->cpu = z80_cpu;
    is_finished = false;

    // copy memory image and init memory with preconditions
    prefill_mem();

    // set registers
    assign_registers();

    // set SP
    assign_sp();

    // set breakpoints on RST0 0xC7
    z80_cpu->addBreakOperand(0xC7, [](void* arg, unsigned char* opcode, int opcodeLength) -> void {
        is_finished = true;
    });

    int max_ticks = MAX_TICKS_PER_TEST;
    if (test->test_run.max_ticks != -1) max_ticks = test->test_run.max_ticks;

    // run CPU ticks loop
    int total_ticks = 0;

    while (!is_finished) {
        if (total_ticks >= max_ticks) { is_finished = true; break; }

        total_ticks += z80_cpu->execute(1);
    }

    int faults_count = 0;
    // save registers and compare with expectations
    faults_count += print_check_regs_expectations();

    // check memory expectations
    faults_count += print_check_mem_expectations();

    // check ports expectations
    faults_count += print_check_ports_expectations();

    // check  ticks expectations
    faults_count += print_check_timing_expectations(total_ticks);

    // save results
    if (faults_count > 0) {
        curr_test->result.code = FAIL;
        suit->failed_count += 1;
    } else {
        curr_test->result.code = PASS;
    }

    delete z80_cpu;
    delete mmu_unit;
}