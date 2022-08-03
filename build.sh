#!/bin/bash
gcc -Wall -Wextra src/list.c src/main.c src/lexer.c src/parser.c src/misc.c -o minic
gcc -Wall -Wextra src/list.c src/list-test.c -o list-test
