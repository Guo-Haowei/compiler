#!/bin/bash

gcc -c assert_impl.c -mpreferred-stack-boundary=3 || exit 1
gcc -E arith.c > tmp.c || exit 1
../minic tmp.c || exit 1
gcc -c tmp.s -o arith.o -mpreferred-stack-boundary=3 || exit 1
gcc -o tmp assert_impl.o arith.o -mpreferred-stack-boundary=3 || exit 1

# rm tmp.c
