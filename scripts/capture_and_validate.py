#
# Copyright (c) 2023-2025 Intel Corporation
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

os.chdir(replay_location)
python_exe = sys.executable
print('\n\nReplaying kernel and validating:')
subprocess.run([str(python_exe), "run.py", "--validate"])

print('Done!\n\n')
