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
    commandToRun = 'iga32 -d -p 9'

if ( len(sys.argv) == 2 ) and ( sys.argv[1] == '-h' or sys.argv[1] == '-?' ):
    printHelp = True

if printHelp:
    print('usage: disassemble_all_gpu.py {required: directory with isabin files} {optional: command to run, default: "iga32 -d -p 9"}')
elif not os.path.exists(isabinDir):
    print('error: directory ' + isabinDir + ' does not exist!')
else:
    print('Running "' + commandToRun + '" to disassemble all isabin files in: ' + isabinDir + ':')

    numberOfFiles = 0

    for file in os.listdir(isabinDir):
        if file.endswith(".isabin"):
            numberOfFiles = numberOfFiles + 1
            binFileName = isabinDir + '/' + file;
            isaFileName = isabinDir + '/' + file[:-7] + '.isa';	# strip .isabin, add .isa
            print('Disassembling ' + binFileName)
            subprocess.call(commandToRun.split() + [binFileName, '-o', isaFileName])

    print('Found ' + str(numberOfFiles) + ' file(s) to disassemble.')
