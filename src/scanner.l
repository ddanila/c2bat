/* C99 lexical patterns adapted from Jeff Lee / Jutta Degener.
 * See third_party/quut/README.md for provenance and permission. */
%option c++ noyywrap nodefault nounput noinput yyclass="c2bat::Scanner"
%x COMMENT
%{
#include "scanner.hpp"
#undef YY_DECL
#define YY_DECL c2bat::Parser::symbol_type c2bat::Scanner::next()
#define YY_USER_ACTION advance(yytext, yyleng);
%}
D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			([Ee][+-]?{D}+)
P                       ([Pp][+-]?{D}+)
FS			(f|F|l|L)
IS                      ((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))
%%
"/*" BEGIN(COMMENT);
<COMMENT>"*/" BEGIN(INITIAL);
<COMMENT>[^*\n]+ ;
<COMMENT>"*" ;
<COMMENT>\n ;
<COMMENT><<EOF>> throw c2bat::Parser::syntax_error(location, "unterminated comment");
"//"[^\n]* ;
"auto" { unsupported(yytext); }
"_Bool" { unsupported(yytext); }
"break" { unsupported(yytext); }
"case" { unsupported(yytext); }
"char" { unsupported(yytext); }
"_Complex" { unsupported(yytext); }
"const" { unsupported(yytext); }
"continue" { unsupported(yytext); }
"default" { unsupported(yytext); }
"do" { unsupported(yytext); }
"double" { unsupported(yytext); }
"else" { return c2bat::Parser::make_ELSE(location); }
"enum" { unsupported(yytext); }
"extern" { unsupported(yytext); }
"float" { unsupported(yytext); }
"for" { unsupported(yytext); }
"goto" { unsupported(yytext); }
"if" { return c2bat::Parser::make_IF(location); }
"_Imaginary" { unsupported(yytext); }
"inline" { unsupported(yytext); }
"int" { return c2bat::Parser::make_INT(location); }
"long" { unsupported(yytext); }
"register" { unsupported(yytext); }
"restrict" { unsupported(yytext); }
"return" { return c2bat::Parser::make_RETURN(location); }
"short" { unsupported(yytext); }
"signed" { unsupported(yytext); }
"sizeof" { unsupported(yytext); }
"static" { unsupported(yytext); }
"struct" { unsupported(yytext); }
"switch" { unsupported(yytext); }
"typedef" { unsupported(yytext); }
"union" { unsupported(yytext); }
"unsigned" { unsupported(yytext); }
"void" { return c2bat::Parser::make_VOID(location); }
"volatile" { unsupported(yytext); }
"while" { return c2bat::Parser::make_WHILE(location); }
{L}({L}|{D})* { return c2bat::Parser::make_IDENTIFIER(std::string(yytext, static_cast<std::size_t>(yyleng)), location); }
0[xX]{H}+{IS}? { return integer(yytext); }
0[0-7]*{IS}? { return integer(yytext); }
[1-9]{D}*{IS}? { return integer(yytext); }
L?'(\\.|[^\\'\n])+' { unsupported(yytext); }
{D}+{E}{FS}? { unsupported(yytext); }
{D}*"."{D}+{E}?{FS}? { unsupported(yytext); }
{D}+"."{D}*{E}?{FS}? { unsupported(yytext); }
0[xX]{H}+{P}{FS}? { unsupported(yytext); }
0[xX]{H}*"."{H}+{P}{FS}? { unsupported(yytext); }
0[xX]{H}+"."{H}*{P}{FS}? { unsupported(yytext); }
L?\"(\\.|[^\\"\n])*\" { unsupported(yytext); }
"..." { unsupported(yytext); }
">>=" { unsupported(yytext); }
"<<=" { unsupported(yytext); }
"+=" { unsupported(yytext); }
"-=" { unsupported(yytext); }
"*=" { unsupported(yytext); }
"/=" { unsupported(yytext); }
"%=" { unsupported(yytext); }
"&=" { unsupported(yytext); }
"^=" { unsupported(yytext); }
"|=" { unsupported(yytext); }
">>" { unsupported(yytext); }
"<<" { unsupported(yytext); }
"++" { unsupported(yytext); }
"--" { unsupported(yytext); }
"->" { unsupported(yytext); }
"&&" { return c2bat::Parser::make_AND(location); }
"||" { return c2bat::Parser::make_OR(location); }
"<=" { return c2bat::Parser::make_LE(location); }
">=" { return c2bat::Parser::make_GE(location); }
"==" { return c2bat::Parser::make_EQ(location); }
"!=" { return c2bat::Parser::make_NE(location); }
";" { return c2bat::Parser::make_SEMI(location); }
("{"|"<%") { return c2bat::Parser::make_LBRACE(location); }
("}"|"%>") { return c2bat::Parser::make_RBRACE(location); }
"," { unsupported(yytext); }
":" { unsupported(yytext); }
"=" { return c2bat::Parser::make_ASSIGN(location); }
"(" { return c2bat::Parser::make_LPAREN(location); }
")" { return c2bat::Parser::make_RPAREN(location); }
("["|"<:") { unsupported(yytext); }
("]"|":>") { unsupported(yytext); }
"." { unsupported(yytext); }
"&" { unsupported(yytext); }
"!" { return c2bat::Parser::make_NOT(location); }
"~" { unsupported(yytext); }
"-" { return c2bat::Parser::make_MINUS(location); }
"+" { return c2bat::Parser::make_PLUS(location); }
"*" { unsupported(yytext); }
"/" { unsupported(yytext); }
"%" { unsupported(yytext); }
"<" { return c2bat::Parser::make_LT(location); }
">" { return c2bat::Parser::make_GT(location); }
"^" { unsupported(yytext); }
"|" { unsupported(yytext); }
"?" { unsupported(yytext); }
[ \t\v\n\f\r]+ ;
. { unsupported(yytext); }
<<EOF>> { location.step(); return c2bat::Parser::make_YYEOF(location); }
%%
