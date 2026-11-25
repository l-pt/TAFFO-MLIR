#!/bin/sh

# Perform FIR allocatables analysis pass

DIR="$( cd -P -- "$(dirname -- "$(command -v -- "$0")")" && pwd -P )"
exec "$DIR"/../build/bin/taffo-opt \
  --pass-pipeline='builtin.module(canonicalize,cse,func.func(fir-allocatables-analysis))' \
  "$@"
