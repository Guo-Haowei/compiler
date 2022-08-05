#!/bin/bash
gcc -Wall -Wextra -Wno-unused-local-typedefs src/list.c \
    src/lexer.c src/parser.c src/codegen-llvm.c \
    src/type.c src/misc.c src/main.c \
    -o minic