#!/bin/sh

# Transform FIR to standard MLIR dialects

DIR="$( cd -P -- "$(dirname -- "$(command -v -- "$0")")" && pwd -P )"
exec "$DIR"/../build/bin/taffo-opt \
  --pass-pipeline='builtin.module(canonicalize,cse,func.func(fir-allocatables-analysis),fir-to-mlir,canonicalize,cse)' \
  "$@"
