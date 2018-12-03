# How to Build the Intercept Layer for OpenCL Applications

CMake is now the primary mechanism to build the Intercept Layer for OpenCL
Applications.  The CMakefile has been tested on Windows (VS2013 and newer),
Linux, and OSX.

Android builds with the CMakefile, but is not regularly tested.

## Tools

You will need:

* CMake
* A C++ Compiler

Windows developers can get CMake [here][CMake].  For Linux, CMake is likely
available via your distribution package manager.  For example, to install
CMake on Ubuntu all that is needed is:

    sudo apt-get install cmake-gui

## Recommendations

For Windows, recommended folders for "where to build the binaries" are `_bin32`
(for 32-bit DLLs) or `_bin64` (for 64-bit DLLs).  Note that for Windows, these
directories are only used for project files and intermediate files, and final
output files are built in the `builds` directory.

For Linux and OSX, recommended folders are `_bin32`, `_bin64`, or just plain `_bin`.

For most 32-bit Windows and Linux usages, you can simply run:

    cmake ..

in one of the `_bin` directories described above, then open the generated solution
file or run make with the generated Makefile.

For 64-bit Windows you'll need to specify a 64-bit generator manually, for
example:

    cmake.exe -G "Visual Studio 14 2015 Win64" .

If this doesn't work, or if you'd rather not bother with command lines, the CMake
gui (`cmake-gui`) or `ccmake` is always an option.

## CMake Variables

The following CMake variables are supported.  To specify one of these variables
via the command line generator, use the CMake syntax `-D<option name>=<value>`.
See your CMake documentation for more details.

| Variable | Type | Description |
|:---------|:-----|:------------|
| CMAKE\_BUILD\_TYPE | STRING | Build type.  Does not affect multi-configuration generators, such as Visual Studio solution files.  Default: `Release`.  Other options: `Debug`
| CMAKE\_INSTALL\_PREFIX | PATH | Install directory prefix.
| ENABLE_CLIPROF | BOOL | Enables building the cliprof loader utility.  Additionally, when required, enables code in the Intercept Layer for OpenCL Applications itself to enable cliprof functionality.  Default: `FALSE`
| ENABLE_ITT | BOOL | Enables support for Instrumentation and Tracing Techology APIs, which can be used to display OpenCL events on Intel(R) VTune(tm) timegraphs.  Default: `FALSE`
| ENABLE_KERNEL_OVERRIDES | BOOL | Enables embedding kernel strings to override precompiled kernels and built-in kernels.  Supported for Linux and Android builds only, since Windows builds always embeds kernel strings, and embedding kernel strings is not support for OSX (yet!).  Default: `TRUE`
| ENABLE_MDAPI | BOOL | For internal use only.  Default: `FALSE`
| VTUNE_INCLUDE_DIR | PATH | Path to the directory containing `ittnotify.h`.  Only used when ENABLE_ITT is set.
| VTUNE_ITTNOTIFY_LIB | FILEPATH | Path to the `ittnotify` lib.  Only used when ENABLE_ITT is set.

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018, Intel(R) Corporation

[CMake]: https://cmake.org
