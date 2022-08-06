#!/bin/bash
gcc -Wall -Wextra -Wno-unused-local-typedefs src/list.c \
    src/lexer.c src/parser.c src/codegen-x64.c \
    src/type.c src/misc.c src/main.c \
    -o minic-x64 \
    -mpreferred-stack-boundary=3 # stack aligned to 8
