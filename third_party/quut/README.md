# Published C99 grammar baseline

`c99.l` and `c99.y` are reference snapshots of the code blocks published by
Jutta Degener, downloaded on 2026-10-07:

- Lex: https://www.quut.com/c/ANSI-C-grammar-l-1999.html
- Yacc: https://www.quut.com/c/ANSI-C-grammar-y-1999.html
- Permission: https://www.quut.com/c/ANSI-C-grammar-FAQ.html

The specifications originate with Jeff Lee's 1985 ANSI C draft grammar,
reposted by Tom Stockfisch in 1987 and maintained by Jutta Degener. The Lex
page records a June 2017 correction to hexadecimal floating constants.

The maintainer's FAQ explicitly permits derived work, with or without
attribution, on behalf of both the maintainer and original poster. These
reference files retain that upstream permission; c2bat's original code and
adaptations are offered under the repository's MIT license.

`src/scanner.l` adapts the published lexical patterns to Flex's C++ scanner
and Bison's typed token constructors. Unsupported tokens produce diagnostics.
Comment handling uses a scanner state; locations include lines and columns;
CRLF whitespace is accepted. Only unsuffixed integer constants are evaluated.

`src/parser.y` adapts the expression hierarchy and statement/block structure
for the executable subset, adds C++ AST actions, and explicitly resolves the
dangling `else` in favor of the closest `if`. It is deliberately not the full
upstream grammar. The reference files are not compiled.

Neither these grammars nor the current frontend supplies a C preprocessor or
complete C semantic analysis. In particular, typedef-name classification is
not implemented in this milestone.
