#include "main.hpp"

using namespace std::string_view_literals;

void configure_parser(cli::Parser& parser) {
	parser.set_optional<bool>("v", "version", false, "Show zspec version and exit");
}

int main(int argc, char** argv) {
    std::cout << "ZSpec testing framework by Yury Batenko © 2024++\n";

    cli::Parser parser(argc, argv);
    configure_parser(parser);
    parser.set_default<std::string>(false, "Path to .toml spec file", "zspec.toml");
    parser.run_and_exit_if_error();

    auto ver = parser.get<bool>("v");
    if (ver) {
        std::cout << "ZSpec version: " << ZSPEC_VERSION << "\n";
        exit(0);
    }

    const auto path = parser.get_default<std::string>();
    std::vector<std::string> file_line = split_string(path, ':');
    auto filepath = file_line[0];
    int test_to_run = 0;

    try {
        test_to_run = (int)std::strtoul(file_line[1].c_str(), nullptr, 10);
    } catch (const std::exception& e) {
       test_to_run = 0;
    }

    if (! std::filesystem::exists(filepath)) {
        std::cout << Colors::RED << "File '" << filepath << "' not found!\n"  << Colors::RESET;
    }

    std::cout << "Run tests suit: " << filepath << "\n\n";
    run_tests(filepath.c_str(), test_to_run);
}
