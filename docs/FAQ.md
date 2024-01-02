# Troubleshooting and Frequently Asked Questions

This document describes common troubleshooting steps, solutions to common problems encountered when using the Intercept Layer for OpenCL Applications, and other answers to other frequently asked questions.

## What is the Intercept Layer for OpenCL Applications?  Why should I use it?

In short, the Intercept Layer for OpenCL Applications is a tool enabling OpenCL developers to quickly debug, analyze, and optimize OpenCL applications.
It is thin and fast.
It can easily be enabled during development and testing, then disabled for deployment.
It almost always works without any application modifications and has been tested with OpenCL implementations from multiple vendors.
It is regularly used on Windows and Linux, generally works on OSX, and has been successfully used on Android.

The Intercept Layer for OpenCL Applications can:

* trace OpenCL API calls and their parameters,
* time (profile) OpenCL kernels and host API calls,
* detect and log OpenCL errors,
* dump the contents of buffers before and after OpenCL kernels.
* and much more!

## It's not working!  Help!

In most cases, when the Intercept Layer for OpenCL Applications is not working it is not installed correctly.
If you think you have installed the Intercept Layer for OpenCL Applications but you don't see any additional output to stderr or the log file when you run your application, then it's very likely that the Intercept Layer was not properly installed.
Some good first things to check are:

* If you are using `cliloader`, add the `--debug` command line option to see additional debug output.
  Do you see any errors or other unexpected output?
* If you are not using `cliloader`, did you follow the instructions to install the Intercept Layer for OpenCL Applications exactly?
  Are you able to see output from the Intercept Layer for OpenCL Applications if you run through `cliloader`?

If you are able to see additional output from the Intercept Layer for OpenCL Applications that is a good first step, and it means that the Intercept Layer for OpenCL Applications is properly installed.
By default, your application should run exactly the same with the Intercept Layer for OpenCL Applications as it does without it.
Use the various controls provided by the Intercept Layer for OpenCL Applications or `cliloader` command line options to selectively enable additional functionality.

## It's still not working!

If the Intercept Layer for OpenCL Applications is properly installed but is still not "working" then the log file is the next logical place to check.
Do you see any errors or unexpected output in the log file?
Are all of the controls you set listed in the log file?

## It crashed!

If your application is running correctly without the Intercept Layer for OpenCL Applications, but crashing with it, then it is very likely that the Intercept Layer for OpenCL Applications was unable to find the "real" OpenCL library (usually an OpenCL ICD loader) to load and pass calls to.
This is more likely to happen on non-Windows operating systems, since different Linux distributions or system configurations may install the real OpenCL library or OpenCL ICD loader to different locations in the file system, but it can happen on Windows as well.
When the Intercept Layer for OpenCL Applications cannot find the "real" OpenCL library, or finds the wrong "real" OpenCL library, the "real" OpenCL library or OpenCL ICD loader may be specified manually via the `OpenCLFileName` control.
In most scenarios, this control should be the full path name to the OpenCL library or OpenCL ICD loader, typically `/path/to/libOpenCL.so` on Linux, or `drive:\path\to\OpenCL.dll` on Windows.

If all goes well, you should see output like the following in your log:

```
Trying to load dispatch from: path/to/OpenCLFileName
... success!
```

It's OK if you see lines like:

```
Couldn't get exported function pointer to: clSetProgramReleaseCallback
Couldn't get exported function pointer to: clSetProgramSpecializationConstant
```

This may happen if your OpenCL library or OpenCL ICD loader does not support the newest OpenCL APIs.
This will only cause a problem if the application you are trying to intercept requires the newer APIs.
Of course, this will cause a problem without the Intercept Layer for OpenCL Applications installed, also!

On Linux operating systems, you may find the path to the real OpenCL library or OpenCL ICD loader by typing:

```sh
$ ldd ./your_opencl_application
```

This should produce output like:

```sh
    libOpenCL.so.1 => /path/to/your/libOpenCL.so.1 (0x00007f9182d27000)
```

In this case, you should set `OpenCLFileName` to `/path/to/your/libOpenCL.so.1`.
It is possible to have multiple OpenCL libraries or OpenCL ICD loaders installed on a system, and this can help to identify which OpenCL library or OpenCL ICD loader the Intercept Layer for OpenCL Applications should pass calls to.

Please remember that environment variable controls have a "CLI_" prefix, so to set `OpenCLFileName` using an environment variable the environment variable should be `CLI_OpenCLFileName`.

## It's still crashing!

If your application is still crashing, here are a few things to try:

* Enable `CallLogging`.  Is the crash occurring in an OpenCL call or outside of an OpenCL call?
* Do you have any controls enabled?  Are any controls changing the application's behavior?
  By default, applications should run the same with the Intercept Layer for OpenCL Applications as they do without, but some controls can affect application behavior and may cause an application to crash.
* Does your application still crash with another OpenCL implementation or another OpenCL device?
  Some features of the Intercept Layer for OpenCL Applications may stress OpenCL implementations and uncover bugs!

## My control is not taking effect.

If you think you are setting a control but it does not appear to be taking effect, here are a few things to check:

* Check the log, just in case.  If a control is set to a non-default value it will appear in the log file, for example:
    ````
    ErrorLogging is set to a non-default value!
    ````
* If you are setting a control via an environment variable, be aware that on many operating systems, such as Linux, environment variables are case sensitive.
  Likewise, if you are executing your application via a non-standard method, such as via `sudo` or a syscall, be extra sure that you are properly inheriting the environment.
* Please remember as well that environment variable controls have a "CLI_" prefix before the name of the control, so to set the `CallLogging` control you would set the environment variable `CLI_CallLogging`.
* If you are setting a variable via the registry or a configuration file, double-check that the config file is in the proper location (typically the user's home directory, as defined by the `$HOME` environment variable), or the proper location in the registry.
* Windows users may find it most convenient to set controls via the `cliconfig` configuration application.
* Many of the most common controls are also supported by `cliloader` command line options.

## My app runs too slow with the Intercept Layer installed.

This is usually caused by one or more optional controls.
Does your application run slowly with all controls disabled?
If not, here are a few common controls to check:

* `CallLogging` can produce a lot of output and can slow down an application, particularly when logging to stderr.
  You may want to try logging to a file or the debugger instead, such as via `LogToFile`.
* `DevicePerformanceTiming` enables event profiling and attaches an event to each OpenCL command, which can serialize command execution on some OpenCL devices, reducing the benefits of out-of-order queues.

## The Intercept Layer is not working with my Python virtual environment (venv).

The root-cause of the issue is that a Python [virtual environment](https://docs.python.org/3/library/venv.html) uses a different Python executable, typically in a "Scripts" directory on Windows.
Activating the virtual environment sets up the Windows PATH so the different Python executable is called.
The different Python executable spawns a separate child process for the real Python executable, which runs Python in the virtual environment, and the separate child process does the actual work.

When `cliloader` is used to intercept OpenCL calls in the virtual environment, `cliloader` sets up the Intercept Layer in the first Python executable, but it has no knowledge of the separate child Python process that is making OpenCL calls, so none of the OpenCL calls are intercepted.

This issue could happen with other applications but it is most commonly encountered with Python virtual environments.
It has only been observed on Windows; it has not been observed on Linux because Linux uses a different mechanism to intercept OpenCL calls.

Here are several ways to work around this issue:

* Explicitly invoke the real Python executable using its full path.
* Enable the OpenCL Intercept Layer using a "local install" or "global install", instead of using `cliloader`.

Note, for a "local install" the OpenCL Intercept Layer should be copied to the directory with the real Python executable, not to the "Scripts" directory with the different virtual environment Python executable.

## How do I submit a bug?

Please file a GitHub issue to report a bug.
Private or sensitive issues may be submitted via email to this project's maintainer (Ben Ashbaugh - ben 'dot' ashbaugh 'at' intel 'dot' com), or to any other Intel GitHub maintainer (see profile for email address).
If possible, please include exact steps to reproduce the issue, and include your log file and a reproducer.

## How do I contribute?

Contributions to the Intercept Layer for OpenCL Applications are welcomed and encouraged.
Please see [CONTRIBUTING](../CONTRIBUTING.md) for details how to contribute to the project.

## How should I cite this project?

If you use the Intercept Layer for OpenCL Applications in your work please cite it as:

```
@inproceedings{Ashbaugh:2018:DAP:3204919.3204933,
 author = {Ashbaugh, Ben},
 title = {Debugging and Analyzing Programs Using the Intercept Layer for OpenCL Applications},
 booktitle = {Proceedings of the International Workshop on OpenCL},
 series = {IWOCL '18},
 year = {2018},
 isbn = {978-1-4503-6439-3},
 location = {Oxford, United Kingdom},
 pages = {14:1--14:2},
 articleno = {14},
 numpages = {2},
 url = {http://doi.acm.org/10.1145/3204919.3204933},
 doi = {10.1145/3204919.3204933},
 acmid = {3204933},
 publisher = {ACM},
 address = {New York, NY, USA},
 keywords = {Debugging, GPGPU, OpenCL, Optimization, Performance},
}
```

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018-2024, Intel(R) Corporation
