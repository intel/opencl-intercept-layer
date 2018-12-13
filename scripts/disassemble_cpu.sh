#!/bin/sh

# Copyright (c) 2018 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set -e

if [ "$#" -ne 3 ]; then
    echo "Usage: disassemble_cpu.sh <opencl_binary.bin> <opencl_binary.isabin> <opencl_binary.isa>"
    exit 1
fi

ocl_elf=$1
ocl_obj=$2
ocl_isa=$3
section=.ocl.obj

start_hex=$(readelf -a $ocl_elf | grep $section | tr -s ' ' | cut -f7 -d' ')
size_hex=$(readelf -a $ocl_elf | grep $section -A1 | tail -n1 | tr -s ' ' | cut -f2 -d' ')

start=$(printf "%d" 0x$start_hex)
size=$(printf "%d" 0x$size_hex)

dd skip=$start count=$size if=$ocl_elf of=$ocl_obj bs=1
objdump -d -m i386:x86-64 $ocl_obj > $ocl_isa
