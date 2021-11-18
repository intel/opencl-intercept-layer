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
        pIntercept->finishAll( context );

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

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_semaphore
typedef struct _cl_semaphore_khr
{
    static _cl_semaphore_khr* create(
        cl_context context,
        const cl_semaphore_properties_khr* props,
        cl_int* errcode_ret)
    {
        // TODO: parse and record properties
        if( errcode_ret )
        {
            errcode_ret[0] = CL_SUCCESS;
        }
        return new _cl_semaphore_khr(context);
    }

    static bool isValid( cl_semaphore_khr semaphore )
    {
        return semaphore && semaphore->Magic == cMagic;
    }

    const cl_uint Magic;
    const cl_context Context;
    const cl_semaphore_type_khr Type;

    cl_uint RefCount;
    cl_event Event;

private:
    static constexpr cl_uint cMagic = 0x53454d41;   // "SEMA"

    _cl_semaphore_khr(cl_context context) :
        Magic(cMagic),
        Context(context),
        Type(CL_SEMAPHORE_TYPE_BINARY_KHR),
        RefCount(1),
        Event(NULL) {}
} cli_semaphore;

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_semaphore
cl_semaphore_khr CL_API_CALL clCreateSemaphoreWithPropertiesKHR_EMU(
    cl_context context,
    const cl_semaphore_properties_khr *sema_props,
    cl_int *errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();
    if( pIntercept == NULL || !pIntercept->config().Emulate_cl_khr_semaphore )
    {
        if( errcode_ret )
        {
            errcode_ret[0] = CL_INVALID_OPERATION;
        }
        return NULL;
    }

    return cli_semaphore::create(
        context,
        sema_props,
        errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_semaphore
cl_int CL_API_CALL clEnqueueWaitSemaphoresKHR_EMU(
    cl_command_queue command_queue,
    cl_uint num_semaphores,
    const cl_semaphore_khr *semaphores,
    const cl_semaphore_payload_khr *semaphore_payloads,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event)
{
    CLIntercept*    pIntercept = GetIntercept();
    if( pIntercept == NULL || !pIntercept->config().Emulate_cl_khr_semaphore )
    {
        return CL_INVALID_OPERATION;
    }
    if( num_semaphores == 0 )
    {
        return CL_INVALID_VALUE;
    }

    std::vector<cl_event> combinedWaitList;
    combinedWaitList.insert(
        combinedWaitList.end(),
        event_wait_list,
        event_wait_list + num_events_in_wait_list);

    for( cl_uint i = 0; i < num_semaphores; i++ )
    {
        if( !cli_semaphore::isValid(semaphores[i]) )
        {
            return CL_INVALID_SEMAPHORE_KHR;
        }
        if( semaphores[i]->Event == NULL )
        {
            // This is a semaphore that is not in a pending signal
            // or signaled state.  What should happen here?
            return CL_INVALID_OPERATION;
        }
        combinedWaitList.push_back(
            semaphores[i]->Event);
    }

    cl_int retVal = pIntercept->dispatch().clEnqueueMarkerWithWaitList(
        command_queue,
        (cl_uint)combinedWaitList.size(),
        combinedWaitList.data(),
        event );

    for( cl_uint i = 0; i < num_semaphores; i++ )
    {
        pIntercept->dispatch().clReleaseEvent(
            semaphores[i]->Event);
        semaphores[i]->Event = NULL;
    }

    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_semaphore
cl_int CL_API_CALL clEnqueueSignalSemaphoresKHR_EMU(
    cl_command_queue command_queue,
    cl_uint num_semaphores,
    const cl_semaphore_khr *semaphores,
    const cl_semaphore_payload_khr *sema_payload_list,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event)
{
    CLIntercept*    pIntercept = GetIntercept();
    if( pIntercept == NULL || !pIntercept->config().Emulate_cl_khr_semaphore )
    {
        return CL_INVALID_OPERATION;
    }
    if( num_semaphores == 0 )
    {
        return CL_INVALID_VALUE;
    }

    for( cl_uint i = 0; i < num_semaphores; i++ )
    {
        if( !cli_semaphore::isValid(semaphores[i]) )
        {
            return CL_INVALID_SEMAPHORE_KHR;
        }
        if( semaphores[i]->Event != NULL )
        {
            // This is a semaphore that is in a pending signal or signaled
            // state.  What should happen here?
            return CL_INVALID_OPERATION;
        }
    }

    cl_event    local_event = NULL;
    if( event == NULL )
    {
        event = &local_event;
    }

    cl_int retVal = pIntercept->dispatch().clEnqueueMarkerWithWaitList(
        command_queue,
        num_events_in_wait_list,
        event_wait_list,
        event );

    for( cl_uint i = 0; i < num_semaphores; i++ )
    {
        semaphores[i]->Event = *event;
        pIntercept->dispatch().clRetainEvent(
            semaphores[i]->Event );
    }

    if( local_event != NULL )
    {
        pIntercept->dispatch().clReleaseEvent(
            local_event );
        local_event = NULL;
    }

    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_semaphore
cl_int CL_API_CALL clGetSemaphoreInfoKHR_EMU(
    cl_semaphore_khr semaphore,
    cl_semaphore_info_khr param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret)
{
    CLIntercept*    pIntercept = GetIntercept();
    if( pIntercept == NULL || !pIntercept->config().Emulate_cl_khr_semaphore )
    {
        return CL_INVALID_OPERATION;
    }
    if( !cli_semaphore::isValid(semaphore) )
    {
        return CL_INVALID_SEMAPHORE_KHR;
    }

    switch( param_name )
    {
    case CL_SEMAPHORE_CONTEXT_KHR:
        {
            auto*   ptr = (cl_context*)param_value;
            return pIntercept->writeParamToMemory(
                param_value_size,
                semaphore->Context,
                param_value_size_ret,
                ptr );
        }
    case CL_SEMAPHORE_REFERENCE_COUNT_KHR:
        {
            auto*   ptr = (cl_uint*)param_value;
            return pIntercept->writeParamToMemory(
                param_value_size,
                semaphore->RefCount,
                param_value_size_ret,
                ptr );
        }
    case CL_SEMAPHORE_PROPERTIES_KHR:
        // TODO!
        return CL_INVALID_VALUE;
    case CL_SEMAPHORE_TYPE_KHR:
        {
            auto*   ptr = (cl_semaphore_type_khr*)param_value;
            return pIntercept->writeParamToMemory(
                param_value_size,
                semaphore->Type,
                param_value_size_ret,
                ptr );
        }
        break;
    case CL_SEMAPHORE_PAYLOAD_KHR:
        {
            // For binary semaphores, the payload should be zero if the
            // semaphore is in the unsignaled state and one if it is in
            // the signaled state.
            cl_semaphore_payload_khr payload = 0;
            if( semaphore->Event != NULL )
            {
                cl_int  eventStatus = 0;
                pIntercept->dispatch().clGetEventInfo(
                    semaphore->Event,
                    CL_EVENT_COMMAND_EXECUTION_STATUS,
                    sizeof( eventStatus ),
                    &eventStatus,
                    NULL );
                if( eventStatus == CL_COMPLETE )
                {
                    payload = 1;
                }
            }

            auto*   ptr = (cl_semaphore_payload_khr*)param_value;
            return pIntercept->writeParamToMemory(
                param_value_size,
                payload,
                param_value_size_ret,
                ptr );
        }
        break;
    default:
        return CL_INVALID_VALUE;
    }

    return CL_INVALID_OPERATION;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_semaphore
cl_int CL_API_CALL clRetainSemaphoreKHR_EMU(
    cl_semaphore_khr semaphore)
{
    CLIntercept*    pIntercept = GetIntercept();
    if( pIntercept == NULL || !pIntercept->config().Emulate_cl_khr_semaphore )
    {
        return CL_INVALID_OPERATION;
    }
    if( !cli_semaphore::isValid(semaphore) )
    {
        return CL_INVALID_SEMAPHORE_KHR;
    }

    semaphore->RefCount++;
    return CL_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_semaphore
cl_int CL_API_CALL clReleaseSemaphoreKHR_EMU(
    cl_semaphore_khr semaphore)
{
    CLIntercept*    pIntercept = GetIntercept();
    if( pIntercept == NULL || !pIntercept->config().Emulate_cl_khr_semaphore )
    {
        return CL_INVALID_OPERATION;
    }
    if( !cli_semaphore::isValid(semaphore) )
    {
        return CL_INVALID_SEMAPHORE_KHR;
    }

    semaphore->RefCount--;
    if( semaphore->RefCount == 0 )
    {
        delete semaphore;
    }
    return CL_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
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

    const cl_uint Magic;
    std::vector<cl_command_queue>   Queues;
    cl_uint RefCount;

private:
    static constexpr cl_uint cMagic = 0x434d4442;   // "CMDB"

    _cl_command_buffer_khr() :
        Magic(cMagic),
        RefCount(1) {}
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
    return CL_INVALID_OPERATION;
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
    return CL_INVALID_OPERATION;
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
    case CL_COMMAND_BUFFER_PROPERTIES_ARRAY_KHR:
        // TODO!
        return CL_INVALID_VALUE;
    default:
        return CL_INVALID_VALUE;
    }

    return CL_INVALID_OPERATION;
}
