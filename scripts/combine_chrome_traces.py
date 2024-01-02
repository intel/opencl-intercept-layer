#!/usr/bin/env python3

#
# Copyright (c) 2018-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

import sys
import datetime
import re
import os

def main():

    printHelp = False

    if (len(sys.argv) < 4 or sys.argv[1] == '-h' or sys.argv[1] == '-?'):
        printHelp = True

    # Help message
    if (printHelp):
        print("")
        print("    A script to combine multiple Chrome traces captured by the opencl-intercept-layer.")
        print("    The combined trace can be viewed on a common timeline in the Chrome browser.")
        print()
        print("    This is useful for analyzing multi-process execution.")
        print("    Set CLI_AppendPid=1 when collecting Chrome traces to obtain separate per-process traces.")
        print("    Can also be useful to compare two or more single process executions on a common timeline.")
        print()
        print("    Use as:")
        print("    combine_chrome_traces.py <number of traces> <space-separated paths to all json traces> [space-separated event filters]")
        print()
        print("        Optional arguments: event-filters are names of OpenCL kernels or OpenCL API calls")
        print("                            that should be retained in the filtered output.")
        print()
        print("    Example:" )
        print()
        print("    combine_chrome_traces.py 4 \ # specifies 4 traces to combine")
        print("    CLIntercept_Dump.45682/clintercept_trace.json \ # paths to the four traces follow")
        print("    CLIntercept_Dump.45683/clintercept_trace.json \ ")
        print("    CLIntercept_Dump.45684/clintercept_trace.json \ ")
        print("    CLIntercept_Dump.45685/clintercept_trace.json \ ")
        print("    kernelA kernelB clEnqueueWriteBuffer clEnqueueReadBuffer # specifies kernel/API names as filters [optional]")
        print()
        print("    Note: This script modifies events records so that all traces have a common epoch.")
        print()
        sys.exit(0)

    # Get input arguments
    numFiles = int(sys.argv[1]);
    numStrings = len(sys.argv) - numFiles - 2;

    # Sanity checks
    if (numFiles < 2):
        print("ERROR: you must specify at least two traces to combine.")
        sys.exit(1)
    for j in range(numFiles):
        if(not os.path.isfile(sys.argv[j+2])):
            print("ERROR: specified file "+sys.argv[j+2]+" cannot be found.")
            sys.exit(1)

    # Read input files
    inputFiles = []
    for j in range(numFiles):
        f = open(sys.argv[j+2],'r')
        currentFile = f.readlines()
        f.close()
        inputFiles.append(currentFile)

    # Figure out epoch (earliest start_time across all records)
    start_times = []
    for j in range(numFiles):
        for k in range(len(inputFiles[j])):
            if (inputFiles[j][k].find("start_time") != -1):
                start_times.append(int(inputFiles[j][2].split(":")[-1].split("}")[0].strip('"')))
                break
        if(len(start_times) != j+1):
            print("ERROR: start_time not found in trace file "+sys.argv[j+2]+". Please check if the trace is valid.")
            sys.exit(1)
    epoch = min(start_times)

    # Perform filtering if necessary
    filteredFiles = []
    if (numStrings == 0):
        filteredFiles = inputFiles
    else:
        for j in range(numFiles):
            flt = [i for i in inputFiles[j] if "\"ph\":\"M\"" in i] # copy metadata
            for k in range(numStrings):
                flt = flt + [i for i in inputFiles[j] if sys.argv[2+numFiles+k] in i]
            filteredFiles.append(flt)

    # Perform epoch normalization
    for j in range(numFiles):
        offset = start_times[j] - epoch
        for k in range(len(filteredFiles[j])):
            if (filteredFiles[j][k].find("\"ts\"") != -1):
                ts = int(filteredFiles[j][k].split("\"ts\":")[-1].split(",")[0]) + offset
                filteredFiles[j][k] = re.sub("\"ts\":\d+", "\"ts\":"+str(ts), filteredFiles[j][k])
            elif (filteredFiles[j][k].find("start_time") != -1):
                filteredFiles[j][k] = re.sub('\"start_time\":["]?\d+["]?', "\"start_time\":"+str(epoch), filteredFiles[j][k])
    
    # Write to output file
    tstamp = datetime.datetime.now()
    fName = "merged_" + str(tstamp.year) + '-' + str(tstamp.month) + '-' + str(tstamp.day) \
            + '-' + str(tstamp.hour) + '-' + str(tstamp.minute)+ '-' + str(tstamp.second) + ".json"
    print("Combining in "+fName)
    fo = open(fName, 'w')
    fo.write("[\n")
    for j in range(numFiles):
      for k in range(1,len(filteredFiles[j])):
          fo.write("%s" % filteredFiles[j][k])
    f.close()

if __name__ == "__main__":
    main()
