/*
// Copyright (c) 2018-2021 Intel Corporation
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

#pragma once

#include "common.h"
#include "cli_ext.h"

#include "CL/cl_icd.h"

// Dispatch table for extension APIs:

struct CLdispatchX
{
    // cl_khr_gl_event
    cl_event    (CL_API_CALL *clCreateEventFromGLsyncKHR) (
                cl_context context,
                cl_GLsync sync,
                cl_int* errcode_ret);

#if defined(_WIN32)
    // cl_khr_d3d10_sharing
    cl_int  (CL_API_CALL *clGetDeviceIDsFromD3D10KHR) (
                cl_platform_id platform,
                cl_d3d10_device_source_khr d3d_device_source,
                void* d3d_object,
                cl_d3d10_device_set_khr d3d_device_set,
                cl_uint num_entries,
                cl_device_id* devices,
                cl_uint* num_devices);

    // cl_khr_d3d10_sharing
    cl_mem  (CL_API_CALL *clCreateFromD3D10BufferKHR) (
                cl_context context,
                cl_mem_flags flags,
                ID3D10Buffer* resource,
                cl_int* errcode_ret);

    // cl_khr_d3d10_sharing
    cl_mem  (CL_API_CALL *clCreateFromD3D10Texture2DKHR) (
                cl_context context,
                cl_mem_flags flags,
                ID3D10Texture2D* resource,
                UINT subresource,
                cl_int* errcode_ret);

    // cl_khr_d3d10_sharing
    cl_mem  (CL_API_CALL *clCreateFromD3D10Texture3DKHR) (
                cl_context context,
                cl_mem_flags flags,
                ID3D10Texture3D* resource,
                UINT subresource,
                cl_int* errcode_ret);

    // cl_khr_d3d10_sharing
    cl_int  (CL_API_CALL *clEnqueueAcquireD3D10ObjectsKHR) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    // cl_khr_d3d10_sharing
    cl_int  (CL_API_CALL *clEnqueueReleaseD3D10ObjectsKHR) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    // cl_khr_d3d11_sharing
    cl_int  (CL_API_CALL *clGetDeviceIDsFromD3D11KHR) (
                cl_platform_id platform,
                cl_d3d11_device_source_khr d3d_device_source,
                void* d3d_object,
                cl_d3d11_device_set_khr d3d_device_set,
                cl_uint num_entries,
                cl_device_id* devices,
                cl_uint* num_devices);

    // cl_khr_d3d11_sharing
    cl_mem  (CL_API_CALL *clCreateFromD3D11BufferKHR) (
                cl_context context,
                cl_mem_flags flags,
                ID3D11Buffer* resource,
                cl_int* errcode_ret);

    // cl_khr_d3d11_sharing
    cl_mem  (CL_API_CALL *clCreateFromD3D11Texture2DKHR) (
                cl_context context,
                cl_mem_flags flags,
                ID3D11Texture2D* resource,
                UINT subresource,
                cl_int* errcode_ret);

    // cl_khr_d3d11_sharing
    cl_mem  (CL_API_CALL *clCreateFromD3D11Texture3DKHR) (
                cl_context context,
                cl_mem_flags flags,
                ID3D11Texture3D* resource,
                UINT subresource,
                cl_int* errcode_ret);

    // cl_khr_d3d11_sharing
    cl_int  (CL_API_CALL *clEnqueueAcquireD3D11ObjectsKHR) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    // cl_khr_d3d11_sharing
    cl_int  (CL_API_CALL *clEnqueueReleaseD3D11ObjectsKHR) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    // cl_khr_dx9_media_sharing
    cl_int  (CL_API_CALL *clGetDeviceIDsFromDX9MediaAdapterKHR) (
                cl_platform_id platform,
                cl_uint num_media_adapters,
                cl_dx9_media_adapter_type_khr* media_adapters_type,
                void* media_adapters,
                cl_dx9_media_adapter_set_khr media_adapter_set,
                cl_uint num_entries,
                cl_device_id* devices,
                cl_uint* num_devices);

    // cl_khr_dx9_media_sharing
    cl_mem  (CL_API_CALL *clCreateFromDX9MediaSurfaceKHR) (
                cl_context context,
                cl_mem_flags flags,
                cl_dx9_media_adapter_type_khr adapter_type,
                void* surface_info,
                cl_uint plane,
                cl_int* errcode_ret);

    // cl_khr_dx9_media_sharing
    cl_int  (CL_API_CALL *clEnqueueAcquireDX9MediaSurfacesKHR) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    // cl_khr_dx9_media_sharing
    cl_int  (CL_API_CALL *clEnqueueReleaseDX9MediaSurfacesKHR) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);
#endif

    // cl_khr_il_program
    cl_program (CL_API_CALL *clCreateProgramWithILKHR) (
                cl_context context,
                const void *il,
                size_t length,
                cl_int *errcode_ret);

    // cl_khr_subgroups
    cl_int  (CL_API_CALL *clGetKernelSubGroupInfoKHR) (
                cl_kernel kernel,
                cl_device_id device,
                cl_kernel_sub_group_info param_name,
                size_t input_value_size,
                const void* input_value,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret);

    // cl_khr_suggested_local_work_size
    cl_int  (CL_API_CALL *clGetKernelSuggestedLocalWorkSizeKHR) (
                cl_command_queue command_queue,
                cl_kernel kernel,
                cl_uint work_dim,
                const size_t* global_work_offset,
                const size_t* global_work_size,
                size_t* suggested_local_work_size);

    // cl_khr_create_command_queue
    cl_command_queue    (CL_API_CALL *clCreateCommandQueueWithPropertiesKHR) (
            cl_context context,
            cl_device_id device,
            const cl_queue_properties_khr* properties,
            cl_int* errcode_ret);

#if defined(_WIN32)
    // cl_intel_dx9_media_sharing
    cl_int  (CL_API_CALL *clGetDeviceIDsFromDX9INTEL) (
                cl_platform_id platform,
                cl_dx9_device_source_intel d3d_device_source,
                void *dx9_object,
                cl_dx9_device_set_intel d3d_device_set,
                cl_uint num_entries,
                cl_device_id* devices,
                cl_uint* num_devices );

    // cl_intel_dx9_media_sharing
    cl_mem  (CL_API_CALL *clCreateFromDX9MediaSurfaceINTEL) (
                cl_context context,
                cl_mem_flags flags,
                IDirect3DSurface9* resource,
                HANDLE sharedHandle,
                UINT plane,
                cl_int* errcode_ret );

    // cl_intel_dx9_media_sharing
    cl_int  (CL_API_CALL *clEnqueueAcquireDX9ObjectsINTEL) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    // cl_intel_dx9_media_sharing
    cl_int  (CL_API_CALL *clEnqueueReleaseDX9ObjectsINTEL) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );
#endif

    // Unofficial MDAPI extension:
    cl_command_queue    (CL_API_CALL *clCreatePerfCountersCommandQueueINTEL) (
                cl_context context,
                cl_device_id device,
                cl_command_queue_properties properties,
                cl_uint configuration,
                cl_int* errcode_ret);

    // Unofficial MDAPI extension:
    cl_int (CL_API_CALL *clSetPerformanceConfigurationINTEL)(
        cl_device_id    device,
        cl_uint         count,
        cl_uint*        offsets,
        cl_uint*        values );

    // Unofficial cl_get_kernel_suggested_local_work_size extension:
    cl_int (CL_API_CALL *clGetKernelSuggestedLocalWorkSizeINTEL)(
        cl_command_queue commandQueue,
        cl_kernel kernel,
        cl_uint workDim,
        const size_t *globalWorkOffset,
        const size_t *globalWorkSize,
        size_t *suggestedLocalWorkSize);

    // cl_intel_accelerator
    cl_accelerator_intel (CL_API_CALL *clCreateAcceleratorINTEL) (
        cl_context context,
        cl_accelerator_type_intel accelerator_type,
        size_t descriptor_size,
        const void* descriptor,
        cl_int* errcode_ret );

    // cl_intel_accelerator
    cl_int (CL_API_CALL *clGetAcceleratorInfoINTEL) (
        cl_accelerator_intel accelerator,
        cl_accelerator_info_intel param_name,
        size_t param_value_size,
        void* param_value,
        size_t* param_value_size_ret );

    // cl_intel_accelerator
    cl_int (CL_API_CALL *clRetainAcceleratorINTEL) (
        cl_accelerator_intel accelerator );

    // cl_intel_accelerator
    cl_int (CL_API_CALL *clReleaseAcceleratorINTEL) (
        cl_accelerator_intel accelerator );

    // cl_intel_va_api_media_sharing
    cl_int (CL_API_CALL *clGetDeviceIDsFromVA_APIMediaAdapterINTEL) (
        cl_platform_id platform,
        cl_va_api_device_source_intel media_adapter_type,
        void *media_adapter,
        cl_va_api_device_set_intel media_adapter_set,
        cl_uint num_entries,
        cl_device_id *devices,
        cl_uint *num_devices);

    // cl_intel_va_api_media_sharing
    cl_mem (CL_API_CALL *clCreateFromVA_APIMediaSurfaceINTEL) (
        cl_context context,
        cl_mem_flags flags,
        VASurfaceID *surface,
        cl_uint plane,
        cl_int *errcode_ret);

    // cl_intel_va_api_media_sharing
    cl_int (CL_API_CALL *clEnqueueAcquireVA_APIMediaSurfacesINTEL) (
        cl_command_queue command_queue,
        cl_uint num_objects,
        const cl_mem *mem_objects,
        cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list,
        cl_event *event);

    // cl_intel_va_api_media_sharing
    cl_int (CL_API_CALL *clEnqueueReleaseVA_APIMediaSurfacesINTEL) (
        cl_command_queue command_queue,
        cl_uint num_objects,
        const cl_mem *mem_objects,
        cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list,
        cl_event *event);

    // cl_intel_unified_shared_memory
    void* (CL_API_CALL *clHostMemAllocINTEL) (
        cl_context context,
        const cl_mem_properties_intel* properties,
        size_t size,
        cl_uint alignment,
        cl_int* errcode_ret);

    // cl_intel_unified_shared_memory
    void* (CL_API_CALL *clDeviceMemAllocINTEL) (
        cl_context context,
        cl_device_id device,
        const cl_mem_properties_intel* properties,
        size_t size,
        cl_uint alignment,
        cl_int* errcode_ret);

    // cl_intel_unified_shared_memory
    void* (CL_API_CALL *clSharedMemAllocINTEL) (
        cl_context context,
        cl_device_id device,
        const cl_mem_properties_intel* properties,
        size_t size,
        cl_uint alignment,
        cl_int* errcode_ret);

    // cl_intel_unified_shared_memory
    cl_int (CL_API_CALL *clMemFreeINTEL) (
        cl_context context,
        void* ptr);

    // cl_intel_unified_shared_memory
    cl_int (CL_API_CALL *clMemBlockingFreeINTEL) (
        cl_context context,
        void* ptr);

    // cl_intel_unified_shared_memory
    cl_int (CL_API_CALL *clGetMemAllocInfoINTEL) (
        cl_context context,
        const void* ptr,
        cl_mem_info_intel param_name,
        size_t param_value_size,
        void* param_value,
        size_t* param_value_size_ret);

    // cl_intel_unified_shared_memory
    cl_int (CL_API_CALL *clSetKernelArgMemPointerINTEL) (
        cl_kernel kernel,
        cl_uint arg_index,
        const void* arg_value);

    // cl_intel_unified_shared_memory
    cl_int (CL_API_CALL *clEnqueueMemsetINTEL) (   // Deprecated
        cl_command_queue command_queue,
        void* dst_ptr,
        cl_int value,
        size_t size,
        cl_uint num_events_in_wait_list,
        const cl_event* event_wait_list,
        cl_event* event);

    // cl_intel_unified_shared_memory
    cl_int (CL_API_CALL *clEnqueueMemFillINTEL) (
        cl_command_queue command_queue,
        void* dst_ptr,
        const void* pattern,
        size_t pattern_size,
        size_t size,
        cl_uint num_events_in_wait_list,
        const cl_event* event_wait_list,
        cl_event* event);

    // cl_intel_unified_shared_memory
    cl_int (CL_API_CALL *clEnqueueMemcpyINTEL) (
        cl_command_queue command_queue,
        cl_bool blocking,
        void* dst_ptr,
        const void* src_ptr,
        size_t size,
        cl_uint num_events_in_wait_list,
        const cl_event* event_wait_list,
        cl_event* event);

    // cl_intel_unified_shared_memory
    cl_int (CL_API_CALL *clEnqueueMigrateMemINTEL) (
        cl_command_queue command_queue,
        const void* ptr,
        size_t size,
        cl_mem_migration_flags flags,
        cl_uint num_events_in_wait_list,
        const cl_event* event_wait_list,
        cl_event* event);

    // cl_intel_unified_shared_memory
    cl_int (CL_API_CALL *clEnqueueMemAdviseINTEL) (
        cl_command_queue command_queue,
        const void* ptr,
        size_t size,
        cl_mem_advice_intel advice,
        cl_uint num_events_in_wait_list,
        const cl_event* event_wait_list,
        cl_event* event);

    // cl_nv_create_buffer
    cl_mem (CL_API_CALL *clCreateBufferNV) (
        cl_context context,
        cl_mem_flags flags,
        cl_mem_flags_NV flags_NV,
        size_t size,
        void* host_ptr,
        cl_int* errcode_ret);
};
