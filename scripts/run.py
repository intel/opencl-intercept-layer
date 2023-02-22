# Copyright (c) 2018-2023 Intel Corporation
#
# SPDX-License-Identifier: MIT

import numpy as np
import pyopencl as cl
import glob as gl
import re
import hashlib
import struct

cpu_buffers = []
buffer_idx = []
buffer_files = gl.glob("./*_Buffer_*")
for buffer in buffer_files:
    buffer_idx.append(int(re.findall(r'\d+', buffer)[1]))
    cpu_buffers.append(np.fromfile(buffer, dtype=np.float32))

arguments = []
argument_idx = []
argument_files = gl.glob("./Argument*.bin")
for argument in argument_files:
    argument_idx.append(int(re.findall(r'\d+', argument)[0]))
    arguments.append(np.fromfile(argument, dtype=bool).tobytes())

# Make sure that we only set the arguments to the non-buffer parameters
argument_idx = list(set(argument_idx) - set(buffer_idx))

ctx = cl.create_some_context()
queue = cl.CommandQueue(ctx)

mf = cl.mem_flags
gpu_buffers = []

for idx in range(len(cpu_buffers)):
    gpu_buffers.append(cl.Buffer(ctx, mf.COPY_HOST_PTR, hostbuf=cpu_buffers[idx]))

with open("kernel.cl", 'r') as file:
    kernel = file.read()

with open("buildOptions.txt", 'r') as file:
    flags = [line.rstrip() for line in file]

print(f"Using flags: {flags}")
prg = cl.Program(ctx, kernel).build(flags)

knl_name = prg.kernel_names
knl = getattr(prg, knl_name)

for idx, pos in enumerate(argument_idx):
    knl.set_arg(pos, arguments[idx])

for idx, pos in enumerate(buffer_idx):
    knl.set_arg(pos, gpu_buffers[idx])

with open("worksizes.txt", 'r') as file:
    lines = [line.rstrip() for line in file]

gws = []
lws = []
gws_offset = []

for idx in range(3):
    gws.append(int(lines[3 * idx]))
    lws.append(int(lines[3 * idx + 1]))
    gws_offset.append(int(lines[3 * idx + 2]))
    
print(f"Global Worksize: {gws}")
print(f"Local Worksize: {lws}")
print(f"Global Worksize Offsets: {gws_offset}")
    
if lws == [0, 0, 0]:
    lws = None
    
cl.enqueue_nd_range_kernel(queue, knl, gws, lws, gws_offset)

for idx in range(len(gpu_buffers)):
    cl.enqueue_copy(queue, cpu_buffers[idx], gpu_buffers[idx])

for idx, cpu_buffer in enumerate(cpu_buffers):
    cpu_buffer.tofile("output_buffer" + buffer_idx[idx] + ".bin")