#!/usr/bin/env python3

# Compile a fortran program with taffo-opt and produce an executable file

import subprocess
import pathlib
import argparse
import os
import tempfile
from dataclasses import dataclass

@dataclass
class State:
    infile: pathlib.Path
    outfile: pathlib.Path
    llvm_path: pathlib.Path
    taffo_opt_path: pathlib.Path
    # tempfiles
    fir_fp: tempfile.NamedTemporaryFile
    fir_path: pathlib.Path
    mlir_fp: tempfile.NamedTemporaryFile
    mlir_path: pathlib.Path
    llvmir_fp: tempfile.NamedTemporaryFile
    llvmir_path: pathlib.Path
    obj_fp: tempfile.NamedTemporaryFile
    obj_path: pathlib.Path

def get_args():
    parser = argparse.ArgumentParser(prog = 'compile.py', description = 'Compile a fortran program with taffo-opt and produce an executable file')
    parser.add_argument('infile', type=pathlib.Path)
    parser.add_argument('outfile', nargs='?', type=pathlib.Path)
    return parser.parse_args()

def fortran_to_fir(state):
    argv = [str(state.llvm_path / 'bin' / 'flang'), '-fc1', '-emit-fir', str(state.infile), '-o', str(state.fir_path)]
    print('running', argv)
    completed = subprocess.run(argv)
    return completed.returncode

def fir_to_mlir(state):
    passes = [
        'canonicalize',
        'cse',
        'fir-to-mlir',
        'loop-invariant-code-motion',
        'convert-linalg-to-loops',
        'convert-scf-to-cf',
        'convert-cf-to-llvm{index-bitwidth=64}',
        'finalize-memref-to-llvm',
        'convert-arith-to-llvm{index-bitwidth=64}',
        'convert-func-to-llvm',
        'convert-math-to-llvm',
        'reconcile-unrealized-casts',
    ]
    passes_arg = '--pass-pipeline=builtin.module(' + ','.join(passes) + ')'
    argv = [str(state.taffo_opt_path), passes_arg, str(state.fir_path), '-o', str(state.mlir_path)]
    print('running', argv)
    completed = subprocess.run(argv)
    return completed.returncode

def mlir_to_llvmir(state):
    argv = [str(state.llvm_path / 'bin' / 'mlir-translate'), '--mlir-to-llvmir', str(state.mlir_path), '-o', str(state.llvmir_path)]
    print('running', argv)
    completed = subprocess.run(argv)
    return completed.returncode

def llvmir_to_asm(state):
    argv = [str(state.llvm_path / 'bin' / 'llc'), '--filetype=obj', str(state.llvmir_path), '-o', str(state.obj_path)]
    print('running', argv)
    completed = subprocess.run(argv)
    return completed.returncode

def asm_to_exe(state):
    argv = [str(state.llvm_path / 'bin' / 'flang'), '-no-pie', str(state.obj_path), '-o', str(state.outfile)]
    print('running', argv)
    completed = subprocess.run(argv)
    return completed.returncode

def main(): 
    args = get_args()

    infile = args.infile
    if not infile.is_file():
        print('error: not a file', infile)
        return 111

    if args.outfile is None:
        outfile = pathlib.Path.cwd() / 'a.out'
    else:
        outfile = args.outfile

    llvm_path = os.getenv('LLVM_PATH')
    if llvm_path is None:
        print('environment variable LLVM_PATH not set')
        return 111

    taffo_opt_path = os.getenv('TAFFO_OPT_PATH')
    if taffo_opt_path is None:
        print('environment variable TAFFO_OPT_PATH not set')
        return 111

    fir_fp = tempfile.NamedTemporaryFile()
    mlir_fp = tempfile.NamedTemporaryFile()
    llvmir_fp = tempfile.NamedTemporaryFile()
    obj_fp = tempfile.NamedTemporaryFile()

    state = State(
        infile,
        outfile,
        pathlib.Path(llvm_path),
        pathlib.Path(taffo_opt_path),
        fir_fp,
        pathlib.Path(fir_fp.name),
        mlir_fp,
        pathlib.Path(mlir_fp.name),
        llvmir_fp,
        pathlib.Path(llvmir_fp.name),
        obj_fp,
        pathlib.Path(obj_fp.name),
    )
    if (e := fortran_to_fir(state)) != 0:
        return e
    if (e := fir_to_mlir(state)) != 0:
        return e
    if (e := mlir_to_llvmir(state)) != 0:
        return e
    if (e := llvmir_to_asm(state)) != 0:
        return e
    if (e := asm_to_exe(state)) != 0:
        return e

if __name__ == '__main__':
    exit(main())
