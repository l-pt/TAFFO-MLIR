# An out-of-tree MLIR dialect for precision tuning
## Overview

This project provides an out-of-tree MLIR dialect designed for precision tuning. It includes custom operations, types, and transformations to facilitate floating-point to fix-point transformation

## Features

- Custom MLIR dialect for precision tuning
- Operations for casting and arithmetic with precision control
- Integration with MLIR's pass infrastructure
- Example passes for lowering to arithmetic operations

## Getting Started

### Prerequisites

- LLVM and MLIR (Compile from source using these [compile flags](llvm_compile_flags.txt) and [commit id](llvm_commit.txt))
- CMake
- Ninja (optional, but recommended)

### Building

1. Clone the repository:
  ```sh
  git clone https://github.com/your-repo/TAFFO-MLIR.git
  cd TAFFO-MLIR
  ```

2. Configure and build the project:
  ```sh
  mkdir build
  cd build
  cmake -G Ninja ..
  ninja
  ```

### Running Tests

To run the tests, use the following command:
```sh
./scripts/run_vra_tests.bash
```

## License

This project is licensed under the Apache License v2.0 with LLVM Exceptions. See the [LICENSE](LICENSE.txt) file for details.
