#include "test_spec_executor.hpp"

unsigned char readByte(void* arg, unsigned short addr)
{
    return ((MMU*)arg)->RAM[addr];
}

void writeByte(void* arg, unsigned short addr, unsigned char value)
{
    if (addr > 0xFFFF) {
        return;
    }
    ((MMU*)arg)->RAM[addr] = value;
}

unsigned char inPort(void* arg, unsigned short port)
{
    return ((MMU*)arg)->IO[port];
}

void outPort(void* arg, unsigned short port, unsigned char value)
{
    ((MMU*)arg)->IO[port] = value;
}


void execute_test_spec(SingleTest* test) {
    if (test->is_skipped) {
        std::cout << Colors::BLUE << "Skip test: " << test->name << Colors::RESET << "\n\n";
        return;
    } 

    std::cout << Colors::CYAN << "runnning test..." << Colors::RESET << "\n";

    mmu_unit = new MMU();

    // copy memory image and init memory with preconditions

    // set registers

    // set SP

    // set breakpoints

    // run CPU ticks loop

    // save registers and compare with expectations

    // handle memory expectations

    // handle ports expectations

    // handle  ticks expectations

    // save results

    // print results

    delete mmu_unit;
}