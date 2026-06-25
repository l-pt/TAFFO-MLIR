cmake_minimum_required(VERSION 3.13.4)

# CMP0116: Ninja generators transform `DEPFILE`s from `add_custom_command()`
# New in CMake 3.20. https://cmake.org/cmake/help/latest/policy/CMP0116.html
if(POLICY CMP0116)
  cmake_policy(SET CMP0116 OLD)
endif()

project(TaffoMLIR LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to conform to")

if(DEFINED LLVM_PATH)
  get_filename_component(LLVM_ABS_PATH ${LLVM_PATH} ABSOLUTE)
  set(LLVM_DIR "${LLVM_ABS_PATH}/lib/cmake/llvm")
  set(MLIR_DIR "${LLVM_ABS_PATH}/lib/cmake/mlir")
  set(Clang_DIR "${LLVM_ABS_PATH}/lib/cmake/clang")
  set(Flang_DIR "${LLVM_ABS_PATH}/lib/cmake/flang")
endif()

find_package(LLVM REQUIRED CONFIG)
find_package(MLIR REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)
find_package(Flang REQUIRED CONFIG)

message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/lib)
set(MLIR_BINARY_DIR ${CMAKE_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(TableGen)
include(AddLLVM)
include(AddMLIR)
include(HandleLLVMOptions)

include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${MLIR_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_BINARY_DIR}/include)
link_directories(${LLVM_BUILD_LIBRARY_DIR})
add_definitions(${LLVM_DEFINITIONS})

set(TAFFO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")
include(sanitizers)

add_subdirectory(include)
add_subdirectory(lib)
add_subdirectory(taffo-opt)

#------------------------------------------------------------------------------
# Tests
#------------------------------------------------------------------------------

set(LIT_ARGS_DEFAULT "-sv")

if(MSVC_IDE OR XCODE)
  set(LIT_ARGS_DEFAULT "${LIT_ARGS_DEFAULT} --no-progress-bar")
endif()

set(LLVM_LIT_ARGS "${LIT_ARGS_DEFAULT}" CACHE STRING "Default options for lit")

add_subdirectory(test)

# Set project git hooks only outside CI.
if(NOT DEFINED ENV{GITHUB_WORKSPACE})
  include(FindClangFormat)
  set(PROJECT_GIT_HOOKS_DIR "${CMAKE_SOURCE_DIR}/.githooks")
  execute_process(
    COMMAND git config --local core.hooksPath "${PROJECT_GIT_HOOKS_DIR}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE _git_cfg_res
  )
  if(NOT _git_cfg_res EQUAL 0)
    message(WARNING "Failed to set core.hooksPath (exit code ${_git_cfg_res})")
  endif()
endif()
