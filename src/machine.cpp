#include "compiler.hpp"
#include <array>
#include <sstream>
#include <stdexcept>

namespace c2bat {
namespace {
constexpr std::array names{"PUSH", "LOAD", "STORE", "ADD", "SUB", "LT", "EQ", "NOT", "JUMP", "JZ", "RETURN"};
}
const char* opcode_name(Op op) { return names.at(static_cast<std::size_t>(op)); }
std::string disassemble(const Program& program) {
    std::ostringstream out;
    for (std::size_t i = 0; i < program.size(); ++i) {
        const auto [op, arg, line] = program[i];
        out << i << ": " << opcode_name(op);
        if (op == Op::push || op == Op::load || op == Op::store || op == Op::jump || op == Op::jump_zero) out << ' ' << arg;
        if (line) out << " ; C line " << line;
        out << '\n';
    }
    return out.str();
}
int interpret(const Program& program) {
    std::array<int, 16> locals{};
    std::vector<int> stack;
    auto pop = [&] {
        if (stack.empty()) throw std::runtime_error("VM stack underflow");
        auto value = stack.back(); stack.pop_back(); return value;
    };
    std::size_t pc = 0;
    for (int steps = 0; steps < 100000; ++steps) {
        auto [op, arg, line] = program.at(pc++);
        switch (op) {
        case Op::push: stack.push_back(arg); break;
        case Op::load: stack.push_back(locals.at(static_cast<std::size_t>(arg))); break;
        case Op::store: locals.at(static_cast<std::size_t>(arg)) = pop(); break;
        case Op::logical_not: stack.push_back(pop() == 0); break;
        case Op::jump: pc = static_cast<std::size_t>(arg); break;
        case Op::jump_zero: if (pop() == 0) pc = static_cast<std::size_t>(arg); break;
        case Op::ret: return pop();
        default: {
            auto right = pop(), left = pop();
            int result = 0;
            if (op == Op::add) result = left + right;
            if (op == Op::sub) result = left - right;
            if (op == Op::less) result = left < right;
            if (op == Op::equal) result = left == right;
            if (result < -32768 || result > 32767) throw std::runtime_error("VM range error (-32768..32767) at C line " + std::to_string(line));
            stack.push_back(result);
        }
        }
        if (stack.size() > 16) throw std::runtime_error("VM stack overflow");
    }
    throw std::runtime_error("reference VM instruction limit exceeded");
}
} // namespace c2bat
