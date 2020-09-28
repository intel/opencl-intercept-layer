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

#include "intercept.h"

// cl_intel_unified_shared_memory

void* CL_API_CALL clHostMemAllocINTEL_EMU(
    cl_context context,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret);

void* CL_API_CALL clDeviceMemAllocINTEL_EMU(
    cl_context context,
    cl_device_id device,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret);

void* CL_API_CALL clSharedMemAllocINTEL_EMU(
    cl_context context,
    cl_device_id device,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret);

cl_int CL_API_CALL clMemFreeINTEL_EMU(
    cl_context context,
    void* ptr);

cl_int CL_API_CALL clMemBlockingFreeINTEL_EMU(
    cl_context context,
    void* ptr);

cl_int CL_API_CALL clGetMemAllocInfoINTEL_EMU(
    cl_context context,
    const void* ptr,
    cl_mem_info_intel param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret);

cl_int CL_API_CALL clSetKernelArgMemPointerINTEL_EMU(
    cl_kernel kernel,
    cl_uint arg_index,
    const void* arg_value);

cl_int CL_API_CALL clEnqueueMemsetINTEL_EMU(   // Deprecated
    cl_command_queue queue,
    void* dst_ptr,
    cl_int value,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

cl_int CL_API_CALL clEnqueueMemFillINTEL_EMU(
    cl_command_queue queue,
    void* dst_ptr,
    const void* pattern,
    size_t pattern_size,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

cl_int CL_API_CALL clEnqueueMemcpyINTEL_EMU(
    cl_command_queue queue,
    cl_bool blocking,
    void* dst_ptr,
    const void* src_ptr,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

cl_int CL_API_CALL clEnqueueMigrateMemINTEL_EMU(
    cl_command_queue queue,
    const void* ptr,
    size_t size,
    cl_mem_migration_flags flags,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

cl_int CL_API_CALL clEnqueueMemAdviseINTEL_EMU(
    cl_command_queue queue,
    const void* ptr,
    size_t size,
    cl_mem_advice_intel advice,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);
