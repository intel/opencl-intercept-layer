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

#include <string>

#include "intercept.h"

///////////////////////////////////////////////////////////////////////////////
//

namespace cmdbuf {

struct command
{
    virtual ~command() = default;
    virtual int playback(
        CLIntercept*,
        cl_command_queue) = 0;
};

struct NDRangeKernel : command
{
    static NDRangeKernel* create(
        CLIntercept* pIntercept,
        cl_kernel kernel,
        cl_uint work_dim,
        const size_t* global_work_offset,
        const size_t* global_work_size,
        const size_t* local_work_size)
    {
        auto* ret = new NDRangeKernel();

        cl_int errorCode = CL_SUCCESS;
        ret->kernel = pIntercept->dispatch().clCloneKernel(kernel, NULL);
        ret->work_dim = work_dim;

        if( global_work_offset )
        {
            ret->global_work_offset.reserve(work_dim);
            ret->global_work_offset.insert(
                ret->global_work_offset.begin(),
                global_work_offset,
                global_work_offset + work_dim);
        }

        if( global_work_size )
        {
            ret->global_work_size.reserve(work_dim);
            ret->global_work_size.insert(
                ret->global_work_size.begin(),
                global_work_size,
                global_work_size + work_dim);
        }

        if( local_work_size )
        {
            ret->local_work_size.reserve(work_dim);
            ret->local_work_size.insert(
                ret->local_work_size.begin(),
                local_work_size,
                local_work_size + work_dim);
        }

        return ret;
    }

    int playback(
        CLIntercept* pIntercept,
        cl_command_queue queue) override
    {
        return pIntercept->dispatch().clEnqueueNDRangeKernel(
            queue,
            kernel,
            work_dim,
            global_work_offset.size() ? global_work_offset.data() : NULL,
            global_work_size.data(),
            local_work_size.size() ? local_work_size.data() : NULL,
            0,
            NULL,
            0);
        return CL_SUCCESS;
    }

    cl_kernel kernel;
    cl_uint work_dim;
    std::vector<size_t> global_work_offset;
    std::vector<size_t> global_work_size;
    std::vector<size_t> local_work_size;

private:
    NDRangeKernel() :
        kernel(NULL),
        work_dim(0) {}
};

}; // namespace cmdbuf

typedef struct _cl_command_buffer_khr
{
    static _cl_command_buffer_khr* create(
        cl_uint num_queues,
        const cl_command_queue* queues,
        const cl_command_buffer_properties_khr* properties,
        cl_int* errcode_ret)
    {
        cl_command_buffer_khr cmdbuf = NULL;
        cl_int errorCode = CL_SUCCESS;
        if( num_queues != 1 || queues == NULL )
        {
            errorCode = CL_INVALID_VALUE;
        }
        if( errcode_ret )
        {
            errcode_ret[0] = errorCode;
        }
        if( errorCode == CL_SUCCESS) {
            cmdbuf = new _cl_command_buffer_khr();
            cmdbuf->Queues.reserve(num_queues);
            cmdbuf->Queues.insert(
                cmdbuf->Queues.begin(),
                queues,
                queues + num_queues );
        }
        return cmdbuf;
    }

    static bool isValid( cl_command_buffer_khr cmdbuf )
    {
        return cmdbuf && cmdbuf->Magic == cMagic;
    }

    cl_int  checkRecordErrors(
                CLIntercept* pIntercept,
                cl_command_queue queue,
                cl_uint num_sync_points_in_wait_list,
                const cl_sync_point_khr* sync_point_wait_list,
                cl_mutable_command_khr* mutable_handle )
    {
        if( State != CL_COMMAND_BUFFER_STATE_RECORDING_KHR )
        {
            return CL_INVALID_OPERATION;
        }
        if( queue != NULL )
        {
            return CL_INVALID_COMMAND_QUEUE;
        }
        if( mutable_handle != NULL )
        {
            return CL_INVALID_VALUE;
        }
        if( ( sync_point_wait_list == NULL && num_sync_points_in_wait_list > 0 ) ||
            ( sync_point_wait_list != NULL && num_sync_points_in_wait_list == 0 ) )
        {
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR;
        }

        // CL_INVALID_CONTEXT if queue and cmdbuf do not have the same context?

        return CL_SUCCESS;
    }

    cl_int  checkPlaybackErrors(
                CLIntercept* pIntercept,
                cl_uint num_queues,
                cl_command_queue* queues,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list )
    {
        if( State != CL_COMMAND_BUFFER_STATE_EXECUTABLE_KHR )
        {
            return CL_INVALID_OPERATION;
        }
        if( ( queues == NULL && num_queues > 0 ) ||
            ( queues != NULL && num_queues == 0 ) )
        {
            return CL_INVALID_VALUE;
        }
        if( num_queues > 1 )
        {
            return CL_INVALID_VALUE;
        }

        // CL_INCOMPATIBLE_COMMAND_QUEUE_KHR if any element of queues is not compatible with the command-queue set on command_buffer creation at the same list index.
        // CL_INVALID_CONTEXT if any element of queues does not have the same context as the command-queue set on command_buffer creation at the same list indes.
        // CL_INVALID_CONTEXT if the context associated with the command buffer and events in event_wait_list are not the same.

        return CL_SUCCESS;
    }

    const cl_uint Magic;
    std::vector<cl_command_queue>   Queues;
    cl_uint RefCount;
    cl_command_buffer_state_khr State;
    std::vector<cmdbuf::command*>   Commands;

private:
    static constexpr cl_uint cMagic = 0x434d4442;   // "CMDB"

    _cl_command_buffer_khr() :
        Magic(cMagic),
        RefCount(1),
        State(CL_COMMAND_BUFFER_STATE_RECORDING_KHR) {}
} cli_command_buffer;

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_command_buffer_khr CL_API_CALL clCreateCommandBufferKHR_EMU(
    cl_uint num_queues,
    const cl_command_queue* queues,
    const cl_command_buffer_properties_khr* properties,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();
    if( pIntercept == NULL || !pIntercept->config().Emulate_cl_khr_command_buffer )
    {
        if( errcode_ret )
        {
            errcode_ret[0] = CL_INVALID_OPERATION;
        }
        return NULL;
    }

    return cli_command_buffer::create(
        num_queues,
        queues,
        properties,
        errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_int CL_API_CALL clFinalizeCommandBufferKHR_EMU(
    cl_command_buffer_khr cmdbuf)
{
    CLIntercept*    pIntercept = GetIntercept();
    if( pIntercept == NULL || !pIntercept->config().Emulate_cl_khr_command_buffer )
    {
        return CL_INVALID_OPERATION;
    }
    if( !cli_command_buffer::isValid(cmdbuf) )
    {
        return CL_INVALID_COMMAND_BUFFER_KHR;
    }
    if( cmdbuf->State != CL_COMMAND_BUFFER_STATE_RECORDING_KHR )
    {
        return CL_INVALID_OPERATION;
    }

    cmdbuf->State = CL_COMMAND_BUFFER_STATE_EXECUTABLE_KHR;
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_int CL_API_CALL clRetainCommandBufferKHR_EMU(
    cl_command_buffer_khr cmdbuf)
{
    CLIntercept*    pIntercept = GetIntercept();
    if( pIntercept == NULL || !pIntercept->config().Emulate_cl_khr_command_buffer )
    {
        return CL_INVALID_OPERATION;
    }
    if( !cli_command_buffer::isValid(cmdbuf) )
    {
        return CL_INVALID_COMMAND_BUFFER_KHR;
    }

    cmdbuf->RefCount++;
    return CL_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_int CL_API_CALL clReleaseCommandBufferKHR_EMU(
    cl_command_buffer_khr cmdbuf)
{
    CLIntercept*    pIntercept = GetIntercept();
    if( pIntercept == NULL || !pIntercept->config().Emulate_cl_khr_command_buffer )
    {
        return CL_INVALID_OPERATION;
    }
    if( !cli_command_buffer::isValid(cmdbuf) )
    {
        return CL_INVALID_COMMAND_BUFFER_KHR;
    }

    cmdbuf->RefCount--;
    if( cmdbuf->RefCount == 0 )
    {
        delete cmdbuf;
    }
    return CL_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_int CL_API_CALL clEnqueueCommandBufferKHR_EMU(
    cl_uint num_queues,
    cl_command_queue* queues,
    cl_command_buffer_khr cmdbuf,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();
    if( pIntercept == NULL || !pIntercept->config().Emulate_cl_khr_command_buffer )
    {
        return CL_INVALID_OPERATION;
    }
    if( !cli_command_buffer::isValid(cmdbuf) )
    {
        return CL_INVALID_COMMAND_BUFFER_KHR;
    }
    if( cl_int errorCode = cmdbuf->checkPlaybackErrors(
            pIntercept,
            num_queues,
            queues,
            num_events_in_wait_list,
            event_wait_list) )
    {
        return errorCode;
    }

    cl_command_queue queue = cmdbuf->Queues[0];
    if( num_queues > 0 )
    {
        queue = queues[0];
    }

    cl_int errorCode = CL_SUCCESS;

    if( errorCode == CL_SUCCESS && num_events_in_wait_list )
    {
        errorCode = pIntercept->dispatch().clEnqueueBarrierWithWaitList(
            queue,
            num_events_in_wait_list,
            event_wait_list,
            NULL );
    }

    for( auto& command : cmdbuf->Commands )
    {
        if( errorCode == CL_SUCCESS )
        {
            errorCode = command->playback(pIntercept, queue);
        }
    }

    if( errorCode == CL_SUCCESS && event )
    {
        errorCode = pIntercept->dispatch().clEnqueueBarrierWithWaitList(
            queue,
            0,
            NULL,
            event );
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_int CL_API_CALL clCommandBarrierWithWaitListKHR_EMU(
    cl_command_buffer_khr cmdbuf,
    cl_command_queue command_queue,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle)
{
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_int CL_API_CALL clCommandCopyBufferKHR_EMU(
    cl_command_buffer_khr cmdbuf,
    cl_command_queue command_queue,
    cl_mem src_buffer,
    cl_mem dst_buffer,
    size_t src_offset,
    size_t dst_offset,
    size_t size,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle)
{
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_int CL_API_CALL clCommandCopyBufferRectKHR_EMU(
    cl_command_buffer_khr cmdbuf,
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
    cl_mutable_command_khr* mutable_handle)
{
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_int CL_API_CALL clCommandCopyBufferToImageKHR_EMU(
    cl_command_buffer_khr cmdbuf,
    cl_command_queue command_queue,
    cl_mem src_buffer,
    cl_mem dst_image,
    size_t src_offset,
    const size_t* dst_origin,
    const size_t* region,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle)
{
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_int CL_API_CALL clCommandCopyImageKHR_EMU(
    cl_command_buffer_khr cmdbuf,
    cl_command_queue command_queue,
    cl_mem src_image,
    cl_mem dst_image,
    const size_t* src_origin,
    const size_t* dst_origin,
    const size_t* region,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle)
{
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_int CL_API_CALL clCommandCopyImageToBufferKHR_EMU(
    cl_command_buffer_khr cmdbuf,
    cl_command_queue command_queue,
    cl_mem src_image,
    cl_mem dst_buffer,
    const size_t* src_origin,
    const size_t* region,
    size_t dst_offset,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle)
{
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_int CL_API_CALL clCommandFillBufferKHR_EMU(
    cl_command_buffer_khr cmdbuf,
    cl_command_queue command_queue,
    cl_mem buffer,
    const void* pattern,
    size_t pattern_size,
    size_t offset,
    size_t size,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle)
{
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_int CL_API_CALL clCommandFillImageKHR_EMU(
    cl_command_buffer_khr cmdbuf,
    cl_command_queue command_queue,
    cl_mem image,
    const void* fill_color,
    const size_t* origin,
    const size_t* region,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle)
{
    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_int CL_API_CALL clCommandNDRangeKernelKHR_EMU(
    cl_command_buffer_khr cmdbuf,
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
    cl_mutable_command_khr* mutable_handle)
{
    CLIntercept*    pIntercept = GetIntercept();
    if( pIntercept == NULL || !pIntercept->config().Emulate_cl_khr_command_buffer )
    {
        return CL_INVALID_OPERATION;
    }
    if( !cli_command_buffer::isValid(cmdbuf) )
    {
        return CL_INVALID_COMMAND_BUFFER_KHR;
    }
    if( cl_int errorCode = cmdbuf->checkRecordErrors(
            pIntercept,
            command_queue,
            num_sync_points_in_wait_list,
            sync_point_wait_list,
            mutable_handle) )
    {
        return errorCode;
    }

    cmdbuf->Commands.push_back(cmdbuf::NDRangeKernel::create(
        pIntercept,
        kernel,
        work_dim,
        global_work_offset,
        global_work_size,
        local_work_size));
    return CL_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
cl_int CL_API_CALL clGetCommandBufferInfoKHR_EMU(
    cl_command_buffer_khr cmdbuf,
    cl_command_buffer_info_khr param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    CLIntercept*    pIntercept = GetIntercept();
    if( pIntercept == NULL || !pIntercept->config().Emulate_cl_khr_command_buffer )
    {
        return CL_INVALID_OPERATION;
    }
    if( !cli_command_buffer::isValid(cmdbuf) )
    {
        return CL_INVALID_COMMAND_BUFFER_KHR;
    }

    switch( param_name )
    {
    case CL_COMMAND_BUFFER_QUEUES_KHR:
        {
            auto*   ptr = (cl_command_queue*)param_value;
            return pIntercept->writeVectorToMemory(
                param_value_size,
                cmdbuf->Queues,
                param_value_size_ret,
                ptr );
        }
        break;
    case CL_COMMAND_BUFFER_NUM_QUEUES_KHR:
        {
            auto*   ptr = (cl_uint*)param_value;
            return pIntercept->writeParamToMemory(
                param_value_size,
                static_cast<cl_uint>(cmdbuf->Queues.size()),
                param_value_size_ret,
                ptr );
        }
        break;
    case CL_COMMAND_BUFFER_REFERENCE_COUNT_KHR:
        {
            auto*   ptr = (cl_uint*)param_value;
            return pIntercept->writeParamToMemory(
                param_value_size,
                cmdbuf->RefCount,
                param_value_size_ret,
                ptr );
        }
        break;
    case CL_COMMAND_BUFFER_STATE_KHR:
        {
            auto*   ptr = (cl_command_buffer_state_khr*)param_value;
            return pIntercept->writeParamToMemory(
                param_value_size,
                cmdbuf->State,
                param_value_size_ret,
                ptr );
        }
        break;
    case CL_COMMAND_BUFFER_PROPERTIES_ARRAY_KHR:
        // TODO!
        return CL_INVALID_VALUE;
    default:
        return CL_INVALID_VALUE;
    }

    return CL_INVALID_OPERATION;
}
