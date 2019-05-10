# AUB File Capture

This document describes how to use the Intercept Layer for OpenCL Applications to
capture AUB files.  An AUB file is an Intel GPU trace file format.  It consists of
HW commands, kernel ISA, and memory state and can be used with various simulators
for debugging and performance analysis.

The Intercept Layer for OpenCL Applications relies on the underlying GPU device
drivers to actually perform AUB file capture.  To reduce the size of an AUB file,
the Intercept Layer for OpenCL Applications can be used to filter AUB file capture
to a specific kernel or sub-section of an application.

This document describes the steps needed to enable AUB file capture.  For details
about specific AUB file controls, please refer to the AubCapture Controls section
in the [controls](controls.md) documentation.

## AUB File Capture Methods

The Intercept Layer for OpenCL Applications supports two methods of AUB file
capture.  The newest and default method is relies on AUB capture functionality in
the [NEO OpenCL implementation](https://github.com/intel/compute-runtime).  This
method is described in detail in this document.  The Intercept Layer for OpenCL
Applications also supports the older KDC method for aub capture, however this
method is deprecated, is not recommended, and is supported for internal use only.

## Enabling NEO AUB File Capture

To initially enable NEO AUB file capture, please enable the NEO control
`AUBDumpSubcaptureMode`.

The mechanism to enable this is operating system dependent.

On Windows, enable NEO AUB file capture by setting a `REG_DWORD` registry value
in the registry key:

    HKEY_CURRENT_USER\SOFTWARE\INTEL\IGFX\OCL

The registry value should be named `AUBDumpSubcaptureMode` and should have the
value `2`, indicating `Toggle` mode.

On Linux, enable NEO file AUB capture by setting the environment variable
`AUBDumpSubcaptureMode` to the value `2`, indicating `Toggle` mode.  The
`AUBDumpSubcaptureMode` variable may also be set via the config file `igdrcl.config`,
if preferred.

## Testing AUB File Capture

Suggested controls to test AUB file capture are:

```c
    AubCapture=1                    // This is the "master" control for AUB Capture.
    AubCaptureIndividualEnqueues=1  // One file per enqueue.
    AubCaptureMaxEnqueue=10         // No more than 10 files.
```

Run your application with these controls, and you should see up to 10 AUB files
captured.  The exact number will depend on the number of kernel enqueues in the
first 10 enqueues.  If there are no kernel enqueues, increase the `AubCaptureMaxEnqueue`
value.

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018-2019, Intel(R) Corporation
