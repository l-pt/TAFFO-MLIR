#!/bin/sh
set -eu
DIR="$( cd -P -- "$(dirname -- "$(command -v -- "$0")")" && pwd -P )"
cd "$DIR" || exit 100
exec clang -O3 -march=native -g -c -DPOLYBENCH_TIME utilities/fpolybench.c -o utilities/fpolybench.o
