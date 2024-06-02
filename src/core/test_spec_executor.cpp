#include "test_spec_executor.hpp"

SingleTest* curr_test; // shared b/w functs
MMU* mmu_unit;
Z80* z80_cpu;
int curr_tick; 

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
    char  * src = &original_memory_image.mem[0];
    int start_addr = original_memory_image.load_addr;
    int size = original_memory_image.size;
    memcpy(&mmu_unit->RAM[start_addr], src, size);

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
            mmu_unit->RAM[mi.start + i] = (char)f;
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
}

void execute_test_spec(SingleTest* test) {
    if (test->is_skipped) {
        std::cout << Colors::BLUE << "Skip test: " << test->name << Colors::RESET << "\n\n";
        return;
    } 

    std::cout << Colors::CYAN << "runnning test..." << Colors::RESET << "\n";

    curr_test = test;
    mmu_unit = new MMU();
    z80_cpu = new Z80(readByte, writeByte, inPort, outPort, mmu_unit);
    mmu_unit->cpu = z80_cpu;
    bool is_finished = false;

    // copy memory image and init memory with preconditions
    prefill_mem();

    // set registers
    assign_registers();

    // set SP
    assign_sp();

    // set breakpoints on RST0 0xC7
    z80_cpu->addBreakOperand(0xC7, [&is_finished](void* arg, unsigned char* opcode, int opcodeLength) -> void {
        is_finished = true;
    });

    curr_tick = 0;
    int max_ticks = 100000;
    if (test->test_run.max_ticks != -1) max_ticks = test->test_run.max_ticks;

    // run CPU ticks loop
    while (is_finished == false) {
        if (curr_tick >= max_ticks) { is_finished = true; break; }
        curr_tick += 1;

        z80_cpu->execute(1);
        // check sp and break if return from our start funct

    }

    // save registers and compare with expectations

    // check memory expectations

    // check ports expectations

    // check  ticks expectations

    // save results

    // print results

    delete z80_cpu;
    delete mmu_unit;
}