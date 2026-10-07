#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace c2bat {
struct Expr;
using ExprPtr = std::unique_ptr<Expr>;
struct Number { int value; };
struct Variable { std::string name; };
struct Unary { std::string op; ExprPtr operand; };
struct Binary { std::string op; ExprPtr left, right; };
struct Expr { std::variant<Number, Variable, Unary, Binary> node; };
struct Stmt;
using StmtPtr = std::unique_ptr<Stmt>;
struct Block { std::vector<StmtPtr> statements; };
struct Declare { std::string name; ExprPtr value; };
struct Assign { std::string name; ExprPtr value; };
struct Return { ExprPtr value; };
struct If { ExprPtr condition; StmtPtr yes, no; };
struct While { ExprPtr condition; StmtPtr body; };
struct Stmt {
    std::variant<Block, Declare, Assign, Return, If, While> node;
    int source_line = 0;
};

enum class Op { push, load, store, add, sub, less, equal, logical_not, jump, jump_zero, ret };
struct Instruction { Op op; int argument = 0; int source_line = 0; };
using Program = std::vector<Instruction>;
StmtPtr parse(std::string_view source);
Program lower(const Stmt& tree);
std::string disassemble(const Program& program);
int interpret(const Program& program);
void emit_batch(const Program& program, const std::filesystem::path& directory);
} // namespace c2bat
