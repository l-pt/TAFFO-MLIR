#!/bin/sh

# Transform FIR to standard MLIR dialects, do TAFFO passes

DIR="$( cd -P -- "$(dirname -- "$(command -v -- "$0")")" && pwd -P )"
exec "$DIR"/../build/bin/taffo-opt --vra-mode=interval \
  --pass-pipeline='builtin.module(canonicalize,cse,func.func(fir-allocatables-analysis),fir-to-mlir,canonicalize,cse,raise-to-taffo,value-range-analysis,lower-to-arith)' \
  "$@"
