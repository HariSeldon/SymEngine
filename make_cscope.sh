#! /bin/bash

find . -name '*.cpp' -o -name '*.h' > cscope.files
cscope -b
