#!/bin/bash
gcc -Wall -Wextra src/list.c src/main.c src/lexer.c -o mini-c
gcc -Wall -Wextra src/list.c src/list-test.c -o list-test
