#! /bin/bash

CLANG=clang
OPT=opt

# Fix the path according to your system configuration.
PROJECT_DIR=$HOME/src/SymEngine/

OCLDEF=$PROJECT_DIR/include/opencl_spir.h
TARGET=spir

INPUT_FILE=$1

# Compile kernel.
$CLANG -x cl \
       -target $TARGET \
       -include $OCLDEF \
       -O3 \
       $INPUT_FILE \
       -S -emit-llvm -fno-builtin -o - 
