# How to Build the Intercept Layer for OpenCL Applications

CMake is now the primary mechanism to build the Intercept Layer for OpenCL
Applications.  The CMakefile has been tested on Windows (VS2013 and newer), 
Linux, and OSX.

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

For Linux, recommended folders are `_bin32`, `_bin64`, or just plain `_bin`.

For most 32-bit Windows and Linux usages, you can simply run:

    cmake ..

in one of the `_bin` directories described above, then open the generated solution
file or run make with the generated Makefile.

For 64-bit Windows you'll need to specify a 64-bit generator manually, for
example:

    cmake.exe -G "Visual Studio 14 2015 Win64" .

If this doesn't work, or if you'd rather not bother with command lines, the CMake
gui (`cmake-gui`) or `ccmake` is always an option.

## Android

Android support is experimental, has not been ported to CMake, and is not regularly
tested.  Building instruction for Android are:

    cd <android repo>
    source build/envsetup.sh
    lunch <your build target>
    export TOP=`pwd`
    export ANDROID_SRC=`pwd`
    cd <source folder>
    mm

The shared library will be named: clIntercept.so and placed in 
`<android repo>/out/target/product/<your build target>/system/lib`. Copy it to target 
manually.

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018, Intel(R) Corporation

[CMake]: https://cmake.org
