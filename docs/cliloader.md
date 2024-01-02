# cliloader: A Intercept Layer for OpenCL Applications Loader

`cliloader` is a loader utility to simplify common usage of the Intercept Layer for OpenCL Applications.

`cliloader` is intended to replace the old `cliprof` utility.
To achieve the same functionality as `cliprof` using `cliloader`, simply pass the `-d` command line argument to enable "device timing".
That's it!

## How to Build cliloader

`cliloader` is built by default.

To not build `cliloader`, pass `-DENABLE_CLILOADER=FALSE` to `cmake`.

Some operating systems require additional code in the Intercept Layer for OpenCL Applications DLL / shared library to function correctly with the `cliloader` loader utility.
When needed, this additional code is included when build files are generated when `ENABLE_CLILOADER` is set.
The Intercept Layer for OpenCL Applications will log whether or not it supports `cliloader` while it is loading.

## How to Use cliloader

To use `cliloader`, simply run it and pass the application to profile and any
arguments via the command line.  For example:

```
> cliloader executable arg0 arg1 ...
```

`cliloader` will invoke the application executable with the specified arguments.
`cliloader` will not set any Intercept Layer for OpenCL Applications controls by default, but `cliloader` supports command line arguments to set many common controls.
To view all `cliloader` command line arguments, simply run `cliloader` with no arguments.

Additionally, `cliloader` will retain and pass through most Intercept Layer for OpenCL applications controls that are set via environment variables or other OS-specific mechanisms.

## Limitations of cliloader

`cliloader` is a very easy way to do simple profiling in many cases, but there are some cases where it will not work.
For more reliable intercepting, follow the instructions to "globally install" or "locally install" the Intercept Layer for OpenCL Applications.
In particular, note that `cliloader` will not work for applications that dynamically load the ICD loader library and query for OpenCL APIs.

The Windows `cliloader` executable must be in the same directory as the Intercept Layer for
OpenCL Applications DLL.
The Linux or OSX `cliloader` executable will search for the Intercept Layer for OpenCL Applications shared library in the same directory as the executable, in the parent directory (`../`), and in a lib directory (`../lib`).
`cliloader` has not been tested on other operating systems.

## Known Bugs

The Windows `cliloader` occasionally crashes.
This appears to happen more often with a debug `cliloader` executable.
Running `cliloader` again usually executes successfully.

If you encounter other bugs or issues running `cliloader`, including the `--debug` command line argument enables `cliloader` debug output and may help to troubleshoot the problem.

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018-2024, Intel(R) Corporation
