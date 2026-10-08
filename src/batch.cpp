#include "compiler.hpp"
#include <array>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace c2bat {
namespace {
void write(const std::filesystem::path& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary);
    for (char ch : text) { if (ch == '\n') file.put('\r'); file.put(ch); }
    if (!file) throw std::runtime_error("cannot write " + path.string());
}
std::string encode(int value) {
    std::string result = value < 0 ? "M" : "P";
    if (value < 0) value = -value;
    for (int divisor : {10000, 1000, 100, 10, 1})
        result += " " + std::to_string(value / divisor % 10);
    return result;
}
// Each Dn.BAT is a single decimal digit's full adder/subtractor. The inputs
// are operation A/S, right digit, carry/borrow; outputs are D and C.
void digit_tables(const std::filesystem::path& directory) {
    for (int left = 0; left < 10; ++left) {
        std::ostringstream table;
        table << "@ECHO OFF\nGOTO %1%2%3\n";
        for (char op : {'A', 'S'}) for (int right = 0; right < 10; ++right) for (int carry = 0; carry < 2; ++carry) {
            const auto value = op == 'A' ? left + right + carry : left - right - carry;
            table << ':' << op << right << carry << "\nSET D=" << (value + 10) % 10
                  << "\nSET C=" << (value < 0 || value > 9 ? 1 : 0) << "\nGOTO END\n";
        }
        table << ":END\n";
        write(directory / ("D" + std::to_string(left) + ".BAT"), table.str());
    }
}
void runtime(const std::filesystem::path& directory) {
    std::ostringstream push, pop, unpack, magnitude, swap, format, limit;
    push << "@ECHO OFF\n";
    for (int i = 15; i > 0; --i) push << "SET S" << i << "=%S" << i - 1 << "%\n";
    push << "SET S0=%1 %2 %3 %4 %5 %6\n";
    pop << "@ECHO OFF\nSET V=%S0%\n";
    for (int i = 0; i < 15; ++i) pop << "SET S" << i << "=%S" << i + 1 << "%\n";
    pop << "SET S15=\n";
    unpack << "@ECHO OFF\nSET %1S=%2\n";
    for (int i = 4; i >= 0; --i) unpack << "SET %1" << i << "=%" << 7 - i << '\n';
    magnitude << "@ECHO OFF\nSET C=0\n";
    swap << "@ECHO OFF\n";
    for (int i = 0; i < 5; ++i) {
        magnitude << "CALL D%A" << i << "%.BAT %1 %B" << i << "% %C%\nSET R" << i << "=%D%\n";
        swap << "SET T=%A" << i << "%\nSET A" << i << "=%B" << i << "%\nSET B" << i << "=%T%\n";
    }
    format << "@ECHO OFF\nSET CBRESULT=0\n";
    for (int i = 2; i <= 6; ++i) format << "IF NOT \"%" << i << "\"==\"0\" GOTO D" << i << '\n';
    format << "GOTO END\n";
    for (int i = 2; i <= 6; ++i) {
        format << ":D" << i << "\nSET CBRESULT=";
        for (int j = i; j <= 6; ++j) format << '%' << j;
        format << "\nGOTO SIGN\n";
    }
    format << ":SIGN\nIF \"%1\"==\"M\" SET CBRESULT=-%CBRESULT%\n:END\n";
    // Accept magnitudes below 32768, and exactly 32768 only when negative.
    limit << "@ECHO OFF\nIF \"%RS%%R4%%R3%%R2%%R1%%R0%\"==\"M32768\" GOTO END\n";
    constexpr std::array bound{3, 2, 7, 6, 7};
    for (int i = 0; i < 5; ++i) {
        limit << ":P" << i << '\n';
        for (int digit = 0; digit < bound[static_cast<std::size_t>(i)]; ++digit)
            limit << "IF \"%R" << 4 - i << "%\"==\"" << digit << "\" GOTO END\n";
        limit << "IF \"%R" << 4 - i << "%\"==\"" << bound[static_cast<std::size_t>(i)] << "\" GOTO ";
        if (i == 4) limit << "END\n"; else limit << 'P' << i + 1 << '\n';
        limit << "GOTO RANGE\n";
    }
    limit << ":RANGE\nSET CBERR=RANGE\n:END\n";
    write(directory / "PUSH.BAT", push.str());
    write(directory / "POP.BAT", pop.str());
    write(directory / "UNPACK.BAT", unpack.str());
    write(directory / "MAG.BAT", magnitude.str());
    write(directory / "SWAP.BAT", swap.str());
    write(directory / "FORMAT.BAT", format.str());
    write(directory / "LIMIT.BAT", limit.str());
    write(directory / "ISZERO.BAT", "@ECHO OFF\nSET Z=0\nIF \"%1%2%3%4%5%6\"==\"P00000\" SET Z=1\n");
    digit_tables(directory);
    write(directory / "OP.BAT", R"(@ECHO OFF
CALL POP.BAT
CALL UNPACK.BAT B %V%
SET BV=%BS%%B4%%B3%%B2%%B1%%B0%
IF "%1"=="NOT" GOTO NOT
CALL POP.BAT
CALL UNPACK.BAT A %V%
SET AV=%AS%%A4%%A3%%A2%%A1%%A0%
GOTO %1
:NOT
IF "%BV%"=="P00000" GOTO TRUE
GOTO FALSE
:EQ
IF "%AV%"=="%BV%" GOTO TRUE
GOTO FALSE
:LT
IF "%AV%"=="%BV%" GOTO FALSE
IF "%AS%"=="%BS%" GOTO LTSAME
IF "%AS%"=="M" GOTO TRUE
GOTO FALSE
:LTSAME
CALL MAG.BAT S
IF "%AS%%C%"=="P1" GOTO TRUE
IF "%AS%%C%"=="M0" GOTO TRUE
GOTO FALSE
:SUB
IF "%BS%"=="P" GOTO SUBPOS
SET BS=P
GOTO ADD
:SUBPOS
SET BS=M
:ADD
SET RS=%AS%
IF NOT "%AS%"=="%BS%" GOTO DIFF
CALL MAG.BAT A
IF "%C%"=="1" GOTO RANGE
GOTO CHECK
:DIFF
CALL MAG.BAT S
IF "%C%"=="0" GOTO CHECK
SET RS=%BS%
CALL SWAP.BAT
CALL MAG.BAT S
:CHECK
CALL LIMIT.BAT
IF NOT "%CBERR%"=="" GOTO END
IF "%R4%%R3%%R2%%R1%%R0%"=="00000" SET RS=P
CALL PUSH.BAT %RS% %R4% %R3% %R2% %R1% %R0%
GOTO END
:TRUE
CALL PUSH.BAT P 0 0 0 0 1
GOTO END
:FALSE
CALL PUSH.BAT P 0 0 0 0 0
GOTO END
:RANGE
SET CBERR=RANGE
:END
)");
}
} // namespace

void emit_batch(const Program& program, const std::filesystem::path& directory) {
    if (!std::filesystem::create_directory(directory)) throw std::runtime_error("output directory must not already exist");
    runtime(directory);
    std::ostringstream main;
    main << "@ECHO OFF\nSET CBERR=\nSET CBRESULT=\n";
    for (int i = 0; i < 16; ++i) main << "SET S" << i << "=\nSET L" << i << "=\n";
    std::set<int> targets;
    for (const auto& instruction : program)
        if (instruction.op == Op::jump || instruction.op == Op::jump_zero)
            targets.insert(instruction.argument);
    for (std::size_t i = 0; i < program.size(); ++i) {
        auto [op, arg, line] = program[i];
        const auto* name = opcode_name(op);
        if (targets.contains(static_cast<int>(i))) main << ":I" << i << '\n';
        // Never embed raw source text: DOS interprets expansion/redirection
        // metacharacters even in some comment contexts.
        main << "REM " << i << " - " << name;
        if (op == Op::push || op == Op::load || op == Op::store || op == Op::jump || op == Op::jump_zero)
            main << ' ' << arg;
        if (line) main << " - C line " << line;
        main << '\n';
        switch (op) {
        case Op::push: main << "CALL PUSH.BAT " << encode(arg) << '\n'; break;
        case Op::load: main << "CALL PUSH.BAT %L" << arg << "%\n"; break;
        case Op::store: main << "CALL POP.BAT\nSET L" << arg << "=%V%\n"; break;
        case Op::jump: main << "GOTO I" << arg << '\n'; break;
        case Op::jump_zero: main << "CALL POP.BAT\nCALL ISZERO.BAT %V%\nIF \"%Z%\"==\"1\" GOTO I" << arg << '\n'; break;
        case Op::ret: main << "CALL POP.BAT\nCALL FORMAT.BAT %V%\nECHO RESULT=%CBRESULT%\nGOTO END\n"; break;
        default: main << "CALL OP.BAT " << name << "\nIF NOT \"%CBERR%\"==\"\" GOTO ERROR\n";
        }
    }
    main << ":ERROR\nECHO C2BAT ERROR: %CBERR%\n:END\n";
    write(directory / "RUN.BAT", main.str());
    std::ofstream listing(directory / "PROGRAM.IR"); listing << disassemble(program);
    if (!listing) throw std::runtime_error("cannot write instruction listing");
}
} // namespace c2bat
