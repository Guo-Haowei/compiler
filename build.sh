#!/bin/bash
gcc -Wall -Wextra -Wno-unused-local-typedefs src/list.c \
    src/lexer.c src/parser.c src/codegen.c \
    src/misc.c src/main.c -o minic

gcc -Wall -Wextra src/list.c src/list-test.c -o list-test
