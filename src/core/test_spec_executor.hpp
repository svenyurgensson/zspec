#pragma once
#include <string>
#include <iostream>
#include "../../libs/Colors.hpp"
#include "../../libs/z80.hpp"
#include "../../libs/utils.hpp"
#include "test_suits_config.hpp"

void execute_test_spec(SingleTest* test);

class MMU
{
  public:
    unsigned char RAM[0x10000];
    unsigned char IO[0x10000];
    Z80* cpu;

    MMU() {
        memset(&RAM, 0, sizeof(RAM));
        memset(&IO, 0, sizeof(IO));
    }


};