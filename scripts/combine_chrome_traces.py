#!/usr/bin/env python3

#
# Copyright (c) 2018-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

import sys
import datetime
import re
import os

def filter(filters, record):
    # Always filter non-records:
    if not "\"ph\":" in record: return False
    # Handle the case where there are no filters:
    if len(filters) == 0: return True
    # Always keep metadata records:
    if "\"ph\":\"M\"" in record: return True
    # Otherwise, check against all filters:
    for filter in filters:
        if filter in record: return True
    return False

def fixup(record):
    if not record.strip().endswith(','):
        record = record.strip() + ",\n"
    return record

def main():
    printHelp = False

    if (len(sys.argv) < 4 or sys.argv[1] == '-h' or sys.argv[1] == '-?'):
        printHelp = True

    # Help message
    if (printHelp):
        print(r"")
        print(r"    A script to combine multiple Chrome traces captured by the opencl-intercept-layer.")
        print(r"    The combined trace can be viewed on a common timeline in the Chrome browser.")
        print()
        print(r"    This is useful for analyzing multi-process execution.")
        print(r"    Set CLI_AppendPid=1 when collecting Chrome traces to obtain separate per-process traces.")
        print(r"    Can also be useful to compare two or more single process executions on a common timeline.")
        print()
        print(r"    Use as:")
        print(r"    combine_chrome_traces.py <number of traces> <space-separated paths to all json traces> [space-separated event filters]")
        print()
        print(r"        Optional arguments: event-filters are names of OpenCL kernels or OpenCL API calls")
        print(r"                            that should be retained in the filtered output.")
        print()
        print(r"    Example:" )
        print()
        print(r"    combine_chrome_traces.py 4 \ # specifies 4 traces to combine")
        print(r"    CLIntercept_Dump.45682/clintercept_trace.json \ # paths to the four traces follow")
        print(r"    CLIntercept_Dump.45683/clintercept_trace.json \ ")
        print(r"    CLIntercept_Dump.45684/clintercept_trace.json \ ")
        print(r"    CLIntercept_Dump.45685/clintercept_trace.json \ ")
        print(r"    kernelA kernelB clEnqueueWriteBuffer clEnqueueReadBuffer # specifies kernel/API names as filters [optional]")
        print()
        print(r"    Note: This script modifies events records so that all traces have a common epoch.")
        print()
        sys.exit(0)

    # Get input arguments
    files = sys.argv[2:2+int(sys.argv[1])]
    filters = sys.argv[2+int(sys.argv[1]):]

    # Sanity checks
    if len(files) < 2:
        print("ERROR: you must specify at least two traces to combine.")
        sys.exit(1)
    for fileName in files:
        if not os.path.isfile(fileName):
            print("ERROR: specified file {} cannot be found.".format(fileName))
            sys.exit(1)

    # Read input files
    inputFiles = []
    for fileName in files:
        f = open(fileName,'r')
        currentFile = f.readlines()
        f.close()
        inputFiles.append(currentFile)

    # Figure out epoch (earliest start_time across all records)
    start_times = []
    for j in range(len(files)):
        for k in range(len(inputFiles[j])):
            if (inputFiles[j][k].find("start_time") != -1):
                start_times.append(int(inputFiles[j][2].split(":")[-1].split("}")[0].strip('"')))
                break
        if(len(start_times) != j+1):
            print("ERROR: start_time not found in trace file "+sys.argv[j+2]+". Please check if the trace is valid.")
            sys.exit(1)
    epoch = min(start_times)
    print("Found minimum start time {}".format(epoch))

    # Perform filtering
    filteredFiles = []
    for j in range(len(files)):
        flt = [fixup(i) for i in inputFiles[j] if filter(filters, i)]
        filteredFiles.append(flt)

    # Perform epoch normalization
    for j in range(len(files)):
        offset = start_times[j] - epoch
        print("Processing file {} with offset {}".format(files[j], offset))
        for k in range(len(filteredFiles[j])):
            if (filteredFiles[j][k].find("\"ts\"") != -1):
                ts = float(filteredFiles[j][k].split("\"ts\":")[-1].split(",")[0]) + offset
                #print('old record was: {}'.format(filteredFiles[j][k].strip()))
                filteredFiles[j][k] = re.sub("\"ts\":[\\d.]+", "\"ts\":"+str(ts), filteredFiles[j][k])
                #print('new record is:  {}'.format(filteredFiles[j][k].strip()))
            elif (filteredFiles[j][k].find("start_time") != -1):
                #print('old record was: {}'.format(filteredFiles[j][k].strip()))
                filteredFiles[j][k] = re.sub('\"start_time\":["]?\\d+["]?', "\"start_time\":"+str(epoch), filteredFiles[j][k])
                #print('new record is:  {}'.format(filteredFiles[j][k].strip()))

    # Write to output file
    tstamp = datetime.datetime.now()
    fName = "merged_" + str(tstamp.year) + '-' + str(tstamp.month) + '-' + str(tstamp.day) \
            + '-' + str(tstamp.hour) + '-' + str(tstamp.minute)+ '-' + str(tstamp.second) + ".json"
    print("Writing to combined file "+fName)
    fo = open(fName, 'w')
    fo.write("[\n")
    for j in range(len(files)):
      for k in range(1,len(filteredFiles[j])):
          fo.write("%s" % filteredFiles[j][k])
    fo.write("{\"ph\":\"M\",\"name\":\"clintercept_merged_eof\",\"pid\":0,\"tid\":0}\n")
    fo.write("]\n")
    f.close()
    print("Done.")

if __name__ == "__main__":
    main()
