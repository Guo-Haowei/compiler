import os
import subprocess
import build
import sys

test_cnt = 0
test_folder = 'tmp'
checkmark = u'\u2713'
crossmark = u'\u2717'

all_tests = []
cur_section = None
sections = {}


def add_test(expect, input):
    testobj = {
        'expect': expect,
        'input': input
    }
    all_tests.append(testobj)
    if cur_section != None:
        cur_section.append(testobj)


def test_section_begin(name):
    if name in sections:
        print(f'section {name} already exists')
        os._exit(1)

    global cur_section
    cur_section = []
    sections[name] = cur_section
    return


def run_case(expect, input, i, total):
    global test_cnt
    test_cnt = test_cnt + 1
    test_name = f'test{test_cnt}'

    # open text file
    text_file = open(f'{test_name}.c', "w")
    text_file.write(input)
    text_file.close()

    # compile
    cmd = [f'./{build.exe_name}', f'{test_name}.c']
    child = subprocess.Popen(cmd)
    child.communicate()
    if child.poll() != 0:
        print(f'Failed to execute: {" ".join(cmd)}')
        os._exit(1)

    if os.system(' '.join(['mv', 'tmp.s', f'{test_name}.s'])) != 0:
        print(f'Failed to rename file')
        os._exit(1)

    cmd = ['gcc', '-static', '-o', 'tmp', f'{test_name}.s']
    cmd = ' '.join(cmd)
    if os.system(cmd) != 0:
        print(f'Failed to execute: {cmd}')
        os._exit(1)

    child = subprocess.Popen(['./tmp'])
    child.communicate()
    actual = child.poll()

    label = f'[{i}/{total}]'
    if actual == expect:
        print(checkmark,
              f'{label} {input} => {expect}')
    else:
        print(crossmark,
              f'{label} {input} => {expect} expected, but got {actual}')
        os._exit(1)

    return


def run_tests(section):
    testcases = all_tests

    if section != None:
        testcases = sections[section]

    total = len(testcases)
    i = 0
    for testcase in testcases:
        i = i + 1
        run_case(testcase['expect'], testcase['input'], i, total)


def test_main():
    # create tmp folder
    if os.path.exists(test_folder):
        os.system(f'rm -r {test_folder}')

    try:
        os.mkdir(test_folder)
    except:
        print(f'failed to create folder {test_folder}')
        return

    # change working directory
    os.chdir(test_folder)

    build.build_exe()

    # test cases
    add_test(0, r'int main() { return 0; }')
    add_test(42, r'int main() { return 42; }')
    add_test(21, r'int main() { return 5+20-4; }')
    add_test(41, r'int main() { return 12+ 34 - 5; }')
    add_test(47, r'int main() { return 5 + 6 * 7; }')
    add_test(77, r'int main() { return (5 + 6) * 7; }')
    add_test(15, r'int main() { return 5 * (9 - 6); }')
    add_test(4, r'int main() { return (3 + 5) / 2; }')
    add_test(11, r'int main() { return (1+2)*3+6/2-1; }')

    test_section_begin('unary')
    # unary
    add_test(10, r'int main() { return -10+20; }')
    add_test(10, r'int main() { return - -10; }')
    add_test(10, r'int main() { return - - +10; }')
    add_test(10, r'int main() { return -(10+20)+40; }')

    test_section_begin('relation')
    # ralation
    add_test(0, r'int main() { return 0==1; }')
    add_test(1, r'int main() { return 42==42; }')
    add_test(1, r'int main() { return 0!=1; }')
    add_test(0, r'int main() { return 42!=42; }')
    add_test(1, r'int main() { return 0<1; }')
    add_test(0, r'int main() { return 1<1; }')
    add_test(0, r'int main() { return 2<1; }')
    add_test(1, r'int main() { return 0<=1; }')
    add_test(1, r'int main() { return 1<=1; }')
    add_test(0, r'int main() { return 2<=1; }')
    add_test(1, r'int main() { return 1>0; }')
    add_test(0, r'int main() { return 1>1; }')
    add_test(0, r'int main() { return 1>2; }')
    add_test(1, r'int main() { return 1>=0; }')
    add_test(1, r'int main() { return 1>=1; }')
    add_test(0, r'int main() { return 1>=2; }')

    add_test(
        25, r'int main() { (1 + 333); (2 - -333); return (- -12 + 13); }')

    test_section_begin('assign')
    # assign
    add_test(3, r'int main() { int a=3; return a; }')
    add_test(8, r'int main() { int a=3; int z=5; return a+z; }')
    add_test(6, r'int main() { int a; int b; a=b=3; return a+b; }')
    add_test(3, r'int main() { int foo=3; return foo; }')
    add_test(
        8, r'int main() { int foo123=3; int bar=5; return foo123+bar; }')

    add_test(1, r'int main() { return 1; 2; 3; }')
    add_test(2, r'int main() { 1; return 2; 3; }')
    add_test(3, r'int main() { 1; 2; return 3; }')
    add_test(3, r'int main() { 1; 2; { return 3; 4; 5; } }')
    add_test(12, r'int main() { ; int abc = 12;; return abc; }')

    test_section_begin('branch')
    # if else
    add_test(3, r'int main() { if (0) return 2; return 3; }')
    add_test(3, r'int main() { if (1-1) return 2; return 3; }')
    add_test(2, r'int main() { if (1) return 2; return 3; }')
    add_test(2, r'int main() { if (2-1) return 2; return 3; }')
    add_test(
        4, r'int main() { if (0) { 1; 2; return 3; } else { return 4; } }')
    add_test(
        3, r'int main() { if (1) { 1; 2; return 3; } else { return 4; } }')
    add_test(
        100, r'int main() { int a = 10; if (a < 5) { return a; } else { return a = 100; } }')

    test_section_begin('loop')
    # loop
    add_test(
        55, r'int main() { int i=0, j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }')
    add_test(3, r'int main() { for (;;) {return 3;} return 5; }')
    add_test(
        10, r'int main() { int i=0; while(i<10) { i=i+1; } return i; }')

    test_section_begin('addr')
    # address
    add_test(3, r'int main() { int x=3; return *&x; }')
    add_test(3, r'int main() { int x=3, * y=&x, ** z=&y; return **z; }')
    add_test(5, r'int main() { int x=3, y=5; return *(&x+1); }')
    add_test(3, r'int main() { int x=3; int y=5; return *(&y-1); }')
    add_test(5, r'int main() { int x=3; int y=5; return *(&x-(-1)); }')
    add_test(5, r'int main() { int x=3; int* y=&x; *y=5; return x; }')
    add_test(7, r'int main() { int x=3; int y=5; *(&x+1)=7; return y; }')
    add_test(7, r'int main() { int x=3; int y=5; *(&y-2+1)=7; return x; }')
    add_test(5, r'int main() { int x=3; return (&x+2)-&x+3; }')

    add_test(10, r'int main() { exit(10); return 20; }')
    add_test(20, r'int main() { exit(20); return 10; }')
    add_test(19, r'int main() { exit(abs(-19)); }')
    add_test(7, r'int ret7() { return 7; } int main() { return ret7(); }')

    add_test(
        13, r'int sub(int a, int b) { return a - b; } int main() { return sub(16, 3); }')
    add_test(
        55, r'int main() { return fib(9); } int fib(int x) { if (x<=1) return 1; return fib(x-1) + fib(x-2); }')
    add_test(
        30, r'int add4(int a, int b, int c, int d) { return a + b + c + d; } int main() { return add4(1, 2, add4(1, 2, 3, 4), add4(1, 2, add4(1, 2, 3, 4), 4)); }')

    test_section_begin('array')
    # array
    add_test(3, r'int main() { int x[2]; int *y=x; *y=3; return *x; }')
    add_test(
        3, r'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *x; }')
    add_test(
        4, r'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+1); }')
    add_test(
        5, r'int main() { int x[3]; *x=3; *(x+1)=4; *(x+2)=5; return *(x+2); }')

    add_test(
        10, r'int main() { int x[2][3]; int *y=x; *y=10; return **x; }')
    add_test(
        1, r'int main() { int x[2][3]; int *y=x; *(y+1)=1; return *(*x+1); }')
    add_test(
        2, r'int main() { int x[2][3]; int *y=x; *(y+2)=2; return *(*x+2); }')
    add_test(
        3, r'int main() { int x[2][3]; int *y=x; *(y+3)=3; return **(x+1); }')
    add_test(
        4, r'int main() { int x[2][3]; int *y=x; *(y+4)=4; return *(*(x+1)+1); }')
    add_test(
        5, r'int main() { int x[2][3]; int *y=x; *(y+5)=5; return *(*(x+1)+2); }')

    add_test(
        3, r'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *x; }')
    add_test(
        4, r'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+1); }')
    add_test(
        5, r'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }')
    add_test(
        5, r'int main() { int x[3]; *x=3; x[1]=4; x[2]=5; return *(x+2); }')
    add_test(
        5, r'int main() { int x[3]; *x=3; x[1]=4; 2[x]=5; return *(x+2); }')
    add_test(
        0, r'int main() { int x[2][3]; int *y=x; y[0]=0; return x[0][0]; }')
    add_test(
        1, r'int main() { int x[2][3]; int *y=x; y[1]=1; return x[0][1]; }')
    add_test(
        2, r'int main() { int x[2][3]; int *y=x; y[2]=2; return x[0][2]; }')
    add_test(
        3, r'int main() { int x[2][3]; int *y=x; y[3]=3; return x[1][0]; }')
    add_test(
        4, r'int main() { int x[2][3]; int *y=x; y[4]=4; return x[1][1]; }')
    add_test(
        5, r'int main() { int x[2][3]; int *y=x; y[5]=5; return x[1][2]; }')

    test_section_begin('sizeof')
    # sizeof
    add_test(8, r'int main() { int x; return sizeof(x); }')
    add_test(8, r'int main() { int x; return sizeof x; }')
    add_test(8, r'int main() { int *x; return sizeof(x); }')
    add_test(32, r'int main() { int x[4]; return sizeof(x); }')
    add_test(96, r'int main() { int x[3][4]; return sizeof(x); }')
    add_test(32, r'int main() { int x[3][4]; return sizeof(*x); }')
    add_test(8, r'int main() { int x[3][4]; return sizeof(**x); }')
    add_test(9, r'int main() { int x[3][4]; return sizeof(**x) + 1; }')
    add_test(9, r'int main() { int x[3][4]; return sizeof **x + 1; }')
    add_test(8, r'int main() { int x[3][4]; return sizeof(**x + 1); }')
    add_test(8, r'int main() { int x=1; return sizeof(x=2); }')
    add_test(1, r'int main() { int x=1; sizeof(x=2); return x; }')

    test_section_begin('global')
    # global variables
    add_test(0, r'int x; int main() { return x; }')
    add_test(3, r'int x; int main() { x=3; return x; }')
    add_test(7, r'int x; int y; int main() { x=3; y=4; return x+y; }')
    add_test(7, r'int x, y; int main() { x=3; y=4; return x+y; }')
    add_test(
        0, r'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[0]; }')
    add_test(
        1, r'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[1]; }')
    add_test(
        2, r'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[2]; }')
    add_test(
        3, r'int x[4]; int main() { x[0]=0; x[1]=1; x[2]=2; x[3]=3; return x[3]; }')
    add_test(8, r'int x; int main() { return sizeof(x); }')
    add_test(32, r'int x[4]; int main() { return sizeof(x); }')

    test_section_begin('char')
    # char
    add_test(1, r'int main() { char x=1; return x; }')
    add_test(1, r'int main() { char x=1; char y=2; return x; }')
    add_test(2, r'int main() { char x=1; char y=2; return y; }')
    add_test(1, r'int main() { char x; return sizeof(x); }')
    add_test(10, r'int main() { char x[10]; return sizeof(x); }')
    add_test(
        1, r'int main() { return sub_char(7, 3, 3); } int sub_char(char a, char b, char c) { return a-b-c; }')

    test_section_begin('string')
    # string literal
    add_test(0, r'int main() { return ""[0]; }')
    add_test(1, r'int main() { return sizeof(""); }')
    add_test(97, r'int main() { return "abc"[0]; }')
    add_test(98, r'int main() { return "abc"[1]; }')
    add_test(99, r'int main() { return "abc"[2]; }')
    add_test(0, r'int main() { return "abc"[3]; }')
    add_test(4, r'int main() { return sizeof("abc"); }')

    # escape sequence
    add_test(7, r'int main() { return "\a8\b\t"[0]; }')
    add_test(48, r'int main() { return "\a0\b\t"[1]; }')
    add_test(8, r'int main() { return "\a0\b\t"[2]; }')
    add_test(9, r'int main() { return "\a0\b\t"[3]; }')

    add_test(10, r'int main() { return "\n"[0]; }')
    add_test(11, r'int main() { return "\v"[0]; }')
    add_test(12, r'int main() { return "\f"[0]; }')
    add_test(13, r'int main() { return "\r"[0]; }')
    add_test(27, r'int main() { return "\e"[0]; }')
    add_test(106, r'int main() { return "\j"[0]; }')
    add_test(107, r'int main() { return "\k"[0]; }')
    add_test(108, r'int main() { return "\l"[0]; }')
    add_test(7, r'int main() { return "\ax\ny"[0]; }')
    add_test(120, r'int main() { return "\ax\ny"[1]; }')
    add_test(10, r'int main() { return "\ax\ny"[2]; }')
    add_test(121, r'int main() { return "\ax\ny"[3]; }')
    add_test(5, r'int main() { return sizeof("\ax\ny"); }')

    add_test(0, r'int main() { return "\0"[0]; }')
    add_test(16, r'int main() { return "\20"[0]; }')
    add_test(65, r'int main() { return "\101"[0]; }')
    add_test(104, r'int main() { return "\1500"[0]; }')

    add_test(0, r'int main() { return "\x00"[0]; }')
    add_test(119, r'int main() { return "\x77"[0]; }')
    # add_test(165, r'int main() { return "\xA5"[0]; }')
    # add_test(255, r'int main() { return "\x00ff"[0]; }')


if __name__ == "__main__":
    test_main()
    if len(sys.argv) > 1:
        run_tests(sys.argv[1])
    else:
        run_tests(None)
    print('Ok')
