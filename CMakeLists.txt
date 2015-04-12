cmake_minimum_required(VERSION 2.8.8)

# Set compiler.
# Set compiler options.
set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
add_definitions("-std=c++11 -Wall -Wextra")

set(SYM_ENGINE "SymEngine")

project(SYM_ENGINE)

set(LIB_DIR "lib")
set(INCLUDE_DIR "include")
set(EXT_DIR "ext")
set(GTEST_DIR "gtest")
set(TEST_DIR "test")
set(SYM_ENGINE_CORE "SymEngineCore")

set(GTEST_PATH ${PROJECT_SOURCE_DIR}/${EXT_DIR}/${GTEST_DIR})

execute_process(COMMAND llvm-config --obj-root OUTPUT_VARIABLE LLVM_ROOT_DIR)

set(LLVM_DIR ${LLVM_ROOT_DIR}"/share/llvm/cmake")

find_package(LLVM REQUIRED)

# Include dirs.
include_directories(${INCLUDE_DIR})
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})

# Link dirs.
link_directories(${LLVM_LIBRARY_DIRS})
# These libs are used only by SymEngineTesting only. Not by the actualy loadable module.
set(LLVM_LIBS Core TransformUtils IRReader X86Info ExecutionEngine)
llvm_map_components_to_libnames(REQ_LLVM_LIBS ${LLVM_LIBS})

# Definitions.
add_definitions("-D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS")

enable_testing()

# Subdir.
add_subdirectory(${LIB_DIR})
add_subdirectory(${TEST_DIR})
