#!/bin/bash
sh llvm-build.sh

assert() {
  expected="$1"
  input="$2"

  ./minic-llvm "$input" > tmp.ll || exit
  clang -o tmp tmp.ll -Wno-override-module || exit
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}
