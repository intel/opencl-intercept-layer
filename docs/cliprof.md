# cliprof: A Intercept Layer for OpenCL Applications Loader

`cliprof` is a loader utility to simplify basic application profiling using
the Intercept Layer for OpenCL Applications.

`cliprof` has been superseded by the `cliloader` utility, which supports
additional capabilities beyond basic profiling.
Since `cliloader` can do everything `cliprof` can do (and more!), support
for `cliprof` may eventually be removed.

## How to Build cliprof

`cliprof` is currently not built by default.  To build `cliprof`, set `ENABLE_CLIPROF`
when generating build files using CMake.  For example:

````
> cmake -DENABLE_CLIPROF=1 ..
````

Some operating systems require additional code in the Intercept Layer
for OpenCL Applications DLL / shared library to function correctly with the
`cliprof` loader utility.  When needed, this additional code is included when
build files are generated when `ENABLE_CLIPROF` is set.

## How to Use cliprof

To use `cliprof`, simply run it and pass the application to profile and any
arguments via the command line.  For example:

```
> cliprof executable arg0 arg1 ...
```

`cliprof` will invoke the application executable with the specified arguments,
and after the application exits the device execution time will be reported
to stderr.

```
Total Enqueues: 117


Device Performance Timing Results:

Total Time (ns): 26602464

          Function Name,  Calls,     Time (ns), Time (%),  Average (ns),      Min (ns),      Max (ns)
       GenerateJuliaSet,     39,       1795456,    6.75%,         46037,         43296,         52160
     clEnqueueMapBuffer,     39,      24778880,   93.15%,        635355,        627008,        674720
clEnqueueUnmapMemObject,     38,         28128,    0.11%,           740,           512,          1024
```

`cliprof` supports command line arguments to enable additional logging and
profiling functionality. To view all `cliprof` command line arguments, simply
run `cliprof` with no arguments.

Additionally, `cliprof` will retain and pass through most Intercept Layer for
OpenCL applications controls that are set via environment variables or other
OS-specific mechanisms.  The additional controls can be used to modify
execution while profiling, or to add additional information to the report.

## Limitations of cliprof

`cliprof` is a very easy way to do simple profiling in many cases, but there
are some cases where it will not work.   For more reliable intercepting,
follow the instructions to "globally install" or "locally install" the
Intercept Layer for OpenCL Applications.  In particular, note that `cliprof`
will not work for applications that dynamically load the ICD loader library
and query for OpenCL APIs.

The `cliprof` executable must be in the same directory as the Intercept Layer for
OpenCL Applications DLL / shared library.

## Known Bugs

The Windows `cliprof` occasionally crashes.  This appears to happen more often
with a debug `cliprof` executable.  Running `cliprof` again usually executes
successfully.

If you encounter other bugs or issues running `cliprof`, including the `--debug`
command line argument enables `cliprof` debug output and may help to
troubleshoot the problem.

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018-2024, Intel(R) Corporation
