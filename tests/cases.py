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
    ('overflow', 'int main(void) { return 255+1; }', None),
    ('underflow', 'int main(void) { return 0-1; }', None),
]
