#!/usr/bin/env python3

import subprocess
import pathlib
import argparse
import os
import tempfile
from dataclasses import dataclass
from concurrent.futures import ProcessPoolExecutor
from concurrent.futures import as_completed

preprocessor_args = [
    '-I', 'utilities',
    '-DPOLYBENCH_STACK_ARRAYS',
    '-DPOLYBENCH_TIME',
    '-DPOLYBENCH_DUMP_ARRAYS',
    '-DDATA_TYPE=real',
    """-DDATA_PRINTF_MODIFIER="(ES24.16E3,1X)", advance='no'""",
    # """-DDATA_PRINTF_MODIFIER="(f0.2,1x)", advance='no'""",
]

@dataclass
class State:
    # positional args
    benchmark_file: pathlib.Path
    runall: bool
    # env
    llvm_path: pathlib.Path
    taffo_opt_path: pathlib.Path
    # options
    verbose: bool

def print_verbose(verbose, *args, **kwargs):
    if verbose:
        print(*args, **kwargs)

def get_args():
    parser = argparse.ArgumentParser(prog = 'run_bench.py', description = 'Compile and run a benchmark')
    parser.add_argument('benchmark_file')
    parser.add_argument('-v', action='store_true')
    return parser.parse_args()

def create_preprocessed(state, benchmark_file):
    print_verbose(state.verbose, 'Creating pre-processed code')
    argv = ['utilities/create_pped_version.sh', str(benchmark_file)]
    argv += preprocessor_args
    print_verbose(state.verbose, 'running', argv)
    completed = subprocess.run(argv)
    return completed.returncode

def compile_flang(state, benchmark_file):
    preproc_file = benchmark_file.with_suffix('.preproc.f90')
    out_file = benchmark_file.with_suffix('.out-flang')
    argv = [
        str(state.llvm_path / 'bin' / 'flang'),
        '-O3',
        '-g',
        'utilities/fpolybench.o',
        str(preproc_file),
        '-o', str(out_file)
    ]
    print_verbose(state.verbose, 'Running', argv)
    completed = subprocess.run(argv)
    return completed.returncode

def compile_taffo(state, benchmark_file):
    preproc_file = benchmark_file.with_suffix('.preproc.f90')
    out_file = benchmark_file.with_suffix('.out-taffo')
    argv = [
        '../../compile.py',
        '-l', 'utilities/fpolybench.o',
        str(preproc_file),
        str(out_file)
    ]
    print_verbose(state.verbose, 'Running', argv)
    if (state.runall):
        completed = subprocess.run(argv, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    else:
        completed = subprocess.run(argv)
    return completed.returncode

def run_program(state, executable, args):
    argv = [str(executable), *args]
    print_verbose(state.verbose, 'Running', argv)
    try:
        completed = subprocess.run(
            argv,
            capture_output=True,
            text=True,
            timeout=300
        )
    except subprocess.TimeoutExpired:
        print('Execution timed out:', executable)
        return None
    return (completed.stdout, completed.stderr)

def check_correctness(state, benchmark_file):
    exe_flang = benchmark_file.with_suffix('.out-flang')
    exe_taffo = benchmark_file.with_suffix('.out-taffo')

    out_flang = run_program(state, exe_flang, [])
    out_taffo = run_program(state, exe_taffo, [])
    if out_flang is None or out_taffo is None:
        print(benchmark_file.stem, 'FAIL', sep=',')
        return False

    if out_flang[1] == out_taffo[1]:
        print(benchmark_file.stem, 'PASS', sep=',')
        return True
    print(benchmark_file.stem, 'FAIL', sep=',')
    return False

def run_all(state):
    paths = [
        pathlib.Path(line)
        for line in pathlib.Path('utilities/benchmark_list').read_text().splitlines()
    ]
    with ProcessPoolExecutor(max_workers=os.cpu_count()) as executor:
        futures = [
            executor.submit(run_one, state, path)
            for path in paths
        ]
        for future in as_completed(futures):
            future.result()

def run_one(state, path):
    if (e := create_preprocessed(state, path)) != 0:
        print(path.stem, 'SKIP', 'create_preprocessed', sep=',')
        return e
    if (e := compile_flang(state, path)) != 0:
        print(path.stem, 'SKIP', 'compile_flang', sep=',')
        return e
    if (e := compile_taffo(state, path)) != 0:
        print(path.stem, 'SKIP', 'compile_taffo', sep=',')
        return e
    check_correctness(state, path)

def main():
    os.chdir(os.path.dirname(__file__))

    runall = False
    args = get_args()
    if args.benchmark_file == 'all':
        runall = True
    elif not os.path.isfile(pathlib.Path(args.benchmark_file)):
        print('Benchmark', args.benchmark_file, 'not found')
        return 111

    llvm_path = os.getenv('LLVM_PATH')
    if llvm_path is None:
        print('environment variable LLVM_PATH not set')
        return 111

    # used in compile.py
    taffo_opt_path = os.getenv('TAFFO_OPT_PATH')
    if taffo_opt_path is None:
        print('environment variable TAFFO_OPT_PATH not set')
        return 111

    state = State(
        pathlib.Path(args.benchmark_file),
        runall,
        pathlib.Path(llvm_path),
        pathlib.Path(taffo_opt_path),
        args.v
    )

    if state.runall:
        run_all(state)
    else:
        run_one(state, state.benchmark_file)
    return 0

if __name__ == '__main__':
    exit(main())
