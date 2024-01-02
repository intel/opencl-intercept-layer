# AUB File Capture

This document describes how to use the Intercept Layer for OpenCL Applications to
capture AUB files.  An AUB file is an Intel GPU trace file format.  It consists of
HW commands, kernel ISA, and memory state, and can be used with various simulators
for debugging and performance analysis.

The Intercept Layer for OpenCL Applications relies on the underlying GPU device
drivers to actually perform AUB file capture.  To reduce the size of an AUB file,
the Intercept Layer for OpenCL Applications can be used to filter AUB file capture
to a specific kernel and/or sub-section of an application.

This document describes the steps needed to enable AUB file capture.  For details
about specific AUB file controls, please refer to the AubCapture Controls section
in the [controls](controls.md) documentation.

## AUB File Capture Methods

The Intercept Layer for OpenCL Applications supports two methods of AUB file
capture.  The newest and default method relies on AUB capture functionality in
the [NEO OpenCL implementation](https://github.com/intel/compute-runtime).  This
method is described in detail in this document.  The Intercept Layer for OpenCL
Applications also supports the older KDC method for aub capture, however this
method is deprecated, is not recommended, and is supported for internal use only.

## Enabling NEO AUB File Capture

**IMPORTANT NOTE**: NEO AUB file capture requires a Debug or Internal driver build.
Most published drivers are Release driver builds, which do not support AUB file capture.

To initially enable NEO AUB file capture, please enable the NEO controls
`AUBDumpSubCaptureMode` and `SetCommandStreamReceiver`.

The mechanism to enable these controls is operating system dependent.

On Windows, enable NEO AUB file capture by setting two `REG_DWORD` registry values
in the registry key:

```
// For 32-bit systems, or 64-bit applications on a 64-bit system:
HKEY_LOCAL_MACHINE\SOFTWARE\INTEL\IGFX\OCL

// For 32-bit applications on a 64-bit system:
HKEY_LOCAL_MACHINE\SOFTWARE\WoW6432Node\INTEL\IGFX\OCL
```

The first registry value should be named `AUBDumpSubCaptureMode` and should have the
value `2`, indicating `Toggle` mode.
The second registry value should be named `SetCommandStreamReceiver` and should have
the value `3`, indicating that commands should both be captured and sent to the GPU.

On Linux, enable NEO file AUB capture by setting the environment variable
`AUBDumpSubCaptureMode` to the value `2`, indicating `Toggle` mode, and by setting
the environment variable `SetCommandStreamReceiver` to the value `3`, indicating that
commands should both be captured and sent to the GPU.
The environment variable method may also be used on Windows.

The `AUBDumpSubCaptureMode` and `SetCommandStreamReceiver` variables may also be set
via the config file `igdrcl.config`, if preferred.

Additionally, it is generally recommended to set the `PrintDebugSettings` variable to
the value `1`, which will print the values of all non-default variables to the console,
to verify they have been properly set.

## Testing AUB File Capture

Suggested Intercept Layer for OpenCL Applications controls to test AUB file capture are:

```c
AubCapture=1                    // This is the top-level control for AUB Capture.
AubCaptureIndividualEnqueues=1  // One file per enqueue.
AubCaptureMaxEnqueue=10         // No more than 10 files.
```

Run your application with these controls, and you should see up to 10 AUB files
captured.
The exact number will depend on the number of kernel enqueues in the first 10 enqueues.
If the first 10 enqueues do not include any kernel enqueues, increase the
`AubCaptureMaxEnqueue` value.

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018-2024, Intel(R) Corporation
