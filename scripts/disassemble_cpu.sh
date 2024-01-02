#!/bin/sh

#
# Copyright (c) 2018-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

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
