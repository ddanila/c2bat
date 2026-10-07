#pragma once
#include "parser.hpp"
#if !defined(yyFlexLexerOnce)
#include <FlexLexer.h>
#endif
#include <istream>

namespace c2bat {
class Scanner : public yyFlexLexer {
public:
    explicit Scanner(std::istream& input) : yyFlexLexer(&input) {}
    Parser::symbol_type next();
    void advance(const char* text, int length) {
        location.step();
        for (int i = 0; i < length; ++i) {
            if (text[i] == '\n') location.lines();
            else location.columns();
        }
    }
    Parser::symbol_type integer(const char* text);
    [[noreturn]] void unsupported(const char* text) const;
    Parser::location_type location;
};
}
