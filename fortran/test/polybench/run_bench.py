#!/usr/bin/env python3

import subprocess
import pathlib
import argparse
import os
import tempfile
from dataclasses import dataclass

@dataclass
class State:
    benchmark_file: pathlib.Path

def get_args():
    parser = argparse.ArgumentParser(prog = 'run_bench.py', description = 'Compile and run a benchmark')
    parser.add_argument('benchmark_file', type=pathlib.Path)
    return parser.parse_args()

def compile_flang(args):
    print('Creating pre-processed code')
    preprocessor_args = '-I utilities -DPOLYBENCH_STACK_ARRAYS -DPOLYBENCH_TIME -DPOLYBENCH_DUMP_ARRAYS'
    argv = ['utilities/create_pped_version.sh', str(args.benchmark_file), preprocessor_args ]
    print('running', argv)
    completed = subprocess.run(argv)
    if e := completed.returncode != 0:
        return e
    
    preproc_file = args.benchmark_file.with_suffix('.preproc.f90')
    out_file = args.benchmark_file.with_suffix('.out-flang')
    argv = [
        '/opt/llvm-for-tm/bin/flang',
        '-O3',
        '-g',
        'utilities/fpolybench.o',
        str(preproc_file),
        '-o', str(out_file)
    ]
    print('Running', argv)
    completed = subprocess.run(argv)
    return completed.returncode


def compile_taffo(args):
    print('Creating pre-processed code')
    preprocessor_args = '-I utilities -DPOLYBENCH_STACK_ARRAYS -DPOLYBENCH_TIME -DPOLYBENCH_DUMP_ARRAYS'
    argv = ['utilities/create_pped_version.sh', str(args.benchmark_file), preprocessor_args ]
    print('running', argv)
    completed = subprocess.run(argv)
    if e := completed.returncode != 0:
        return e
    
    preproc_file = args.benchmark_file.with_suffix('.preproc.f90')
    out_file = args.benchmark_file.with_suffix('.out-taffo')
    argv = [
        '../../compile.py',
        '-l', 'utilities/fpolybench.o',
        str(preproc_file),
        str(out_file)
    ]
    print('Running', argv)
    completed = subprocess.run(argv)
    return completed.returncode

def main():
    os.chdir(os.path.dirname(__file__))

    args = get_args()
    if not os.path.isfile(args.benchmark_file):
        print('Benchmark', args.benchmark_file, 'not found')
        return 111

    compile_flang(args)
    compile_taffo(args)

    return 0

if __name__ == '__main__':
    exit(main())
