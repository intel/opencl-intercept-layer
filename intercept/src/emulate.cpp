/*
// Copyright (c) 2018-2020 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
*/

#include <string>

#include "intercept.h"

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
void* CL_API_CALL clHostMemAllocINTEL_EMU(
    cl_context context,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->config().Emulate_cl_intel_unified_shared_memory )
    {
        return pIntercept->emulatedHostMemAlloc(
            context,
            properties,
            size,
            alignment,
            errcode_ret );
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
void* CL_API_CALL clDeviceMemAllocINTEL_EMU(
    cl_context context,
    cl_device_id device,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->config().Emulate_cl_intel_unified_shared_memory )
    {
        return pIntercept->emulatedDeviceMemAlloc(
            context,
            device,
            properties,
            size,
            alignment,
            errcode_ret );
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
void* CL_API_CALL clSharedMemAllocINTEL_EMU(
    cl_context context,
    cl_device_id device,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->config().Emulate_cl_intel_unified_shared_memory )
    {
        return pIntercept->emulatedSharedMemAlloc(
            context,
            device,
            properties,
            size,
            alignment,
            errcode_ret );
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
cl_int CL_API_CALL clMemFreeINTEL_EMU(
    cl_context context,
    void* ptr)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->config().Emulate_cl_intel_unified_shared_memory )
    {
        return pIntercept->emulatedMemFree(
            context,
            ptr );
    }

    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
cl_int CL_API_CALL clMemBlockingFreeINTEL_EMU(
    cl_context context,
    void* ptr)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->config().Emulate_cl_intel_unified_shared_memory )
    {
        // TODO: Track queues and block all.
        return pIntercept->emulatedMemFree(
            context,
            ptr );
    }

    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
cl_int CL_API_CALL clGetMemAllocInfoINTEL_EMU(
    cl_context context,
    const void* ptr,
    cl_mem_info_intel param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->config().Emulate_cl_intel_unified_shared_memory )
    {
        return pIntercept->emulatedGetMemAllocInfoINTEL(
            context,
            ptr,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret );
    }

    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
cl_int CL_API_CALL clSetKernelArgMemPointerINTEL_EMU(
    cl_kernel kernel,
    cl_uint arg_index,
    const void* arg_value)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->config().Emulate_cl_intel_unified_shared_memory &&
        pIntercept->dispatch().clSetKernelArgSVMPointer )
    {
        return pIntercept->dispatch().clSetKernelArgSVMPointer(
            kernel,
            arg_index,
            arg_value );
    }

    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
cl_int CL_API_CALL clEnqueueMemsetINTEL_EMU(   // Deprecated
    cl_command_queue queue,
    void* dst_ptr,
    cl_int value,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->config().Emulate_cl_intel_unified_shared_memory &&
        pIntercept->dispatch().clEnqueueSVMMemFill )
    {
        const cl_uchar  pattern = (cl_uchar)value;
        return pIntercept->dispatch().clEnqueueSVMMemFill(
            queue,
            dst_ptr,
            &pattern,
            sizeof(pattern),
            size,
            num_events_in_wait_list,
            event_wait_list,
            event );
    }

    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
cl_int CL_API_CALL clEnqueueMemFillINTEL_EMU(
    cl_command_queue queue,
    void* dst_ptr,
    const void* pattern,
    size_t pattern_size,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->config().Emulate_cl_intel_unified_shared_memory &&
        pIntercept->dispatch().clEnqueueSVMMemFill )
    {
        return pIntercept->dispatch().clEnqueueSVMMemFill(
            queue,
            dst_ptr,
            &pattern,
            sizeof(pattern),
            size,
            num_events_in_wait_list,
            event_wait_list,
            event );
    }

    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
cl_int CL_API_CALL clEnqueueMemcpyINTEL_EMU(
    cl_command_queue queue,
    cl_bool blocking,
    void* dst_ptr,
    const void* src_ptr,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->config().Emulate_cl_intel_unified_shared_memory &&
        pIntercept->dispatch().clEnqueueSVMMemcpy )
    {
        return pIntercept->dispatch().clEnqueueSVMMemcpy(
            queue,
            blocking,
            dst_ptr,
            src_ptr,
            size,
            num_events_in_wait_list,
            event_wait_list,
            event );
    }

    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
cl_int CL_API_CALL clEnqueueMigrateMemINTEL_EMU(
    cl_command_queue queue,
    const void* ptr,
    size_t size,
    cl_mem_migration_flags flags,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->config().Emulate_cl_intel_unified_shared_memory )
    {
        // We could check for OpenCL 2.1 and call the SVM migrate
        // functions, but for now we'll just enqueue a marker.
#if 0
        return pIntercept->dispatch().clEnqueueSVMMigrateMem(
            queue,
            1,
            &ptr,
            &size,
            flags,
            num_events_in_wait_list,
            event_wait_list,
            event );
#else
        return pIntercept->dispatch().clEnqueueMarkerWithWaitList(
            queue,
            num_events_in_wait_list,
            event_wait_list,
            event );
#endif
    }

    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
cl_int CL_API_CALL clEnqueueMemAdviseINTEL_EMU(
    cl_command_queue queue,
    const void* ptr,
    size_t size,
    cl_mem_advice_intel advice,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->config().Emulate_cl_intel_unified_shared_memory )
    {
        // TODO: What should we do here?
        return pIntercept->dispatch().clEnqueueMarkerWithWaitList(
            queue,
            num_events_in_wait_list,
            event_wait_list,
            event );
    }

    return CL_INVALID_OPERATION;
}
