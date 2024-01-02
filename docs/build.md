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

```sh
sudo apt-get install cmake-gui
```

## CMake Recommendations

For Windows, recommended folders for "where to build the binaries" are `_bin32`
(for 32-bit DLLs) or `_bin64` (for 64-bit DLLs).  Note that for Windows, these
directories are only used for project files and intermediate files, and final
output files are built in the `builds` directory.

For Linux and OSX, recommended folders are `_bin32`, `_bin64`, or just plain `_bin`.

For most 32-bit Windows and Linux usages, you can simply run:

```sh
cmake ..
```

in one of the `_bin` directories described above to generate build files.

For 64-bit Windows you may need to specify a 64-bit generator manually, for
example, to create a 64-bit solution files for Visual Studio 2015, you
could type:

```sh
cmake.exe -G "Visual Studio 14 2015 Win64" ..
```

To view all generators supported by your platform, run `cmake --help`.
Please refer to CMake documentation for more details.

Other Intercept Layer for OpenCL Applications options may be specified via the CMake command line; please refer the table of supported CMake variables below.

Some users may prefer to use the CMake GUI (`cmake-gui`) or `ccmake` to generate build files.

## Building

The exact build steps will depend on the type of generated build files, however this section describes common build scenarios.

### Using CMake

To use CMake to build from a command line, after creating build files, consider usage such as:

```sh
cmake --build <dir> --config <config> --target <target>
```

For example, to build a "debug" `cliloader` and its dependencies using build files in the current directory (`.`), use:

```sh
cmake --build . --config Debug --target cliloader
```

For some types of build files, the "config" specified on the command line is ignored; please refer to CMake documentation for details.

Building the `install` target will build all files and copy to an "install" directory, for example:

```sh
cmake --build . --config RelWithDebInfo --target install
```

### Using Makefiles

To build using generated Makefiles, consider usage such as:

```sh
make <target>
```

For example, to build `cliloader` and its dependencies using generated Makefiles, use:

```sh
make cliloader
```

Note that for Makefiles, the build configuration is determined when Makefiles are generated.
By default, Makefiles are generated for the `RelWithDebInfo` build configuration.
To generate Makefiles for another configuration, pass the build type to CMake, for example:

```sh
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

Building the `install` target will build all files and copy to an "install" directory, for example:

```sh
make install
```

### Using Visual Studio Solutions

To build using generated Visual Studio Solution files, open the generated `CLIntercept.sln` solution file, then build the entire solution or a specific project and its depedencies.
Building the `INSTALL` target (via Right Click -> Build) will build the entire solution and copy to an "install" directory.

## CMake Variables

The following CMake variables are supported.  To specify one of these variables
via the command line generator, use the CMake syntax `-D<option name>=<value>`.
See your CMake documentation for more details.

| Variable | Type | Description |
|:---------|:-----|:------------|
| CMAKE\_BUILD\_TYPE | STRING | Build type.  Does not affect multi-configuration generators, such as Visual Studio solution files.  Default: `RelWithDebInfo`.  Other options: `Debug`, `Release`
| CMAKE\_INSTALL\_PREFIX | PATH | Install directory prefix.
| ENABLE_CLILOADER | BOOL | Enables building the cliloader utility (cliloader is a replacement for the old cliprof utility).  Additionally, when required, enables code in the Intercept Layer for OpenCL Applications itself to enable cliloader functionality.  Default: `TRUE`
| ENABLE_CLIPROF | BOOL | Enables building the old cliprof loader utility.  Additionally, when required, enables code in the Intercept Layer for OpenCL Applications itself to enable cliprof functionality.  Default: `FALSE`
| ENABLE_ITT | BOOL | Enables support for Instrumentation and Tracing Technology APIs, which can be used to display OpenCL events on Intel(R) VTune(tm) timegraphs.  Default: `FALSE`
| ENABLE_KERNEL_OVERRIDES | BOOL | Enables embedding kernel strings to override precompiled kernels and built-in kernels.  Supported for Linux and Android builds only, since Windows builds always embeds kernel strings, and embedding kernel strings is not support for OSX (yet!).  Default: `TRUE`
| ENABLE_MDAPI | BOOL | Enables support for the Intel Metrics Discovery API, which can be used to collect and aggregate Intel GPU performance metrics.  Default: `TRUE`
| ENABLE\_HIGH\_RESOLUTION\_CLOCK | BOOL | Use the `high_resolution_clock` for host timing instead of the default `steady_clock`.  Default: `FALSE`
| VTUNE_INCLUDE_DIR | PATH | Path to the directory containing `ittnotify.h`.  Only used when ENABLE_ITT is set.
| VTUNE_ITTNOTIFY_LIB | FILEPATH | Path to the `ittnotify` lib.  Only used when ENABLE_ITT is set.

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018-2024, Intel(R) Corporation

[CMake]: https://cmake.org
