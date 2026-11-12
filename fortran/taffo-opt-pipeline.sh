#!/bin/sh

# Transform FIR all the way to "llvm" dialect

DIR="$( cd -P -- "$(dirname -- "$(command -v -- "$0")")" && pwd -P )"
exec "$DIR"/../build/bin/taffo-opt \
  --pass-pipeline='builtin.module(canonicalize,cse,fir-to-mlir,loop-invariant-code-motion,convert-linalg-to-loops,convert-scf-to-cf,convert-cf-to-llvm{index-bitwidth=64},finalize-memref-to-llvm,convert-arith-to-llvm{index-bitwidth=64},convert-func-to-llvm,convert-math-to-llvm,reconcile-unrealized-casts)' \
  "$@"
