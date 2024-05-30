#pragma once
#include <string>
#include <iostream>
#include "../../libs/Colors.hpp"
#include "../../libs/utils.hpp"
#include "test_suits_config.hpp"

// Labels file formatted by from millfork compiler
static void parse_lbl_type(std::ifstream *labels_stream) {
    std::string line;
    std::cout << std::hex;

    for (;std::getline(*labels_stream, line);) {
        std::vector<std::string> line_arr = split_string(line, ' '); 
        int addr = std::stoi(line_arr[1], nullptr, 16);
        std::string name = line_arr[2].erase(0, 1); // remove front dot .
        std::cout << "  0x" << addr << " " << name << "\n";
        PredefinedConstant pc = { .name = name, .value = addr };
        original_memory_image.constants.push_back(pc);       
    }

    std::cout << Colors::RESET << std::dec;
}

// Labels file formatted by sjasmplus assembler
static void parse_sld_type(std::ifstream *labels_stream) {
    std::string line;
    std::getline(*labels_stream, line);
    std::getline(*labels_stream, line);
    std::getline(*labels_stream, line); // skip first 3 lines
    std::cout << std::hex;

    for (;std::getline(*labels_stream, line);) {
        std::vector<std::string> line_arr = split_string(line, '|'); 
        if (line_arr[5] == "F") {
            int addr = std::stoi(line_arr[4]);
            std::cout << "  0x" << addr << " " << line_arr[6] << "\n";
            PredefinedConstant pc = { .name = line_arr[6], .value = addr };
            original_memory_image.constants.push_back(pc);
        }        
    }

    std::cout << Colors::RESET << std::dec;
}

static void labels_parser(std::ifstream *labels_stream, std::string extension) {
    std::cout << Colors::MAGENTA << "Parsing labels:" << "\n";

    if (string_tolower(extension) == ".lbl") {
        parse_lbl_type(labels_stream);
    } else if (string_tolower(extension) == ".sld") {
        parse_sld_type(labels_stream);
    } else {
        fail(string_format("Unknown labels file format: %s. Provide either '.lbl' or '.sld' formats!", extension.c_str()));
    }
}