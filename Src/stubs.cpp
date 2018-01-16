/*
// Copyright (c) 2018 Intel Corporation
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

#include "dispatch.h"

#define DUMMY_ASSERT()  CLI_DEBUG_BREAK()

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetPlatformIDs(
    cl_uint num_entries, 
    cl_platform_id* platforms, 
    cl_uint* num_platforms )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetPlatformInfo(
    cl_platform_id platform, 
    cl_platform_info param_name, 
    size_t param_value_size, 
    void* param_value, 
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}
    
///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetDeviceIDs(
    cl_platform_id platform,
    cl_device_type device_type, 
    cl_uint num_entries, 
    cl_device_id* devices, 
    cl_uint* num_devices )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetDeviceInfo(
    cl_device_id device,
    cl_device_info param_name, 
    size_t param_value_size, 
    void* param_value,
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CLI_API_ENTRY cl_int CLI_API_CALL dummyCreateSubDevices(
    cl_device_id in_device,
    const cl_device_partition_property* properties,
    cl_uint num_devices,
    cl_device_id* out_devices,
    cl_uint* num_devices_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CLI_API_ENTRY cl_int CLI_API_CALL dummyRetainDevice(
    cl_device_id device )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CLI_API_ENTRY cl_int CLI_API_CALL dummyReleaseDevice(
    cl_device_id device )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_context CLI_API_CALL dummyCreateContext(
    const cl_context_properties* properties,
    cl_uint num_devices,
    const cl_device_id* devices,
    void (CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
    void* user_data,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_context CLI_API_CALL dummyCreateContextFromType(
    const cl_context_properties* properties,
    cl_device_type device_type,
    void (CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
    void* user_data,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyRetainContext(
    cl_context context )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyReleaseContext(
    cl_context context )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetContextInfo(
    cl_context context, 
    cl_context_info param_name, 
    size_t param_value_size, 
    void* param_value, 
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_command_queue CLI_API_CALL dummyCreateCommandQueue(
    cl_context context,
    cl_device_id device,
    cl_command_queue_properties properties,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyRetainCommandQueue(
    cl_command_queue command_queue )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyReleaseCommandQueue(
    cl_command_queue command_queue )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetCommandQueueInfo(
    cl_command_queue command_queue,
    cl_command_queue_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummySetCommandQueueProperty(
    cl_command_queue command_queue,
    cl_command_queue_properties properties,
    cl_bool enable,
    cl_command_queue_properties* old_properties )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_mem CLI_API_CALL dummyCreateBuffer(
    cl_context context,
    cl_mem_flags flags,
    size_t size,
    void* host_ptr,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CLI_API_ENTRY cl_mem CLI_API_CALL dummyCreateSubBuffer(
    cl_mem buffer,
    cl_mem_flags flags,
    cl_buffer_create_type buffer_create_type,
    const void *buffer_create_info,
    cl_int *errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CLI_API_ENTRY cl_mem CLI_API_CALL dummyCreateImage(
    cl_context context,
    cl_mem_flags flags,
    const cl_image_format* image_format,
    const cl_image_desc* image_desc,
    void* host_ptr,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_mem CLI_API_CALL dummyCreateImage2D(
    cl_context context,
    cl_mem_flags flags,
    const cl_image_format* image_format,
    size_t image_width,
    size_t image_height,
    size_t image_row_pitch, 
    void* host_ptr,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_mem CLI_API_CALL dummyCreateImage3D(
    cl_context context,
    cl_mem_flags flags,
    const cl_image_format* image_format,
    size_t image_width, 
    size_t image_height,
    size_t image_depth, 
    size_t image_row_pitch, 
    size_t image_slice_pitch, 
    void* host_ptr,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}
                        
///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyRetainMemObject(
    cl_mem memobj )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyReleaseMemObject(
    cl_mem memobj )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetSupportedImageFormats(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_object_type image_type,
    cl_uint num_entries,
    cl_image_format* image_formats,
    cl_uint* num_image_formats )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetMemObjectInfo(
    cl_mem memobj,
    cl_mem_info param_name, 
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetImageInfo(
    cl_mem image,
    cl_image_info param_name, 
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CLI_API_ENTRY cl_int CLI_API_CALL dummySetMemObjectDestructorCallback(
    cl_mem memobj, 
    void (CL_CALLBACK *pfn_notify)( cl_mem, void* ), 
    void *user_data )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_sampler CLI_API_CALL dummyCreateSampler(
    cl_context context,
    cl_bool normalized_coords, 
    cl_addressing_mode addressing_mode, 
    cl_filter_mode filter_mode,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyRetainSampler(
    cl_sampler sampler )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyReleaseSampler(
    cl_sampler sampler )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetSamplerInfo(
    cl_sampler sampler,
    cl_sampler_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_program CLI_API_CALL dummyCreateProgramWithSource(
    cl_context context,
    cl_uint count,
    const char** strings,
    const size_t* lengths,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_program CLI_API_CALL dummyCreateProgramWithBinary(
    cl_context context,
    cl_uint num_devices,
    const cl_device_id* device_list,
    const size_t* lengths,
    const unsigned char** binaries,
    cl_int* binary_status,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CLI_API_ENTRY cl_program CLI_API_CALL dummyCreateProgramWithBuiltInKernels(
    cl_context context,
    cl_uint num_devices,
    const cl_device_id* device_list,
    const char* kernel_names,
    cl_int* errcode_ret)
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyRetainProgram(
    cl_program program )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyReleaseProgram(
    cl_program program )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyBuildProgram(
    cl_program program,
    cl_uint num_devices,
    const cl_device_id* device_list,
    const char* options, 
    void (CL_CALLBACK *pfn_notify)(cl_program program, void* user_data),
    void* user_data )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CLI_API_ENTRY cl_int CLI_API_CALL dummyCompileProgram(
    cl_program program,
    cl_uint num_devices,
    const cl_device_id* device_list,
    const char* options,
    cl_uint num_input_headers,
    const cl_program* input_headers,
    const char** header_include_names,
    void (CL_CALLBACK *pfn_notify)(cl_program program , void* user_data),
    void* user_data )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CLI_API_ENTRY cl_program CLI_API_CALL dummyLinkProgram(
    cl_context context,
    cl_uint num_devices,
    const cl_device_id* device_list,
    const char* options,
    cl_uint num_input_programs,
    const cl_program* input_programs,
    void (CL_CALLBACK *pfn_notify)(cl_program program, void* user_data),
    void* user_data,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.2
CLI_API_ENTRY cl_int CLI_API_CALL dummySetProgramReleaseCallback(
    cl_program program,
    void (CL_CALLBACK *pfn_notify)(cl_program program, void* user_data),
    void* user_data )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.2
CLI_API_ENTRY cl_int CLI_API_CALL dummySetProgramSpecializationConstant(
    cl_program program,
    cl_uint spec_id,
    size_t spec_size,
    const void* spec_value )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CLI_API_ENTRY cl_int CLI_API_CALL dummyUnloadPlatformCompiler(
    cl_platform_id platform )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyUnloadCompiler( void )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetProgramInfo(
    cl_program program,
    cl_program_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetProgramBuildInfo(
    cl_program program,
    cl_device_id device,
    cl_program_build_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

                            
///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_kernel CLI_API_CALL dummyCreateKernel(
    cl_program program,
    const char* kernel_name,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyCreateKernelsInProgram(
    cl_program program,
    cl_uint num_kernels,
    cl_kernel* kernels,
    cl_uint* num_kernels_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyRetainKernel(
    cl_kernel kernel )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyReleaseKernel(
    cl_kernel kernel )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummySetKernelArg(
    cl_kernel kernel,
    cl_uint arg_index,
    size_t arg_size,
    const void* arg_value )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetKernelInfo(
    cl_kernel kernel,
    cl_kernel_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetKernelArgInfo(
    cl_kernel kernel,
    cl_uint arg_indx,
    cl_kernel_arg_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetKernelWorkGroupInfo(
    cl_kernel kernel,
    cl_device_id device,
    cl_kernel_work_group_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyWaitForEvents(
    cl_uint num_events,
    const cl_event* event_list )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetEventInfo(
    cl_event event,
    cl_event_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CLI_API_ENTRY cl_event CLI_API_CALL dummyCreateUserEvent(
    cl_context context,
    cl_int *errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyRetainEvent(
    cl_event event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyReleaseEvent(
    cl_event event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CLI_API_ENTRY cl_int CLI_API_CALL dummySetUserEventStatus(
    cl_event event,
    cl_int execution_status )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CLI_API_ENTRY cl_int CLI_API_CALL dummySetEventCallback(
    cl_event event,
    cl_int command_exec_callback_type,
    void (CL_CALLBACK *pfn_notify)( cl_event, cl_int, void * ),
    void *user_data )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetEventProfilingInfo(
    cl_event event,
    cl_profiling_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}
                                
///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyFlush(
    cl_command_queue command_queue )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyFinish(
    cl_command_queue command_queue )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueReadBuffer(
    cl_command_queue command_queue,
    cl_mem buffer,
    cl_bool blocking_read,
    size_t offset,
    size_t cb, 
    void* ptr,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueReadBufferRect(
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
    cl_event *event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueWriteBuffer(
    cl_command_queue command_queue, 
    cl_mem buffer, 
    cl_bool blocking_write, 
    size_t offset, 
    size_t cb, 
    const void* ptr, 
    cl_uint num_events_in_wait_list, 
    const cl_event* event_wait_list, 
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueWriteBufferRect(
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
    cl_event *event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueFillBuffer(
    cl_command_queue command_queue,
    cl_mem buffer,
    const void* pattern,
    size_t pattern_size,
    size_t offset,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueCopyBuffer(
    cl_command_queue command_queue, 
    cl_mem src_buffer,
    cl_mem dst_buffer, 
    size_t src_offset,
    size_t dst_offset,
    size_t cb, 
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueCopyBufferRect(
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
    cl_event *event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueReadImage(
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
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueWriteImage(
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
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueFillImage(
    cl_command_queue command_queue,
    cl_mem image,
    const void* fill_color,
    const size_t* origin,
    const size_t* region,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueCopyImage(
    cl_command_queue command_queue,
    cl_mem src_image,
    cl_mem  dst_image, 
    const size_t* src_origin,
    const size_t* dst_origin,
    const size_t* region, 
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueCopyImageToBuffer(
    cl_command_queue command_queue,
    cl_mem src_image,
    cl_mem dst_buffer, 
    const size_t* src_origin,
    const size_t* region, 
    size_t dst_offset,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueCopyBufferToImage(
    cl_command_queue command_queue,
    cl_mem src_buffer,
    cl_mem dst_image, 
    size_t src_offset,
    const size_t* dst_origin,
    const size_t* region, 
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY void* CLI_API_CALL dummyEnqueueMapBuffer(
    cl_command_queue command_queue,
    cl_mem buffer,
    cl_bool blocking_map, 
    cl_map_flags map_flags,
    size_t offset,
    size_t cb,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY void* CLI_API_CALL dummyEnqueueMapImage(
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
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueUnmapMemObject(
    cl_command_queue command_queue,
    cl_mem memobj,
    void* mapped_ptr,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueMigrateMemObjects(
    cl_command_queue command_queue,
    cl_uint num_mem_objects,
    const cl_mem* mem_objects,
    cl_mem_migration_flags flags,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueNDRangeKernel(
    cl_command_queue command_queue,
    cl_kernel kernel,
    cl_uint work_dim,
    const size_t* global_work_offset,
    const size_t* global_work_size,
    const size_t* local_work_size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueTask(
    cl_command_queue command_queue,
    cl_kernel kernel,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueNativeKernel(
    cl_command_queue command_queue,
    void (CL_CALLBACK *user_func)(void *), 
    void* args,
    size_t cb_args, 
    cl_uint num_mem_objects,
    const cl_mem* mem_list,
    const void** args_mem_loc,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueMarker(
    cl_command_queue command_queue,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueWaitForEvents(
    cl_command_queue command_queue,
    cl_uint num_events,
    const cl_event* event_list )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueBarrier(
    cl_command_queue command_queue )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueMarkerWithWaitList(
    cl_command_queue command_queue,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueBarrierWithWaitList(
    cl_command_queue command_queue,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// Optional?
CLI_API_ENTRY void* CLI_API_CALL dummyGetExtensionFunctionAddress(
    const char* func_name )
{
    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// Optional?
// OpenCL 1.2
CLI_API_ENTRY void* CLI_API_CALL dummyGetExtensionFunctionAddressForPlatform(
    cl_platform_id platform,
    const char* func_name )
{
    DUMMY_ASSERT();
    return NULL;
}

// CL-GL Sharing

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_mem CLI_API_CALL dummyCreateFromGLBuffer(
    cl_context context,
    cl_mem_flags flags,
    cl_GLuint bufobj,
    int* errcode_ret)   // Not cl_int*?
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// Optional?
// OpenCL 1.2
CLI_API_ENTRY cl_mem CLI_API_CALL dummyCreateFromGLTexture(
    cl_context context,
    cl_mem_flags flags,
    cl_GLenum target,
    cl_GLint miplevel,
    cl_GLuint texture,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_mem CLI_API_CALL dummyCreateFromGLTexture2D(
    cl_context context,
    cl_mem_flags flags,
    cl_GLenum target,
    cl_GLint miplevel,
    cl_GLuint texture,
    cl_int* errcode_ret)
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_mem CLI_API_CALL dummyCreateFromGLTexture3D(
    cl_context context,
    cl_mem_flags flags,
    cl_GLenum target,
    cl_GLint miplevel,
    cl_GLuint texture,
    cl_int* errcode_ret)
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_mem CLI_API_CALL dummyCreateFromGLRenderbuffer(
    cl_context context,
    cl_mem_flags flags,
    cl_GLuint renderbuffer,
    cl_int* errcode_ret)
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetGLObjectInfo(
    cl_mem memobj,
    cl_gl_object_type* gl_object_type,
    cl_GLuint* gl_object_name)
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetGLTextureInfo(
    cl_mem memobj,
    cl_gl_texture_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueAcquireGLObjects(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueReleaseGLObjects(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CLI_API_ENTRY void* CLI_API_CALL dummySVMAlloc(
    cl_context context,
    cl_svm_mem_flags flags,
    size_t size,
    cl_uint alignment)
{
    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CLI_API_ENTRY void CLI_API_CALL dummySVMFree(
    cl_context context,
    void* svm_pointer)
{
    DUMMY_ASSERT();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueSVMFree(
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
    cl_event* event)
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueSVMMemcpy(
    cl_command_queue command_queue,
    cl_bool blocking_copy,
    void* dst_ptr,
    const void* src_ptr,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueSVMMemFill(
    cl_command_queue command_queue,
    void* svm_ptr,
    const void* pattern,
    size_t pattern_size,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueSVMMap(
    cl_command_queue command_queue,
    cl_bool blocking_map,
    cl_map_flags map_flags,
    void* svm_ptr,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueSVMUnmap(
    cl_command_queue command_queue,
    void* svm_ptr,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CLI_API_ENTRY cl_int CLI_API_CALL dummySetKernelArgSVMPointer(
    cl_kernel kernel,
    cl_uint arg_index,
    const void* arg_value)
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CLI_API_ENTRY cl_int CLI_API_CALL dummySetKernelExecInfo(
    cl_kernel kernel,
    cl_kernel_exec_info param_name,
    size_t param_value_size,
    const void* param_value)
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CLI_API_ENTRY cl_mem CLI_API_CALL dummyCreatePipe(
    cl_context context,
    cl_mem_flags flags,
    cl_uint pipe_packet_size,
    cl_uint pipe_max_packets,
    const cl_pipe_properties* properties,
    cl_int* errcode_ret)
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetPipeInfo(
    cl_mem pipe,
    cl_pipe_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CLI_API_ENTRY cl_command_queue CLI_API_CALL dummyCreateCommandQueueWithProperties(
    cl_context context,
    cl_device_id device,
    const cl_queue_properties* properties,
    cl_int* errcode_ret)
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CLI_API_ENTRY cl_sampler CLI_API_CALL dummyCreateSamplerWithProperties(
    cl_context context,
    const cl_sampler_properties* sampler_properties,
    cl_int* errcode_ret)
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.1
CLI_API_ENTRY cl_int CLI_API_CALL dummySetDefaultCommandQueue(
    cl_context context,
    cl_device_id device,
    cl_command_queue command_queue )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.1
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetDeviceAndHostTimer(
    cl_device_id device,
    cl_ulong* device_timestamp,
    cl_ulong* host_timestamp )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.1
CLI_API_ENTRY cl_program CLI_API_CALL dummyCreateProgramWithIL(
    cl_context context,
    const void* il,
    size_t length,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.1
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetHostTimer(
    cl_device_id device,
    cl_ulong* host_timestamp )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.1
CLI_API_ENTRY cl_kernel CLI_API_CALL dummyCloneKernel(
    cl_kernel source_kernel,
    cl_int* errcode_ret )
{
    if( errcode_ret )
    {
        errcode_ret[0] = CL_INVALID_OPERATION;
    }

    DUMMY_ASSERT();
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.1
CLI_API_ENTRY cl_int CLI_API_CALL dummyGetKernelSubGroupInfo(
    cl_kernel kernel,
    cl_device_id device,
    cl_kernel_sub_group_info param_name,
    size_t input_value_size,
    const void* input_value,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.1
CLI_API_ENTRY cl_int CLI_API_CALL dummyEnqueueSVMMigrateMem(
    cl_command_queue /* command_queue */,
    cl_uint /* num_svm_pointers */,
    const void** /* svm_pointers */,
    const size_t* /* sizes */,
    cl_mem_migration_flags /* flags */,
    cl_uint /* num_events_in_wait_list */,
    const cl_event* /* event_wait_list */,
    cl_event* /* event */ )
{
    DUMMY_ASSERT();
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
CLdispatch dummyDispatch = {
    dummyGetPlatformIDs,
    dummyGetPlatformInfo,
    dummyGetDeviceIDs,
    dummyGetDeviceInfo,
    dummyCreateSubDevices,
    dummyRetainDevice,
    dummyReleaseDevice,
    dummyCreateContext,
    dummyCreateContextFromType,
    dummyRetainContext,
    dummyReleaseContext,
    dummyGetContextInfo,
    dummyCreateCommandQueue,
    dummyRetainCommandQueue,
    dummyReleaseCommandQueue,
    dummyGetCommandQueueInfo,
    dummySetCommandQueueProperty,
    dummyCreateBuffer,
    dummyCreateSubBuffer,
    dummyCreateImage,
    dummyCreateImage2D,
    dummyCreateImage3D,
    dummyRetainMemObject,
    dummyReleaseMemObject,
    dummyGetSupportedImageFormats,
    dummyGetMemObjectInfo,
    dummyGetImageInfo,
    dummySetMemObjectDestructorCallback,
    dummyCreateSampler,
    dummyRetainSampler,
    dummyReleaseSampler,
    dummyGetSamplerInfo,
    dummyCreateProgramWithSource,
    dummyCreateProgramWithBinary,
    dummyCreateProgramWithBuiltInKernels,
    dummyRetainProgram,
    dummyReleaseProgram,
    dummyBuildProgram,
    dummyCompileProgram,
    dummyLinkProgram,
    dummySetProgramReleaseCallback,
    dummySetProgramSpecializationConstant,
    dummyUnloadPlatformCompiler,
    dummyUnloadCompiler,
    dummyGetProgramInfo,
    dummyGetProgramBuildInfo,
    dummyCreateKernel,
    dummyCreateKernelsInProgram,
    dummyRetainKernel,
    dummyReleaseKernel,
    dummySetKernelArg,
    dummyGetKernelInfo,
    dummyGetKernelArgInfo,
    dummyGetKernelWorkGroupInfo,
    dummyWaitForEvents,
    dummyGetEventInfo,
    dummyCreateUserEvent,
    dummyRetainEvent,
    dummyReleaseEvent,
    dummySetUserEventStatus,
    dummySetEventCallback,
    dummyGetEventProfilingInfo,
    dummyFlush,
    dummyFinish,
    dummyEnqueueReadBuffer,
    dummyEnqueueReadBufferRect,
    dummyEnqueueWriteBuffer,
    dummyEnqueueWriteBufferRect,
    dummyEnqueueFillBuffer,
    dummyEnqueueCopyBuffer,
    dummyEnqueueCopyBufferRect,
    dummyEnqueueReadImage,
    dummyEnqueueWriteImage,
    dummyEnqueueFillImage,
    dummyEnqueueCopyImage,
    dummyEnqueueCopyImageToBuffer,
    dummyEnqueueCopyBufferToImage,
    dummyEnqueueMapBuffer,
    dummyEnqueueMapImage,
    dummyEnqueueUnmapMemObject,
    dummyEnqueueMigrateMemObjects,
    dummyEnqueueNDRangeKernel,
    dummyEnqueueTask,
    dummyEnqueueNativeKernel,
    dummyEnqueueMarker,
    dummyEnqueueWaitForEvents,
    dummyEnqueueBarrier,
    dummyEnqueueMarkerWithWaitList,
    dummyEnqueueBarrierWithWaitList,
    dummyGetExtensionFunctionAddress,
    dummyGetExtensionFunctionAddressForPlatform,

    // Expoerted CL-GL Sharing Functions
    // Even though these are exported from the ICD, we'll still
    // initialize them to NULL so they can be obtained with
    // clGetExtensionFunctionAddress(), just in case an intercepted
    // DLL decides not to export them.
    NULL, //dummyCreateFromGLBuffer,
    NULL, //dummyCreateFromGLTexture,
    NULL, //dummyCreateFromGLTexture2D,
    NULL, //dummyCreateFromGLTexture3D,
    NULL, //dummyCreateFromGLRenderbuffer,
    NULL, //dummyGetGLObjectInfo,
    NULL, //dummyGetGLTextureInfo,
    NULL, //dummyEnqueueAcquireGLObjects,
    NULL, //dummyEnqueueReleaseGLObjects,

    // OpenCL 2.0
    dummySVMAlloc,
    dummySVMFree,
    dummyEnqueueSVMFree,
    dummyEnqueueSVMMemcpy,
    dummyEnqueueSVMMemFill,
    dummyEnqueueSVMMap,
    dummyEnqueueSVMUnmap,
    dummySetKernelArgSVMPointer,
    dummySetKernelExecInfo,
    dummyCreatePipe,
    dummyGetPipeInfo,
    dummyCreateCommandQueueWithProperties,
    dummyCreateSamplerWithProperties,

    // OpenCL 2.1
    dummySetDefaultCommandQueue,
    dummyGetDeviceAndHostTimer,
    dummyGetHostTimer,
    dummyCreateProgramWithIL,
    dummyCloneKernel,
    dummyGetKernelSubGroupInfo,
    dummyEnqueueSVMMigrateMem,

    // KHR Extensions

    // cl_khr_gl_sharing
    NULL, //dummyGetGLContextInfoKHR
    // cl_khr_gl_event
    NULL, //dummyCreateEventFromGLsyncKHR
#if defined(_WIN32)
    // cl_khr_d3d10_sharing
    NULL, //dummyGetDeviceIDsFromD3D10KHR
    NULL, //dummyCreateFromD3D10BufferKHR
    NULL, //dummyCreateFromD3D10Texture2DKHR
    NULL, //dummyCreateFromD3D10Texture3DKHR
    NULL, //dummyEnqueueAcquireD3D10ObjectsKHR
    NULL, //dummyEnqueueReleaseD3D10ObjectsKHR
    // cl_khr_d3d11_sharing
    NULL, //dummyGetDeviceIDsFromD3D11KHR
    NULL, //dummyCreateFromD3D11BufferKHR
    NULL, //dummyCreateFromD3D11Texture2DKHR
    NULL, //dummyCreateFromD3D11Texture3DKHR
    NULL, //dummyEnqueueAcquireD3D11ObjectsKHR
    NULL, //dummyEnqueueReleaseD3D11ObjectsKHR
    // cl_khr_dx9_media_sharing
    NULL, //dummyGetDeviceIDsFromDX9MediaAdapterKHR
    NULL, //dummyCreateFromDX9MediaSurfaceKHR
    NULL, //dummyEnqueueAcquireDX9MediaSurfacesKHR
    NULL, //dummyEnqueueReleaseDX9MediaSurfacesKHR
#endif

    NULL, //dummyCreateProgramWithILKHR
    NULL, //dummyGetKernelSubGroupInfoKHR

    // Intel Extensions
#if defined(_WIN32)
    // cl_intel_dx9_media_sharing
    NULL, //dummyGetDeviceIDsFromDX9INTEL
    NULL, //dummyCreateFromDX9MediaSurfaceINTEL
    NULL, //dummyEnqueueAcquireDX9ObjectsINTEL
    NULL, //dummyEnqueueReleaseDX9ObjectsINTEL
#endif
    // Unofficial MDAPI Extension:
    NULL, //dummyCreatePerfCountersCommandQueueINTEL
    NULL, //dummySetPerformanceConfigurationINTEL
    // cl_intel_accelerator
    NULL, //dummyCreateAcceleratorINTEL
    NULL, //dummyGetAcceleratorInfoINTEL
    NULL, //dummyRetainAcceleratorINTEL
    NULL, //dummyReleaseAcceleratorINTEL
    // cl_intel_va_api_media_sharing
    NULL, //dummyGetDeviceIDsFromVA_APIMediaAdapterINTEL
    NULL, //dummyCreateFromVA_APIMediaSurfaceINTEL
    NULL, //dummyEnqueueAcquireVA_APIMediaSurfacesINTEL
    NULL, //dummyEnqueueReleaseVA_APIMediaSurfacesINTEL
};
