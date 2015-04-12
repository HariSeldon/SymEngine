#! /bin/bash

CLANG=clang
OPT=opt

LIB_SYM_ENGINE=$HOME/root/lib/libSymEngine.so
PROJECT_DIR=$HOME/src/SymEngine/

OCLDEF=$PROJECT_DIR/include/opencl_spir.h
TARGET=spir

if [ $# -ne 2 ]
then
  echo "Must specify: input file, kernel name"
exit 1;
fi

INPUT_FILE=$1
KERNEL_NAME=$2

# Compile kernel.
$CLANG -x cl \
       -target $TARGET \
       -include $OCLDEF \
       -O0 \
       $INPUT_FILE \
       -S -emit-llvm -fno-builtin -o - | \
$OPT -dot-cfg-only \
     -mem2reg \
     -inline -inline-threshold=10000 \
     -instnamer -load ${LIB_SYM_ENGINE} \
     -symbolic-execution -symbolic-kernel-name ${KERNEL_NAME}  \
     -full-simulation \
     -S -o /dev/null
