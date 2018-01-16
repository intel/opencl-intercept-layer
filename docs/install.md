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

1. Find the location of the real icd loader library (libOpenCL.so):

       sudo find . -name libOpenCL*

   To find the libraries and follow symbolic links in one go use:

       sudo find . -name libOpenCL* | while read -r line; do ll "$line"; done

2. Rename the real icd loader library:

       sudo mv /path/to/lib/libOpenCL.so.1.2 path/to/lib/real_libOpenCL.so.1.2

3. Create a symbolic link from real icd loader library to the Intercept Layer for OpenCL Applications library:

       sudo ln -s /path/to/CLIBin/builds/x86_64/libOpenCL.so.1 /path/to/lib/libOpenCL.so.1.2

4. Create a config file to control the Intercept Layer for OpenCL Applications.
   Behavior is controlled via a config file on the user's root folder (~). To 
   change the behavior create/edit the configuration file and set the value 
   for the desired options. Refer to the list below for the available options. 
   To create the config file or open it for edit:

       gedit ~/clintercept.conf

   Sample content:

       DllName=path/to/lib/real_libOpenCL.so.1
       LogToFile=1                                   // Enable LogToFile feature
       CallLogging=1                                 // Enable CallLogging feature

5.  Run an OpenCL application, and output will be in ~/CLIntercept_Dump/AppName

### Targeted Usage

To intercept many Linux OpenCL applications, instrumentation can be performed 
using only environment variables.  If the application specifies an rpath or 
otherwise circumvents the OS's method of identifying an appropriate 
libOpenCL.so, this method won't work.  Example:

    LD_LIBRARY_PATH=/path/to/clintercept/build/output CLI_DLLName=/opt/intel/opencl/libOpenCL.so \
    CLI_DumpProgramSource=1 ./oclapplication

## Mac OSX - Experimental

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

Copyright (c) 2018, Intel(R) Corporation
