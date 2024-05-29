#include "main.hpp"

using namespace std::string_view_literals;

int main(int argc, char** argv) {
    const auto path = argc > 1 ? argv[1] : "zspec.toml";

    if (! std::filesystem::exists(path)) {
        std::cout << Colors::RED << "File '" << path << "' not found!\n"  << Colors::RESET;
    }

    config_parser(path);  
    // TODO: tests_run()
    // TODO: tests_results_print()      
}
