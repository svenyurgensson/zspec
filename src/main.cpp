#include "main.hpp"

using namespace std::string_view_literals;

void configure_parser(cli::Parser& parser) {
	parser.set_optional<bool>("v", "version", false, "Show zspec version and exit");
}

int main(int argc, char** argv) {
    cli::Parser parser(argc, argv);
    configure_parser(parser);
    parser.run_and_exit_if_error();

    auto ver = parser.get<bool>("v");
    if (ver) {
        std::cout << "ZSpec version: " << ZSPEC_VERSION << "\n";
        exit(0);
    }
    

    const auto path = argc > 1 ? argv[1] : "zspec.toml";

    if (! std::filesystem::exists(path)) {
        std::cout << Colors::RED << "File '" << path << "' not found!\n"  << Colors::RESET;
    }


    run_tests(path);  
}
