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

1. Set the top-level control to enable kernel capturing and replay: `CaptureReplay`
2. Set any additional controls to capture a specific range of kernels, or specific kernel names.  For example:
    * `CaptureReplayMinEnqueue` and `CaptureReplayMaxEnqueue`, to capture a specific range of kernel enqueues.
    * `CaptureReplayKernelName`, to capture a specific kernel name.
    * `CaptureReplayUniqueKernels`, to capture only unique kernel and dispatch parameter combinations.
    * `CaptureReplayNumKernelEnqueuesSkip`, to skip initial captures.
    * `CaptureReplayNumKernelEnqueuesCapture`, to capture a limited number of kernel enqueues.
3. Then, simply run the program as usual!

For more details, please see the Capture and Replay Controls section in the [controls](controls.md) documentation.

## Step by Step for Automatic Capturing and Validation

Use the [capture_and_validate.py](../scripts/capture_and_validate.py) script to capture a workload and validate that the replayed results match.

Arguments for the capture and validate script are:

* `-c` or `--cliloader`: Path to `cliloader`.  This can be a full path, or a relative path, or just `cliloader` if `cliloader` is already in the system path.
* `-p` or `--program`: The command to execute the program to capture.
* `-a` or `--args`: Any optional arguments to pass to the program to capture.
* Either one of:
    * `-k` or `--kernel_name`: The kernel name to capture.
    * `-n` or `--enqueue_number`: The enqueue number that should be captured.

The capture and validate script will then run the program using `cliloader` with the given arguments to capture the the specified kernel or enqueue number.
The script will then verify that the buffers calculated by the standalone replay agree with the buffers calculated by the original program.
If the buffers don't agree, it will show a message in the terminal.

## Supported Features

* OpenCL Buffers
  * These may be aliased, then only one buffer is used.
    * Only true if the buffers use the same memory address, so not when using sub-buffers and having offsets.
  * `__local` kernel arguments, i.e. those set by `clSetKernelArg(kernel, arg_index, local_size, NULL)`.
  * Device only buffers, i.e. those with `CL_MEM_HOST_NO_ACCESS`.  When kernel capture is enabled, any device-only access flags are removed.
* OpenCL Images
  * 2D, and 3D images are supported.
* OpenCL SVM and USM
  * Pointers to the base of an SVM or USM allocation are supported.
* OpenCL Samplers
* OpenCL Kernels from source or IL
* OpenCL Kernels from device binary

## Limitations (incomplete)

* Does not work with pointers to the middle of an OpenCL SVM or USM allocation.
* Does not work with SVM or USM indirect access, where the SVM or USM allocation is not set as a kernel argument.
* Does not work with OpenCL pipes.
* Untested for out-of-order queues.
* Sub-buffers are not dealt with explicitly, this may affect the results for both debugging and performance.
* The capture and validate script may not work with some GUI apps.

## Advice

* Use the following environment variables for `pyopencl`: `PYOPENCL_NO_CACHE=1` and `PYOPENCL_COMPILER_OUTPUT=1`.
* Minimize usage of other controls, to prevent unexpected behavior, however:
  * Consider enabling `InitializeBuffers` for more predictable results between runs.
* When executing the capture and validate script consider removing any other kernel captures, or verifying that the validate script is using the correct capture.
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
