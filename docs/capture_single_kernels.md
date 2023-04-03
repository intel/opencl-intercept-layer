# Automatic Capture And Validation of Single OpenCL Kernels from App

## Introduction

Often problems in an OpenCL accelerated program such as bugs or performance, only affect single kernels. The functionality described in here can assist in finding these problems by allowing you to extract a single kernel with the corresponding arguments, buffers (also device-only buffers, i.e. with CL_MEM_HOST_NO_ACCESS), build options, global offsets, global and local worksizes, and either the kernel source or a device binary.

These are then combined by a python script, "run.py" which is automatically placed in the right directory. Running this script will prompt you to choose an OpenCL device/platform, and then will run ("replay") this kernel and output the buffers it calculated.

## Requirements

To replay the captured kernels, you need the following Python libraries:

* pyopencl
* numpy

If you want to replay a kernel for which only an OpenCL device binary is available, it is only (somewhat) guaranteed to work on the same system. Often you just have to remove the cached kernels, then on the next run you will get the kernel sources.

## Step by step for automatic capturing

* Set the environment variable
  * CLI_DumpReplayKernelName if you want to capture a kernel by its name
  * CLI_DumpReplayKernelEnqueue if you want to capture a kernel by its enqueue number
* Then simply run the program via the OpenCL-Intercept-Layer
* Example on Linux: `CLI_DumpReplayKernelName=${NameOfKernel} cliloader /path/to/executable`
* Example on Linux: `CLI_DumpReplayKernelEnqueue=${EnqueueCounter} cliloader /path/to/executable`

## Step by Step for automatic capturing and validation

* Copy /scripts/capture_and_validate.py to the place where you run the app from
  * Not strictly necessary, but makes life easier
* Run this script with the following arguments
  - --num EnqueueNumberToBeCaptured **XOR** --name NameOfKernelToBeCaptured
  - -cli "/path/to/cliloader"
  - --p "/path/to/program"
  - --a ArgsForProgram

Please make sure to follow this order of arguments

This will then run the program via the CLI with the given arguments, capture the necessary information for the specified kernel and finally verifies that the buffers calculated by the standalone replayer agree with the buffers calculated by the original program. If the buffers don't agree, it will show a message in the terminal/command line.

## Supports

* OpenCl Buffers
  * These may be aliasing, then only one buffer is used
    * Only true if they use the same memory address, so not when using sub buffers and having offsets
  * `__local` OpenCL buffers as parameter (i.e. where you have `setKernelArg(ctx, arg_idx, sizeOfLocalBuffer, nullptr)`)
  * Device only buffers, i.e. those with `CL_MEM_HOST_NO_ACCESS`
* OpenCL Images
* OpenCL Samplers
* Build/replay from source
* Build/replay from a device binary


## Limitations (incomplete)

* Does not work with OpenCL pipes
* Untested for OOO queues
* Sub buffers are not dealt with explictly, this may affect the results for both debugging and performance
* The capture and validate script doesn't work with GUI apps

## Advice

* Use the following environment variables for pyopencl: PYOPENCL_NO_CACHE=1 and PYOPENCL_COMPILER_OUTPUT=1
* Make sure that no CLI environment variables are set, to prevent unexpected behavior
  * You can consider setting `CLI_InitializeBuffers=1` for predictable results between runs
* Only set one of {CLI_DumpReplayKernelName, CLI_DumpReplayKernelEnqueue}
* Always make sure to check if your results make sense. 
* For some apps using `cliloader[.exe]` doesn't work properly, [then install the build OpenCL library manually](install.md).

## Request

* Please send in bug reports
* Great care has been made to test the currently supported features
  * But programs may do something unexpected
  * We probably have missed some edge cases
* Please also send in feature requests!
  * Please let us know your use case:
    * Debugging
    * Profiling/performance tuning
  * Submit a convenient way to test this feature
