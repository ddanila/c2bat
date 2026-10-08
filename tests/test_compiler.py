"""Behavioral checks; no external DOS installation required."""
import pathlib
import subprocess
import sys
import tempfile
from cases import CASES
from differential import check_programs

compiler = str(pathlib.Path(sys.argv[1]).resolve())
with tempfile.TemporaryDirectory() as temporary:
    root = pathlib.Path(temporary)
    source = root / 'input.c'

    def invoke(text, *args):
        source.write_text(text)
        return subprocess.run([compiler, *args, str(source)], capture_output=True, text=True)

    native_count = check_programs(compiler, CASES, root / "differential")

    for text in [
        'int main(void) { return 1++2; }',
        'int main(void) { return 1--2; }',
        'int main(void) { int for=1; return for; }',
        'int main(void) { return missing; }',
        'int main(void) { int x=1; int x=2; return x; }',
        'int main(void) { int x=x; return x; }',
        'int main(void) { int x=1; { int x=x; } return x; }',
        'int main(void) { return 32768; }',
        'int main(void) { return 0x8000; }',
        'int main(void) { return 1 & 2; }',
        'int main(void) { return 1 | 2; }',
        'int main(void) { return 0 && missing; }',
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

    # Boundaries: distinguish exactly supported stack/local counts from overflow.
    for count in (16, 17):
        expr = '0'
        for _ in range(count - 1):
            expr = '0+(' + expr + ')'
        result = invoke('int main(void) { return ' + expr + '; }', '--run')
        assert result.returncode == (0 if count == 16 else 1), result
        declarations = ''.join(f'int x{i}={i};' for i in range(count))
        result = invoke('int main(void) {' + declarations + 'return x0;}', '--run')
        assert result.returncode == (0 if count == 16 else 1), result

    # Multiline source locations survive lowering; source metacharacters never
    # become commands through the diagnostic comments.
    source.write_text('int main(void) {\n'
                      ' /* %PATH% > BAD.TXT | ECHO surprise */\n'
                      ' int x=1;\n'
                      ' x=x+2;\n'
                      ' return x;\n}\n')
    annotated = root / 'annotated'
    subprocess.run([compiler, str(source), '-o', str(annotated)], check=True)
    batch = (annotated / 'RUN.BAT').read_text()
    assert 'STORE 0 - C line 3' in batch and 'ADD - C line 4' in batch
    assert 'RETURN - C line 5' in batch and 'BAD.TXT' not in batch
    result = invoke('int main(void) {\n return 32767+1;\n}', '--run')
    assert result.returncode == 1 and 'C line 2' in result.stderr, result

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
    text = (out / 'RUN.BAT').read_text()
    assert 'REM 0 - PUSH 1 - C line 1' in text
    assert ':I0\n' in text  # while condition is a jump target
    assert ':I1\n' not in text  # ordinary instructions have no labels
    before = (out / 'RUN.BAT').read_bytes()
    result = subprocess.run([compiler, str(source), '-o', str(out)], capture_output=True)
    assert result.returncode == 1 and (out / 'RUN.BAT').read_bytes() == before
print(f'PASS: {len(CASES)} VM cases, {native_count} native C comparisons, diagnostics, and DOS file format')
