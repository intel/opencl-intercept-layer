# Copyright (c) 2023 Intel Corporation
#
# SPDX-License-Identifier: MIT

import numpy as np
import pyopencl as cl
import glob as gl
import re
import hashlib
import struct
import os
import argparse 

parser = argparse.ArgumentParser(description='Script to replay captured kernels')
parser.add_argument('-repetitions', '--rep', type=int, dest='repetitions', default=1,
                    help='How often the kernel should be enqueued')
args = parser.parse_args()

buffer_idx = []
input_buffers = {}
output_buffers = {}
buffer_files = gl.glob("./Buffer*.bin")
for buffer in buffer_files:
    idx = int(re.findall(r'\d+', buffer)[0])
    buffer_idx.append(idx)
    input_buffers[idx] = np.fromfile(buffer, dtype='uint8').tobytes()
    output_buffers[idx] = np.empty_like(input_buffers[idx])

arguments = {}
argument_files = gl.glob("./Argument*.bin")
for argument in argument_files:
    idx = int(re.findall(r'\d+', argument)[0])
    arguments[idx] = np.fromfile(argument, dtype='uint8').tobytes()

# Make sure that we only set the arguments to the non-buffer parameters
for idx in list(input_buffers):
    del arguments[idx]

ctx = cl.create_some_context()
queue = cl.CommandQueue(ctx)
devices = ctx.get_info(cl.context_info.DEVICES)

mf = cl.mem_flags

gpu_buffers = {}
for idx in buffer_idx:
    gpu_buffers[idx] = cl.Buffer(ctx, mf.COPY_HOST_PTR, hostbuf=input_buffers[idx])

with open("buildOptions.txt", 'r') as file:
    flags = [line.rstrip() for line in file]
    print(f"Using flags: {flags}")

with open('knlName.txt') as file:
        knl_name = file.read()

if os.path.isfile("kernel.cl"):
    print("Using kernel source code")
    with open("kernel.cl", 'r') as file:
        kernel = file.read()
    prg = cl.Program(ctx, kernel).build(flags)
else:
    print("Using device binary")
    binary_files = gl.glob("./binary_Device*.bin")
    binaries = []
    for file in binary_files:
        binaries.append(np.fromfile(file, dtype='uint8').tobytes())

    # Try the binaries to find one that works
    for idx in range(len(binaries)):
        try:
            prg = cl.Program(ctx, [devices[0]], [binaries[idx]]).build(flags)
            getattr(prg, knl_name)
            break
        except Exception as e:
            pass

knl = getattr(prg, knl_name)
for pos, argument in arguments.items():
    knl.set_arg(pos, argument)

for pos, buffer in gpu_buffers.items():
    knl.set_arg(pos, buffer)

gws = []
lws = []
gws_offset = []

with open("worksizes.txt", 'r') as file:
    lines = file.read().splitlines()
    
gws.extend([int(value) for value in lines[0].split()])
lws.extend([int(value) for value in lines[1].split()])
gws_offset.extend([int(value) for value in lines[2].split()])
    
print(f"Global Worksize: {gws}")
print(f"Local Worksize: {lws}")
print(f"Global Worksize Offsets: {gws_offset}")
    
if lws == [0] or lws == [0, 0] or lws == [0, 0, 0]:
    lws = None

for _ in range(args.repetitions):
    cl.enqueue_nd_range_kernel(queue, knl, gws, lws, gws_offset)

for pos in gpu_buffers.keys():
    cl.enqueue_copy(queue, output_buffers[pos], gpu_buffers[pos])

for pos, cpu_buffer in output_buffers.items():
    cpu_buffer.tofile("output_buffer" + str(pos) + ".bin")