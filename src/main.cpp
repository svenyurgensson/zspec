#include "main.hpp"

using namespace std::string_view_literals;

int main(int argc, char** argv) {
    const auto path = argc > 1 ? argv[1] : "zspec.toml";

    if (! std::filesystem::exists(path)) {
        std::cout << Colors::RED << "File '" << path << "' not found!\n"  << Colors::RESET;
    }

    run_tests(path);  
}
