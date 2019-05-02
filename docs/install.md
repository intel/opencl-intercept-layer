# How to Install the Intercept Layer for OpenCL Applications

There are multiple ways to install the Intercept Layer for OpenCL Applications:

## Windows

### Local Install

The easiest (and least obtrusive!) way to install the Intercept Layer for
OpenCL Applications is to:

1. Put the Intercept Layer for OpenCL Applications OpenCL.dll into your
   application's working directory, typically the directory with the
   application executable.  Since DLLs are often loaded from the current
   working directory before other directories in the system path, the
   Intercept Layer for OpenCL Applications OpenCL.dll will be loaded
   instead of the real OpenCL.dll.
2. To uninstall, simply delete the Intercept Layer for OpenCL Applications
   OpenCL.dll from the application's working directory.

### Global Install

To install the Intercept Layer for OpenCL Applications globally (for all
OpenCL applications):

1. Rename your existing OpenCL.dll (typically in c:\windows\system32 for
   32-bit systems or 64-bit DLLs on 64-bit systems, or c:\windows\syswow64 
   for 32-bit DLLs on 64-bit systems).
    * You may need to rename your existing OpenCL.dll from safe mode, or
      from a command prompt with administrative privileges.
    * If you rename your existing DLL to real_OpenCL.dll then the renamed
      DLL will be automatically loaded by the Intercept Layer for OpenCL
      Applications, otherwise you'll need to tell the Intercept Layer for
      OpenCL Applications what your real DLL name is.  See below.
2. After renaming your real OpenCL.dll, copy the Intercept Layer for
   OpenCL Applications version of OpenCL.dll in its place.
3. To uninstall the Intercept Layer for OpenCL Applications using this
   method, reverse the steps: First, delete the Intercept Layer for OpenCL
   Applications version of OpenCL.dll, then rename the real OpenCL.dll
   back to OpenCL.dll.

This method also works for applications that load OpenCL.dll from an explicit path.

## Linux

### Global Install

Note: If the real ICD loader library is in a system directory, you may need to perform some of these steps with elevated privledges - be careful!

1. Find the location of the ICD loader library or OpenCL implementation your application is using:

       ldd /path/to/your/application | grep OpenCL

   This will list where your application is searching for the ICD loader, similar to:

	   libOpenCL.so.1 => /path/to/your/libOpenCL.so.1 (0x00007f9182d27000)

   The rest of this document assumes that the real ICD loader library is `libOpenCL.so.1`, as shown above.
   If your ICD loader is named something different, please use its name instead.

2. Rename (or copy) the real ICD loader library to a different name, such as real_libOpenCL.so:

       mv /path/to/your/libOpenCL.so.1 path/to/your/real_libOpenCL.so

3. Copy or symbolically link the Intercept Layer for OpenCL Applications library in place of the real ICD loader:

       sudo cp /path/to/build/output/libOpenCL.so /path/to/your/libOpenCL.so.1

4. Create a config file or setup environment variables to tell the Intercept Layer for OpenCL Applications where to find the real ICD loader.
For example, you could create a `clintercept.conf` file with content:

       DllName=path/to/your/real_libOpenCL.so

    Or, you could set an environment variable:

       export CLI_DllName=path/to/your/real_libOpenCL.so

    See the [controls documentation](controls.md) for more detail.

5. Run your OpenCL application.
You should observe that the Intercept Layer for OpenCL Applications is active.

6. To uninstall, rename or copy the real ICD loader library back to its original location:

        rm /path/to/your/libOpenCL.so.1
        mv /path/to/your/real_libOpenCL.so /path/to/your/libOpenCL.so.1

5. Run your OpenCL application.
You should observe that the Intercept Layer for OpenCL Applications is no longer active.

### Targeted Usage

To intercept many Linux OpenCL applications, instrumentation can be performed
using only environment variables.  If the application specifies an rpath or
otherwise circumvents the OS's method of identifying an appropriate
libOpenCL.so, this method won't work.  Example:

    LD_LIBRARY_PATH=/path/to/build/output CLI_DllName=/path/to/real/libOpenCL.so \
    CLI_DumpProgramSource=1 ./oclapplication

## Mac OSX

The Intercept Layer for OpenCL Applications on OSX uses an OS capability called
"interposition" to intercept OpenCL calls.  As such, there is no "global
install" for OSX.  To use the Intercept Layer for OpenCL Applications on OSX,
run your application with the environment variable DYLD_INSERT_LIBRARIES set
to the full path to the CLIntercept library.  For example:

    OSX Command Prompt$ DYLD_INSERT_LIBRARIES=/full/path/to/clIntercept/OpenCL ./HelloWorld

## Android - Experimental

Only global install was tested

1. on target:

       cd /system/vendor/lib
       mv libOpenCL.so real_libOpenCL.so

2. on host:

       adb push clIntercept.so /system/vendor/lib/libOpenCL.so

3. configuration file will be in $HOME/clintercept.conf. If $HOME variable is
   undefined (GUI application) it is in /sdcard/clintercept.conf.

   Sample config:

       LogToFile=1
       CallLogging=0
       HostPerformanceTiming=1
       DevicePerformanceTiming=1
       HostPerformanceTimeLogging=1
       DevicePerformanceTimeLogging=1

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018-2019, Intel(R) Corporation
