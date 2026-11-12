#!/bin/sh
set -eu

# fc1 is the compiler driver: https://flang.llvm.org/docs/FlangDriver.html
exec "$LLVM_PATH/bin/flang" -fc1 -emit-fir "$1"
