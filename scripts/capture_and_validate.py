#
# Copyright (c) 2023-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

import os
import argparse
import subprocess
import glob as gl
import re
import hashlib
import numpy as np
import sys

parser = argparse.ArgumentParser(description='Helper script to extract and validate single kernel from OpenCL app.')

# Users can specify either an exact kernel name, xor an enqueue number
group = parser.add_mutually_exclusive_group()
group.add_argument('-k', '--kernel_name', dest='kernel_name',
                    help='Name of the kernel that you want to capture, will capture the first time that sees this kernel', default=None)
group.add_argument('-n', '--enqueue_number', type=int, dest='enqueue_number',
                    help='The enqueue number that should be captured', default=-1)

parser.add_argument('-c', '--clioader', dest='cli_location',
                    help='Location of the cliloader loader')
parser.add_argument('-p', '--program', dest='app_location',
                    help='Location of the app from which should be captured')
parser.add_argument('-a', '--args', nargs=argparse.REMAINDER, default=[], dest='args')

args = parser.parse_args()
print_help = False
if len(sys.argv) == 1:
    print_help = True
if args.app_location == None:
    print('No program to capture specified!')
    print_help = True
if args.kernel_name == None and args.enqueue_number < 0:
    print('No enqueue to capture specified!')
    print_help = True
if print_help:
    parser.print_help(sys.stderr)
    sys.exit(1)

intercept_location_win = 'C:\\Intel\\CLIntercept_Dump\\'
intercept_location_posix = '~/CLIntercept_Dump/'
intercept_location = ""

if os.name == 'nt':
    intercept_location = intercept_location_win
elif os.name == 'posix':
    intercept_location = intercept_location_posix
else:
    print("Unknown platform, exiting!")
    exit()

app_name = os.path.basename(args.app_location)

os.environ['CLI_CaptureReplay'] = "1"
os.environ['CLI_InitializeBuffers'] = "1"
os.environ['CLI_AppendBuildOptions'] = "-cl-kernel-arg-info"

if args.enqueue_number != -1:
    os.environ['CLI_CaptureReplayMinEnqueue'] = str(args.enqueue_number)
    os.environ['CLI_CaptureReplayMaxEnqueue'] = str(args.enqueue_number)
else:
    os.environ['CLI_CaptureReplayKernelName'] = args.kernel_name
    os.environ['CLI_CaptureReplayNumKernelEnqueuesCapture'] = "1"

if args.cli_location == None:
    print('No cliloader executable was specified!')
    command = [args.app_location]
else:
    command = [args.cli_location, args.app_location]
command = [os.path.expanduser(p) for p in command]
command.extend(args.args)

print('\n\nRunning test application: {}'.format(' '.join(map(str,command))))
subprocess.run(command)

intercept_location = os.path.expanduser(intercept_location)
search_dir = os.path.join(os.path.join(intercept_location, app_name, "Replay"))
print('Done!\n\n')
print('Looking for output in directory: {}'.format(search_dir))

replay_location = None
if args.enqueue_number >= 0:
    check = 'Enqueue_' + str(args.enqueue_number) + '_.*'
else:
    check = 'Enqueue_[0-9]+_' + args.kernel_name
if os.path.exists(search_dir):
    for f in os.scandir(search_dir):
        if f.is_dir() and re.match(check, f.name):
            print('Using replay information from: {}'.format(f.path))
            replay_location = f.path
            break
if replay_location == None:
    print('No replay information was found!')
    print('Please check that the kernel name and replay number was set correctly.')
    exit()

# Run extracted kernel to dump output buffers
os.chdir(replay_location)
python_exe = sys.executable
print('\n\nReplaying kernel:')
subprocess.run([str(python_exe), "run.py"])

print('Done!\n\n')
print('Now starting validation...')

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
