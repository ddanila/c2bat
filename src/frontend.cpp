#include "compiler.hpp"
#include <cctype>
#include <map>
#include <stdexcept>
#include <utility>

namespace c2bat {
namespace {
class Lowerer {
    Program code;
    std::vector<std::map<std::string, int>> scopes;
    int slots = 0;
    int depth = 0;
    void emit(Op op, int arg = 0) { code.push_back({op, arg}); }
    int address() const { return static_cast<int>(code.size()); }
    int lookup(const std::string& name) const {
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it)
            if (auto found = it->find(name); found != it->end()) return found->second;
        throw std::runtime_error("undeclared variable: " + name);
    }
    void expression(const Expr& e) {
        std::visit([&](const auto& node) { expression_node(node); }, e.node);
        if (depth > 16) throw std::runtime_error("prototype expression stack exceeds 16 values");
    }
    void expression_node(const Number& n) { emit(Op::push, n.value); ++depth; }
    void expression_node(const Variable& n) { emit(Op::load, lookup(n.name)); ++depth; }
    void expression_node(const Unary& n) {
        if (n.op == "-") { emit(Op::push, 0); ++depth; }
        expression(*n.operand);
        if (n.op == "!") emit(Op::logical_not);
        if (n.op == "-") { emit(Op::sub); --depth; }
    }
    void expression_node(const Binary& n) {
        const bool reverse = n.op == ">" || n.op == "<=";
        expression(reverse ? *n.right : *n.left);
        expression(reverse ? *n.left : *n.right);
        if (n.op == "+") emit(Op::add);
        else if (n.op == "-") emit(Op::sub);
        else if (n.op == "==" || n.op == "!=") emit(Op::equal);
        else emit(Op::less);
        --depth;
        if (n.op == "!=" || n.op == "<=" || n.op == ">=") emit(Op::logical_not);
    }
    void statement(const Stmt& s) { std::visit([&](const auto& node) { statement_node(node); }, s.node); }
    void statement_node(const Block& n) {
        scopes.emplace_back();
        for (const auto& s : n.statements) statement(*s);
        scopes.pop_back();
    }
    void statement_node(const Declare& n) {
        if (scopes.back().contains(n.name)) throw std::runtime_error("duplicate variable: " + n.name);
        if (slots == 16) throw std::runtime_error("prototype supports at most 16 local declarations");
        // A C declarator is in scope in its initializer; reject self-use explicitly.
        scopes.back()[n.name] = -1;
        expression(*n.value);
        const auto slot = slots++;
        scopes.back()[n.name] = slot;
        emit(Op::store, slot); --depth;
    }
    void statement_node(const Assign& n) {
        auto slot = lookup(n.name); expression(*n.value); emit(Op::store, slot); --depth;
    }
    void statement_node(const Return& n) { expression(*n.value); emit(Op::ret); --depth; }
    void statement_node(const If& n) {
        expression(*n.condition); auto branch = code.size(); emit(Op::jump_zero); --depth;
        statement(*n.yes);
        auto end = code.size(); emit(Op::jump);
        code[branch].argument = address();
        if (n.no) statement(*n.no);
        code[end].argument = address();
    }
    void statement_node(const While& n) {
        auto begin = address(); expression(*n.condition);
        auto branch = code.size(); emit(Op::jump_zero); --depth;
        statement(*n.body); emit(Op::jump, begin); code[branch].argument = address();
    }
public:
    Program run(const Stmt& tree) {
        statement(tree); emit(Op::push, 0); emit(Op::ret);
        for (auto i : code) if (i.op == Op::load && i.argument < 0)
            throw std::runtime_error("variable used in its own initializer");
        if (code.size() > 10000) throw std::runtime_error("prototype instruction limit exceeded");
        return std::move(code);
    }
};
} // namespace
Program lower(const Stmt& tree) { return Lowerer().run(tree); }
} // namespace c2bat
