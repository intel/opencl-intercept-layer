/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#if defined(_WIN32) && defined(CLINTERCEPT_CLILOADER)

#include "common.h"
#include <windows.h>
#include <Psapi.h>

static void replaceFunction(
    PIMAGE_THUNK_DATA thunk,
    void* pFunction )
{
    // Make page writable temporarily:
    MEMORY_BASIC_INFORMATION mbinfo;
    VirtualQuery( thunk, &mbinfo, sizeof(mbinfo) );
    if( !VirtualProtect(
            mbinfo.BaseAddress,
            mbinfo.RegionSize,
            PAGE_EXECUTE_READWRITE,
            &mbinfo.Protect ) )
    {
        return;
    }

    // Replace function pointer with our implementation:
    thunk->u1.Function = (ULONG64)pFunction;

    // Restore page protection:
    DWORD zero = 0;
    if( !VirtualProtect(
            mbinfo.BaseAddress,
            mbinfo.RegionSize,
            mbinfo.Protect,
            &zero ) )
    {
        return;
    }
}

#define REPLACE_FUNCTION( _name, _fname ) if( !strcmp(_name, #_fname) ) { replaceFunction( firstThunk, _fname ); }

extern "C" __declspec(dllexport)
DWORD cliprof_init( void* dummy )
{
    char *base = (char*)GetModuleHandle(NULL);

    // Get pointer to NT headers:
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)(base);
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(base + dosHeader->e_lfanew);
    if( ntHeaders->Signature != IMAGE_NT_SIGNATURE )
    {
        return false;
    }

    // Get pointer to import directory:
    DWORD importOffset =
        ntHeaders->OptionalHeader.
        DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    PIMAGE_IMPORT_DESCRIPTOR importDesc =
        (PIMAGE_IMPORT_DESCRIPTOR)(base + importOffset);

    // Loop over directory entries
    while( importDesc->Name )
    {
        // Look for OpenCL.dll:
        const char *modname = (const char*)(base + importDesc->Name);
        if( !_stricmp(modname, "opencl.dll") )
        {
            // We use the OriginalFirstThunk to match the name,
            // and then replace the function pointer in FirstThunk
            PIMAGE_THUNK_DATA origThunk =
                (PIMAGE_THUNK_DATA)(base + importDesc->OriginalFirstThunk);
            PIMAGE_THUNK_DATA firstThunk =
                (PIMAGE_THUNK_DATA)(base + importDesc->FirstThunk);

            // Loop over functions
            while( origThunk->u1.AddressOfData )
            {
                // Skip unnamed functions
                if( !(origThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG) )
                {
                    // Replace the function with a CLIntercept function if it
                    // is a function we recognize:

                    PIMAGE_IMPORT_BY_NAME import =
                        (PIMAGE_IMPORT_BY_NAME)(base + origThunk->u1.AddressOfData);
                    const char* name = (const char*)import->Name;

                    REPLACE_FUNCTION( name, clBuildProgram );
                    REPLACE_FUNCTION( name, clCloneKernel );
                    REPLACE_FUNCTION( name, clCompileProgram );
                    REPLACE_FUNCTION( name, clCreateBuffer );
                    REPLACE_FUNCTION( name, clCreateBufferWithProperties );
                    REPLACE_FUNCTION( name, clCreateCommandQueue );
                    REPLACE_FUNCTION( name, clCreateCommandQueueWithProperties );
                    REPLACE_FUNCTION( name, clCreateContext );
                    REPLACE_FUNCTION( name, clCreateContextFromType );
                    REPLACE_FUNCTION( name, clCreateFromGLBuffer );
                    REPLACE_FUNCTION( name, clCreateFromGLRenderbuffer );
                    REPLACE_FUNCTION( name, clCreateFromGLTexture );
                    REPLACE_FUNCTION( name, clCreateFromGLTexture2D );
                    REPLACE_FUNCTION( name, clCreateFromGLTexture3D );
                    REPLACE_FUNCTION( name, clCreateImage );
                    REPLACE_FUNCTION( name, clCreateImageWithProperties );
                    REPLACE_FUNCTION( name, clCreateImage2D );
                    REPLACE_FUNCTION( name, clCreateImage3D );
                    REPLACE_FUNCTION( name, clCreateKernel );
                    REPLACE_FUNCTION( name, clCreateKernelsInProgram );
                    REPLACE_FUNCTION( name, clCreatePipe );
                    REPLACE_FUNCTION( name, clCreateProgramWithBinary );
                    REPLACE_FUNCTION( name, clCreateProgramWithBuiltInKernels );
                    REPLACE_FUNCTION( name, clCreateProgramWithIL );
                    REPLACE_FUNCTION( name, clCreateProgramWithSource );
                    REPLACE_FUNCTION( name, clCreateSampler );
                    REPLACE_FUNCTION( name, clCreateSamplerWithProperties );
                    REPLACE_FUNCTION( name, clCreateSubBuffer );
                    REPLACE_FUNCTION( name, clCreateSubDevices );
                    REPLACE_FUNCTION( name, clCreateUserEvent );
                    REPLACE_FUNCTION( name, clEnqueueAcquireGLObjects );
                    REPLACE_FUNCTION( name, clEnqueueBarrier );
                    REPLACE_FUNCTION( name, clEnqueueBarrierWithWaitList );
                    REPLACE_FUNCTION( name, clEnqueueCopyBuffer );
                    REPLACE_FUNCTION( name, clEnqueueCopyBufferRect );
                    REPLACE_FUNCTION( name, clEnqueueCopyBufferToImage );
                    REPLACE_FUNCTION( name, clEnqueueCopyImage );
                    REPLACE_FUNCTION( name, clEnqueueCopyImageToBuffer );
                    REPLACE_FUNCTION( name, clEnqueueFillBuffer );
                    REPLACE_FUNCTION( name, clEnqueueFillImage );
                    REPLACE_FUNCTION( name, clEnqueueMapBuffer );
                    REPLACE_FUNCTION( name, clEnqueueMapImage );
                    REPLACE_FUNCTION( name, clEnqueueMarker );
                    REPLACE_FUNCTION( name, clEnqueueMarkerWithWaitList );
                    REPLACE_FUNCTION( name, clEnqueueMigrateMemObjects );
                    REPLACE_FUNCTION( name, clEnqueueNDRangeKernel );
                    REPLACE_FUNCTION( name, clEnqueueNativeKernel );
                    REPLACE_FUNCTION( name, clEnqueueReadBuffer );
                    REPLACE_FUNCTION( name, clEnqueueReadBufferRect );
                    REPLACE_FUNCTION( name, clEnqueueReadImage );
                    REPLACE_FUNCTION( name, clEnqueueReleaseGLObjects );
                    REPLACE_FUNCTION( name, clEnqueueSVMFree );
                    REPLACE_FUNCTION( name, clEnqueueSVMMap );
                    REPLACE_FUNCTION( name, clEnqueueSVMMemcpy );
                    REPLACE_FUNCTION( name, clEnqueueSVMMemFill );
                    REPLACE_FUNCTION( name, clEnqueueSVMMigrateMem );
                    REPLACE_FUNCTION( name, clEnqueueSVMUnmap );
                    REPLACE_FUNCTION( name, clEnqueueTask );
                    REPLACE_FUNCTION( name, clEnqueueUnmapMemObject );
                    REPLACE_FUNCTION( name, clEnqueueWaitForEvents );
                    REPLACE_FUNCTION( name, clEnqueueWriteBuffer );
                    REPLACE_FUNCTION( name, clEnqueueWriteBufferRect );
                    REPLACE_FUNCTION( name, clEnqueueWriteImage );
                    REPLACE_FUNCTION( name, clFinish );
                    REPLACE_FUNCTION( name, clFlush );
                    REPLACE_FUNCTION( name, clGetCommandQueueInfo );
                    REPLACE_FUNCTION( name, clGetContextInfo );
                    REPLACE_FUNCTION( name, clGetDeviceAndHostTimer );
                    REPLACE_FUNCTION( name, clGetDeviceIDs );
                    REPLACE_FUNCTION( name, clGetDeviceInfo );
                    REPLACE_FUNCTION( name, clGetEventInfo );
                    REPLACE_FUNCTION( name, clGetEventProfilingInfo );
                    REPLACE_FUNCTION( name, clGetExtensionFunctionAddress );
                    REPLACE_FUNCTION( name, clGetExtensionFunctionAddressForPlatform );
                    REPLACE_FUNCTION( name, clGetGLObjectInfo );
                    REPLACE_FUNCTION( name, clGetGLTextureInfo );
                    REPLACE_FUNCTION( name, clGetHostTimer );
                    REPLACE_FUNCTION( name, clGetImageInfo );
                    REPLACE_FUNCTION( name, clGetKernelArgInfo );
                    REPLACE_FUNCTION( name, clGetKernelInfo );
                    REPLACE_FUNCTION( name, clGetKernelSubGroupInfo );
                    REPLACE_FUNCTION( name, clGetKernelWorkGroupInfo );
                    REPLACE_FUNCTION( name, clGetMemObjectInfo );
                    REPLACE_FUNCTION( name, clGetPipeInfo );
                    REPLACE_FUNCTION( name, clGetPlatformIDs );
                    REPLACE_FUNCTION( name, clGetPlatformInfo );
                    REPLACE_FUNCTION( name, clGetProgramBuildInfo );
                    REPLACE_FUNCTION( name, clGetProgramInfo );
                    REPLACE_FUNCTION( name, clGetSamplerInfo );
                    REPLACE_FUNCTION( name, clGetSupportedImageFormats );
                    REPLACE_FUNCTION( name, clLinkProgram );
                    REPLACE_FUNCTION( name, clReleaseCommandQueue );
                    REPLACE_FUNCTION( name, clReleaseContext );
                    REPLACE_FUNCTION( name, clReleaseDevice );
                    REPLACE_FUNCTION( name, clReleaseEvent );
                    REPLACE_FUNCTION( name, clReleaseKernel );
                    REPLACE_FUNCTION( name, clReleaseMemObject );
                    REPLACE_FUNCTION( name, clReleaseProgram );
                    REPLACE_FUNCTION( name, clReleaseSampler );
                    REPLACE_FUNCTION( name, clRetainCommandQueue );
                    REPLACE_FUNCTION( name, clRetainContext );
                    REPLACE_FUNCTION( name, clRetainDevice );
                    REPLACE_FUNCTION( name, clRetainEvent );
                    REPLACE_FUNCTION( name, clRetainKernel );
                    REPLACE_FUNCTION( name, clRetainMemObject );
                    REPLACE_FUNCTION( name, clRetainProgram );
                    REPLACE_FUNCTION( name, clRetainSampler );
                    REPLACE_FUNCTION( name, clSetCommandQueueProperty );
                    REPLACE_FUNCTION( name, clSetContextDestructorCallback );
                    REPLACE_FUNCTION( name, clSetDefaultDeviceCommandQueue );
                    REPLACE_FUNCTION( name, clSetEventCallback );
                    REPLACE_FUNCTION( name, clSetKernelArg );
                    REPLACE_FUNCTION( name, clSetKernelArgSVMPointer );
                    REPLACE_FUNCTION( name, clSetKernelExecInfo );
                    REPLACE_FUNCTION( name, clSetMemObjectDestructorCallback );
                    REPLACE_FUNCTION( name, clSetUserEventStatus );
                    REPLACE_FUNCTION( name, clSVMAlloc );
                    REPLACE_FUNCTION( name, clSVMFree );
                    REPLACE_FUNCTION( name, clUnloadCompiler );
                    REPLACE_FUNCTION( name, clUnloadPlatformCompiler );
                    REPLACE_FUNCTION( name, clWaitForEvents );
                }

                origThunk++;
                firstThunk++;
            }
        }
        importDesc++;
    }

    return 0;
}

#endif
