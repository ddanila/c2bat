# c2bat

Compile a small subset of C into a stack machine made entirely of MS-DOS
`COMMAND.COM` batch files. A fun compiler experiment, implemented in C++23
with Flex and Bison.

```c
int main(void) {
    int sum = 0;
    int i = 1;
    while (i <= 5) {
        sum = sum + i;
        i = i + 1;
    }
    return sum;
}
```

The generated batch program prints `RESULT=15` on actual MS-DOS 6.22.
Arithmetic executes inside the batch runtime; the compiler does not evaluate
the source program and emit a canned answer.

## Build

Requirements: CMake 3.20+, a C++23 compiler, Bison 3.8+, Flex 2.6+, and Python 3
plus a native C compiler for tests. No libraries are needed by the compiler at runtime.

On macOS, install `cmake bison flex` with Homebrew. Apple's bundled Bison is
older than the required version, so select the Homebrew tools explicitly:

```sh
cmake -S . -B build \
  -DBISON_EXECUTABLE="$(brew --prefix bison)/bin/bison" \
  -DFLEX_EXECUTABLE="$(brew --prefix flex)/bin/flex" \
  -DFLEX_INCLUDE_DIR="$(brew --prefix flex)/include"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

On Debian/Ubuntu, install the development headers as well as the generators:

```sh
sudo apt-get install build-essential cmake bison flex libfl-dev python3
```

Then build:

```sh
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Compile and run

```sh
build/c2bat --ir examples/sum.c       # inspect stack instructions
build/c2bat --run examples/sum.c      # execute the C++ reference VM
build/c2bat examples/sum.c -o out     # emit into a NEW directory
```

Copy all `.BAT` files from `out` to one DOS directory and change into it.
Start a disposable command interpreter with enough environment space:

```dos
COMMAND /E:4096
RUN.BAT
EXIT
```

The runtime uses environment variables for 16 stack slots, local variables,
and scratch registers. A child interpreter keeps those names separate from
your interactive shell. Success prints `RESULT=n` and sets `CBRESULT`; a
range error prints `C2BAT ERROR: RANGE` and sets `CBERR=RANGE`. The return value
is not conveyed through DOS `ERRORLEVEL`.

Generated files use CRLF, 8.3 names, short labels, and lines below 128 bytes.
`RUN.BAT` includes `REM` comments identifying each stack instruction and its
C statement's source line, and emits instruction labels only at jump targets.
`PROGRAM.IR` includes the same source line references. Raw source text is not
embedded in batch comments, so C operators and comments cannot accidentally
become DOS redirection or variable expansion.
The executable runtime uses only `ECHO`, `SET`, `IF`, `GOTO`, and `CALL`.
There are no runtime EXE/COM helpers, Windows command extensions, `SET /A`,
delayed expansion, or `CALL :label`.

## Current language and machine limits

This is a working prototype, not a conforming C implementation:

- One entry point: `int main(void)`; falling off the end returns zero.
- Initialized `int` locals, assignment statements, nested scopes, `if`/`else`,
  `while`, and `return`.
- Unsuffixed decimal, octal, and hexadecimal constants; parentheses; binary
  `+`, `-`, comparisons, short-circuit `&&`/`||`, and unary `+`, `-`, `!`.
- `int` values span **−32768..32767**. Addition, subtraction, and negation
  report an error on signed overflow; they do not wrap.
- Integer literals must fit a positive `int` (0..32767). Larger constants
  require types that are not implemented yet. Write the minimum value as
  `-32767 - 1`, not `-32768`; the latter contains the unsupported literal
  `32768`. Unsigned constants, suffixes, and integer conversions remain unsupported.
- Logical operators evaluate left to right, skip the right operand when
  possible, and return exactly 0 or 1. Bitwise `&` and `|` are not implemented.
- At most 16 local declarations and 16 simultaneously stacked values.
- No preprocessing, line splicing, typedefs, pointers, arrays, strings,
  function calls, multiplication/division, or increment operators yet.
- The host reference VM stops after 100,000 instructions. DOS execution has
  no instruction budget; the test harness applies a timeout.

The frontend uses published Jeff Lee/Jutta Degener C99 lexical rules and a
restricted adaptation of their grammar. See [provenance and upstream
references](third_party/quut/README.md). Unsupported tokens are rejected;
recognizing C lexical forms does not imply support for all C semantics.

## Architecture

```text
C source → Flex scanner → Bison C++ parser → AST
                                              ↓
                           scope resolution + stack IR
                                      ↙              ↘
                          C++ reference VM      batch emitter
                                                     ↓
                                             COMMAND.COM
```

- `src/scanner.l`, `src/parser.y`: lexical rules and grammar; actions construct
  typed AST nodes with `std::unique_ptr` ownership.
- `src/compiler.hpp`: AST and stack instruction types.
- `src/frontend.cpp`: name resolution and lowering, independent of the parser.
- `src/machine.cpp`: reference interpreter and instruction listing.
- `src/batch.cpp`: batch emitter and generated runtime.

`RUN.BAT` is the emitted instruction stream. `PUSH.BAT`/`POP.BAT` shift the
fixed stack; each value is a sign (`P` or `M`) and five decimal digits. For
example, −123 is stored as `M 0 0 1 2 3`. Zero always uses `P`.

`OP.BAT` handles signs, comparisons, and arithmetic. `UNPACK.BAT` extracts
registers; `MAG.BAT` uses generated `D0.BAT` through `D9.BAT` tables for
five-digit addition/subtraction with carry/borrow. `SWAP.BAT` exchanges
magnitudes, `ISZERO.BAT` tests for zero, `LIMIT.BAT` checks the signed range,
and `FORMAT.BAT` produces
ordinary decimal output. Arithmetic takes a fixed number of digit operations,
rather than iterating once per unit of operand value.

Short-circuit logic lowers to existing conditional jumps; it needs no special
batch runtime instruction. `PROGRAM.IR` is a human-readable listing, not a
file interpreted by DOS. See [examples/signed.c](examples/signed.c) for a
program combining negative arithmetic and short-circuit conditions.

## Check under real DOS

Supply a bootable DOS floppy image containing `COMMAND.COM`. The harness
copies it, replaces startup files in the copy, installs generated tests,
boots QEMU, and checks serial results. The input image remains unchanged.
DOS images and binaries are not distributed in this repository.

Requirements: `qemu-system-i386`, `mcopy` from mtools, Python 3, and a built
compiler. From this repository, using the neighboring workspace's reference
media:

```sh
python3 tests/check_dos.py \
  --image ../msdos-reference-media/msdos-6.22/disk1.img \
  --compiler build/c2bat --work build/dos622
```

If specified, `--work` must be a new directory. Omit it to allocate a fresh
temporary directory automatically; the harness prints its location and retains
its artifacts for inspection. The serial transcript is written live to
`build/dos622/serial.log`. A source-built DOS image can also be supplied, for
example `../msdos/out/floppy.img`.

The test harness adds a tiny `QEXIT.COM` solely to stop QEMU after testing;
it is not part of generated programs or their runtime. CI runs the host
checks without proprietary DOS media. Both the host suite and DOS harness
first compare results with a native C compiler (`cc`, `clang`, or `gcc`, required
for testing) and the C++ reference VM. The shared corpus includes fixed edge
cases and generated expressions/loops using a reproducible random seed.

To include DOS in regular CTest runs, configure a local image once:

```sh
cmake -S . -B build -DC2BAT_DOS_IMAGE=/absolute/path/to/dos.img
ctest --test-dir build --output-on-failure
```

The DOS test gets a fresh output directory on every run. Set
`-DC2BAT_DOS_IMAGE=` to disable it; no image path is committed to Git.
The shared suite includes native-C comparisons and intentional VM overflow
errors. Cases cover signed boundaries, decimal carry/borrow chains, negative
comparisons, short-circuit evaluation, nested loops, early returns, skipped
branches, scope, precedence, integer literal forms, dangling `else`, and
deterministically generated programs. Native C comparisons cover expressions
whose intermediate results fit our 16-bit target; host C usually has wider
integers, so target overflow errors are checked separately.
Host-only checks also exercise stack/local limits and invalid inputs.

## Next steps

1. Multiplication, division, and remainder with explicit overflow handling.
2. Compound assignments, increment/decrement, and more structured control flow.
3. Function calls and stack frames.
4. Simulated memory, arrays, and pointers.
5. Broader C grammar support and a preprocessing strategy.

## License

[MIT](LICENSE). Published grammar provenance and permission are documented
separately in [third_party/quut](third_party/quut/README.md).
