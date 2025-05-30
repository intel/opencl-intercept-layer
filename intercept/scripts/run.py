#
# Copyright (c) 2023-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

import numpy as np
import pyopencl as cl
import glob as gl
import re
import hashlib
import struct
import os
import argparse
from collections import defaultdict

def get_image_metadata(idx: int):
    fileName = f"./Image_MetaData_{idx}.txt"
    with open(fileName) as metadata:
        lines = metadata.readlines()

    image_type = int(lines[8])
    if image_type in [cl.mem_object_type.IMAGE1D]:
        shape = [int(lines[0])]
    elif image_type in [cl.mem_object_type.IMAGE2D]:
        shape = [int(lines[0]), int(lines[1])]
    elif image_type in [cl.mem_object_type.IMAGE3D]:
        shape = [int(lines[0]), int(lines[1]), int(lines[2])]
    else:
        print('Unsupported image type for playback!')
        shape = [int(lines[0]), int(lines[1]), int(lines[2])]

    format = cl.ImageFormat(int(lines[7]), int(lines[6]))
    return format, shape

def sampler_from_string(ctx, sampler_descr):
    normalized = True if "TRUE" in sampler_descr else False
    addressing_mode = cl.addressing_mode.CLAMP_TO_EDGE   if "CLAMP_TO_EDGE" in sampler_descr   else \
                      cl.addressing_mode.CLAMP           if "CLAMP" in sampler_descr           else \
                      cl.addressing_mode.MIRRORED_REPEAT if "MIRRORED_REPEAT" in sampler_descr else \
                      cl.addressing_mode.REPEAT          if "REPEAT" in sampler_descr          else \
                      cl.addressing_mode.NONE

    filter_mode = cl.filter_mode.LINEAR                  if "LINEAR" in sampler_descr          else \
                  cl.filter_mode.NEAREST
    return cl.Sampler(ctx, False, addressing_mode, filter_mode)

def replay(repetitions):
    # Read the enqueue number from the file
    with open('./enqueueNumber.txt') as file:
        enqueue_number = file.read().splitlines()[0]

    padded_enqueue_num = str(enqueue_number).rjust(4, "0")

    arguments = {}
    argument_files = gl.glob("./Argument*.bin")
    for argument in argument_files:
        idx = int(re.findall(r'\d+', argument)[0])
        arguments[idx] = np.fromfile(argument, dtype='uint8').tobytes()

    buffer_idx = []
    input_buffers = {}
    output_buffers = {}
    buffer_files = gl.glob("./Pre/Enqueue_" + padded_enqueue_num + "*.bin")
    input_buffer_ptrs = defaultdict(list)
    for buffer in buffer_files:
        start = buffer.find("_Arg_")
        idx = int(re.findall(r'\d+', buffer[start:])[0])
        buffer_idx.append(idx)
        input_buffers[idx] = np.fromfile(buffer, dtype='uint8').tobytes()
        input_buffer_ptrs[arguments[idx]].append(idx)
        output_buffers[idx] = np.empty_like(input_buffers[idx])

    image_idx = []
    input_images = {}
    output_images = {}
    image_files = gl.glob("./Pre/Enqueue_" + padded_enqueue_num + "*.raw")
    input_images_ptrs = defaultdict(list)
    for image in image_files:
        start = image.find("_Arg_")
        idx = int(re.findall(r'\d+', image[start:])[0])
        image_idx.append(idx)
        input_images[idx] = np.fromfile(image, dtype='uint8').tobytes()
        input_images_ptrs[arguments[idx]].append(idx)
        output_images[idx] = np.empty_like(input_images[idx])

    local_sizes = {}
    local_files = gl.glob("./Local*.txt")
    for local in local_files:
        with open(local) as file:
            size = int(file.read())
        local_sizes[int(re.findall(r'\d+', local)[0])] = size

    # Check if we have pointer aliasing for the buffers
    tmp_args = []
    for idx in buffer_idx:
        tmp_args.append(arguments[idx])

    # Check if all input pointer addresses are unique
    if len(tmp_args) != len(set(tmp_args)):
        print("Some of the buffers are aliasing, we will replicate this behavior.")

    ctx = cl.create_some_context()
    queue = cl.CommandQueue(ctx)
    device = queue.get_info(cl.command_queue_info.DEVICE)
    platform = device.get_info(cl.device_info.PLATFORM)

    print(f"Running on platform: {platform.get_info(cl.platform_info.NAME)}")
    print(f"Running on device: {device.get_info(cl.device_info.NAME)}")

    samplers = {}
    sampler_files = gl.glob("./Sampler*.txt")
    for sampler in sampler_files:
        idx = int(re.findall(r'\d+', sampler)[0])
        with open(sampler) as file:
            samplers[idx] = sampler_from_string(ctx, file.readline())

    # Make sure that we only set the arguments to the non-buffer parameters
    for idx in list(input_buffers):
        del arguments[idx]
    for idx in list(input_images):
        del arguments[idx]
    for idx in list(samplers):
        del arguments[idx]

    mf = cl.mem_flags

    gpu_buffers = {}
    for idxs in input_buffer_ptrs.values():
        gpu_buffers[tuple(idxs)] = cl.Buffer(ctx, mf.COPY_HOST_PTR, hostbuf=input_buffers[idxs[0]])

    gpu_images = {}
    for idx in image_idx:
        format, shape = get_image_metadata(idx)
        gpu_images[idx] = cl.Image(ctx, mf.COPY_HOST_PTR, format, shape, hostbuf=input_images[idx])

    with open("buildOptions.txt", 'r') as file:
        options = [line.rstrip() for line in file]
        print(f"Using build options: {options}")

    with open('kernelName.txt') as file:
        kernel_name = file.read()

    if os.path.isfile("kernel.cl"):
        print("Using kernel source")
        with open("kernel.cl", 'r') as file:
            kernel = file.read()
        prg = cl.Program(ctx, kernel).build(options)
    elif os.path.isfile("kernel.spv"):
        print("Using kernel IL")
        with open("kernel.spv", 'r') as file:
            kernel = np.fromfile(file, dtype='uint8').tobytes()
        prg = cl.Program(ctx, kernel).build(options)
    else:
        print("Using kernel device binary")
        binary_files = gl.glob("./DeviceBinary*.bin")
        binaries = []
        for file in binary_files:
            binaries.append(np.fromfile(file, dtype='uint8').tobytes())

        # Try the binaries to find one that works
        for idx in range(len(binaries)):
            try:
                prg = cl.Program(ctx, [device], [binaries[idx]]).build(options)
                getattr(prg, kernel_name)
                print(f"Successfully loaded kernel device binary file: {binary_files[idx]}")
                break
            except Exception as e:
                print(f"Failed to load kernel device binary file: {binary_files[idx]}")
                pass

    kernel = getattr(prg, kernel_name)
    for pos, argument in arguments.items():
        kernel.set_arg(pos, argument)

    for pos, buffer in gpu_buffers.items():
        for idx in pos:
            kernel.set_arg(idx, buffer)

    for pos, image in gpu_images.items():
        kernel.set_arg(pos, image)

    for pos, size in local_sizes.items():
        kernel.set_arg(pos, cl.LocalMemory(size))

    for pos, sampler in samplers.items():
        kernel.set_arg(pos, sampler)

    gws = []
    lws = []
    gwo = []

    with open("worksizes.txt", 'r') as file:
        lines = file.read().splitlines()

    gws.extend([int(value) for value in lines[0].split()])
    lws.extend([int(value) for value in lines[1].split()])
    gwo.extend([int(value) for value in lines[2].split()])

    print(f"Global Work Size: {gws}")
    print(f"Local Work Size: {lws}")
    print(f"Global Work Offsets: {gwo}")

    if lws == [0] or lws == [0, 0] or lws == [0, 0, 0]:
        lws = None

    for _ in range(args.repetitions):
        cl.enqueue_nd_range_kernel(queue, kernel, gws, lws, gwo)

    for pos in gpu_buffers.keys():
        if len(pos) == 1:
            cl.enqueue_copy(queue, output_buffers[pos[0]], gpu_buffers[pos])
        else:
            for idx in range(len(pos)):
                cl.enqueue_copy(queue, output_buffers[pos[idx]], gpu_buffers[pos])

    for pos in gpu_images.keys():
        cl.enqueue_copy(queue, output_images[pos], gpu_images[pos], region=shape, origin=(0,0,0))

    if not os.path.exists("./Test"):
        os.makedirs("./Test")

    for pos, cpu_buffer in output_buffers.items():
        outbuf = "./Test/Enqueue_" + padded_enqueue_num + "_Kernel_" + kernel_name + "_Arg_" + str(pos) + "_Buffer.bin"
        print(f"Writing buffer output to file: {outbuf}")
        cpu_buffer.tofile(outbuf)

    for pos, cpu_image in output_images.items():
        outimg = "./Test/Enqueue_" + padded_enqueue_num + "_Kernel_" + kernel_name + "_Arg_" + str(pos) + "_Image.raw"
        print(f"Writing image output to file: {outimg}")
        cpu_image.tofile(outimg)

def validate():
    # Read the enqueue number from the file
    with open('./enqueueNumber.txt') as file:
        enqueue_number = file.read().splitlines()[0]

    # Pad the enqueue number to at least 4 digits
    padded_enqueue_num = str(enqueue_number).rjust(4, "0")

    # Compare the buffers for binary equality
    replayed_buffers = gl.glob("./Test/Enqueue_" + padded_enqueue_num + "_Kernel_*.bin")
    replayed_hashes = {}
    for replayed_buffer in replayed_buffers:
        start = replayed_buffer.find("_Arg_")
        idx = int(re.findall(r'\d+', replayed_buffer[start:])[0])
        replayed_hashes[idx] = hashlib.md5(np.fromfile(replayed_buffer)).hexdigest()

    # Compare the images for binary equality
    replayed_images = gl.glob("./Test/Enqueue_" + padded_enqueue_num + "_Kernel_*.raw")
    for replayed_image in replayed_images:
        start = replayed_image.find("_Arg_")
        idx = int(re.findall(r'\d+', replayed_image[start:])[0])
        replayed_hashes[idx] = hashlib.md5(np.fromfile(replayed_image)).hexdigest()

    try:
        with open('./ArgumentDataTypes.txt') as file:
            data_types = [line.rsplit() for line in file.readlines()]
    except:
        print("Information about argument data types not available!")

    dumped_buffers = gl.glob("./Post/Enqueue_" + padded_enqueue_num + "_Kernel_*.bin")
    dumped_hashes = {}
    for dumped_buffer in dumped_buffers:
        start = dumped_buffer.find("_Arg_")
        idx = int(re.findall(r'\d+', dumped_buffer[start:])[0])
        dumped_hashes[idx] = hashlib.md5(np.fromfile(dumped_buffer)).hexdigest()

    dumped_images = gl.glob("./Post/Enqueue_" + padded_enqueue_num + "_Kernel_*.raw")
    for dumped_image in dumped_images:
        start = dumped_image.find("_Arg_")
        idx = int(re.findall(r'\d+', dumped_image[start:])[0])
        dumped_hashes[idx] = hashlib.md5(np.fromfile(dumped_image)).hexdigest()

    all_equal = True
    for pos in sorted(replayed_hashes.keys()):
        if replayed_hashes[pos] == dumped_hashes[pos]:
            print(f"Check: Argument {pos} is equal.")
        else:
            try:
                print(f"Check: Argument {pos} is not equal, data type={data_types[pos]}!")
            except:
                print(f"Check: Argument {pos} is not equal!")
            all_equal = False

    print()
    if all_equal:
        print("Replayed standalone kernel produces correct results.")
    else:
        print("Replayed standalone kernel differs from app's result, \n"
            "this may due a slightly different order of operations on floating point \n"
            "numbers. Please check manually if the differences are significant. \n"
            "If they are completely different, please open an issue on Github \n"
            "so that we can look into it!")

parser = argparse.ArgumentParser(description='Script to replay and validate captured kernels')
parser.add_argument('-r', '--repetitions', type=int, dest='repetitions', default=1,
                    help='How often the kernel should be enqueued')
parser.add_argument('-s', '--skipreplay', action='store_true', dest='skip', default=False,
                    help='Skip replaying the captured kernel and do not dump data')
parser.add_argument('-v', '--validate', action='store_true', dest='validate', default=False,
                    help='Validate the replayed kernel against the dumped data')
args = parser.parse_args()

if args.skip:
    print("Skipping replay of the captured kernel.")
else:
    replay(args.repetitions)

if args.validate:
    validate()
else:
    print("Skipping validation of the replayed kernel against the dumped data.")
