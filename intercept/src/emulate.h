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

// cl_khr_semaphore

cl_semaphore_khr CL_API_CALL clCreateSemaphoreWithPropertiesKHR_EMU(
    cl_context context,
    const cl_semaphore_properties_khr *sema_props,
    cl_int *errcode_ret);

cl_int CL_API_CALL clEnqueueWaitSemaphoresKHR_EMU(
    cl_command_queue command_queue,
    cl_uint num_sema_objects,
    const cl_semaphore_khr *sema_objects,
    const cl_semaphore_payload_khr *sema_payload_list,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event);

cl_int CL_API_CALL clEnqueueSignalSemaphoresKHR_EMU(
    cl_command_queue command_queue,
    cl_uint num_sema_objects,
    const cl_semaphore_khr *sema_objects,
    const cl_semaphore_payload_khr *sema_payload_list,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event);

cl_int CL_API_CALL clGetSemaphoreInfoKHR_EMU(
    cl_semaphore_khr semaphore,
    cl_semaphore_info_khr param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret);

cl_int CL_API_CALL clRetainSemaphoreKHR_EMU(
    cl_semaphore_khr semaphore);

cl_int CL_API_CALL clReleaseSemaphoreKHR_EMU(
    cl_semaphore_khr semaphore);

// cl_khr_command_buffer

cl_command_buffer_khr CL_API_CALL clCreateCommandBufferKHR_EMU(
    cl_uint num_queues,
    const cl_command_queue* queues,
    const cl_command_buffer_properties_khr* properties,
    cl_int* errcode_ret);

cl_int CL_API_CALL clFinalizeCommandBufferKHR_EMU(
    cl_command_buffer_khr command_buffer);

cl_int CL_API_CALL clRetainCommandBufferKHR_EMU(
    cl_command_buffer_khr command_buffer);

cl_int CL_API_CALL clReleaseCommandBufferKHR_EMU(
    cl_command_buffer_khr command_buffer);

cl_int CL_API_CALL clEnqueueCommandBufferKHR_EMU(
    cl_uint num_queues,
    cl_command_queue* queues,
    cl_command_buffer_khr command_buffer,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

cl_int CL_API_CALL clCommandBarrierWithWaitListKHR_EMU(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle);

cl_int CL_API_CALL clCommandCopyBufferKHR_EMU(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    cl_mem src_buffer,
    cl_mem dst_buffer,
    size_t src_offset,
    size_t dst_offset,
    size_t size,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle);

cl_int CL_API_CALL clCommandCopyBufferRectKHR_EMU(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    cl_mem src_buffer,
    cl_mem dst_buffer,
    const size_t* src_origin,
    const size_t* dst_origin,
    const size_t* region,
    size_t src_row_pitch,
    size_t src_slice_pitch,
    size_t dst_row_pitch,
    size_t dst_slice_pitch,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle);

cl_int CL_API_CALL clCommandCopyBufferToImageKHR_EMU(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    cl_mem src_buffer,
    cl_mem dst_image,
    size_t src_offset,
    const size_t* dst_origin,
    const size_t* region,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle);

cl_int CL_API_CALL clCommandCopyImageKHR_EMU(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    cl_mem src_image,
    cl_mem dst_image,
    const size_t* src_origin,
    const size_t* dst_origin,
    const size_t* region,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle);

cl_int CL_API_CALL clCommandCopyImageToBufferKHR_EMU(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    cl_mem src_image,
    cl_mem dst_buffer,
    const size_t* src_origin,
    const size_t* region,
    size_t dst_offset,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle);

cl_int CL_API_CALL clCommandFillBufferKHR_EMU(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    cl_mem buffer,
    const void* pattern,
    size_t pattern_size,
    size_t offset,
    size_t size,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle);

cl_int CL_API_CALL clCommandFillImageKHR_EMU(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    cl_mem image,
    const void* fill_color,
    const size_t* origin,
    const size_t* region,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle);

cl_int CL_API_CALL clCommandNDRangeKernelKHR_EMU(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    const cl_ndrange_kernel_command_properties_khr* properties,
    cl_kernel kernel,
    cl_uint work_dim,
    const size_t* global_work_offset,
    const size_t* global_work_size,
    const size_t* local_work_size,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle);

cl_int CL_API_CALL clGetCommandBufferInfoKHR_EMU(
    cl_command_buffer_khr command_buffer,
    cl_command_buffer_info_khr param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret);
