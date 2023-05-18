# Capture and Replay Single Kernels

## Introduction

Often, problems in an OpenCL accelerated program, such as bugs or performance issues, only affect single kernels.
The functionality described in this document can assist in finding and fixing these problems by extracting ("capturing") a single kernel from an application with its corresponding arguments, buffers, build options, global offsets, global and local work-group sizes, and either the kernel source or a device binary.

The different parts of extracted kernel can then be combined by a python script.
Executing the python script will run ("replay") the single kernel and output the buffers the kernel calculated.

## Requirements

To replay the captured kernels, you will need the following Python packages:

* `pyopencl`
* `numpy`

## Step by Step for Automatic Capturing

* Set one of the two controls:
  * `DumpReplayKernelName`, if you want to capture a kernel by its name.
  * `DumpReplayKernelEnqueue`, if you want to capture a kernel by its enqueue number.
* Then, simply run the program as usual!
* Example on Linux: `CLI_DumpReplayKernelName=${NameOfKernel} cliloader /path/to/executable`

## Step by Step for Automatic Capturing and Validation

* Copy the [capture_and_validate.py](../scripts/capture_and_validate.py) script to the place where you run the app from.
  * Not strictly necessary, but makes life easier.
* Run this script with the following arguments:
  - One of `--num EnqueueNumberToBeCaptured` or `--name NameOfKernelToBeCaptured`
  - `-cli "/path/to/cliloader"`
  - `--p "/path/to/program"`
  - `--a ArgsForProgram`

Please make sure to follow this order of arguments!

This will then run the program using `cliloader` with the given arguments, capture the the specified kernel, and verify that the buffers calculated by the standalone replay agree with the buffers calculated by the original program.
If the buffers don't agree, it will show a message in the terminal.

## Supported Features

* OpenCL Buffers
  * These may be aliased, then only one buffer is used.
    * Only true if the buffers use the same memory address, so not when using sub-buffers and having offsets.
  * `__local` kernel arguments, i.e. those set by `clSetKernelArg(kernel, arg_index, local_size, nullptr)`.
  * Device only buffers, i.e. those with `CL_MEM_HOST_NO_ACCESS`.  When kernel capture is enabled, any device-only access flags are removed.
* OpenCL Images
* OpenCL Samplers
* Build/replay from source
* Build/replay from a device binary

## Limitations (incomplete)

* Does not work with OpenCL pipes
* Untested for out-of-order queues
* Sub-buffers are not dealt with explicitly, this may affect the results for both debugging and performance
* The capture and validate script doesn't work with GUI apps

## Advice

* Use the following environment variables for `pyopencl`: `PYOPENCL_NO_CACHE=1` and `PYOPENCL_COMPILER_OUTPUT=1`
* Minimize usage of other controls, to prevent unexpected behavior.
  * Consider enabling `InitializeBuffers` for more predictable results between runs.
  * Only set one of `DumpReplayKernelName` and `DumpReplayKernelEnqueue`.
* Always make sure to check if your results make sense.
* For some apps using `cliloader` doesn't work properly.  If this happens for your application, please try other [install](install.md) options.

## Requests

* Please send in bug reports if something does not work as expected!
* Great care has been made to test the currently supported features...
  * ... but programs may do something unexpected.
  * We probably have missed some edge cases.
* Please also send in feature requests!
  * Please let us know your use case:
    * Debugging?
    * Profiling/performance tuning?
  * Submit a convenient way to test this feature.

Thank you!
