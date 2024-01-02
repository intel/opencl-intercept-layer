# Kernel ISA

This document describes how to use the Intercept Layer for OpenCL Applications to
retrieve kernel ISA for Intel CPU Devices.

## Tools

You will need:

* The Intercept Layer for OpenCL Applications
* `readelf`, `dd`, and `objdump` Utilities (or equivalents)

## Process

Retrieving kernel ISA is a two step process:  The first step is to dump program
binaries while your application is executing.  Then, the program binaries can be
disassembled to view the kernel ISA in text form.

### Dumping Program Binaries

Dumping program binaries is the easy part!  Simply install the Intercept
Layer for OpenCL Applications, set `DumpProgramBinaries`, and execute your
application.  Then, every time our application calls `clBuildProgram`, the
Intercept Layer for OpenCL Applications will dump the program binary.

NOTE: The control to dump program binaries to disassemble for Intel CPU devices
is `DumpProgramBinaries`, which is different than the control to dump kernel
ISA binaries to disassemble for Intel GPU devices!

### Disassembling Program Binaries

For the Intel CPU device, the program binary is an ELF file.  ELF files consist
of multiple "sections".  The kernel ISA is stored in the ".ocl.obj" section.
So, the steps to dissemble the program binary consists of:

1. Extracting the kernel ISA from the ".ocl.obj" section of the program binary.
1. Disassembling the extracted kernel ISA.

There are many possible ways to do this, but the script
[disassemble_cpu.sh](../scripts/disassemble_cpu.sh) shows one method using the
common tools `readelf`, `dd`, and `objdump`.

### Disassembling Many Kernel ISA Binaries

The Python script [../scripts/disassemble_all_cpu.py](../scripts/disassemble_all_cpu.py)
may be useful to disassemble all program binaries in a specified directory.

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018-2024, Intel(R) Corporation
