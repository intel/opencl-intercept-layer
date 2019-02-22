# Copyright (c) 2018-2019 Intel Corporation
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
