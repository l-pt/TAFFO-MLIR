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
    object_file_path: pathlib.Path
    debug: bool
    # tempfiles
    fir_fp: tempfile.NamedTemporaryFile
    fir_path: pathlib.Path
    mlir_fp: tempfile.NamedTemporaryFile
    mlir_path: pathlib.Path
    llvmir_fp: tempfile.NamedTemporaryFile
    llvmir_path: pathlib.Path
    llvmir_opt_fp: tempfile.NamedTemporaryFile
    llvmir_opt_path: pathlib.Path
    obj_fp: tempfile.NamedTemporaryFile
    obj_path: pathlib.Path

def get_args():
    parser = argparse.ArgumentParser(prog = 'compile.py', description = 'Compile a fortran program with taffo-opt and produce an executable file')
    parser.add_argument('-d', action='store_true')
    parser.add_argument('-l', type=pathlib.Path)
    parser.add_argument('infile', type=pathlib.Path)
    parser.add_argument('outfile', nargs='?', type=pathlib.Path)
    return parser.parse_args()

def fortran_to_fir(state):
    argv = [
        str(state.llvm_path / 'bin' / 'flang'),
        '-fc1',
        '-emit-fir',
        str(state.infile),
        '-o', str(state.fir_path)
    ]
    print('running', argv)
    completed = subprocess.run(argv)
    return completed.returncode

def fir_to_mlir(state):
    passes = [
        'canonicalize',
        'cse',
        'external-name-interop',
        'func.func(fir-allocatables-analysis)',
        'fir-to-mlir',
        'canonicalize',
        'cse',
        # 'raise-to-taffo',
        # 'value-range-analysis',
        # 'lower-to-arith',
        'loop-invariant-code-motion',
        'convert-linalg-to-loops',
        'convert-scf-to-cf',
        'convert-cf-to-llvm{index-bitwidth=64}',
        'expand-strided-metadata',
        'finalize-memref-to-llvm',
        'convert-arith-to-llvm{index-bitwidth=64}',
        'convert-index-to-llvm{index-bitwidth=64}',
        'convert-func-to-llvm',
        'convert-math-to-llvm',
        'reconcile-unrealized-casts',
    ]
    passes_arg = '--pass-pipeline=builtin.module(' + ','.join(passes) + ')'
    argv = [
        str(state.taffo_opt_path),
        passes_arg,
        str(state.fir_path),
    ]
    if state.debug:
        argv += ['--debug']
    argv += ['-o', str(state.mlir_path)]
    print('running', argv)
    completed = subprocess.run(argv)
    return completed.returncode

def mlir_to_llvmir(state):
    argv = [
        str(state.llvm_path / 'bin' / 'mlir-translate'),
        '--mlir-to-llvmir',
        str(state.mlir_path),
        '-o', str(state.llvmir_path)
    ]
    print('running', argv)
    completed = subprocess.run(argv)
    return completed.returncode

def opt_llvmir(state):
    argv = [
        str(state.llvm_path / 'bin' / 'opt'),
        '-O3',
        str(state.llvmir_path),
        '-o', str(state.llvmir_opt_path)
    ]
    print('running', argv)
    completed = subprocess.run(argv)
    return completed.returncode

def llvmir_to_asm(state):
    argv = [
        str(state.llvm_path / 'bin' / 'llc'),
        '-O3',
        '--filetype=obj',
        '--relocation-model=pic',
        str(state.llvmir_opt_path),
    ]
    argv += [
        '-o', str(state.obj_path)
    ]
    print('running', argv)
    completed = subprocess.run(argv)
    return completed.returncode

def asm_to_exe(state):
    argv = [str(state.llvm_path / 'bin' / 'flang'), '-o', str(state.outfile), str(state.obj_path)]
    if state.object_file_path is not None:
        argv += [str(state.object_file_path)]
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
    llvmir_opt_fp = tempfile.NamedTemporaryFile()
    obj_fp = tempfile.NamedTemporaryFile()

    state = State(
        infile,
        outfile,
        pathlib.Path(llvm_path),
        pathlib.Path(taffo_opt_path),
        args.l,
        args.d,
        fir_fp,
        pathlib.Path(fir_fp.name),
        mlir_fp,
        pathlib.Path(mlir_fp.name),
        llvmir_fp,
        pathlib.Path(llvmir_fp.name),
        llvmir_opt_fp,
        pathlib.Path(llvmir_opt_fp.name),
        obj_fp,
        pathlib.Path(obj_fp.name),
    )
    if (e := fortran_to_fir(state)) != 0:
        return e
    if (e := fir_to_mlir(state)) != 0:
        print('File contents:')
        print(state.fir_fp.read().decode())
        return e
    if (e := mlir_to_llvmir(state)) != 0:
        print('File contents:')
        print(state.mlir_fp.read().decode())
        return e
    if (e := opt_llvmir(state)) != 0:
        print('File contents:')
        print(state.llvmir_fp.read().decode())
        return e
    if (e := llvmir_to_asm(state)) != 0:
        return e
    if (e := asm_to_exe(state)) != 0:
        return e

if __name__ == '__main__':
    exit(main())
