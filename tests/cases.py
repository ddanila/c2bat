CASES = [
    ('literals', 'int main(void) <% /* C digraphs */ return 010 + 0x10; %>', 24),
    ('else', 'int main(void) { if(1) if(0) return 1; else return 2; return 3; }', 2),
    ('sum', 'int main(void) { int n=0; int i=1; while(i<=5) { n=n+i; i=i+1; } if(n==15) return n; else return 255; }', 15),
    ('sub', 'int main(void) { return 9-(4-2); }', 7),
    ('scope', 'int main(void) { int x=3; { int x=9; x=x+1; } return x; }', 3),
    ('cmp', 'int main(void) { return (2<3)+(3>2)+(3>=3)+(2<=2)+(4!=5)+(3==3)+!0; }', 7),
    ('false', 'int main(void) { return (3<2)+(2>3)+(2>=3)+(3<=2)+(4!=4)+(3==4)+!1; }', 0),
    ('edge', 'int main(void) { int x=254; return x+1; }', 255),
    ('branch', 'int main(void) { int x=2; if(x==0) return 99; else if(x==2) return 42; return 1; }', 42),
    ('overflow', 'int main(void) { return 32767+1; }', None),
    ('underflow', 'int main(void) { return (-32767-1)-1; }', None),
]

# Each case stays inside the prototype's range unless it explicitly expects
# a runtime error. In particular, unreachable bad arithmetic must stay skipped.
CASES += [
    ('skip', 'int main(void) { if(0) return 32767+1; while(0) { return (-32767-1)-1; } return 7; }', 7),
    ('early', 'int main(void) { int i=0; while(i<5) { if(i==2) return i; i=i+1; } return 99; }', 2),
    ('nested', 'int main(void) { int s=0; int i=0; while(i<3) { int j=0; while(j<2) { s=s+1; j=j+1; } i=i+1; } return s; }', 6),
    ('falloff', 'int main(void) { int x=1; x=x+1; }', 0),
    ('unary', 'int main(void) { return +3 + -0 + !!2; }', 4),
    ('precedence', 'int main(void) { return 1 + 2 < 4 == 1; }', 1),
    ('comment', 'int main(void) { /* %PATH% > BAD.TXT | ECHO surprise */\nreturn 7; }', 7),
    ('longid', 'int main(void) { int ' + 'x' * 160 + '=3; return ' + 'x' * 160 + '; }', 3),
]


def generated_cases(seed=622, count=32):
    """Small deterministic expression trees with independently computed values."""
    import random
    rng = random.Random(seed)

    def expression(depth):
        if not depth or rng.randrange(4) == 0:
            value = rng.randrange(-100, 101)
            return str(value), value
        left, a = expression(depth - 1)
        right, b = expression(depth - 1)
        op = rng.choice(['+', '-', '<', '>', '<=', '>=', '==', '!=', '&&', '||'])
        value = {'+': lambda: a + b, '-': lambda: a - b,
                 '<': lambda: int(a < b), '>': lambda: int(a > b),
                 '<=': lambda: int(a <= b), '>=': lambda: int(a >= b),
                 '==': lambda: int(a == b), '!=': lambda: int(a != b),
                 '&&': lambda: int(bool(a) and bool(b)),
                 '||': lambda: int(bool(a) or bool(b))}[op]()
        return f'({left} {op} {right})', value

    result = []
    for i in range(count):
        expr, value = expression(3)
        result.append((f'expr{i}', f'int main(void) {{ return {expr}; }}', value))
    for i in range(8):
        bound = rng.randrange(1, 5)
        increment = rng.randrange(1, 4)
        source = (f'int main(void) {{ int n=0; int i=0; while(i<{bound}) {{ '
                  f'if(i!=1) n=n+{increment}; else n=n+1; i=i+1; }} return n; }}')
        expected = sum(1 if j == 1 else increment for j in range(bound))
        result.append((f'loop{i}', source, expected))
    return result


CASES += generated_cases()


CASES += [
    ('and_skip', 'int main(void) { return 0 && (32767+1); }', 0),
    ('or_skip', 'int main(void) { return -3 || (32767+1); }', 1),
    ('and_rhs', 'int main(void) { return 2 && -7; }', 1),
    ('or_rhs', 'int main(void) { return 0 || -7; }', 1),
    ('logic_zero', 'int main(void) { return (0 || 0) + (1 && 0); }', 0),
    ('logic_prec', 'int main(void) { return 1 || 0 && (32767+1); }', 1),
    ('logic_nest', 'int main(void) { return 7 + (0 || (1 && -5)); }', 8),
    ('and_trap', 'int main(void) { return 1 && (32767+1); }', None),
    ('or_trap', 'int main(void) { return 0 || (32767+1); }', None),
    ('max', 'int main(void) { return 32767; }', 32767),
    ('min', 'int main(void) { return -32767-1; }', -32768),
    ('neg', 'int main(void) { return -12345; }', -12345),
    ('cancel', 'int main(void) { return -12345+12345; }', 0),
    ('mixed', 'int main(void) { return -12345+23456; }', 11111),
    ('mixedneg', 'int main(void) { return 12345-23456; }', -11111),
    ('negsub', 'int main(void) { return -100-(-12345); }', 12245),
    ('minplus', 'int main(void) { return (-32767-1)+32767; }', -1),
    ('minzero', 'int main(void) { int m=-32767-1; return m-m; }', 0),
    ('negcmp', 'int main(void) { return (-100 < -2) + (-2 > -100) + (-1 < 0) + (0 > -1); }', 4),
    ('cmpedge', 'int main(void) { return ((-32767-1)<32767) + ((-32767-1)<=(-32767-1)) + (32767>=32767); }', 3),
    ('signedloop', 'int main(void) { int i=-3; int n=0; while(i<3 && n<10) { n=n+1; i=i+1; } return n; }', 6),
    ('neg_over', 'int main(void) { int m=-32767-1; return -m; }', None),
    ('sub_over', 'int main(void) { return 32767-(-1); }', None),
    ('big_over', 'int main(void) { return 30000+30000; }', None),
    ('big_under', 'int main(void) { return -30000-30000; }', None),
]

# Carry/borrow must propagate across each digit boundary, in both signs.
for digits in range(1, 5):
    boundary = 10 ** digits
    CASES += [
        (f'carry{digits}', f'int main(void) {{ return {boundary-1}+1; }}', boundary),
        (f'borrow{digits}', f'int main(void) {{ return {boundary}-1; }}', boundary-1),
        (f'negcarry{digits}', f'int main(void) {{ return -{boundary-1}-1; }}', -boundary),
    ]
