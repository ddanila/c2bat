"""Compare bounded C programs with the reference VM and a native C compiler."""
import pathlib
import shutil
import subprocess


def check_programs(compiler, cases, directory):
    directory = pathlib.Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    successful = []
    for name, source, expected in cases:
        path = directory / f'{name}.c'
        path.write_text(source)
        result = subprocess.run([str(compiler), '--run', str(path)],
                                capture_output=True, text=True, timeout=10)
        if expected is None:
            assert result.returncode == 1 and 'range error' in result.stderr, (name, result)
        else:
            assert result.returncode == 0 and result.stdout == f'RESULT={expected}\n', (name, result)
            successful.append((name, source, expected))

    cc = shutil.which('cc') or shutil.which('clang') or shutil.which('gcc')
    if not cc:
        raise RuntimeError('Differential tests require a native C compiler (cc, clang, or gcc)')
    # One native executable keeps generated-case testing inexpensive. Use main
    # itself, renamed by the preprocessor, rather than rewriting source tokens.
    unit = ['#include <stdio.h>']
    for i, (_, source, _) in enumerate(successful):
        # main's implicit return 0 does not apply after renaming it. Insert an
        # explicit return at the closing brace (including the C digraph form).
        stripped = source.rstrip()
        closing = '%>' if stripped.endswith('%>') else '}'
        source = stripped[:-len(closing)] + ' return 0; ' + closing
        unit += [f'#define main case_{i}', source, '#undef main']
    unit += ['int main(void) {']
    for i, (name, _, _) in enumerate(successful):
        unit += [f'printf("{name}=%d\\n", case_{i}());']
    unit += ['return 0; }']
    path = directory / 'native.c'
    path.write_text('\n'.join(unit))
    executable = directory / 'native'
    build = subprocess.run([cc, '-std=c99', str(path), '-o', str(executable)],
                           capture_output=True, text=True, timeout=30)
    (directory / 'native-build.log').write_text(build.stdout + build.stderr)
    if build.returncode:
        raise RuntimeError('Native C build failed:\n' + build.stderr)
    result = subprocess.run([str(executable)], check=True, capture_output=True, text=True, timeout=10)
    expected = ''.join(f'{name}={value}\n' for name, _, value in successful)
    assert result.stdout == expected, ('native C disagrees', result.stdout, expected)
    return len(successful)
