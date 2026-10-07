"""Behavioral checks; no external DOS installation required."""
import pathlib
import subprocess
import sys
import tempfile
from cases import CASES

compiler = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as temporary:
    root = pathlib.Path(temporary)
    source = root / 'input.c'

    def invoke(text, *args):
        source.write_text(text)
        return subprocess.run([compiler, *args, str(source)], capture_output=True, text=True)

    for name, text, expected in CASES:
        result = invoke(text, '--run')
        if expected is None:
            assert result.returncode == 1 and 'range error' in result.stderr, (name, result)
        else:
            assert result.returncode == 0 and result.stdout == f'RESULT={expected}\n', (name, result)

    for text in [
        'int main(void) { return 1++2; }',
        'int main(void) { return 1--2; }',
        'int main(void) { int for=1; return for; }',
        'int main(void) { return missing; }',
        'int main(void) { int x=1; int x=2; return x; }',
        'int main(void) { int x=x; return x; }',
        'int main(void) { int x=1; { int x=x; } return x; }',
        'int main(void) { return 256; }',
        'int main(void) { return 09; }',
        'int main(void) { return 1.5; }',
        'int main(void) { return 1u; }',
        'int main(void) { return 1*2; }',
        'int main(void) { if(1) int x=2; return 0; }',
        'int main(void) { return 0; } trailing',
        'int main(void) { /* unfinished',
    ]:
        result = invoke(text, '--ir')
        assert result.returncode == 1, (text, result)

    # Compare against a native C compiler for additional precedence/scope cases.
    import shutil
    cc = shutil.which('cc')
    if cc:
        for name, text, expected in CASES:
            if expected is None:
                continue
            source.write_text(text)
            executable = root / 'native'
            subprocess.run([cc, '-std=c99', str(source), '-o', str(executable)], check=True)
            assert subprocess.run([str(executable)]).returncode == expected, name

    # Emission is not evaluation: even a nonterminating program can be compiled.
    source.write_text('int main(void) { while(1) { } }')
    out = root / 'batch'
    subprocess.run([compiler, str(source), '-o', str(out)], check=True)
    for path in out.glob('*.BAT'):
        data = path.read_bytes()
        assert b'\r\n' in data and b'\n' not in data.replace(b'\r\n', b'')
        assert max(map(len, data.splitlines())) < 128
        assert len(path.stem) <= 8
        for line in data.splitlines():
            if line.startswith(b':'):
                assert len(line[1:]) <= 8
    before = (out / 'RUN.BAT').read_bytes()
    result = subprocess.run([compiler, str(source), '-o', str(out)], capture_output=True)
    assert result.returncode == 1 and (out / 'RUN.BAT').read_bytes() == before
print('PASS: reference VM, native C comparisons, diagnostics, and DOS file format')
