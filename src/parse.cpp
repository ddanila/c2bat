#include "scanner.hpp"
#include <sstream>
#include <stdexcept>

namespace c2bat {
StmtPtr parse(std::string_view source) {
    std::istringstream input{std::string(source)};
    Scanner scanner(input);
    StmtPtr tree;
    Parser parser(scanner, tree);
    if (parser.parse() != 0 || !tree) throw std::runtime_error("parsing failed");
    return tree;
}
Parser::symbol_type Scanner::integer(const char* text) {
    std::string value(text);
    std::size_t consumed = 0;
    unsigned long number;
    try { number = std::stoul(value, &consumed, 0); }
    catch (const std::exception&) { throw Parser::syntax_error(location, "invalid or oversized integer constant"); }
    if (consumed != value.size()) unsupported(text);
    if (number > 32767) throw Parser::syntax_error(location, "integer literal requires an unsupported type (maximum 32767)");
    return Parser::make_CONSTANT(static_cast<int>(number), location);
}
void Scanner::unsupported(const char* text) const {
    throw Parser::syntax_error(location, "unsupported C token: " + std::string(text));
}
void Parser::error(const location_type& loc, const std::string& message) {
    std::ostringstream out;
    out << loc << ": " << message;
    throw std::runtime_error(out.str());
}
}
