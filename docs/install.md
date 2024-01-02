# How to Install the Intercept Layer for OpenCL Applications

There are multiple ways to install the Intercept Layer for OpenCL Applications.

## Use the cliloader Loader!

In most cases, the `cliloader` utility is the easiest way to use the Intercept
Layer for OpenCL Applications.
The `cliloader` utility only requires elevated privileges if it is installed
into a system directory, and no elevated privileges are required if it is
installed into a user-accessible directory, such as a home directory.

After the initial installation, no further installation or uninstallation is
required to selectively execute an application with the Intercept Layer for
OpenCL Applications or to revert back to normal operation.

See the [cliloader](cliloader.md) documentation for more detail.

## Windows

### Local Install

This section describes how to install the Intercept Layer for OpenCL
Applications locally, to intercept a specific Windows application.

For this method, copy the Intercept Layer for OpenCL Applications `OpenCL.dll`
into your application's working directory, typically the directory with the
application executable.
Since DLLs are often loaded from the current working directory before other
directories in the system path, the Intercept Layer for OpenCL Applications
`OpenCL.dll` will be loaded instead of the real `OpenCL.dll`.

To uninstall the Intercept Layer for OpenCL Applications using this method,
simply delete the Intercept Layer for OpenCL Applications `OpenCL.dll` from the
application's working directory.

### Global Install

This section describes how to install the Intercept Layer for OpenCL
Applications globally, for all Windows OpenCL applications.
This method works for applications that load `OpenCL.dll` from an explicit path.

For this method, first rename your existing `OpenCL.dll` (typically in
`c:\windows\system32` for 32-bit systems or 64-bit DLLs on 64-bit systems, or
`c:\windows\syswow64` for 32-bit DLLs on 64-bit systems).
Note that you may need to rename your existing `OpenCL.dll` from safe mode, or
from a command prompt with administrative privileges.
If you rename your existing DLL to `real_OpenCL.dll` then the renamed DLL will be
automatically loaded by the Intercept Layer for OpenCL Applications, otherwise
you'll need to tell the Intercept Layer for OpenCL Applications what your real
DLL name is using the `OpenCLFileName` control.
See the [controls documentation](controls.md) for more detail.
After renaming your real `OpenCL.dll`, copy the Intercept Layer for OpenCL
Applications version of `OpenCL.dll` in its place.

To uninstall the Intercept Layer for OpenCL Applications using this method,
reverse the steps: First, delete the Intercept Layer for OpenCL Applications
version of `OpenCL.dll`, then rename the real `OpenCL.dll` back to `OpenCL.dll`.

## Linux

### Targeted Usage

Because Linux applications typically do not load shared libraries preferentially
from the application's working directory, the steps for targeted usage on Linux
are slightly different than the "local install" method on Windows.

This method uses environment variables to instruct the operating system loader
to preferentially use the Intercept Layer for OpenCL Applications shared library
instead of the system defaults.
The `LD_LIBRARY_PATH` or the `LD_PRELOAD` environment variables (or both) are
used to do this.
For example:

```sh
$ LD_LIBRARY_PATH=/path/to/build/output \
  LD_PRELOAD=/path/to/build/output/libOpenCL.so \
  CLI_OpenCLFileName=/path/to/real/libOpenCL.so \
  ./your_application
```

This is the same mechanism that is used internally by the `cliloader` utility.
It will work in most cases, but it will not work if the real OpenCL ICD loader
is found in the application's `rpath`.

### Global Install

This section describes how to install the Intercept Layer for OpenCL
Applications globally, for all Linux applications.

Note: Some Linux systems may have multiple copies of the OpenCL ICD loader
installed.
On these systems, these steps may need to be performed multiple times, for each
copy of the OpenCL ICD loader applications are using.

Note: The real OpenCL ICD loader library is usually in a system directory, so
you may need to perform some of these steps with elevated privileges - be
careful!

First, find the location of the OpenCL ICD loader library or OpenCL
implementation your application is using:

```
ldd /path/to/your/application | grep OpenCL
```

This will list where your application is searching for the OpenCL ICD loader, similar to:

```
libOpenCL.so.1 => /path/to/your/libOpenCL.so.1 (0x00007f9182d27000)
```

The rest of this document assumes that the real OpenCL ICD loader library is
`libOpenCL.so.1`, as shown above. If your OpenCL ICD loader has a different
name, please use its name instead.

Next, rename (or copy) the real OpenCL ICD loader library to a different name,
such as `real_libOpenCL.so`:

```
mv /path/to/your/libOpenCL.so.1 /path/to/your/real_libOpenCL.so
```

Then, copy or symbolically link the Intercept Layer for OpenCL Applications
library in place of the real OpenCL ICD loader:

```
cp /path/to/build/output/libOpenCL.so /path/to/your/libOpenCL.so.1
```

Create a config file or setup environment variables to tell the Intercept Layer
for OpenCL Applications where to find the real ICD loader.
For example, you could create a `clintercept.conf` file with content:

```
OpenCLFileName=/path/to/your/real_libOpenCL.so
```

Or, you could set an environment variable:

```
export CLI_OpenCLFileName=path/to/your/real_libOpenCL.so
```

See the [controls documentation](controls.md) for more detail.

Finally, run your OpenCL application.
You should observe that the Intercept Layer for OpenCL Applications is active.

To uninstall, rename or copy the real OpenCL ICD loader library back to its
original location:

```
rm /path/to/your/libOpenCL.so.1
mv /path/to/your/real_libOpenCL.so /path/to/your/libOpenCL.so.1
```

## Mac OSX

The Intercept Layer for OpenCL Applications on OSX uses an OS capability called
"interposition" to intercept OpenCL calls.  As such, there is no "global
install" for OSX.  To use the Intercept Layer for OpenCL Applications on OSX,
run your application with the environment variable `DYLD_INSERT_LIBRARIES` set
to the full path to the Intercept Layer for OpenCL Applications library.  For
example:

```sh
OSX Command Prompt$ DYLD_INSERT_LIBRARIES=/full/path/to/clIntercept/OpenCL ./HelloWorld
```

## Android - Experimental

Only the global install was tested.

On the target:

```
cd /system/vendor/lib
mv libOpenCL.so real_libOpenCL.so
```

On the host:

```
adb push clIntercept.so /system/vendor/lib/libOpenCL.so
```

The configuration file will be in `$HOME/clintercept.conf`.
If the `$HOME` variable is undefined (GUI application) it is in `/sdcard/clintercept.conf`.

Sample config:

```
LogToFile=1
CallLogging=0
HostPerformanceTiming=1
DevicePerformanceTiming=1
HostPerformanceTimeLogging=1
DevicePerformanceTimeLogging=1
```

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018-2024, Intel(R) Corporation
