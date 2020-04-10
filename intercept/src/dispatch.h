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

#pragma once

#include "common.h"
#include "cli_ext.h"

#define CLI_API_ENTRY   CL_API_ENTRY
#define CLI_API_CALL    CL_API_CALL

struct CLdispatch
{
    cl_int  (CLI_API_CALL *clGetPlatformIDs) (
                cl_uint num_entries,
                cl_platform_id* platforms,
                cl_uint* num_platforms );

    cl_int  (CLI_API_CALL *clGetPlatformInfo) (
                cl_platform_id platform,
                cl_platform_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    cl_int  (CLI_API_CALL *clGetDeviceIDs) (
                cl_platform_id platform,
                cl_device_type device_type,
                cl_uint num_entries,
                cl_device_id* devices,
                cl_uint* num_devices );

    cl_int  (CLI_API_CALL *clGetDeviceInfo) (
                cl_device_id device,
                cl_device_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    // OpenCL 1.2
    cl_int  (CLI_API_CALL *clCreateSubDevices) (
                cl_device_id in_device,
                const cl_device_partition_property* properties,
                cl_uint num_devices,
                cl_device_id* out_devices,
                cl_uint* num_devices_ret );

    // OpenCL 1.2
    cl_int  (CLI_API_CALL *clRetainDevice) (
                cl_device_id device );

    // OpenCL 1.2
    cl_int  (CLI_API_CALL *clReleaseDevice) (
                cl_device_id device );

    cl_context  (CLI_API_CALL *clCreateContext) (
                const cl_context_properties* properties,
                cl_uint num_devices,
                const cl_device_id* devices,
                void (CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
                void* user_data,
                cl_int* errcode_ret );

    cl_context  (CLI_API_CALL *clCreateContextFromType) (
                const cl_context_properties* properties,
                cl_device_type device_type,
                void (CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
                void* user_data,
                cl_int* errcode_ret );

    cl_int  (CLI_API_CALL *clRetainContext) (
                cl_context context );

    cl_int  (CLI_API_CALL *clReleaseContext) (
                cl_context context );

    cl_int  (CLI_API_CALL *clGetContextInfo) (
                cl_context context,
                cl_context_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    cl_command_queue    (CLI_API_CALL *clCreateCommandQueue) (
                cl_context context,
                cl_device_id device,
                cl_command_queue_properties properties,
                cl_int* errcode_ret );

    cl_int  (CLI_API_CALL *clRetainCommandQueue) (
                cl_command_queue command_queue );

    cl_int  (CLI_API_CALL *clReleaseCommandQueue) (
                cl_command_queue command_queue );

    cl_int  (CLI_API_CALL *clGetCommandQueueInfo) (
                cl_command_queue command_queue,
                cl_command_queue_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    // deprecated OpenCL 1.0
    cl_int  (CLI_API_CALL *clSetCommandQueueProperty) (
                cl_command_queue command_queue,
                cl_command_queue_properties properties,
                cl_bool enable,
                cl_command_queue_properties* old_properties );

    cl_mem  (CLI_API_CALL *clCreateBuffer) (
                cl_context context,
                cl_mem_flags flags,
                size_t size,
                void* host_ptr,
                cl_int* errcode_ret );

    // OpenCL 1.1
    cl_mem  (CLI_API_CALL *clCreateSubBuffer) (
                cl_mem buffer,
                cl_mem_flags flags,
                cl_buffer_create_type buffer_create_type,
                const void *buffer_create_info,
                cl_int *errcode_ret );

    // OpenCL 1.2
    cl_mem  (CLI_API_CALL *clCreateImage) (
                cl_context context,
                cl_mem_flags flags,
                const cl_image_format* image_format,
                const cl_image_desc* image_desc,
                void* host_ptr,
                cl_int* errcode_ret );

    // deprecated OpenCL 1.1
    cl_mem  (CLI_API_CALL *clCreateImage2D) (
                cl_context context,
                cl_mem_flags flags,
                const cl_image_format* image_format,
                size_t image_width,
                size_t image_height,
                size_t image_row_pitch,
                void* host_ptr,
                cl_int* errcode_ret );

    // deprecated OpenCL 1.1
    cl_mem  (CLI_API_CALL *clCreateImage3D) (
                cl_context context,
                cl_mem_flags flags,
                const cl_image_format* image_format,
                size_t image_width,
                size_t image_height,
                size_t image_depth,
                size_t image_row_pitch,
                size_t image_slice_pitch,
                void* host_ptr,
                cl_int* errcode_ret );

    cl_int  (CLI_API_CALL *clRetainMemObject) (
                cl_mem memobj );

    cl_int  (CLI_API_CALL *clReleaseMemObject) (
                cl_mem memobj );

    cl_int  (CLI_API_CALL *clGetSupportedImageFormats) (
                cl_context context,
                cl_mem_flags flags,
                cl_mem_object_type image_type,
                cl_uint num_entries,
                cl_image_format* image_formats,
                cl_uint* num_image_formats );

    cl_int  (CLI_API_CALL *clGetMemObjectInfo) (
                cl_mem memobj,
                cl_mem_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    cl_int  (CLI_API_CALL *clGetImageInfo) (
                cl_mem image,
                cl_image_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    // OpenCL 1.1
    cl_int  (CLI_API_CALL *clSetMemObjectDestructorCallback) (
                cl_mem memobj,
                void (CL_CALLBACK *pfn_notify)( cl_mem, void* ),
                void *user_data );

    cl_sampler  (CLI_API_CALL *clCreateSampler) (
                cl_context context,
                cl_bool normalized_coords,
                cl_addressing_mode addressing_mode,
                cl_filter_mode filter_mode,
                cl_int* errcode_ret );

    cl_int  (CLI_API_CALL *clRetainSampler) (
                cl_sampler sampler );

    cl_int  (CLI_API_CALL *clReleaseSampler) (
                cl_sampler sampler );

    cl_int  (CLI_API_CALL *clGetSamplerInfo) (
                cl_sampler sampler,
                cl_sampler_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    cl_program  (CLI_API_CALL *clCreateProgramWithSource) (
                cl_context context,
                cl_uint count,
                const char** strings,
                const size_t* lengths,
                cl_int* errcode_ret );

    cl_program  (CLI_API_CALL *clCreateProgramWithBinary) (
                cl_context context,
                cl_uint num_devices,
                const cl_device_id* device_list,
                const size_t* lengths,
                const unsigned char** binaries,
                cl_int* binary_status,
                cl_int* errcode_ret );

    // OpenCL 1.2
    cl_program  (CLI_API_CALL *clCreateProgramWithBuiltInKernels) (
                cl_context context,
                cl_uint num_devices,
                const cl_device_id* device_list,
                const char* kernel_names,
                cl_int* errcode_ret);

    cl_int  (CLI_API_CALL *clRetainProgram) (
                cl_program program );

    cl_int  (CLI_API_CALL *clReleaseProgram) (
                cl_program program );

    cl_int  (CLI_API_CALL *clBuildProgram) (
                cl_program program,
                cl_uint num_devices,
                const cl_device_id* device_list,
                const char* options,
                void (CL_CALLBACK *pfn_notify)(cl_program program, void* user_data),
                void* user_data );

    // OpenCL 1.2
    cl_int  (CLI_API_CALL *clCompileProgram) (
                cl_program program,
                cl_uint num_devices,
                const cl_device_id* device_list,
                const char* options,
                cl_uint num_input_headers,
                const cl_program* input_headers,
                const char** header_include_names,
                void (CL_CALLBACK *pfn_notify)(cl_program program , void* user_data),
                void* user_data );

    // OpenCL 1.2
    cl_program  (CLI_API_CALL *clLinkProgram) (
                cl_context context,
                cl_uint num_devices,
                const cl_device_id* device_list,
                const char* options,
                cl_uint num_input_programs,
                const cl_program* input_programs,
                void (CL_CALLBACK *pfn_notify)(cl_program program, void* user_data),
                void* user_data,
                cl_int* errcode_ret );

    // OpenCL 2.2
    cl_int  (CLI_API_CALL *clSetProgramReleaseCallback) (
                cl_program program,
                void (CL_CALLBACK *pfn_notify)(cl_program program, void* user_data),
                void* user_data );

    // OpenCL 2.2
    cl_int  (CLI_API_CALL *clSetProgramSpecializationConstant) (
                cl_program program,
                cl_uint spec_id,
                size_t spec_size,
                const void* spec_value );

    // OpenCL 1.2
    cl_int  (CLI_API_CALL *clUnloadPlatformCompiler) (
                cl_platform_id platform );

    // deprecated OpenCL 1.1
    cl_int  (CLI_API_CALL *clUnloadCompiler) ( void );

    cl_int  (CLI_API_CALL *clGetProgramInfo) (
                cl_program program,
                cl_program_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    cl_int  (CLI_API_CALL *clGetProgramBuildInfo) (
                cl_program program,
                cl_device_id device,
                cl_program_build_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    cl_kernel   (CLI_API_CALL *clCreateKernel) (
                cl_program program,
                const char* kernel_name,
                cl_int* errcode_ret );

    cl_int  (CLI_API_CALL *clCreateKernelsInProgram) (
                cl_program program,
                cl_uint num_kernels,
                cl_kernel* kernels,
                cl_uint* num_kernels_ret );

    cl_int  (CLI_API_CALL *clRetainKernel) (
                cl_kernel kernel );

    cl_int  (CLI_API_CALL *clReleaseKernel) (
                cl_kernel kernel );

    cl_int  (CLI_API_CALL *clSetKernelArg) (
                cl_kernel kernel,
                cl_uint arg_index,
                size_t arg_size,
                const void* arg_value );

    cl_int  (CLI_API_CALL *clGetKernelInfo) (
                cl_kernel kernel,
                cl_kernel_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    // OpenCL 1.2
    cl_int  (CLI_API_CALL *clGetKernelArgInfo) (
                cl_kernel kernel,
                cl_uint arg_indx,
                cl_kernel_arg_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    cl_int  (CLI_API_CALL *clGetKernelWorkGroupInfo) (
                cl_kernel kernel,
                cl_device_id device,
                cl_kernel_work_group_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    cl_int  (CLI_API_CALL *clWaitForEvents) (
                cl_uint num_events,
                const cl_event* event_list );

    cl_int  (CLI_API_CALL *clGetEventInfo) (
                cl_event event,
                cl_event_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    // OpenCL 1.1
    cl_event (CLI_API_CALL *clCreateUserEvent) (
                cl_context context,
                cl_int *errcode_ret );

    cl_int  (CLI_API_CALL *clRetainEvent) (
                cl_event event );

    cl_int  (CLI_API_CALL *clReleaseEvent) (
                cl_event event );

    // OpenCL 1.1
    cl_int  (CLI_API_CALL *clSetUserEventStatus) (
                cl_event event,
                cl_int execution_status );

    // OpenCL 1.1
    cl_int  (CLI_API_CALL *clSetEventCallback) (
                cl_event event,
                cl_int command_exec_callback_type,
                void (CL_CALLBACK *pfn_notify)( cl_event, cl_int, void * ),
                void *user_data );

    cl_int  (CLI_API_CALL *clGetEventProfilingInfo) (
                cl_event event,
                cl_profiling_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    cl_int  (CLI_API_CALL *clFlush) (
                cl_command_queue command_queue );

    cl_int  (CLI_API_CALL *clFinish) (
                cl_command_queue command_queue );

    cl_int  (CLI_API_CALL *clEnqueueReadBuffer) (
                cl_command_queue command_queue,
                cl_mem buffer,
                cl_bool blocking_read,
                size_t offset,
                size_t cb,
                void* ptr,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    // OpenCL 1.1
    cl_int  (CLI_API_CALL *clEnqueueReadBufferRect) (
                cl_command_queue command_queue,
                cl_mem buffer,
                cl_bool blocking_read,
                const size_t *buffer_origin,
                const size_t *host_origin,
                const size_t *region,
                size_t buffer_row_pitch,
                size_t buffer_slice_pitch,
                size_t host_row_pitch,
                size_t host_slice_pitch,
                void *ptr,
                cl_uint num_events_in_wait_list,
                const cl_event *event_wait_list,
                cl_event *event );

    cl_int  (CLI_API_CALL *clEnqueueWriteBuffer) (
                cl_command_queue command_queue,
                cl_mem buffer,
                cl_bool blocking_write,
                size_t offset,
                size_t cb,
                const void* ptr,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    // OpenCL 1.1
    cl_int  (CLI_API_CALL *clEnqueueWriteBufferRect) (
                cl_command_queue command_queue,
                cl_mem buffer,
                cl_bool blocking_write,
                const size_t *buffer_origin,
                const size_t *host_origin,
                const size_t *region,
                size_t buffer_row_pitch,
                size_t buffer_slice_pitch,
                size_t host_row_pitch,
                size_t host_slice_pitch,
                const void *ptr,
                cl_uint num_events_in_wait_list,
                const cl_event *event_wait_list,
                cl_event *event );

    // OpenCL 1.2
    cl_int  (CLI_API_CALL *clEnqueueFillBuffer) (
                cl_command_queue command_queue,
                cl_mem buffer,
                const void* pattern,
                size_t pattern_size,
                size_t offset,
                size_t size,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    cl_int  (CLI_API_CALL *clEnqueueCopyBuffer) (
                cl_command_queue command_queue,
                cl_mem src_buffer,
                cl_mem dst_buffer,
                size_t src_offset,
                size_t dst_offset,
                size_t cb,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    // OpenCL 1.1
    cl_int  (CLI_API_CALL *clEnqueueCopyBufferRect) (
                cl_command_queue command_queue,
                cl_mem src_buffer,
                cl_mem dst_buffer,
                const size_t *src_origin,
                const size_t *dst_origin,
                const size_t *region,
                size_t src_row_pitch,
                size_t src_slice_pitch,
                size_t dst_row_pitch,
                size_t dst_slice_pitch,
                cl_uint num_events_in_wait_list,
                const cl_event *event_wait_list,
                cl_event *event );

    cl_int  (CLI_API_CALL *clEnqueueReadImage) (
                cl_command_queue command_queue,
                cl_mem image,
                cl_bool blocking_read,
                const size_t* origin,
                const size_t* region,
                size_t row_pitch,
                size_t slice_pitch,
                void* ptr,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    cl_int  (CLI_API_CALL *clEnqueueWriteImage) (
                cl_command_queue command_queue,
                cl_mem image,
                cl_bool blocking_write,
                const size_t* origin,
                const size_t* region,
                size_t input_row_pitch,
                size_t input_slice_pitch,
                const void* ptr,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    // OpenCL 1.2
    cl_int  (CLI_API_CALL *clEnqueueFillImage) (
                cl_command_queue command_queue,
                cl_mem image,
                const void* fill_color,
                const size_t* origin,
                const size_t* region,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    cl_int  (CLI_API_CALL *clEnqueueCopyImage) (
                cl_command_queue command_queue,
                cl_mem src_image,
                cl_mem dst_image,
                const size_t* src_origin,
                const size_t* dst_origin,
                const size_t* region,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    cl_int  (CLI_API_CALL *clEnqueueCopyImageToBuffer) (
                cl_command_queue command_queue,
                cl_mem src_image,
                cl_mem dst_buffer,
                const size_t* src_origin,
                const size_t* region,
                size_t dst_offset,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    cl_int  (CLI_API_CALL *clEnqueueCopyBufferToImage) (
                cl_command_queue command_queue,
                cl_mem src_buffer,
                cl_mem dst_image,
                size_t src_offset,
                const size_t* dst_origin,
                const size_t* region,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    void*   (CLI_API_CALL *clEnqueueMapBuffer) (
                cl_command_queue command_queue,
                cl_mem buffer,
                cl_bool blocking_map,
                cl_map_flags map_flags,
                size_t offset,
                size_t cb,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event,
                cl_int* errcode_ret );

    void*  (CLI_API_CALL *clEnqueueMapImage) (
                cl_command_queue command_queue,
                cl_mem image,
                cl_bool blocking_map,
                cl_map_flags map_flags,
                const size_t* origin,
                const size_t* region,
                size_t* image_row_pitch,
                size_t* image_slice_pitch,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event,
                cl_int* errcode_ret );

    cl_int  (CLI_API_CALL *clEnqueueUnmapMemObject) (
                cl_command_queue command_queue,
                cl_mem memobj,
                void* mapped_ptr,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    // OpenCL 1.2
    cl_int  (CLI_API_CALL *clEnqueueMigrateMemObjects) (
                cl_command_queue command_queue,
                cl_uint num_mem_objects,
                const cl_mem* mem_objects,
                cl_mem_migration_flags flags,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    cl_int  (CLI_API_CALL *clEnqueueNDRangeKernel) (
                cl_command_queue command_queue,
                cl_kernel kernel,
                cl_uint work_dim,
                const size_t* global_work_offset,
                const size_t* global_work_size,
                const size_t* local_work_size,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    cl_int  (CLI_API_CALL *clEnqueueTask) (
                cl_command_queue command_queue,
                cl_kernel kernel,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    cl_int  (CLI_API_CALL *clEnqueueNativeKernel) (
                cl_command_queue command_queue,
                void (CL_CALLBACK *user_func)(void *),
                void* args,
                size_t cb_args,
                cl_uint num_mem_objects,
                const cl_mem* mem_list,
                const void** args_mem_loc,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    // deprecated OpenCL 1.1
    cl_int  (CLI_API_CALL *clEnqueueMarker) (
                cl_command_queue command_queue,
                cl_event* event );

    // deprecated OpenCL 1.1
    cl_int  (CLI_API_CALL *clEnqueueWaitForEvents) (
                cl_command_queue command_queue,
                cl_uint num_events,
                const cl_event* event_list );

    // deprecated OpenCL 1.1
    cl_int  (CLI_API_CALL *clEnqueueBarrier) (
                cl_command_queue command_queue );

    // OpenCL 1.2
    cl_int  (CLI_API_CALL *clEnqueueMarkerWithWaitList) (
                cl_command_queue command_queue,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    // OpenCL 1.2
    cl_int  (CLI_API_CALL *clEnqueueBarrierWithWaitList) (
                cl_command_queue command_queue,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    // Optional?
    // deprecated OpenCL 1.1
    void*   (CLI_API_CALL *clGetExtensionFunctionAddress) (
                const char* func_name );

    // Optional?
    // OpenCL 1.2
    void*   (CLI_API_CALL *clGetExtensionFunctionAddressForPlatform)(
                cl_platform_id platform,
                const char* func_name );

    // CL-GL Sharing

    cl_mem  (CLI_API_CALL *clCreateFromGLBuffer) (
                cl_context context,
                cl_mem_flags flags,
                cl_GLuint bufobj,
                int* errcode_ret);  // Not cl_int*?

    // OpenCL 1.2
    cl_mem  (CLI_API_CALL *clCreateFromGLTexture) (
                cl_context context,
                cl_mem_flags flags,
                cl_GLenum target,
                cl_GLint miplevel,
                cl_GLuint texture,
                cl_int* errcode_ret );

    // deprecated OpenCL 1.1
    cl_mem  (CLI_API_CALL *clCreateFromGLTexture2D) (
                cl_context context,
                cl_mem_flags flags,
                cl_GLenum target,
                cl_GLint miplevel,
                cl_GLuint texture,
                cl_int* errcode_ret);

    // deprecated OpenCL 1.1
    cl_mem  (CLI_API_CALL *clCreateFromGLTexture3D) (
                cl_context context,
                cl_mem_flags flags,
                cl_GLenum target,
                cl_GLint miplevel,
                cl_GLuint texture,
                cl_int* errcode_ret);

    cl_mem  (CLI_API_CALL *clCreateFromGLRenderbuffer) (
                cl_context context,
                cl_mem_flags flags,
                cl_GLuint renderbuffer,
                cl_int* errcode_ret);

    cl_int  (CLI_API_CALL *clGetGLObjectInfo) (
                cl_mem memobj,
                cl_gl_object_type* gl_object_type,
                cl_GLuint* gl_object_name);

    cl_int  (CLI_API_CALL *clGetGLTextureInfo) (
                cl_mem memobj,
                cl_gl_texture_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret);

    cl_int  (CLI_API_CALL *clEnqueueAcquireGLObjects) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    cl_int  (CLI_API_CALL *clEnqueueReleaseGLObjects) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    // OpenCL 2.0

    void* (CLI_API_CALL *clSVMAlloc) (
                cl_context context,
                cl_svm_mem_flags flags,
                size_t size,
                cl_uint alignment);

    void (CLI_API_CALL *clSVMFree) (
                cl_context context,
                void* svm_pointer);

    cl_int (CLI_API_CALL *clEnqueueSVMFree) (
                cl_command_queue command_queue,
                cl_uint num_svm_pointers,
                void* svm_pointers [],
                void (CL_CALLBACK* pfn_free_func)(
                      cl_command_queue queue,
                      cl_uint num_svm_pointers,
                      void* svm_pointers [],
                      void* user_data ),
                void* user_data,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    cl_int (CLI_API_CALL *clEnqueueSVMMemcpy) (
                cl_command_queue command_queue,
                cl_bool blocking_copy,
                void* dst_ptr,
                const void* src_ptr,
                size_t size,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    cl_int (CLI_API_CALL *clEnqueueSVMMemFill) (
                cl_command_queue command_queue,
                void* svm_ptr,
                const void* pattern,
                size_t pattern_size,
                size_t size,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    cl_int (CLI_API_CALL *clEnqueueSVMMap) (
                cl_command_queue command_queue,
                cl_bool blocking_map,
                cl_map_flags map_flags,
                void* svm_ptr,
                size_t size,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    cl_int (CLI_API_CALL *clEnqueueSVMUnmap) (
                cl_command_queue command_queue,
                void* svm_ptr,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    cl_int (CLI_API_CALL *clSetKernelArgSVMPointer) (
                cl_kernel kernel,
                cl_uint arg_index,
                const void* arg_value);

    cl_int (CLI_API_CALL *clSetKernelExecInfo) (
                cl_kernel kernel,
                cl_kernel_exec_info param_name,
                size_t param_value_size,
                const void* param_value);

    cl_mem (CLI_API_CALL *clCreatePipe) (
                cl_context context,
                cl_mem_flags flags,
                cl_uint pipe_packet_size,
                cl_uint pipe_max_packets,
                const cl_pipe_properties* properties,
                cl_int* errcode_ret);

    cl_int (CLI_API_CALL *clGetPipeInfo) (
                cl_mem pipe,
                cl_pipe_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret);

    cl_command_queue (CLI_API_CALL *clCreateCommandQueueWithProperties) (
                cl_context context,
                cl_device_id device,
                const cl_queue_properties* properties,
                cl_int* errcode_ret);

    cl_sampler (CLI_API_CALL *clCreateSamplerWithProperties) (
                cl_context context,
                const cl_sampler_properties* sampler_properties,
                cl_int* errcode_ret);

    // OpenCL 2.1

    cl_int (CLI_API_CALL *clSetDefaultDeviceCommandQueue) (
                cl_context context,
                cl_device_id device,
                cl_command_queue command_queue );

    cl_int (CLI_API_CALL *clGetDeviceAndHostTimer) (
                cl_device_id device,
                cl_ulong* device_timestamp,
                cl_ulong* host_timestamp );

    cl_int (CLI_API_CALL *clGetHostTimer) (
                cl_device_id device,
                cl_ulong* host_timestamp );

    cl_program (CLI_API_CALL *clCreateProgramWithIL) (
                cl_context context,
                const void *il,
                size_t length,
                cl_int *errcode_ret);

    cl_kernel (CLI_API_CALL *clCloneKernel) (
                cl_kernel source_kernel,
                cl_int* errcode_ret );

    cl_int (CLI_API_CALL *clGetKernelSubGroupInfo) (
                cl_kernel kernel,
                cl_device_id device,
                cl_kernel_sub_group_info param_name,
                size_t input_value_size,
                const void* input_value,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret );

    cl_int (CLI_API_CALL *clEnqueueSVMMigrateMem) (
                cl_command_queue command_queue,
                cl_uint num_svm_pointers,
                const void** svm_pointers,
                const size_t* sizes,
                cl_mem_migration_flags flags,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    // These are Khronos Extensions.
    // They aren't exported from the ICD or from this DLL, but we'll still
    // put a pointer to them in the CLIntercept dispatch table.

    // cl_khr_gl_sharing
    cl_int  (CLI_API_CALL *clGetGLContextInfoKHR) (
                const cl_context_properties *properties,
                cl_gl_context_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret);

    // cl_khr_gl_event
    cl_event    (CLI_API_CALL *clCreateEventFromGLsyncKHR) (
                cl_context context,
                cl_GLsync sync,
                cl_int* errcode_ret);

#if defined(_WIN32)
    // cl_khr_d3d10_sharing
    cl_int  (CLI_API_CALL *clGetDeviceIDsFromD3D10KHR) (
                cl_platform_id platform,
                cl_d3d10_device_source_khr d3d_device_source,
                void* d3d_object,
                cl_d3d10_device_set_khr d3d_device_set,
                cl_uint num_entries,
                cl_device_id* devices,
                cl_uint* num_devices);

    // cl_khr_d3d10_sharing
    cl_mem  (CLI_API_CALL *clCreateFromD3D10BufferKHR) (
                cl_context context,
                cl_mem_flags flags,
                ID3D10Buffer* resource,
                cl_int* errcode_ret);

    // cl_khr_d3d10_sharing
    cl_mem  (CLI_API_CALL *clCreateFromD3D10Texture2DKHR) (
                cl_context context,
                cl_mem_flags flags,
                ID3D10Texture2D* resource,
                UINT subresource,
                cl_int* errcode_ret);

    // cl_khr_d3d10_sharing
    cl_mem  (CLI_API_CALL *clCreateFromD3D10Texture3DKHR) (
                cl_context context,
                cl_mem_flags flags,
                ID3D10Texture3D* resource,
                UINT subresource,
                cl_int* errcode_ret);

    // cl_khr_d3d10_sharing
    cl_int  (CLI_API_CALL *clEnqueueAcquireD3D10ObjectsKHR) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    // cl_khr_d3d10_sharing
    cl_int  (CLI_API_CALL *clEnqueueReleaseD3D10ObjectsKHR) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    // cl_khr_d3d11_sharing
    cl_int  (CLI_API_CALL *clGetDeviceIDsFromD3D11KHR) (
                cl_platform_id platform,
                cl_d3d11_device_source_khr d3d_device_source,
                void* d3d_object,
                cl_d3d11_device_set_khr d3d_device_set,
                cl_uint num_entries,
                cl_device_id* devices,
                cl_uint* num_devices);

    // cl_khr_d3d11_sharing
    cl_mem  (CLI_API_CALL *clCreateFromD3D11BufferKHR) (
                cl_context context,
                cl_mem_flags flags,
                ID3D11Buffer* resource,
                cl_int* errcode_ret);

    // cl_khr_d3d11_sharing
    cl_mem  (CLI_API_CALL *clCreateFromD3D11Texture2DKHR) (
                cl_context context,
                cl_mem_flags flags,
                ID3D11Texture2D* resource,
                UINT subresource,
                cl_int* errcode_ret);

    // cl_khr_d3d11_sharing
    cl_mem  (CLI_API_CALL *clCreateFromD3D11Texture3DKHR) (
                cl_context context,
                cl_mem_flags flags,
                ID3D11Texture3D* resource,
                UINT subresource,
                cl_int* errcode_ret);

    // cl_khr_d3d11_sharing
    cl_int  (CLI_API_CALL *clEnqueueAcquireD3D11ObjectsKHR) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    // cl_khr_d3d11_sharing
    cl_int  (CLI_API_CALL *clEnqueueReleaseD3D11ObjectsKHR) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    // cl_khr_dx9_media_sharing
    cl_int  (CLI_API_CALL *clGetDeviceIDsFromDX9MediaAdapterKHR) (
                cl_platform_id platform,
                cl_uint num_media_adapters,
                cl_dx9_media_adapter_type_khr* media_adapters_type,
                void* media_adapters,
                cl_dx9_media_adapter_set_khr media_adapter_set,
                cl_uint num_entries,
                cl_device_id* devices,
                cl_uint* num_devices);

    // cl_khr_dx9_media_sharing
    cl_mem  (CLI_API_CALL *clCreateFromDX9MediaSurfaceKHR) (
                cl_context context,
                cl_mem_flags flags,
                cl_dx9_media_adapter_type_khr adapter_type,
                void* surface_info,
                cl_uint plane,
                cl_int* errcode_ret);

    // cl_khr_dx9_media_sharing
    cl_int  (CLI_API_CALL *clEnqueueAcquireDX9MediaSurfacesKHR) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);

    // cl_khr_dx9_media_sharing
    cl_int  (CLI_API_CALL *clEnqueueReleaseDX9MediaSurfacesKHR) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event);
#endif

    // cl_khr_il_program
    cl_program (CLI_API_CALL *clCreateProgramWithILKHR) (
                cl_context context,
                const void *il,
                size_t length,
                cl_int *errcode_ret);

    // cl_khr_subgroups
    cl_int  (CLI_API_CALL *clGetKernelSubGroupInfoKHR) (
                cl_kernel kernel,
                cl_device_id device,
                cl_kernel_sub_group_info param_name,
                size_t input_value_size,
                const void* input_value,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret);

    // cl_khr_create_command_queue
    cl_command_queue    (CLI_API_CALL *clCreateCommandQueueWithPropertiesKHR) (
            cl_context context,
            cl_device_id device,
            const cl_queue_properties_khr* properties,
            cl_int* errcode_ret);

    // These are Intel Vendor Extensions.
    // They aren't exported from the ICD or from this DLL, but we'll still
    // put a pointer to them in the CLIntercept dispatch table.

#if defined(_WIN32)
    // cl_intel_dx9_media_sharing
    cl_int  (CLI_API_CALL *clGetDeviceIDsFromDX9INTEL) (
                cl_platform_id platform,
                cl_dx9_device_source_intel d3d_device_source,
                void *dx9_object,
                cl_dx9_device_set_intel d3d_device_set,
                cl_uint num_entries,
                cl_device_id* devices,
                cl_uint* num_devices );

    // cl_intel_dx9_media_sharing
    cl_mem  (CLI_API_CALL *clCreateFromDX9MediaSurfaceINTEL) (
                cl_context context,
                cl_mem_flags flags,
                IDirect3DSurface9* resource,
                HANDLE sharedHandle,
                UINT plane,
                cl_int* errcode_ret );

    // cl_intel_dx9_media_sharing
    cl_int  (CLI_API_CALL *clEnqueueAcquireDX9ObjectsINTEL) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    // cl_intel_dx9_media_sharing
    cl_int  (CLI_API_CALL *clEnqueueReleaseDX9ObjectsINTEL) (
                cl_command_queue command_queue,
                cl_uint num_objects,
                const cl_mem* mem_objects,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );
#endif

    // Unofficial MDAPI extension:
    cl_command_queue    (CLI_API_CALL *clCreatePerfCountersCommandQueueINTEL) (
                cl_context context,
                cl_device_id device,
                cl_command_queue_properties properties,
                cl_uint configuration,
                cl_int* errcode_ret);

    // Unofficial MDAPI extension:
    cl_int (CLI_API_CALL *clSetPerformanceConfigurationINTEL)(
        cl_device_id    device,
        cl_uint         count,
        cl_uint*        offsets,
        cl_uint*        values );

    // cl_intel_accelerator
    cl_accelerator_intel (CLI_API_CALL *clCreateAcceleratorINTEL) (
        cl_context context,
        cl_accelerator_type_intel accelerator_type,
        size_t descriptor_size,
        const void* descriptor,
        cl_int* errcode_ret );

    // cl_intel_accelerator
    cl_int (CLI_API_CALL *clGetAcceleratorInfoINTEL) (
        cl_accelerator_intel accelerator,
        cl_accelerator_info_intel param_name,
        size_t param_value_size,
        void* param_value,
        size_t* param_value_size_ret );

    // cl_intel_accelerator
    cl_int (CLI_API_CALL *clRetainAcceleratorINTEL) (
        cl_accelerator_intel accelerator );

    // cl_intel_accelerator
    cl_int (CLI_API_CALL *clReleaseAcceleratorINTEL) (
        cl_accelerator_intel accelerator );

    // cl_intel_va_api_media_sharing
    cl_int (CLI_API_CALL *clGetDeviceIDsFromVA_APIMediaAdapterINTEL) (
        cl_platform_id platform,
        cl_va_api_device_source_intel media_adapter_type,
        void *media_adapter,
        cl_va_api_device_set_intel media_adapter_set,
        cl_uint num_entries,
        cl_device_id *devices,
        cl_uint *num_devices);

    // cl_intel_va_api_media_sharing
    cl_mem (CLI_API_CALL *clCreateFromVA_APIMediaSurfaceINTEL) (
        cl_context context,
        cl_mem_flags flags,
        VASurfaceID *surface,
        cl_uint plane,
        cl_int *errcode_ret);

    // cl_intel_va_api_media_sharing
    cl_int (CLI_API_CALL *clEnqueueAcquireVA_APIMediaSurfacesINTEL) (
        cl_command_queue command_queue,
        cl_uint num_objects,
        const cl_mem *mem_objects,
        cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list,
        cl_event *event);

    // cl_intel_va_api_media_sharing
    cl_int (CLI_API_CALL *clEnqueueReleaseVA_APIMediaSurfacesINTEL) (
        cl_command_queue command_queue,
        cl_uint num_objects,
        const cl_mem *mem_objects,
        cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list,
        cl_event *event);

    // cl_intel_unified_shared_memory
    void* (CLI_API_CALL *clHostMemAllocINTEL) (
        cl_context context,
        const cl_mem_properties_intel* properties,
        size_t size,
        cl_uint alignment,
        cl_int* errcode_ret);

    // cl_intel_unified_shared_memory
    void* (CLI_API_CALL *clDeviceMemAllocINTEL) (
        cl_context context,
        cl_device_id device,
        const cl_mem_properties_intel* properties,
        size_t size,
        cl_uint alignment,
        cl_int* errcode_ret);

    // cl_intel_unified_shared_memory
    void* (CLI_API_CALL *clSharedMemAllocINTEL) (
        cl_context context,
        cl_device_id device,
        const cl_mem_properties_intel* properties,
        size_t size,
        cl_uint alignment,
        cl_int* errcode_ret);

    // cl_intel_unified_shared_memory
    cl_int (CLI_API_CALL *clMemFreeINTEL) (
        cl_context context,
        void* ptr);

    // cl_intel_unified_shared_memory
    cl_int (CLI_API_CALL *clMemBlockingFreeINTEL) (
        cl_context context,
        void* ptr);

    // cl_intel_unified_shared_memory
    cl_int (CLI_API_CALL *clGetMemAllocInfoINTEL) (
        cl_context context,
        const void* ptr,
        cl_mem_info_intel param_name,
        size_t param_value_size,
        void* param_value,
        size_t* param_value_size_ret);

    // cl_intel_unified_shared_memory
    cl_int (CLI_API_CALL *clSetKernelArgMemPointerINTEL) (
        cl_kernel kernel,
        cl_uint arg_index,
        const void* arg_value);

    // cl_intel_unified_shared_memory
    cl_int (CLI_API_CALL *clEnqueueMemsetINTEL) (   // Deprecated
        cl_command_queue command_queue,
        void* dst_ptr,
        cl_int value,
        size_t size,
        cl_uint num_events_in_wait_list,
        const cl_event* event_wait_list,
        cl_event* event);

    // cl_intel_unified_shared_memory
    cl_int (CLI_API_CALL *clEnqueueMemFillINTEL) (
        cl_command_queue command_queue,
        void* dst_ptr,
        const void* pattern,
        size_t pattern_size,
        size_t size,
        cl_uint num_events_in_wait_list,
        const cl_event* event_wait_list,
        cl_event* event);

    // cl_intel_unified_shared_memory
    cl_int (CLI_API_CALL *clEnqueueMemcpyINTEL) (
        cl_command_queue command_queue,
        cl_bool blocking,
        void* dst_ptr,
        const void* src_ptr,
        size_t size,
        cl_uint num_events_in_wait_list,
        const cl_event* event_wait_list,
        cl_event* event);

    // cl_intel_unified_shared_memory
    cl_int (CLI_API_CALL *clEnqueueMigrateMemINTEL) (
        cl_command_queue command_queue,
        const void* ptr,
        size_t size,
        cl_mem_migration_flags flags,
        cl_uint num_events_in_wait_list,
        const cl_event* event_wait_list,
        cl_event* event);

    // cl_intel_unified_shared_memory
    cl_int (CLI_API_CALL *clEnqueueMemAdviseINTEL) (
        cl_command_queue command_queue,
        const void* ptr,
        size_t size,
        cl_mem_advice_intel advice,
        cl_uint num_events_in_wait_list,
        const cl_event* event_wait_list,
        cl_event* event);
};

extern CLdispatch dummyDispatch;
