/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
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
