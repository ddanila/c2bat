/* C99 grammar subset adapted from Jeff Lee / Jutta Degener's published
 * grammar. See third_party/quut/README.md for source and permission.
 * AST actions and the C++ interface are specific to c2bat. */
%skeleton "lalr1.cc"
%require "3.8"
%define api.namespace {c2bat}
%define api.parser.class {Parser}
%define api.value.type variant
%define api.token.constructor
%define parse.error detailed
%define parse.lac full
%locations
%expect 0
%parse-param { c2bat::Scanner& scanner } { c2bat::StmtPtr& result }
%lex-param { c2bat::Scanner& scanner }
%code requires {
#include "compiler.hpp"
namespace c2bat { class Scanner; }
}
%code {
#include "scanner.hpp"
static c2bat::Parser::symbol_type yylex(c2bat::Scanner& scanner) { return scanner.next(); }
static c2bat::ExprPtr binary(std::string op, c2bat::ExprPtr left, c2bat::ExprPtr right) {
    return std::make_unique<c2bat::Expr>(c2bat::Binary{std::move(op), std::move(left), std::move(right)});
}
}
%token <std::string> IDENTIFIER "identifier"
%token <int> CONSTANT "integer constant"
%token INT "int" VOID "void" IF "if" ELSE "else" WHILE "while" RETURN "return"
%token LPAREN "(" RPAREN ")" LBRACE "{" RBRACE "}" SEMI ";" ASSIGN "="
%token PLUS "+" MINUS "-" NOT "!" LT "<" GT ">" LE "<=" GE ">=" EQ "==" NE "!="
%precedence IF_WITHOUT_ELSE
%precedence ELSE
%nterm <c2bat::StmtPtr> compound_statement statement declaration
%nterm <c2bat::Block> block_item_list
%nterm <c2bat::ExprPtr> primary_expression unary_expression additive_expression relational_expression equality_expression expression
%start translation_unit
%%
translation_unit
    : INT IDENTIFIER LPAREN VOID RPAREN compound_statement {
        if ($2 != "main") throw c2bat::Parser::syntax_error(@2, "only main is supported");
        result = std::move($6);
    }
    ;
primary_expression
    : IDENTIFIER { $$ = std::make_unique<c2bat::Expr>(c2bat::Variable{std::move($1)}); }
    | CONSTANT { $$ = std::make_unique<c2bat::Expr>(c2bat::Number{$1}); }
    | LPAREN expression RPAREN { $$ = std::move($2); }
    ;
unary_expression
    : primary_expression { $$ = std::move($1); }
    | PLUS unary_expression { $$ = std::make_unique<c2bat::Expr>(c2bat::Unary{"+", std::move($2)}); }
    | MINUS unary_expression { $$ = std::make_unique<c2bat::Expr>(c2bat::Unary{"-", std::move($2)}); }
    | NOT unary_expression { $$ = std::make_unique<c2bat::Expr>(c2bat::Unary{"!", std::move($2)}); }
    ;
additive_expression
    : unary_expression { $$ = std::move($1); }
    | additive_expression PLUS unary_expression { $$ = binary("+", std::move($1), std::move($3)); }
    | additive_expression MINUS unary_expression { $$ = binary("-", std::move($1), std::move($3)); }
    ;
relational_expression
    : additive_expression { $$ = std::move($1); }
    | relational_expression LT additive_expression { $$ = binary("<", std::move($1), std::move($3)); }
    | relational_expression GT additive_expression { $$ = binary(">", std::move($1), std::move($3)); }
    | relational_expression LE additive_expression { $$ = binary("<=", std::move($1), std::move($3)); }
    | relational_expression GE additive_expression { $$ = binary(">=", std::move($1), std::move($3)); }
    ;
equality_expression
    : relational_expression { $$ = std::move($1); }
    | equality_expression EQ relational_expression { $$ = binary("==", std::move($1), std::move($3)); }
    | equality_expression NE relational_expression { $$ = binary("!=", std::move($1), std::move($3)); }
    ;
expression : equality_expression { $$ = std::move($1); } ;
declaration
    : INT IDENTIFIER ASSIGN expression SEMI {
        $$ = std::make_unique<c2bat::Stmt>(c2bat::Declare{std::move($2), std::move($4)});
        $$->source_line = @$.begin.line;
    }
    ;
compound_statement
    : LBRACE block_item_list RBRACE {
        $$ = std::make_unique<c2bat::Stmt>(std::move($2));
        $$->source_line = @$.begin.line;
    }
    ;
block_item_list
    : %empty { $$ = c2bat::Block{}; }
    | block_item_list statement { $1.statements.push_back(std::move($2)); $$ = std::move($1); }
    | block_item_list declaration { $1.statements.push_back(std::move($2)); $$ = std::move($1); }
    ;
statement
    : compound_statement { $$ = std::move($1); }
    | IDENTIFIER ASSIGN expression SEMI {
        $$ = std::make_unique<c2bat::Stmt>(c2bat::Assign{std::move($1), std::move($3)});
        $$->source_line = @$.begin.line;
    }
    | RETURN expression SEMI {
        $$ = std::make_unique<c2bat::Stmt>(c2bat::Return{std::move($2)});
        $$->source_line = @$.begin.line;
    }
    | IF LPAREN expression RPAREN statement %prec IF_WITHOUT_ELSE {
        $$ = std::make_unique<c2bat::Stmt>(c2bat::If{std::move($3), std::move($5), nullptr});
        $$->source_line = @$.begin.line;
    }
    | IF LPAREN expression RPAREN statement ELSE statement {
        $$ = std::make_unique<c2bat::Stmt>(c2bat::If{std::move($3), std::move($5), std::move($7)});
        $$->source_line = @$.begin.line;
    }
    | WHILE LPAREN expression RPAREN statement {
        $$ = std::make_unique<c2bat::Stmt>(c2bat::While{std::move($3), std::move($5)});
        $$->source_line = @$.begin.line;
    }
    ;
%%
