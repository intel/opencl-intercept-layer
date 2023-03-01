# Automatic Capture And Validation of Single OpenCL Kernels from App

## Introduction
Often problems in an OpenCL accelerated program such as bugs or performance, only affect single kernels. The functionality described in here can assist in finding these problems by allowing you to extract a single kernel with the corresponding arguments, buffers (also device-only buffers, i.e. with CL_MEM_HOST_NO_ACCESS), build options, global offsets, global and local worksizes, and either the kernel source or a device binary.

These are then combined by a python script, "run.py" which is automatically placed in the right directory. Running this script will prompt you to choose an OpenCL device/platform, and then will run ("replay") this kernel and output the buffers it calculated.

## Requirements
To replay the captured kernels, you need the following Python libraries:
* pyopencl
* numpy

If you want to replay a kernel for which only an OpenCL device binary is available, it is only (somewhat) guaranteed to work on the same system.

## Step by Step
* Copy /scripts/capture_and_validate.py to the place where you run the app from
  * Not strictly necessary, but makes life easier
* Run this script with the following arguments
  - --num EnqueueNumberToBeCaptured
  - -cli "/path/to/cliloader"
  - --p "/path/to/program"
  - --a ArgsForProgram

Please make sure to follow this order of arguments

This will then run the program via the CLI with the given arguments, capture the necessary information for the specified kernel and finally verifies that the buffers calculated by the standalone replayer agree with the buffers calculated. If the buffers don't agree, it will show a message in the terminal/command line.

## Limitations (incomplete)
* Does not work with OpenCL images (yet)
* Untested for OOO queues