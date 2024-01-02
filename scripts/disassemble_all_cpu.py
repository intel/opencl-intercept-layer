#
# Copyright (c) 2018-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

import os
import subprocess
import sys

printHelp = False

if len(sys.argv) > 1:
    isabinDir = sys.argv[1]
else:
    printHelp = True

if len(sys.argv) > 2:
    commandToRun = sys.argv[2]
else:
    commandToRun = './disassemble_cpu.sh'

if ( len(sys.argv) == 2 ) and ( sys.argv[1] == '-h' or sys.argv[1] == '-?' ):
    printHelp = True

if printHelp:
    print('usage: disassemble_all_cpu.py {required: directory with .bin files} {optional: command to run, default: "disassemble_cpu.sh"}')
elif not os.path.exists(isabinDir):
    print('error: directory ' + isabinDir + ' does not exist!')
else:
    print('Running "' + commandToRun + '" to disassemble all isabin files in: ' + isabinDir + ':')

    numberOfFiles = 0

    for file in os.listdir(isabinDir):
        if file.endswith("_CPU.bin"):
            numberOfFiles = numberOfFiles + 1
            binFileName = isabinDir + '/' + file;
            objFileName = isabinDir + '/' + file[:-8] + '.obj';	# strip _CPU.bin, add .obj
            isaFileName = isabinDir + '/' + file[:-8] + '.isa';	# strip _CPU.bin, add .isa
            print('Disassembling ' + binFileName)
            subprocess.call(commandToRun.split() + [binFileName, objFileName, isaFileName])

    print('Found ' + str(numberOfFiles) + ' file(s) to disassemble.')
