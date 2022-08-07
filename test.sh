#!/bin/bash

arch="x64"
if [ "$1" = "llvm" ]
then
    arch="$1"
fi

sh ./clean.sh
source ./$arch-test.sh

assert 0 'int main() { return 0; }'
assert 42 'int main() { return 42; }'
assert 21 'int main() { return 5+20-4; }'
assert 41 'int main() { return 12+ 34 - 5; }'
assert 47 'int main() { return 5 + 6 * 7; }'
assert 77 'int main() { return (5 + 6) * 7; }'
assert 15 'int main() { return 5 * (9 - 6); }'
assert 4 'int main() { return (3 + 5) / 2; }'
assert 11 'int main() { return (1+2)*3+6/2-1; }'

# unary
assert 10 'int main() { return -10+20; }'
assert 10 'int main() { return - -10; }'
assert 10 'int main() { return - - +10; }'
assert 10 'int main() { return -(10+20)+40; }'

# ralation
assert 0 'int main() { return 0==1; }'
assert 1 'int main() { return 42==42; }'
assert 1 'int main() { return 0!=1; }'
assert 0 'int main() { return 42!=42; }'
assert 1 'int main() { return 0<1; }'
assert 0 'int main() { return 1<1; }'
assert 0 'int main() { return 2<1; }'
assert 1 'int main() { return 0<=1; }'
assert 1 'int main() { return 1<=1; }'
assert 0 'int main() { return 2<=1; }'
assert 1 'int main() { return 1>0; }'
assert 0 'int main() { return 1>1; }'
assert 0 'int main() { return 1>2; }'
assert 1 'int main() { return 1>=0; }'
assert 1 'int main() { return 1>=1; }'
assert 0 'int main() { return 1>=2; }'

assert 25 'int main() { (1 + 333); (2 - -333); return (- -12 + 13); }'

# assign
assert 3 'int main() { int a=3; return a; }'
assert 8 'int main() { int a=3; int z=5; return a+z; }'
assert 6 'int main() { int a; int b; a=b=3; return a+b; }'
assert 3 'int main() { int foo=3; return foo; }'
assert 8 'int main() { int foo123=3; int bar=5; return foo123+bar; }'

assert 1 'int main() { return 1; 2; 3; }'
assert 2 'int main() { 1; return 2; 3; }'
assert 3 'int main() { 1; 2; return 3; }'
assert 3 'int main() { 1; 2; { return 3; 4; 5; } }'
assert 12 'int main() { ; int abc = 12;; return abc; }'

# if else
assert 3 'int main() { if (0) return 2; return 3; }'
assert 3 'int main() { if (1-1) return 2; return 3; }'
assert 2 'int main() { if (1) return 2; return 3; }'
assert 2 'int main() { if (2-1) return 2; return 3; }'
assert 4 'int main() { if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 'int main() { if (1) { 1; 2; return 3; } else { return 4; } }'
assert 100 'int main() { int a = 10; if (a < 5) { return a; } else { return a = 100; } }'

# loop
assert 55 'int main() { int i=0, j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 3 'int main() { for (;;) {return 3;} return 5; }'
assert 10 'int main() { int i=0; while(i<10) { i=i+1; } return i; }'

# address
assert 3 'int main() { int x=3; return *&x; }'
assert 3 'int main() { int x=3, * y=&x, ** z=&y; return **z; }'
assert 5 'int main() { int x=3, y=5; return *(&x+1); }'
assert 3 'int main() { int x=3; int y=5; return *(&y-1); }'
assert 5 'int main() { int x=3; int y=5; return *(&x-(-1)); }'
assert 5 'int main() { int x=3; int* y=&x; *y=5; return x; }'
assert 7 'int main() { int x=3; int y=5; *(&x+1)=7; return y; }'
assert 7 'int main() { int x=3; int y=5; *(&y-2+1)=7; return x; }'
assert 5 'int main() { int x=3; return (&x+2)-&x+3; }'

assert 10 'int main() { exit(10); return 20; }'
assert 20 'int main() { exit(20); return 10; }'
assert 19 'int main() { exit(abs(-19)); }'
assert 7 'int ret7() { return 7; } int main() { return ret7(); }'

assert 13 'int sub(int a, int b) { return a - b; } int main() { return sub(16, 3); }'
assert 55 'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }'
assert 30 'int add4(int a, int b, int c, int d) { return a + b + c + d; } int main() { return add4(1, 2, add4(1, 2, 3, 4), add4(1, 2, add4(1, 2, 3, 4), 4)); }'

echo OK
