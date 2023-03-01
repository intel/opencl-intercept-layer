import os
import argparse
import subprocess
import glob as gl
import re
import hashlib
import numpy as np

# args:
#   Enqueue number
#   Link to CLI
#   Link to app
#   Arguments to app
parser = argparse.ArgumentParser(description='Helper script to extract and validate single kernel from OpenCL app.')
parser.add_argument('-enqueue_number', '--num', type=int, dest='enqueue_number',
                    help='The enqueue number that should be captured ')
parser.add_argument('-cli', dest='cli_location',
                    help='Location of the OpenCL-Intercept-Layer')
parser.add_argument('-program', '--p', dest='app_location',
                    help='Location of the app from which should be captured')
parser.add_argument('-args', '--a', nargs=argparse.REMAINDER, default=[], dest='args')

args = parser.parse_args()

intercept_location_win = "C:\Intel\CLIntercept_Dump\\"
intercept_location_posix = "~/CLIntercept_Dump/"
intercept_location = ""
delim = ""

if os.name == 'nt':
    intercept_location = intercept_location_win
    delim = "\\"
elif os.name == 'posix':
    intercept_location = intercept_location_posix
    delim = "/"
else:
    print("Unknown platform, exiting!")
    exit()

app_name = args.app_location[args.app_location.rfind(delim) + 1:]

os.environ['CLI_DumpReplayKernelEnqueue'] = str(args.enqueue_number)
os.environ['CLI_DumpBuffersAfterEnqueue'] = str(1)
os.environ['CLI_DumpBuffersMinEnqueue'] = str(args.enqueue_number)
os.environ['CLI_DumpBuffersMaxEnqueue'] = str(args.enqueue_number)
os.environ['CLI_InitializeBuffers'] = str(1)

# Run ./cliloader with CLI_DumpReplayKernelEnqueue=${EnqueueNumber}
command = [args.cli_location, args.app_location]
command.extend(args.args)

subprocess.run(command)

replay_location = os.path.join(intercept_location, app_name, "Replay", "Enqueue_" + str(args.enqueue_number), "")
replay_location = os.path.expanduser(replay_location)

# Run extracted kernel to dump output buffers
os.chdir(replay_location)
subprocess.run(["python", "run.py"])

print("\nNow starting validation!")

# Compare the buffers for binary equality
replayed_buffers = gl.glob("./output_buffer*.bin")
replayed_hashes = {}
for replayed_buffer in replayed_buffers:
    idx = int(re.findall(r'\d+', replayed_buffer)[0])
    replayed_hashes[idx] = hashlib.md5(np.fromfile(replayed_buffer)).hexdigest()

dumped_location = os.path.join(intercept_location, app_name, "memDumpPostEnqueue")
dumped_location = os.path.expanduser(dumped_location)
os.chdir(dumped_location)

# The CLI padds enqueue number so that it is at least 4 digits, do so too
padded_enqueue_num = ""
if args.enqueue_number < 10000:
    padded_enqueue_num = str(args.enqueue_number).rjust(4, "0")
else:
    padded_enqueue_num = str(args.enqueue_number)

dumped_buffers = gl.glob("./Enqueue_" + padded_enqueue_num + "_Kernel_*.bin")
dumped_hashes = {}
for dumped_buffer in dumped_buffers:
    start = dumped_buffer.find("_Arg_")
    idx = int(re.findall(r'\d+', dumped_buffer[start:])[0])
    dumped_hashes[idx] = hashlib.md5(np.fromfile(dumped_buffer)).hexdigest()

all_equal = False
if len(replayed_hashes) == len(dumped_hashes):
    for pos in replayed_hashes.keys():
        if replayed_hashes[pos] == dumped_hashes[pos]:
            all_equal = True
            print(f"Pos: {pos} is equal")
        else:
            print(f"Pos: {pos} is not equal")
            all_equal = False
else:
    print("Replayed standalone kernel differs from app's result, " 
           "please open an issue on the Github so that we can look into it!")
if all_equal:
    print("Replayed standalone kernel produces correct results")