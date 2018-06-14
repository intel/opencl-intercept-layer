# Kernel ISA

This document describes how to use the Intercept Layer for OpenCL Applications to
retrieve kernel ISA.  This feature currently works with most Intel GPU OpenCL
devices and most driver versions.  Support for other devices may be added at a
later date.

## Tools

You will need:

* The Intercept Layer for OpenCL Applications
* A Kernel ISA Binary Disassembler

## Process

Retrieving kernel ISA is a two step process:  The first step is to dump kernel
ISA in binary form.  Then, the kernel ISA binaries can be disassembled to view
the kernel ISA in text form.

### Dumping Kernel ISA Binaries

Dumping kernel ISA binaries is the easy part!  Simply install the Intercept
Layer for OpenCL Applications, set `DumpKernelISABinaries`, and execute your
application.  Then, every time our application calls `clBuildProgram`, the
Intercept Layer for OpenCL Applications will dump an ISA binary for each
kernel in the program.

How does this work?  Drivers for Intel GPU OpenCL devices support a kernel
query for the kernel ISA binary.

### Building an Intel GPU ISA Disassembler

This section describes how to build an Intel GPU ISA disassembler, which can
be used to disassemble a kernel ISA binary to view the kernel ISA in text form.
The particular Intel GPU ISA disassembler we'll be using is `IGA`, which is
part of the Intel Graphics Compiler (IGC).

The first step is to get the Intel Graphics Compiler source code.  From a
directory where you want to build `IGA`:

    git clone https://github.com/intel/intel-graphics-compiler.git

We're only going to build `IGA`, and not all of IGC.  So, change to the
`IGA` directory in the repo we just cloned:

    cd intel-graphics-compiler/visa/iga

`IGA` creates its build files using CMake, just like the Intercept Layer for
OpenCL Applications.  So, create a `build` directory for build files, and
run cmake from this build directory:

    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release

This should create build files without errors or warnings.  After build
files have been created, either use them directly to build `IGA`, or invoke
CMake to perform the build.  For example:

    cmake --build . --config Release

If all goes well, this will successfully build an `IGA` executable.
Note that the exact location of the executable will be dependent on
the operating system you are building on, and the configuration you
are building.

### Using the Intel GPU ISA Disassembler

Disassembling a kernel ISA binary using `IGA` is a mostly straightforward
process.  You'll want to tell `IGA` to disassemble your file (`-d`), the
device your kernel ISA binary was compiled for (`-p`), and your kernel ISA
binary file name.  For example:

    ./iga32 -d -p 8 CLI_0000_3F40E1CD_0000_GPU_GenerateJuliaSet.isabin

The table below describes the mapping between Intel processor code names and
GPU devices:

| Processor | Code Name | Device |
|:----------|:---------:|:------:|
|[Broadwell](https://ark.intel.com/products/codename/38530/Broadwell) | BDW | GEN8 |
|[Cherry Trail](https://ark.intel.com/products/codename/46629/Cherry-Trail) | CHV | GEN8LP |
|[Skylake](https://ark.intel.com/products/codename/37572/Skylake) | SKL | GEN9 |
|[Apollo Lake](https://ark.intel.com/products/codename/80644/Apollo-Lake) | BXT | GEN9LP |
|[Kaby Lake](https://ark.intel.com/products/codename/82879/Kaby-Lake) | KBL | GEN9.5 |

Here is a helpful link that describes the Intel GPU ISA:

https://software.intel.com/en-us/articles/introduction-to-gen-assembly

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018, Intel(R) Corporation
