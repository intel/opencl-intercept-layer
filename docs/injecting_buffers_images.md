# Using the Intercept Layer for OpenCL Applications to Inject Buffers and Images

Buffer and Image Injection allows an application to experiment with different kernel inputs.
A typical use-case is to dump buffers or images while an application is running on a known good device or driver, and then to inject these known good buffers or images at certain points during application execution on a different device or with a different driver that is producing incorrect results.
This can help to identify differences that are irrelevant and unrelated to the incorrect results from important differences.

The process to inject buffers and images is very similar to the process to inject programs, and requires several steps.

## Step 1: Dump Programs and Program Options for Injection

Run your application with `DumpBuffersBeforeEnqueue` or `DumpImagesBeforeEnqueue` enabled. This dumps buffers and images that are set as kernel arguments to the directory:

    %SYSTEMDRIVE%\Intel\CLIntercept_Dump\<Process Name>\memDumpPreEnqueue (Windows)

or:

    ~/CLIntercept_Dump/<Process Name>/memDumpPreEnqueue (Linux)

Because this can generate a lot of data, you may want to dump buffers and images only for a small region of the application's execution, using controls like `DumpBuffersMinEnqueue`, `DumpImagesMaxEnqueue`, or `DumpBuffersForKernel`.

**Note.** If the `AppendPid` option is enabled, dumps will be saved to the `<Process Name>.PID` directory, but the `<Process Name>` directory without the process ID will be used for buffer and image injection.

## Step 2: Find the Buffer or Image to Inject

Look through the dump directory to find the buffer or image you would like to inject.

## Step 3: Copy the Buffer or Image

Copy the buffer or image file to inject to the directory:

    %SYSTEMDRIVE%\Intel\CLIntercept_Dump\<Process Name>\Inject (Windows)

or:

    ~/CLIntercept_Dump/<Process Name>/Inject (Linux)

This is the directory that is searched when looking for buffers or images to inject.
Note that this is the same directory that is used to inject programs, and is a subdirectory of the top-level dump directory.

## Step 4: Modify the Buffer or Image (Optional)

For some use-cases you will want to manually modify the buffer or image before injecting.
For other use-cases, you will inject the dumped buffer or image, unmodified.

## Step 5: Set the InjectBuffers or InjectImages Controls and Go!

If all goes well you will see a line similar to

    Injecting buffer file: <file name>

or:

    Injecting image file: <file name>

in your log.

## Notes:

* The process to inject buffers also works for SVM or USM allocations.
* For buffers, the size of the buffer must be at least as big as the file to inject.
If the size of the buffer is smaller than the size of the file then injection will fail.
If the size of the buffer is larger than the size of the file then only the initial contents of the buffer will be modified.
* For OpenCL images, the size of the image must match the size of the file or injection will fail.
* Because buffer and image injection modifies the memory object itself and not a copy of the memory object, any usage of the memory object after injection will also be affected.

---

\* Other names and brands may be claimed as the property of others.

Copyright (c) 2024, Intel(R) Corporation
