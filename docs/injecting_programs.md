# Using the Intercept Layer for OpenCL Applications to Inject Programs

Program Injection allows OpenCL programs or program options to be selectively
modified for debugging or performance analysis without application knowledge.
The process for injecting programs is not particularly complicated but requires
several steps.

## Step 1: Dump Programs and Program Options for Injection

Run your application with DumpProgramSource enabled. This dumps all programs to
the directory

    %SYSTEMDRIVE%\Intel\CLIntercept_Dump\<Process Name> (Windows)

or:

    ~/CLIntercept_Dump/<Process Name> (Linux)

This directory will also be used for program injection.

**Note.** If `AppendPid` option is enabled, dumps will be saved to the `<Process Name>.PID` directory, but `<Process Name>` directory will be used for program injection.

## Step 2: Find the Program or Program Options to Modify

Look through the dump directory to find the program(s) or program options you'd
like to modify. Note that a program options file may not exist for every program
source file if the application did not provide options to clBuildProgram(). If
this is the case, simply create a new program options file.

## Step 3: Copy the Program or Program Options to Modify

Copy the program(s) or program options you'd like to modify to the directory:

    %SYSTEMDRIVE%\Intel\CLIntercept_Dump\<Process Name>\Inject (Windows)

or:

    ~/CLIntercept_Dump/<Process Name>/Inject (Linux)

This is the directory that is searched when looking for programs to inject.  Note
that this is a subdirectory of the dump directory.

If the application compiles programs deterministically the program(s) or program
options can be copied unchanged. If the application compiles programs
non-deterministically, you may need rename the programs to modify to remove the
program number or compile count from the filename.

The Intercept Layer for OpenCL Applications searches for program source filenames
to inject in this order:

* `CLI_<program number>_<hash>_source.cl` - This is the default filename dumped
  by DumpProgramSource.
* `CLI_<hash>_source.cl` - This is the default filename with the program number
  removed, so the order the application calls clCreateProgramWithSource() does
  not matter.

The Intercept Layer for OpenCL Applications searches for program option filenames
to inject in this order:

* `CLI_<program number>_<hash>_<count>_options.txt` - This is the default filename
  dumped by DumpProgramSource.
* `CLI_<hash>_<count>_options.txt` - This is the default filename with the program
  number removed, so the order the application calls clCreateProgramWithSource()
  does not matter.
* `CLI_<hash>_options.txt` - This has both the program number and compile count
  removed, so it will apply the same options every time the program is built.
* `CLI_options.txt` - This injects the same options globally, for all programs,
  unless one of the program-specific filenames exists.

## Step 4: Modify the Program

Modify the program as desired. Ideas: Change the program source to a more optimal
code sequence. Switch conformant built-ins to native built-ins. Add program
attributes, e.g. for required work group size.

## Step 5: Set the InjectProgramSource Registry Key and Go!

If all goes well you will see a line similar to

    Injecting source file: <file name>

or:

    Injecting options file: <file name>

in your log.

## Notes:

* The instructions above describe how to dump and inject program source, but you
  can also dump and inject program binaries or SPIR-V intermediate representation.

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2018-2024, Intel(R) Corporation
