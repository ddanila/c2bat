#include "compiler.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

int main(int argc, char** argv) {
    try {
        if (argc != 3 && argc != 4) {
            std::cerr << "usage: c2bat --run source.c | --ir source.c | source.c -o new-directory\n";
            return 2;
        }
        const bool run = std::string_view(argv[1]) == "--run";
        const bool ir = std::string_view(argv[1]) == "--ir";
        if ((argc == 3 && !run && !ir) || (argc == 4 && (run || ir || std::string_view(argv[2]) != "-o")))
            throw std::runtime_error("invalid arguments");
        const auto path = argc == 3 ? argv[2] : argv[1];
        std::ifstream file(path);
        if (!file) throw std::runtime_error("cannot open " + std::string(path));
        std::string source{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        auto tree = c2bat::parse(source);
        auto program = c2bat::lower(*tree);
        if (run) { auto result = c2bat::interpret(program); std::cout << "RESULT=" << result << '\n'; }
        else if (ir) std::cout << c2bat::disassemble(program);
        else c2bat::emit_batch(program, argv[3]);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "c2bat: " << error.what() << '\n';
        return 1;
    }
}
