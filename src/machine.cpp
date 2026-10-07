#include "compiler.hpp"
#include <array>
#include <fstream>
#include <sstream>
#include <set>
#include <stdexcept>

namespace c2bat {
namespace {
constexpr std::array names{"PUSH", "LOAD", "STORE", "ADD", "SUB", "LT", "EQ", "NOT", "JUMP", "JZ", "RETURN"};
const char* name(Op op) { return names.at(static_cast<std::size_t>(op)); }
void write(const std::filesystem::path& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary);
    for (char ch : text) { if (ch == '\n') file.put('\r'); file.put(ch); }
    if (!file) throw std::runtime_error("cannot write " + path.string());
}
}
std::string disassemble(const Program& program) {
    std::ostringstream out;
    for (std::size_t i = 0; i < program.size(); ++i) {
        const auto [op, arg, line] = program[i];
        out << i << ": " << name(op);
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
            if (result < 0 || result > 255) throw std::runtime_error("VM range error (0..255) at C line " + std::to_string(line));
            stack.push_back(result);
        }
        }
        if (stack.size() > 16) throw std::runtime_error("VM stack overflow");
    }
    throw std::runtime_error("reference VM instruction limit exceeded");
}
void emit_batch(const Program& program, const std::filesystem::path& directory) {
    // A fresh destination prevents accidental replacement of unrelated batch files.
    if (!std::filesystem::create_directory(directory)) throw std::runtime_error("output directory must not already exist");
    std::ostringstream push, pop, step, main;
    push << "@ECHO OFF\n";
    for (int i = 15; i > 0; --i) push << "SET S" << i << "=%S" << i - 1 << "%\n";
    push << "SET S0=%1\n";
    pop << "@ECHO OFF\nSET V=%S0%\n";
    for (int i = 0; i < 15; ++i) pop << "SET S" << i << "=%S" << i + 1 << "%\n";
    pop << "SET S15=\n";
    // COMMAND.COM has no arithmetic: a bounded successor/predecessor table.
    step << "@ECHO OFF\nGOTO N%1\n";
    for (int i = 0; i <= 255; ++i) {
        step << ":N" << i << "\nIF \"%2\"==\"D\" GOTO D" << i << "\n";
        if (i == 255) step << "SET CBERR=RANGE\n";
        else step << "SET N=" << i + 1 << "\n";
        step << "GOTO END\n:D" << i << '\n';
        if (i == 0) step << "SET CBERR=RANGE\n";
        else step << "SET N=" << i - 1 << "\n";
        step << "GOTO END\n";
    }
    step << ":END\n";
    write(directory / "PUSH.BAT", push.str());
    write(directory / "POP.BAT", pop.str());
    write(directory / "STEP.BAT", step.str());
    write(directory / "OP.BAT", R"(@ECHO OFF
CALL POP.BAT
SET B=%V%
IF "%1"=="NOT" GOTO NOT
CALL POP.BAT
SET A=%V%
GOTO %1
:ADD
IF "%B%"=="0" GOTO VALUE
CALL STEP.BAT %A% I
IF NOT "%CBERR%"=="" GOTO END
SET A=%N%
CALL STEP.BAT %B% D
SET B=%N%
GOTO ADD
:SUB
IF "%B%"=="0" GOTO VALUE
CALL STEP.BAT %A% D
IF NOT "%CBERR%"=="" GOTO END
SET A=%N%
CALL STEP.BAT %B% D
SET B=%N%
GOTO SUB
:LT
IF "%B%"=="0" GOTO FALSE
IF "%A%"=="0" GOTO TRUE
CALL STEP.BAT %A% D
SET A=%N%
CALL STEP.BAT %B% D
SET B=%N%
GOTO LT
:EQ
IF "%A%"=="%B%" GOTO TRUE
GOTO FALSE
:NOT
IF "%B%"=="0" GOTO TRUE
GOTO FALSE
:TRUE
SET A=1
GOTO VALUE
:FALSE
SET A=0
:VALUE
CALL PUSH.BAT %A%
:END
)");
    main << "@ECHO OFF\nSET CBERR=\nSET CBRESULT=\n";
    for (int i = 0; i < 16; ++i) main << "SET S" << i << "=\nSET L" << i << "=\n";
    std::set<int> targets;
    for (const auto& instruction : program)
        if (instruction.op == Op::jump || instruction.op == Op::jump_zero)
            targets.insert(instruction.argument);
    for (std::size_t i = 0; i < program.size(); ++i) {
        auto [op, arg, line] = program[i];
        if (targets.contains(static_cast<int>(i))) main << ":I" << i << '\n';
        // Only compiler-owned text and numbers go into REM: raw C source can
        // contain DOS expansion/redirection characters, even inside comments.
        main << "REM " << i << " - " << name(op);
        if (op == Op::push || op == Op::load || op == Op::store || op == Op::jump || op == Op::jump_zero)
            main << ' ' << arg;
        if (line) main << " - C line " << line;
        main << '\n';
        switch (op) {
        case Op::push: main << "CALL PUSH.BAT " << arg << '\n'; break;
        case Op::load: main << "CALL PUSH.BAT %L" << arg << "%\n"; break;
        case Op::store: main << "CALL POP.BAT\nSET L" << arg << "=%V%\n"; break;
        case Op::jump: main << "GOTO I" << arg << '\n'; break;
        case Op::jump_zero: main << "CALL POP.BAT\nIF \"%V%\"==\"0\" GOTO I" << arg << '\n'; break;
        case Op::ret: main << "CALL POP.BAT\nSET CBRESULT=%V%\nECHO RESULT=%CBRESULT%\nGOTO END\n"; break;
        default: main << "CALL OP.BAT " << name(op) << "\nIF NOT \"%CBERR%\"==\"\" GOTO ERROR\n";
        }
    }
    main << ":ERROR\nECHO C2BAT ERROR: %CBERR%\n:END\n";
    write(directory / "RUN.BAT", main.str());
    std::ofstream listing(directory / "PROGRAM.IR"); listing << disassemble(program);
    if (!listing) throw std::runtime_error("cannot write instruction listing");
}
} // namespace c2bat
