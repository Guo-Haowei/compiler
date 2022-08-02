#!/bin/bash
gcc src/list.c src/main.c src/lexer.c -o mini-c
gcc src/list.c src/list-test.c -o list-test
