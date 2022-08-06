#!/bin/bash
sh x64-build.sh

assert() {
  expected="$1"
  input="$2"

  ./minic-x64 "$input" > tmp.s || exit
  gcc -static -o tmp tmp.s || exit
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}
