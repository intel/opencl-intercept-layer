/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#define CLINTERCEPT_DYLD_INTERPOSE(_funcname)                                       \
    __attribute__((used)) static const struct                                       \
    {                                                                               \
        const void* replacement;                                                    \
        const void* replacee;                                                       \
    }                                                                               \
    _interpose_ ## _funcname __attribute__ ((section ("__DATA,__interpose"))) =     \
    {                                                                               \
        (const void*)(unsigned long)&CLIRN( _funcname ),                            \
        (const void*)(unsigned long)&_funcname                                      \
    };

CLINTERCEPT_DYLD_INTERPOSE(clGetPlatformIDs);
CLINTERCEPT_DYLD_INTERPOSE(clGetPlatformInfo);
CLINTERCEPT_DYLD_INTERPOSE(clGetDeviceIDs);
CLINTERCEPT_DYLD_INTERPOSE(clGetDeviceInfo);
CLINTERCEPT_DYLD_INTERPOSE(clCreateContext);
CLINTERCEPT_DYLD_INTERPOSE(clCreateContextFromType);
CLINTERCEPT_DYLD_INTERPOSE(clRetainContext);
CLINTERCEPT_DYLD_INTERPOSE(clReleaseContext);
CLINTERCEPT_DYLD_INTERPOSE(clGetContextInfo);
CLINTERCEPT_DYLD_INTERPOSE(clCreateCommandQueue);
CLINTERCEPT_DYLD_INTERPOSE(clRetainCommandQueue);
CLINTERCEPT_DYLD_INTERPOSE(clReleaseCommandQueue);
CLINTERCEPT_DYLD_INTERPOSE(clGetCommandQueueInfo);
CLINTERCEPT_DYLD_INTERPOSE(clSetCommandQueueProperty);
CLINTERCEPT_DYLD_INTERPOSE(clCreateBuffer);
CLINTERCEPT_DYLD_INTERPOSE(clCreateImage2D);
CLINTERCEPT_DYLD_INTERPOSE(clCreateImage3D);
CLINTERCEPT_DYLD_INTERPOSE(clRetainMemObject);
CLINTERCEPT_DYLD_INTERPOSE(clReleaseMemObject);
CLINTERCEPT_DYLD_INTERPOSE(clGetSupportedImageFormats);
CLINTERCEPT_DYLD_INTERPOSE(clGetMemObjectInfo);
CLINTERCEPT_DYLD_INTERPOSE(clGetImageInfo);
CLINTERCEPT_DYLD_INTERPOSE(clCreateSampler);
CLINTERCEPT_DYLD_INTERPOSE(clRetainSampler);
CLINTERCEPT_DYLD_INTERPOSE(clReleaseSampler);
CLINTERCEPT_DYLD_INTERPOSE(clGetSamplerInfo);
CLINTERCEPT_DYLD_INTERPOSE(clCreateProgramWithSource);
CLINTERCEPT_DYLD_INTERPOSE(clCreateProgramWithBinary);
CLINTERCEPT_DYLD_INTERPOSE(clRetainProgram);
CLINTERCEPT_DYLD_INTERPOSE(clReleaseProgram);
CLINTERCEPT_DYLD_INTERPOSE(clBuildProgram);
CLINTERCEPT_DYLD_INTERPOSE(clUnloadCompiler);
CLINTERCEPT_DYLD_INTERPOSE(clGetProgramInfo);
CLINTERCEPT_DYLD_INTERPOSE(clGetProgramBuildInfo);
CLINTERCEPT_DYLD_INTERPOSE(clCreateKernel);
CLINTERCEPT_DYLD_INTERPOSE(clCreateKernelsInProgram);
CLINTERCEPT_DYLD_INTERPOSE(clRetainKernel);
CLINTERCEPT_DYLD_INTERPOSE(clReleaseKernel);
CLINTERCEPT_DYLD_INTERPOSE(clSetKernelArg);
CLINTERCEPT_DYLD_INTERPOSE(clGetKernelInfo);
CLINTERCEPT_DYLD_INTERPOSE(clGetKernelWorkGroupInfo);
CLINTERCEPT_DYLD_INTERPOSE(clWaitForEvents);
CLINTERCEPT_DYLD_INTERPOSE(clGetEventInfo);
CLINTERCEPT_DYLD_INTERPOSE(clRetainEvent);
CLINTERCEPT_DYLD_INTERPOSE(clReleaseEvent);
CLINTERCEPT_DYLD_INTERPOSE(clGetEventProfilingInfo);
CLINTERCEPT_DYLD_INTERPOSE(clFlush);
CLINTERCEPT_DYLD_INTERPOSE(clFinish);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueReadBuffer);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueWriteBuffer);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueCopyBuffer);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueReadImage);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueWriteImage);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueCopyImage);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueCopyImageToBuffer);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueCopyBufferToImage);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueMapBuffer);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueMapImage);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueUnmapMemObject);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueNDRangeKernel);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueTask);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueNativeKernel);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueMarker);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueWaitForEvents);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueBarrier);

// Optional features?
CLINTERCEPT_DYLD_INTERPOSE(clGetExtensionFunctionAddress);
CLINTERCEPT_DYLD_INTERPOSE(clGetExtensionFunctionAddressForPlatform);

// OpenCL 1.1 Entry Points (optional)
CLINTERCEPT_DYLD_INTERPOSE(clCreateSubBuffer);
CLINTERCEPT_DYLD_INTERPOSE(clSetMemObjectDestructorCallback);
CLINTERCEPT_DYLD_INTERPOSE(clCreateUserEvent);
CLINTERCEPT_DYLD_INTERPOSE(clSetUserEventStatus);
CLINTERCEPT_DYLD_INTERPOSE(clSetEventCallback);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueReadBufferRect);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueWriteBufferRect);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueCopyBufferRect);

// OpenCL 1.2 Entry Points (optional)
CLINTERCEPT_DYLD_INTERPOSE(clCompileProgram);
CLINTERCEPT_DYLD_INTERPOSE(clCreateFromGLTexture);
CLINTERCEPT_DYLD_INTERPOSE(clCreateImage);
CLINTERCEPT_DYLD_INTERPOSE(clCreateProgramWithBuiltInKernels);
CLINTERCEPT_DYLD_INTERPOSE(clCreateSubDevices);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueBarrierWithWaitList);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueFillBuffer);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueFillImage);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueMarkerWithWaitList);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueMigrateMemObjects);
CLINTERCEPT_DYLD_INTERPOSE(clGetKernelArgInfo);
CLINTERCEPT_DYLD_INTERPOSE(clLinkProgram);
CLINTERCEPT_DYLD_INTERPOSE(clReleaseDevice);
CLINTERCEPT_DYLD_INTERPOSE(clRetainDevice);
CLINTERCEPT_DYLD_INTERPOSE(clUnloadPlatformCompiler);

// OpenCL 2.0 Entry Points (optional)
#if 0
// Disabled for now, until Apple supports OpenCL 2.0.
CLINTERCEPT_DYLD_INTERPOSE(clSVMAlloc);
CLINTERCEPT_DYLD_INTERPOSE(clSVMFree);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueSVMFree);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueSVMMemcpy);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueSVMMemFill);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueSVMMap);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueSVMUnmap);
CLINTERCEPT_DYLD_INTERPOSE(clSetKernelArgSVMPointer);
CLINTERCEPT_DYLD_INTERPOSE(clSetKernelExecInfo);
CLINTERCEPT_DYLD_INTERPOSE(clCreatePipe);
CLINTERCEPT_DYLD_INTERPOSE(clGetPipeInfo);
CLINTERCEPT_DYLD_INTERPOSE(clCreateCommandQueueWithProperties);
CLINTERCEPT_DYLD_INTERPOSE(clCreateSamplerWithProperties);
#endif

// OpenCL 2.1 Entry Points (optional)
#if 0
// Disabled for now, until Apple supports OpenCL 2.1.
CLINTERCEPT_DYLD_INTERPOSE(clSetDefaultDeviceCommandQueue);
CLINTERCEPT_DYLD_INTERPOSE(clGetDeviceAndHostTimer);
CLINTERCEPT_DYLD_INTERPOSE(clGetHostTimer);
CLINTERCEPT_DYLD_INTERPOSE(clCreateProgramWithIL);
CLINTERCEPT_DYLD_INTERPOSE(clCloneKernel);
CLINTERCEPT_DYLD_INTERPOSE(clGetKernelSubGroupInfo);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueSVMMigrateMem);
#endif

// CL-GL Entry Points (optional)
CLINTERCEPT_DYLD_INTERPOSE(clCreateFromGLBuffer);
CLINTERCEPT_DYLD_INTERPOSE(clCreateFromGLTexture2D);
CLINTERCEPT_DYLD_INTERPOSE(clCreateFromGLTexture3D);
CLINTERCEPT_DYLD_INTERPOSE(clCreateFromGLRenderbuffer);
CLINTERCEPT_DYLD_INTERPOSE(clGetGLObjectInfo);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueAcquireGLObjects);
CLINTERCEPT_DYLD_INTERPOSE(clEnqueueReleaseGLObjects);
