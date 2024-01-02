/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include <string>

#include "intercept.h"

///////////////////////////////////////////////////////////////////////////////
//
static std::string getFormattedEventWaitList(
    const CLIntercept* pIntercept,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list )
{
    std::string eventWaitListString;
    if( pIntercept->config().CallLogging && num_events_in_wait_list )
    {
        eventWaitListString += ", event_wait_list = ";
        pIntercept->getEventListString(
            num_events_in_wait_list,
            event_wait_list,
            eventWaitListString );
    }
    return eventWaitListString;
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetPlatformIDs)(
    cl_uint num_entries,
    cl_platform_id* platforms,
    cl_uint* num_platforms )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetPlatformIDs )
    {
        LOG_CLINFO();

        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER();
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetPlatformIDs(
            num_entries,
            platforms,
            num_platforms );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetPlatformInfo)(
    cl_platform_id platform,
    cl_platform_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetPlatformInfo )
    {
        GET_ENQUEUE_COUNTER();

        std::string platformInfo;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getPlatformInfoString(
                platform,
                platformInfo );
        }
        CALL_LOGGING_ENTER( "platform = %s, param_name = %s (%08X)",
            platformInfo.c_str(),
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = CL_SUCCESS;

        if( pIntercept->overrideGetPlatformInfo(
                platform,
                param_name,
                param_value_size,
                param_value,
                param_value_size_ret,
                retVal ) == false )
        {
            retVal = pIntercept->dispatch().clGetPlatformInfo(
                platform,
                param_name,
                param_value_size,
                param_value,
                param_value_size_ret );
        }

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetDeviceIDs)(
    cl_platform_id platform,
    cl_device_type device_type,
    cl_uint num_entries,
    cl_device_id* devices,
    cl_uint* num_devices )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetDeviceIDs )
    {
        GET_ENQUEUE_COUNTER();

        std::string platformInfo;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getPlatformInfoString(
                platform,
                platformInfo );
        }
        CALL_LOGGING_ENTER( "platform = %s, device_type = %s (%llX)",
            platformInfo.c_str(),
            pIntercept->enumName().name_device_type( device_type ).c_str(),
            device_type );
        HOST_PERFORMANCE_TIMING_START();

        cl_int retVal = CL_INVALID_OPERATION;

        device_type = pIntercept->filterDeviceType( device_type );

        if( pIntercept->config().AutoPartitionAllDevices ||
            pIntercept->config().AutoPartitionAllSubDevices ||
            pIntercept->config().AutoPartitionSingleSubDevice )
        {
            retVal = pIntercept->autoPartitionGetDeviceIDs(
                platform,
                device_type,
                num_entries,
                devices,
                num_devices );
        }

        if( retVal != CL_SUCCESS )
        {
            retVal = pIntercept->dispatch().clGetDeviceIDs(
                platform,
                device_type,
                num_entries,
                devices,
                num_devices );
        }

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////

CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetDeviceInfo)(
    cl_device_id device,
    cl_device_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetDeviceInfo )
    {
        GET_ENQUEUE_COUNTER();

        std::string deviceInfo;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getDeviceInfoString(
                1,
                &device,
                deviceInfo );
        }
        CALL_LOGGING_ENTER( "device = %s, param_name = %s (%08X)",
            deviceInfo.c_str(),
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = CL_SUCCESS;

        if( pIntercept->overrideGetDeviceInfo(
                device,
                param_name,
                param_value_size,
                param_value,
                param_value_size_ret,
                retVal ) == false )
        {
            retVal = pIntercept->dispatch().clGetDeviceInfo(
                device,
                param_name,
                param_value_size,
                param_value,
                param_value_size_ret );
        }

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clCreateSubDevices)(
    cl_device_id in_device,
    const cl_device_partition_property* properties,
    cl_uint num_devices,
    cl_device_id* out_devices,
    cl_uint* num_devices_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateSubDevices )
    {
        GET_ENQUEUE_COUNTER();

        cl_uint local_num_devices_ret = 0;
        if( num_devices_ret == NULL )
        {
            num_devices_ret = &local_num_devices_ret;
        }

        std::string deviceInfo;
        std::string propsStr;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getDeviceInfoString(
                1,
                &in_device,
                deviceInfo );
            pIntercept->getDevicePartitionPropertiesString(
                properties,
                propsStr );
        }
        CALL_LOGGING_ENTER( "in_device = %s, properties = [ %s ], num_devices = %u",
            deviceInfo.c_str(),
            propsStr.c_str(),
            num_devices );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clCreateSubDevices(
            in_device,
            properties,
            num_devices,
            out_devices,
            num_devices_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        if( pIntercept->config().LeakChecking &&
            out_devices &&
            num_devices_ret )
        {
            for( cl_uint d = 0; d < num_devices_ret[0]; d++ )
            {
                ADD_OBJECT_ALLOCATION( out_devices[d] );
            }
        }
        CALL_LOGGING_EXIT( retVal );

        if( retVal == CL_SUCCESS &&
            out_devices &&
            num_devices_ret )
        {
            pIntercept->addSubDeviceInfo(
                in_device,
                out_devices,
                num_devices_ret[0] );
        }

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clRetainDevice)(
    cl_device_id device )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clRetainDevice )
    {
        GET_ENQUEUE_COUNTER();

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( device ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] device = %p",
            ref_count,
            device );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clRetainDevice(
            device );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RETAIN( device );
        ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( device ) : 0;
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clReleaseDevice)(
    cl_device_id device )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clReleaseDevice )
    {
        GET_ENQUEUE_COUNTER();

        // Reference counts are only decremented for devices that are
        // are sub-devices (that have a parent device).
        cl_device_id parent = NULL;
        pIntercept->dispatch().clGetDeviceInfo(
            device,
            CL_DEVICE_PARENT_DEVICE,
            sizeof(parent),
            &parent,
            NULL);

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( device ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] device = %p",
            ref_count,
            device );
        pIntercept->checkRemoveDeviceInfo( device );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clReleaseDevice(
            device );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RELEASE( device );
        ref_count = ( parent != NULL ) ? ref_count - 1 : ref_count;
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

#ifdef __ANDROID__
//Workaround for Android, shared library destructor isn't called
static int contextCount = 0;
static std::mutex mContextCount;
#endif

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_context CL_API_CALL CLIRN(clCreateContext)(
    const cl_context_properties* properties,
    cl_uint num_devices,
    const cl_device_id* devices,
    void (CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
    void* user_data,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateContext )
    {
        GET_ENQUEUE_COUNTER();

        cl_context_properties*  newProperties = NULL;
        cl_context  retVal = NULL;

        std::string contextProperties;
        std::string deviceInfo;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getContextPropertiesString(
                properties,
                contextProperties );
            pIntercept->getDeviceInfoString(
                num_devices,
                devices,
                deviceInfo );
        }
        CALL_LOGGING_ENTER( "properties = [ %s ], num_devices = %u, devices = [ %s ]",
            contextProperties.c_str(),
            num_devices,
            deviceInfo.c_str() );
        CREATE_CONTEXT_OVERRIDE_INIT( properties, pfn_notify, user_data, newProperties );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        if( ( retVal == NULL ) && newProperties )
        {
            retVal = pIntercept->dispatch().clCreateContext(
                newProperties,
                num_devices,
                devices,
                pfn_notify,
                user_data,
                errcode_ret );
        }
        if( retVal == NULL )
        {
            retVal = pIntercept->dispatch().clCreateContext(
                properties,
                num_devices,
                devices,
                pfn_notify,
                user_data,
                errcode_ret );
        }

        ITT_ADD_PARAM_AS_METADATA( retVal );

        INIT_PRECOMPILED_KERNEL_OVERRIDES( retVal );
        INIT_BUILTIN_KERNEL_OVERRIDES( retVal );

        HOST_PERFORMANCE_TIMING_END();
        CREATE_CONTEXT_OVERRIDE_CLEANUP( retVal, newProperties );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

#ifdef __ANDROID__
        mContextCount.lock();
        contextCount ++;
        mContextCount.unlock();
#endif
        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_context CL_API_CALL CLIRN(clCreateContextFromType)(
    const cl_context_properties* properties,
    cl_device_type device_type,
    void (CL_CALLBACK *pfn_notify)(const char *, const void *, size_t, void *),
    void* user_data,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateContextFromType )
    {
        GET_ENQUEUE_COUNTER();

        cl_context_properties*  newProperties = NULL;
        cl_context  retVal = NULL;

        std::string contextProperties;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getContextPropertiesString(
                properties,
                contextProperties );
        }
        CALL_LOGGING_ENTER( "properties = [ %s ], device_type = %s (%llX)",
            contextProperties.c_str(),
            pIntercept->enumName().name_device_type( device_type ).c_str(),
            device_type );
        CREATE_CONTEXT_OVERRIDE_INIT( properties, pfn_notify, user_data, newProperties );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        device_type = pIntercept->filterDeviceType( device_type );

        if( ( retVal == NULL ) && newProperties )
        {
            retVal = pIntercept->dispatch().clCreateContextFromType(
                newProperties,
                device_type,
                pfn_notify,
                user_data,
                errcode_ret );
        }
        if( retVal == NULL )
        {
            retVal = pIntercept->dispatch().clCreateContextFromType(
                properties,
                device_type,
                pfn_notify,
                user_data,
                errcode_ret );
        }

        ITT_ADD_PARAM_AS_METADATA( retVal );

        INIT_PRECOMPILED_KERNEL_OVERRIDES( retVal );
        INIT_BUILTIN_KERNEL_OVERRIDES( retVal );

        HOST_PERFORMANCE_TIMING_END();
        CREATE_CONTEXT_OVERRIDE_CLEANUP( retVal, newProperties );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

#ifdef __ANDROID__
        mContextCount.lock();
        contextCount ++;
        mContextCount.unlock();
#endif
        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clRetainContext)(
    cl_context context )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clRetainContext )
    {
        GET_ENQUEUE_COUNTER();

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( context ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] context = %p",
            ref_count,
            context );
        HOST_PERFORMANCE_TIMING_START();

        cl_int retVal = pIntercept->dispatch().clRetainContext(
            context );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RETAIN( context );
        ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( context ) : 0;
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clReleaseContext)(
    cl_context context )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clReleaseContext )
    {
        GET_ENQUEUE_COUNTER();

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( context ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] context = %p",
            ref_count,
            context );
        HOST_PERFORMANCE_TIMING_START();

        cl_int retVal = pIntercept->dispatch().clReleaseContext(
            context );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RELEASE( context );
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", --ref_count );
        DEVICE_PERFORMANCE_TIMING_CHECK_CONDITIONAL( ref_count == 0 );
        FLUSH_CHROME_TRACE_BUFFERING_CONDITIONAL( ref_count == 0 );

#if 0
        pIntercept->report();
#endif

#ifdef __ANDROID__
        mContextCount.lock();
        contextCount --;
        mContextCount.unlock();

        if( contextCount == 0 )
        {
            pIntercept->report();
        }
#endif
        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetContextInfo)(
    cl_context context,
    cl_context_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetContextInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "param_name = %s (%08X)",
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int retVal = pIntercept->dispatch().clGetContextInfo(
            context,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 3.0
CL_API_ENTRY cl_int CL_API_CALL clSetContextDestructorCallback(
    cl_context context,
    void (CL_CALLBACK *pfn_notify)( cl_context, void* ),
    void *user_data )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clSetContextDestructorCallback )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER();
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clSetContextDestructorCallback(
            context,
            pfn_notify,
            user_data );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_command_queue CL_API_CALL CLIRN(clCreateCommandQueue)(
    cl_context context,
    cl_device_id device,
    cl_command_queue_properties properties,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateCommandQueue )
    {
        GET_ENQUEUE_COUNTER();

        cl_queue_properties*    newProperties = NULL;
        cl_command_queue    retVal = NULL;

        std::string deviceInfo;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getDeviceInfoString(
                1,
                &device,
                deviceInfo );
        }
        CALL_LOGGING_ENTER( "context = %p, device = %s, properties = %s (%llX)",
            context,
            deviceInfo.c_str(),
            pIntercept->enumName().name_command_queue_properties( properties ).c_str(),
            properties );
        DUMMY_COMMAND_QUEUE( context, device );
        pIntercept->modifyCommandQueueProperties( properties );
        CREATE_COMMAND_QUEUE_PROPERTIES( device, properties, newProperties );

        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

#if defined(USE_MDAPI)
        if( pIntercept->config().DevicePerfCounterEventBasedSampling )
        {
            if( ( retVal == NULL ) && newProperties )
            {
                retVal = pIntercept->createMDAPICommandQueue(
                    context,
                    device,
                    newProperties,
                    errcode_ret );
            }
            if( retVal == NULL )
            {
                retVal = pIntercept->createMDAPICommandQueue(
                    context,
                    device,
                    properties,
                    errcode_ret );
            }
        }
#endif

        if( ( retVal == NULL ) && newProperties )
        {
            retVal = pIntercept->createCommandQueueWithProperties(
                context,
                device,
                newProperties,
                errcode_ret );
        }
        if( retVal == NULL )
        {
            retVal = pIntercept->dispatch().clCreateCommandQueue(
                context,
                device,
                properties,
                errcode_ret );
        }

        HOST_PERFORMANCE_TIMING_END();
        COMMAND_QUEUE_PROPERTIES_CLEANUP( newProperties );
        CHECK_ERROR( errcode_ret[0] );
        ITT_REGISTER_COMMAND_QUEUE( retVal, false );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );
        ADD_QUEUE( context, retVal );
        QUEUE_INFO_LOGGING( device, retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clRetainCommandQueue)(
    cl_command_queue command_queue )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clRetainCommandQueue )
    {
        GET_ENQUEUE_COUNTER();

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( command_queue ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] command_queue = %p",
            ref_count,
            command_queue );
        HOST_PERFORMANCE_TIMING_START();

        cl_int retVal = pIntercept->dispatch().clRetainCommandQueue(
            command_queue );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RETAIN( command_queue );
        ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( command_queue ) : 0;
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clReleaseCommandQueue)(
    cl_command_queue command_queue )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clReleaseCommandQueue )
    {
        GET_ENQUEUE_COUNTER();
        REMOVE_QUEUE( command_queue );

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( command_queue ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] command_queue = %p",
            ref_count,
            command_queue );
        HOST_PERFORMANCE_TIMING_START();

        cl_int retVal = pIntercept->dispatch().clReleaseCommandQueue(
            command_queue );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ITT_RELEASE_COMMAND_QUEUE( command_queue );
        ADD_OBJECT_RELEASE( command_queue );
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", --ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetCommandQueueInfo)(
    cl_command_queue command_queue,
    cl_command_queue_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetCommandQueueInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "command_queue = %p, param_name = %s (%08X)",
            command_queue,
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int retVal = pIntercept->dispatch().clGetCommandQueueInfo(
            command_queue,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clSetCommandQueueProperty)(
    cl_command_queue command_queue,
    cl_command_queue_properties properties,
    cl_bool enable,
    cl_command_queue_properties* old_properties )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clSetCommandQueueProperty )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER();
        HOST_PERFORMANCE_TIMING_START();

        cl_int retVal = pIntercept->dispatch().clSetCommandQueueProperty(
            command_queue,
            properties,
            enable,
            old_properties );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_mem CL_API_CALL CLIRN(clCreateBuffer)(
    cl_context context,
    cl_mem_flags flags,
    size_t size,
    void* host_ptr,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateBuffer )
    {
        GET_ENQUEUE_COUNTER();

        CALL_LOGGING_ENTER( "context = %p, flags = %s (%llX), size = %zu, host_ptr = %p",
            context,
            pIntercept->enumName().name_mem_flags( flags ).c_str(),
            flags,
            size,
            host_ptr );

        if( pIntercept->config().DumpReplayKernelEnqueue != -1 ||
            !pIntercept->config().DumpReplayKernelName.empty() )
        {
            // Make sure that there are no device only buffers
            // Since we need them to replay the kernel
            flags &= ~CL_MEM_HOST_NO_ACCESS;
        }
        INITIALIZE_BUFFER_CONTENTS_INIT( flags, size, host_ptr );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_mem  retVal = pIntercept->dispatch().clCreateBuffer(
            context,
            flags,
            size,
            host_ptr,
            errcode_ret );


        HOST_PERFORMANCE_TIMING_END();
        ADD_BUFFER( retVal );
        INITIALIZE_BUFFER_CONTENTS_CLEANUP( flags, host_ptr );
        DUMP_BUFFER_AFTER_CREATE( retVal, flags, host_ptr, size );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 3.0
CL_API_ENTRY cl_mem CL_API_CALL CLIRN(clCreateBufferWithProperties)(
    cl_context context,
    const cl_mem_properties* properties,
    cl_mem_flags flags,
    size_t size,
    void* host_ptr,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateBufferWithProperties )
    {
        GET_ENQUEUE_COUNTER();

        std::string propsStr;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getMemPropertiesString(
                properties,
                propsStr );
        }
        CALL_LOGGING_ENTER( "context = %p, properties = [ %s ], flags = %s (%llX), size = %zu, host_ptr = %p",
            context,
            propsStr.c_str(),
            pIntercept->enumName().name_mem_flags( flags ).c_str(),
            flags,
            size,
            host_ptr );
        INITIALIZE_BUFFER_CONTENTS_INIT( flags, size, host_ptr );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_mem  retVal = pIntercept->dispatch().clCreateBufferWithProperties(
            context,
            properties,
            flags,
            size,
            host_ptr,
            errcode_ret );

        HOST_PERFORMANCE_TIMING_END();
        ADD_BUFFER( retVal );
        INITIALIZE_BUFFER_CONTENTS_CLEANUP( flags, host_ptr );
        DUMP_BUFFER_AFTER_CREATE( retVal, flags, host_ptr, size );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_nv_create_buffer
CL_API_ENTRY cl_mem CL_API_CALL clCreateBufferNV(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_flags_NV flags_NV,
    size_t size,
    void* host_ptr,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreateBufferNV )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "context = %p, flags = %s (%llX), flags_NV = %s (%llX), size = %zu, host_ptr = %p",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags,
                pIntercept->enumName().name_mem_flags_NV( flags_NV ).c_str(),
                flags_NV,
                size,
                host_ptr );
            INITIALIZE_BUFFER_CONTENTS_INIT( flags, size, host_ptr );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_mem  retVal = dispatchX.clCreateBufferNV(
                context,
                flags,
                flags_NV,
                size,
                host_ptr,
                errcode_ret );

            HOST_PERFORMANCE_TIMING_END();
            ADD_BUFFER( retVal );
            INITIALIZE_BUFFER_CONTENTS_CLEANUP( flags, host_ptr );
            DUMP_BUFFER_AFTER_CREATE( retVal, flags, host_ptr, size );
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CL_API_ENTRY cl_mem CL_API_CALL CLIRN(clCreateSubBuffer)(
    cl_mem buffer,
    cl_mem_flags flags,
    cl_buffer_create_type buffer_create_type,
    const void *buffer_create_info,
    cl_int *errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateSubBuffer )
    {
        GET_ENQUEUE_COUNTER();

        std::string argsString;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getCreateSubBufferArgsString(
                buffer_create_type,
                buffer_create_info,
                argsString );
        }
        CALL_LOGGING_ENTER( "buffer = %p, flags = %s (%llX), %s",
            buffer,
            pIntercept->enumName().name_mem_flags( flags ).c_str(),
            flags,
            argsString.c_str() );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_mem  retVal = pIntercept->dispatch().clCreateSubBuffer(
            buffer,
            flags,
            buffer_create_type,
            buffer_create_info,
            errcode_ret );

        HOST_PERFORMANCE_TIMING_END();
        ADD_BUFFER( retVal );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY cl_mem CL_API_CALL CLIRN(clCreateImage)(
    cl_context context,
    cl_mem_flags flags,
    const cl_image_format* image_format,
    const cl_image_desc* image_desc,
    void* host_ptr,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateImage )
    {
        GET_ENQUEUE_COUNTER();

        if( image_desc && image_format )
        {
            CALL_LOGGING_ENTER(
                "context = %p, "
                "flags = %s (%llX), "
                "format->channel_order = %s, "
                "format->channel_data_type = %s, "
                "desc->type = %s, "
                "desc->width = %zu, "
                "desc->height = %zu, "
                "desc->depth = %zu, "
                "desc->array_size = %zu, "
                "desc->row_pitch = %zu, "
                "desc->slice_pitch = %zu, "
                "desc->num_mip_levels = %u, "
                "desc->num_samples = %u, "
                "desc->mem_object = %p, "
                "host_ptr = %p ",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags,
                pIntercept->enumName().name( image_format->image_channel_order ).c_str(),
                pIntercept->enumName().name( image_format->image_channel_data_type ).c_str(),
                pIntercept->enumName().name( image_desc->image_type ).c_str(),
                image_desc->image_width,
                image_desc->image_height,
                image_desc->image_depth,
                image_desc->image_array_size,
                image_desc->image_row_pitch,
                image_desc->image_slice_pitch,
                image_desc->num_mip_levels,
                image_desc->num_samples,
                image_desc->mem_object,
                host_ptr );
        }
        else
        {
            CALL_LOGGING_ENTER();
        }

        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_mem  retVal = pIntercept->dispatch().clCreateImage(
            context,
            flags,
            image_format,
            image_desc,
            host_ptr,
            errcode_ret );

        HOST_PERFORMANCE_TIMING_END();
        ADD_IMAGE( retVal );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 3.0
CL_API_ENTRY cl_mem CL_API_CALL CLIRN(clCreateImageWithProperties)(
    cl_context context,
    const cl_mem_properties* properties,
    cl_mem_flags flags,
    const cl_image_format* image_format,
    const cl_image_desc* image_desc,
    void* host_ptr,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateImageWithProperties )
    {
        GET_ENQUEUE_COUNTER();

        if( image_desc && image_format )
        {
            std::string propsStr;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getMemPropertiesString(
                    properties,
                    propsStr );
            }
            CALL_LOGGING_ENTER(
                "context = %p, "
                "properties = [ %s ], "
                "flags = %s (%llX), "
                "format->channel_order = %s, "
                "format->channel_data_type = %s, "
                "desc->type = %s, "
                "desc->width = %zu, "
                "desc->height = %zu, "
                "desc->depth = %zu, "
                "desc->array_size = %zu, "
                "desc->row_pitch = %zu, "
                "desc->slice_pitch = %zu, "
                "desc->num_mip_levels = %u, "
                "desc->num_samples = %u, "
                "desc->mem_object = %p, "
                "host_ptr = %p ",
                context,
                propsStr.c_str(),
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags,
                pIntercept->enumName().name( image_format->image_channel_order ).c_str(),
                pIntercept->enumName().name( image_format->image_channel_data_type ).c_str(),
                pIntercept->enumName().name( image_desc->image_type ).c_str(),
                image_desc->image_width,
                image_desc->image_height,
                image_desc->image_depth,
                image_desc->image_array_size,
                image_desc->image_row_pitch,
                image_desc->image_slice_pitch,
                image_desc->num_mip_levels,
                image_desc->num_samples,
                image_desc->mem_object,
                host_ptr );
        }
        else
        {
            CALL_LOGGING_ENTER();
        }

        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_mem  retVal = pIntercept->dispatch().clCreateImageWithProperties(
            context,
            properties,
            flags,
            image_format,
            image_desc,
            host_ptr,
            errcode_ret );

        HOST_PERFORMANCE_TIMING_END();
        ADD_IMAGE( retVal );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_mem CL_API_CALL CLIRN(clCreateImage2D)(
    cl_context context,
    cl_mem_flags flags,
    const cl_image_format* image_format,
    size_t image_width,
    size_t image_height,
    size_t image_row_pitch,
    void* host_ptr,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateImage2D )
    {
        GET_ENQUEUE_COUNTER();

        if( image_format )
        {
            CALL_LOGGING_ENTER(
                "context = %p, "
                "flags = %s (%llX), "
                "format->channel_order = %s, "
                "format->channel_data_type = %s, "
                "image_width = %zu, "
                "image_height = %zu, "
                "image_row_pitch = %zu, "
                "host_ptr = %p ",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags,
                pIntercept->enumName().name( image_format->image_channel_order ).c_str(),
                pIntercept->enumName().name( image_format->image_channel_data_type ).c_str(),
                image_width,
                image_height,
                image_row_pitch,
                host_ptr );
        }
        else
        {
            CALL_LOGGING_ENTER();
        }

        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_mem  retVal = pIntercept->dispatch().clCreateImage2D(
            context,
            flags,
            image_format,
            image_width,
            image_height,
            image_row_pitch,
            host_ptr,
            errcode_ret );

        HOST_PERFORMANCE_TIMING_END();
        ADD_IMAGE( retVal );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_mem CL_API_CALL CLIRN(clCreateImage3D)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateImage3D )
    {
        GET_ENQUEUE_COUNTER();

        if( image_format )
        {
            CALL_LOGGING_ENTER(
                "context = %p, "
                "flags = %s (%llX), "
                "format->channel_order = %s, "
                "format->channel_data_type = %s, "
                "image_width = %zu, "
                "image_height = %zu, "
                "image_depth = %zu, "
                "image_row_pitch = %zu, "
                "image_slice_pitch = %zu, "
                "host_ptr = %p ",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags,
                pIntercept->enumName().name( image_format->image_channel_order ).c_str(),
                pIntercept->enumName().name( image_format->image_channel_data_type ).c_str(),
                image_width,
                image_height,
                image_depth,
                image_row_pitch,
                image_slice_pitch,
                host_ptr );
        }
        else
        {
            CALL_LOGGING_ENTER();
        }

        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_mem  retVal = pIntercept->dispatch().clCreateImage3D(
            context,
            flags,
            image_format,
            image_width,
            image_height,
            image_depth,
            image_row_pitch,
            image_slice_pitch,
            host_ptr,
            errcode_ret );

        HOST_PERFORMANCE_TIMING_END();
        ADD_IMAGE( retVal );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clRetainMemObject)(
    cl_mem memobj )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clRetainMemObject )
    {
        GET_ENQUEUE_COUNTER();

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( memobj ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] mem = %p",
            ref_count,
            memobj );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clRetainMemObject(
            memobj );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RETAIN( memobj );
        ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( memobj ) : 0;
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clReleaseMemObject)(
    cl_mem memobj )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clReleaseMemObject )
    {
        GET_ENQUEUE_COUNTER();
        REMOVE_MEMOBJ( memobj );

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( memobj ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] mem = %p",
            ref_count,
            memobj );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clReleaseMemObject(
            memobj );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RELEASE( memobj );
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", --ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetSupportedImageFormats)(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_object_type image_type,
    cl_uint num_entries,
    cl_image_format* image_formats,
    cl_uint* num_image_formats )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetSupportedImageFormats )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "context = %p, flags = %s (%llX), image_type = %s (%X)",
            context,
            pIntercept->enumName().name_mem_flags( flags ).c_str(),
            flags,
            pIntercept->enumName().name( image_type ).c_str(),
            image_type );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetSupportedImageFormats(
            context,
            flags,
            image_type,
            num_entries,
            image_formats,
            num_image_formats );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetMemObjectInfo)(
    cl_mem memobj,
    cl_mem_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetMemObjectInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "mem = %p, param_name = %s (%08X)",
            memobj,
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetMemObjectInfo(
            memobj,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetImageInfo)(
    cl_mem image,
    cl_image_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetImageInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "mem = %p, param_name = %s (%08X)",
            image,
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetImageInfo(
            image,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clSetMemObjectDestructorCallback)(
    cl_mem memobj,
    void (CL_CALLBACK *pfn_notify)( cl_mem, void* ),
    void *user_data )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clSetMemObjectDestructorCallback )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER();
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clSetMemObjectDestructorCallback(
            memobj,
            pfn_notify,
            user_data );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_sampler CL_API_CALL CLIRN(clCreateSampler)(
    cl_context context,
    cl_bool normalized_coords,
    cl_addressing_mode addressing_mode,
    cl_filter_mode filter_mode,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateSampler )
    {
        GET_ENQUEUE_COUNTER();

        std::string propsStr;
        if( pIntercept->config().CallLogging ||
            pIntercept->config().DumpReplayKernelEnqueue != -1 ||
            !pIntercept->config().DumpReplayKernelName.empty() )
        {
            cl_sampler_properties sampler_properties[] = {
                CL_SAMPLER_NORMALIZED_COORDS, normalized_coords,
                CL_SAMPLER_ADDRESSING_MODE,   addressing_mode,
                CL_SAMPLER_FILTER_MODE,       filter_mode,
                0
            };
            pIntercept->getSamplerPropertiesString(
                sampler_properties,
                propsStr );
        }

        CALL_LOGGING_ENTER( "context = %p, properties = [ %s ]",
            context,
            propsStr.c_str() );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_sampler  retVal = pIntercept->dispatch().clCreateSampler(
            context,
            normalized_coords,
            addressing_mode,
            filter_mode,
            errcode_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );
        ADD_SAMPLER( retVal, propsStr );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clRetainSampler)(
    cl_sampler sampler )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clRetainSampler )
    {
        GET_ENQUEUE_COUNTER();

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( sampler ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] sampler = %p",
            ref_count,
            sampler );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clRetainSampler(
            sampler );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RETAIN( sampler );
        ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( sampler ) : 0;
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clReleaseSampler)(
    cl_sampler sampler )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clReleaseSampler )
    {
        GET_ENQUEUE_COUNTER();
        REMOVE_SAMPLER( sampler );

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( sampler ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] sampler = %p",
            ref_count,
            sampler );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clReleaseSampler(
            sampler );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RELEASE( sampler );
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", --ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetSamplerInfo)(
    cl_sampler sampler,
    cl_sampler_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetSamplerInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "param_name = %s (%08X)",
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetSamplerInfo(
            sampler,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_program CL_API_CALL CLIRN(clCreateProgramWithSource)(
    cl_context context,
    cl_uint count,
    const char** strings,
    const size_t* lengths,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateProgramWithSource )
    {
        GET_ENQUEUE_COUNTER();

        char*       singleString = NULL;
        uint64_t    hash = 0;

        CREATE_COMBINED_PROGRAM_STRING( count, strings, lengths, singleString, hash );
        INJECT_PROGRAM_SOURCE( count, strings, lengths, singleString, hash );
        PREPEND_PROGRAM_SOURCE( count, strings, lengths, singleString, hash );

        CALL_LOGGING_ENTER( "context = %p, count = %u",
            context,
            count );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_program  retVal = NULL;

        if( ( retVal == NULL ) &&
            pIntercept->config().InjectProgramBinaries )
        {
            retVal = pIntercept->createProgramWithInjectionBinaries(
                hash,
                context,
                errcode_ret );
        }

        if( ( retVal == NULL ) &&
            pIntercept->config().InjectProgramSPIRV )
        {
            retVal = pIntercept->createProgramWithInjectionSPIRV(
                hash,
                context,
                errcode_ret );
        }

        if( retVal == NULL )
        {
            retVal = pIntercept->dispatch().clCreateProgramWithSource(
                context,
                count,
                strings,
                lengths,
                errcode_ret );
        }

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p, program number = %04d",
            retVal,
            pIntercept->getProgramNumber() );

        DUMP_PROGRAM_SOURCE( retVal, singleString, hash );
        SAVE_PROGRAM_HASH( retVal, hash );
        DELETE_COMBINED_PROGRAM_STRING( singleString );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_program CL_API_CALL CLIRN(clCreateProgramWithBinary)(
    cl_context context,
    cl_uint num_devices,
    const cl_device_id* device_list,
    const size_t* lengths,
    const unsigned char** binaries,
    cl_int* binary_status,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateProgramWithBinary )
    {
        GET_ENQUEUE_COUNTER();

        uint64_t    hash = 0;
        COMPUTE_BINARY_HASH( num_devices, lengths, binaries, hash );

        CALL_LOGGING_ENTER( "context = %p, num_devices = %u",
            context,
            num_devices );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_program  retVal = NULL;

        if( pIntercept->config().RejectProgramBinaries )
        {
            if( errcode_ret != NULL )
            {
                errcode_ret[0] = CL_INVALID_BINARY;
            }
        }
        else
        {
            retVal = pIntercept->dispatch().clCreateProgramWithBinary(
                context,
                num_devices,
                device_list,
                lengths,
                binaries,
                binary_status,
                errcode_ret );
        }

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        DUMP_INPUT_PROGRAM_BINARIES(
            retVal,
            num_devices,
            device_list,
            lengths,
            binaries,
            hash );
        SAVE_PROGRAM_HASH( retVal, hash );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY cl_program CL_API_CALL CLIRN(clCreateProgramWithBuiltInKernels)(
    cl_context context,
    cl_uint num_devices,
    const cl_device_id* device_list,
    const char* kernel_names,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateProgramWithBuiltInKernels )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "context = %p, num_devices = %u, kernel_names = [ %s ]",
            context,
            num_devices,
            kernel_names );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_program  retVal = NULL;

        if( ( retVal == NULL ) &&
            pIntercept->config().OverrideBuiltinKernels )
        {
            retVal = pIntercept->createProgramWithBuiltinKernels(
                context );
        }

        if( retVal == NULL )
        {
            retVal = pIntercept->dispatch().clCreateProgramWithBuiltInKernels(
                context,
                num_devices,
                device_list,
                kernel_names,
                errcode_ret);
        }

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clRetainProgram)(
    cl_program program )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clRetainProgram )
    {
        GET_ENQUEUE_COUNTER();

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( program ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] program = %p",
            ref_count,
            program );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clRetainProgram(
            program );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RETAIN( program );
        ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( program ) : 0;
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clReleaseProgram)(
    cl_program program )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clReleaseProgram )
    {
        GET_ENQUEUE_COUNTER();

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( program ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] program = %p",
            ref_count,
            program );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clReleaseProgram(
            program );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RELEASE( program );
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", --ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clBuildProgram)(
    cl_program program,
    cl_uint num_devices,
    const cl_device_id* device_list,
    const char* options,
    void (CL_CALLBACK *pfn_notify)(cl_program program, void* user_data),
    void* user_data )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clBuildProgram )
    {
        GET_ENQUEUE_COUNTER();

        const bool isCompile = false;
        const bool isLink = false;
        char*   newOptions = NULL;

        SAVE_PROGRAM_OPTIONS_HASH( program, options );
        PROGRAM_OPTIONS_OVERRIDE_INIT( program, options, newOptions, isCompile );
        DUMP_PROGRAM_OPTIONS( program, newOptions ? newOptions : options, isCompile, isLink );

        CALL_LOGGING_ENTER( "program = %p, pfn_notify = %p", program, pfn_notify );
        BUILD_LOGGING_INIT();
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = CL_INVALID_OPERATION;

        if( newOptions != NULL )
        {
            retVal = pIntercept->dispatch().clBuildProgram(
                program,
                num_devices,
                device_list,
                newOptions,
                pfn_notify,
                user_data );
        }

        if( retVal != CL_SUCCESS )
        {
            retVal = pIntercept->dispatch().clBuildProgram(
                program,
                num_devices,
                device_list,
                options,
                pfn_notify,
                user_data );
        }

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        BUILD_LOGGING( program, num_devices, device_list );
        CALL_LOGGING_EXIT( retVal );

        DUMP_OUTPUT_PROGRAM_BINARIES( program );
        DUMP_KERNEL_ISA_BINARIES( program );
        // Note: this uses the original program options!
        AUTO_CREATE_SPIRV( program, options );
        INCREMENT_PROGRAM_COMPILE_COUNT( program );
        PROGRAM_OPTIONS_CLEANUP( newOptions );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clCompileProgram)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCompileProgram )
    {
        GET_ENQUEUE_COUNTER();

        const bool isCompile = true;
        const bool isLink = false;
        char*   newOptions = NULL;

        SAVE_PROGRAM_OPTIONS_HASH( program, options );
        PROGRAM_OPTIONS_OVERRIDE_INIT( program, options, newOptions, isCompile );
        DUMP_PROGRAM_OPTIONS( program, newOptions ? newOptions : options, isCompile, isLink );

        CALL_LOGGING_ENTER( "program = %p, pfn_notify = %p", program, pfn_notify );
        BUILD_LOGGING_INIT();
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = CL_INVALID_OPERATION;

        if( newOptions != NULL )
        {
            retVal = pIntercept->dispatch().clCompileProgram(
                program,
                num_devices,
                device_list,
                newOptions,
                num_input_headers,
                input_headers,
                header_include_names,
                pfn_notify,
                user_data );
        }

        if( retVal != CL_SUCCESS )
        {
            retVal = pIntercept->dispatch().clCompileProgram(
                program,
                num_devices,
                device_list,
                options,
                num_input_headers,
                input_headers,
                header_include_names,
                pfn_notify,
                user_data );
        }

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        BUILD_LOGGING( program, num_devices, device_list );
        CALL_LOGGING_EXIT( retVal );

        INCREMENT_PROGRAM_COMPILE_COUNT( program );
        PROGRAM_OPTIONS_CLEANUP( newOptions );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY cl_program CL_API_CALL CLIRN(clLinkProgram)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clLinkProgram )
    {
        GET_ENQUEUE_COUNTER();

        const bool isCompile = false;
        const bool isLink = true;
        char*   newOptions = NULL;

        PROGRAM_LINK_OPTIONS_OVERRIDE_INIT( num_devices, device_list, options, newOptions );

        CALL_LOGGING_ENTER( "context = %p, num_input_programs = %u, pfn_notify = %p",
            context,
            num_input_programs,
            pfn_notify );
        CHECK_ERROR_INIT( errcode_ret );
        BUILD_LOGGING_INIT();
        HOST_PERFORMANCE_TIMING_START();

        cl_program  retVal = NULL;

        if( newOptions != NULL )
        {
            retVal = pIntercept->dispatch().clLinkProgram(
                context,
                num_devices,
                device_list,
                newOptions,
                num_input_programs,
                input_programs,
                pfn_notify,
                user_data,
                errcode_ret );
        }

        if( retVal == NULL )
        {
            retVal = pIntercept->dispatch().clLinkProgram(
                context,
                num_devices,
                device_list,
                options,
                num_input_programs,
                input_programs,
                pfn_notify,
                user_data,
                errcode_ret );
        }

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( errcode_ret[0] );
        BUILD_LOGGING( retVal, num_devices, device_list );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        // TODO: How do we compute a hash for the linked program?
        // This is a new program object, so we don't currently have a hash for it.
        SAVE_PROGRAM_NUMBER( retVal );
        SAVE_PROGRAM_OPTIONS_HASH( retVal, options );
        DUMP_PROGRAM_OPTIONS( retVal, newOptions ? newOptions : options, isCompile, isLink );
        DUMP_OUTPUT_PROGRAM_BINARIES( retVal );
        DUMP_KERNEL_ISA_BINARIES( retVal );
        INCREMENT_PROGRAM_COMPILE_COUNT( retVal );
        PROGRAM_OPTIONS_CLEANUP( newOptions );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.2
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clSetProgramReleaseCallback)(
    cl_program program,
    void (CL_CALLBACK *pfn_notify)(cl_program program, void* user_data),
    void* user_data )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clSetProgramReleaseCallback )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "program = %p", program );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clSetProgramReleaseCallback(
            program,
            pfn_notify,
            user_data );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.2
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clSetProgramSpecializationConstant)(
    cl_program program,
    cl_uint spec_id,
    size_t spec_size,
    const void* spec_value )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clSetProgramSpecializationConstant )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "program = %p, spec_id = %u, spec_size = %zu",
            program,
            spec_id,
            spec_size );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clSetProgramSpecializationConstant(
            program,
            spec_id,
            spec_size,
            spec_value );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clUnloadPlatformCompiler)(
    cl_platform_id platform )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clUnloadPlatformCompiler )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER();
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clUnloadPlatformCompiler(
            platform );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clUnloadCompiler)( void )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clUnloadCompiler )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER();
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clUnloadCompiler();

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetProgramInfo)(
    cl_program program,
    cl_program_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetProgramInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "param_name = %s (%08X)",
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetProgramInfo(
            program,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetProgramBuildInfo)(
    cl_program program,
    cl_device_id device,
    cl_program_build_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetProgramBuildInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "param_name = %s (%08X)",
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetProgramBuildInfo(
            program,
            device,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}


///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_kernel CL_API_CALL CLIRN(clCreateKernel)(
    cl_program program,
    const char* kernel_name,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateKernel )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "program = %p, kernel_name = %s",
            program,
            kernel_name );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_kernel   retVal = NULL;

        if( ( retVal == NULL ) &&
            pIntercept->config().OverrideBuiltinKernels )
        {
            retVal = pIntercept->createBuiltinKernel(
                program,
                kernel_name,
                errcode_ret );
        }

        if( retVal == NULL )
        {
            retVal = pIntercept->dispatch().clCreateKernel(
                program,
                kernel_name,
                errcode_ret );
        }

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        if( retVal != NULL )
        {
            pIntercept->addKernelInfo(
                retVal,
                program,
                kernel_name );
            if( pIntercept->config().KernelInfoLogging ||
                pIntercept->config().PreferredWorkGroupSizeMultipleLogging )
            {
                pIntercept->logKernelInfo(
                    &retVal,
                    1 );
            }
        }

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clCreateKernelsInProgram)(
    cl_program program,
    cl_uint num_kernels,
    cl_kernel* kernels,
    cl_uint* num_kernels_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateKernelsInProgram )
    {
        GET_ENQUEUE_COUNTER();

        cl_uint local_num_kernels_ret = 0;
        if( num_kernels_ret == NULL )
        {
            num_kernels_ret = &local_num_kernels_ret;
        }

        CALL_LOGGING_ENTER( "program = %p", program );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clCreateKernelsInProgram(
            program,
            num_kernels,
            kernels,
            num_kernels_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        if( pIntercept->config().LeakChecking &&
            kernels &&
            num_kernels_ret )
        {
            for( cl_uint k = 0; k < num_kernels_ret[0]; k++ )
            {
                ADD_OBJECT_ALLOCATION( kernels[k] );
            }
        }

        std::string retString;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getCreateKernelsInProgramRetString(
                retVal,
                kernels,
                num_kernels_ret,
                retString );
        }
        CALL_LOGGING_EXIT( retVal, "%s", retString.c_str() );

        if( retVal == CL_SUCCESS &&
            kernels &&
            num_kernels_ret )
        {
            pIntercept->addKernelInfo(
                kernels,
                program,
                num_kernels_ret[0] );
            if( pIntercept->config().KernelInfoLogging ||
                pIntercept->config().PreferredWorkGroupSizeMultipleLogging )
            {
                pIntercept->logKernelInfo(
                    kernels,
                    num_kernels_ret[0] );
            }
        }

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clRetainKernel)(
    cl_kernel kernel )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clRetainKernel )
    {
        GET_ENQUEUE_COUNTER();

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( kernel ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] kernel = %p",
            ref_count,
            kernel );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clRetainKernel(
            kernel );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RETAIN( kernel );
        ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( kernel ) : 0;
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clReleaseKernel)(
    cl_kernel kernel )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clReleaseKernel )
    {
        GET_ENQUEUE_COUNTER();

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( kernel ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] kernel = %p",
            ref_count,
            kernel );
        pIntercept->checkRemoveKernelInfo( kernel );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clReleaseKernel(
            kernel );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RELEASE( kernel );
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", --ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clSetKernelArg)(
    cl_kernel kernel,
    cl_uint arg_index,
    size_t arg_size,
    const void* arg_value )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clSetKernelArg )
    {
        GET_ENQUEUE_COUNTER();

        std::string argsString;
        if( pIntercept->config().CallLogging ||
            pIntercept->config().DumpReplayKernelEnqueue != -1 ||
            !pIntercept->config().DumpReplayKernelName.empty() )
        {
            pIntercept->getKernelArgString(
                arg_index,
                arg_size,
                arg_value,
                argsString );
        }
        CALL_LOGGING_ENTER_KERNEL(
            kernel,
            "kernel = %p, %s",
            kernel,
            argsString.c_str() );

        if( pIntercept->config().DumpReplayKernelEnqueue != -1 ||
            !pIntercept->config().DumpReplayKernelName.empty() )
        {
            if( argsString.find( "CL_SAMPLER_NORMALIZED_COORDS" ) != std::string::npos && arg_value != nullptr )
            {
                // This argument is a sampler, dump it
                pIntercept->saveSampler( kernel, arg_index, argsString );
            }
        }
        SET_KERNEL_ARG( kernel, arg_index, arg_size, arg_value );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clSetKernelArg(
            kernel,
            arg_index,
            arg_size,
            arg_value );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetKernelInfo)(
    cl_kernel kernel,
    cl_kernel_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetKernelInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER_KERNEL( kernel, "param_name = %s (%X)",
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetKernelInfo(
            kernel,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetKernelArgInfo)(
    cl_kernel kernel,
    cl_uint arg_indx,
    cl_kernel_arg_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetKernelArgInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER_KERNEL( kernel, "param_name = %s (%X)",
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetKernelArgInfo(
            kernel,
            arg_indx,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetKernelWorkGroupInfo)(
    cl_kernel kernel,
    cl_device_id device,
    cl_kernel_work_group_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetKernelWorkGroupInfo )
    {
        GET_ENQUEUE_COUNTER();

        std::string deviceInfo;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getDeviceInfoString(
                1,
                &device,
                deviceInfo );
        }
        CALL_LOGGING_ENTER_KERNEL( kernel, "device = %s, param_name = %s (%X)",
            deviceInfo.c_str(),
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetKernelWorkGroupInfo(
            kernel,
            device,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clWaitForEvents)(
    cl_uint num_events,
    const cl_event* event_list )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clWaitForEvents )
    {
        GET_ENQUEUE_COUNTER();

        std::string eventList;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getEventListString(
                num_events,
                event_list,
                eventList );
        }
        CALL_LOGGING_ENTER( "event_list = %s",
            eventList.c_str() );
        CHECK_EVENT_LIST( num_events, event_list, NULL );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clWaitForEvents(
            num_events,
            event_list );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );
        DEVICE_PERFORMANCE_TIMING_CHECK();
        FLUSH_CHROME_TRACE_BUFFERING();

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetEventInfo)(
    cl_event event,
    cl_event_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetEventInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "event = %p, param_name = %s (%08X)",
            event,
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetEventInfo(
            event,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CL_API_ENTRY cl_event CL_API_CALL CLIRN(clCreateUserEvent)(
    cl_context context,
    cl_int *errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateUserEvent )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "context = %p",
            context );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_event    retVal = pIntercept->dispatch().clCreateUserEvent(
            context,
            errcode_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clRetainEvent)(
    cl_event event )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clRetainEvent )
    {
        GET_ENQUEUE_COUNTER();

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( event ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] event = %p",
            ref_count,
            event );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clRetainEvent(
            event );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RETAIN( event );
        ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( event ) : 0;
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clReleaseEvent)(
    cl_event event )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clReleaseEvent )
    {
        GET_ENQUEUE_COUNTER();
        REMOVE_EVENT( event );

        cl_uint ref_count =
            pIntercept->config().CallLogging ?
            pIntercept->getRefCount( event ) : 0;
        CALL_LOGGING_ENTER( "[ ref count = %d ] event = %p",
            ref_count,
            event );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clReleaseEvent(
            event );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        ADD_OBJECT_RELEASE( event );
        CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", --ref_count );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clSetUserEventStatus)(
    cl_event event,
    cl_int execution_status )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clSetUserEventStatus )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "event = %p, status = %s (%d)",
            event,
            pIntercept->enumName().name_command_exec_status( execution_status ).c_str(),
            execution_status );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clSetUserEventStatus(
            event,
            execution_status );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clSetEventCallback)(
    cl_event event,
    cl_int command_exec_callback_type,
    void (CL_CALLBACK *pfn_notify)( cl_event, cl_int, void * ),
    void *user_data )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clSetEventCallback )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "event = %p, callback_type = %s (%d)",
            event,
            pIntercept->enumName().name_command_exec_status( command_exec_callback_type ).c_str(),
            command_exec_callback_type );
        EVENT_CALLBACK_OVERRIDE_INIT( pfn_notify, user_data );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clSetEventCallback(
            event,
            command_exec_callback_type,
            pfn_notify,
            user_data );

        HOST_PERFORMANCE_TIMING_END();
        EVENT_CALLBACK_OVERRIDE_CLEANUP( retVal );
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetEventProfilingInfo)(
    cl_event event,
    cl_profiling_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetEventProfilingInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "event = %p, param_name = %s (%08X)",
            event,
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetEventProfilingInfo(
            event,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clFlush)(
    cl_command_queue command_queue )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clFlush )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "queue = %p", command_queue );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clFlush(
            command_queue );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clFinish)(
    cl_command_queue command_queue )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clFinish )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "queue = %p", command_queue );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clFinish(
            command_queue );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );
        DEVICE_PERFORMANCE_TIMING_CHECK();
        FLUSH_CHROME_TRACE_BUFFERING();

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueReadBuffer)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueReadBuffer )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER(
                "queue = %p, buffer = %p, %s, offset = %zu, cb = %zu, ptr = %p%s",
                command_queue,
                buffer,
                blocking_read ? "blocking" : "non-blocking",
                offset,
                cb,
                ptr,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_BLOCKING( blocking_read, cb );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            ITT_ADD_PARAM_AS_METADATA( blocking_read );

            if( pIntercept->config().OverrideReadBuffer )
            {
                retVal = pIntercept->ReadBuffer(
                    command_queue,
                    buffer,
                    blocking_read,
                    offset,
                    cb,
                    ptr,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );
            }
            else
            {
                retVal = pIntercept->dispatch().clEnqueueReadBuffer(
                    command_queue,
                    buffer,
                    blocking_read,
                    offset,
                    cb,
                    ptr,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );
            }

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
            DEVICE_PERFORMANCE_TIMING_CHECK_CONDITIONAL( blocking_read );
            FLUSH_CHROME_TRACE_BUFFERING_CONDITIONAL( blocking_read );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueReadBufferRect)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueReadBufferRect )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(pIntercept, num_events_in_wait_list, event_wait_list);

            if( ( buffer_origin != NULL ) &&
                ( host_origin != NULL ) &&
                ( region != NULL ) )
            {
                CALL_LOGGING_ENTER(
                    "queue = %p, buffer = %p, %s, buffer_origin = < %zu, %zu, %zu >, host_origin = < %zu, %zu, %zu >, region = < %zu, %zu, %zu >, ptr = %p%s",
                    command_queue,
                    buffer,
                    blocking_read ? "blocking" : "non-blocking",
                    buffer_origin[0], buffer_origin[1], buffer_origin[2],
                    host_origin[0], host_origin[1], host_origin[2],
                    region[0], region[1], region[2],
                    ptr,
                    eventWaitListString.c_str() );
            }
            else
            {
                CALL_LOGGING_ENTER(
                    "queue = %p, buffer = %p, %s, ptr = %p%s",
                    command_queue,
                    buffer,
                    blocking_read ? "blocking" : "non-blocking",
                    ptr,
                    eventWaitListString.c_str() );
            }
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_BLOCKING( blocking_read, region ? region[0] * region[1] * region[2] : 0 );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            ITT_ADD_PARAM_AS_METADATA( blocking_read );

            retVal = pIntercept->dispatch().clEnqueueReadBufferRect(
                command_queue,
                buffer,
                blocking_read,
                buffer_origin,
                host_origin,
                region,
                buffer_row_pitch,
                buffer_slice_pitch,
                host_row_pitch,
                host_slice_pitch,
                ptr,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
            DEVICE_PERFORMANCE_TIMING_CHECK_CONDITIONAL( blocking_read );
            FLUSH_CHROME_TRACE_BUFFERING_CONDITIONAL( blocking_read );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueWriteBuffer)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueWriteBuffer )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER(
                "queue = %p, buffer = %p, %s, offset = %zu, cb = %zu, ptr = %p%s",
                command_queue,
                buffer,
                blocking_write ? "blocking" : "non-blocking",
                offset,
                cb,
                ptr,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_BLOCKING( blocking_write, cb );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            ITT_ADD_PARAM_AS_METADATA( blocking_write );

            if( pIntercept->config().OverrideWriteBuffer )
            {
                retVal = pIntercept->WriteBuffer(
                    command_queue,
                    buffer,
                    blocking_write,
                    offset,
                    cb,
                    ptr,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );
            }
            else
            {
                retVal = pIntercept->dispatch().clEnqueueWriteBuffer(
                    command_queue,
                    buffer,
                    blocking_write,
                    offset,
                    cb,
                    ptr,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );
            }

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
            DEVICE_PERFORMANCE_TIMING_CHECK_CONDITIONAL( blocking_write );
            FLUSH_CHROME_TRACE_BUFFERING_CONDITIONAL( blocking_write );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueWriteBufferRect)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueWriteBufferRect )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(pIntercept, num_events_in_wait_list, event_wait_list);

            if( ( buffer_origin != NULL ) &&
                ( host_origin != NULL ) &&
                ( region != NULL ) )
            {
                CALL_LOGGING_ENTER(
                    "queue = %p, buffer = %p, %s, buffer_origin = < %zu, %zu, %zu >, host_origin = < %zu, %zu, %zu >, region = < %zu, %zu, %zu >, ptr = %p%s",
                    command_queue,
                    buffer,
                    blocking_write ? "blocking" : "non-blocking",
                    buffer_origin[0], buffer_origin[1], buffer_origin[2],
                    host_origin[0], host_origin[1], host_origin[2],
                    region[0], region[1], region[2],
                    ptr,
                    eventWaitListString.c_str() );
            }
            else
            {
                CALL_LOGGING_ENTER(
                    "queue = %p, buffer = %p, %s, ptr = %p%s",
                    command_queue,
                    buffer,
                    blocking_write ? "blocking" : "non-blocking",
                    ptr,
                    eventWaitListString.c_str() );
            }
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_BLOCKING( blocking_write, region ? region[0] * region[1] * region[2] : 0 );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            ITT_ADD_PARAM_AS_METADATA( blocking_write );

            retVal = pIntercept->dispatch().clEnqueueWriteBufferRect(
                command_queue,
                buffer,
                blocking_write,
                buffer_origin,
                host_origin,
                region,
                buffer_row_pitch,
                buffer_slice_pitch,
                host_row_pitch,
                host_slice_pitch,
                ptr,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
            DEVICE_PERFORMANCE_TIMING_CHECK_CONDITIONAL( blocking_write );
            FLUSH_CHROME_TRACE_BUFFERING_CONDITIONAL( blocking_write );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueFillBuffer)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueFillBuffer )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p, buffer = %p, pattern_size = %zu, offset = %zu, size = %zu%s",
                command_queue,
                buffer,
                pattern_size,
                offset,
                size,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_BLOCKING( CL_FALSE, size );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueFillBuffer(
                command_queue,
                buffer,
                pattern,
                pattern_size,
                offset,
                size,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueCopyBuffer)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueCopyBuffer )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER("queue = %p, src_buffer = %p, dst_buffer = %p, src_offset = %zu, dst_offset = %zu, cb = %zu%s",
                command_queue,
                src_buffer,
                dst_buffer,
                src_offset,
                dst_offset,
                cb,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_BLOCKING( CL_FALSE, cb );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            if( pIntercept->config().OverrideCopyBuffer )
            {
                retVal = pIntercept->CopyBuffer(
                    command_queue,
                    src_buffer,
                    dst_buffer,
                    src_offset,
                    dst_offset,
                    cb,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );
            }
            else
            {
                retVal = pIntercept->dispatch().clEnqueueCopyBuffer(
                    command_queue,
                    src_buffer,
                    dst_buffer,
                    src_offset,
                    dst_offset,
                    cb,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );
            }

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.1
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueCopyBufferRect)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueCopyBufferRect )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(pIntercept, num_events_in_wait_list, event_wait_list);

            if( ( src_origin != NULL ) &&
                ( dst_origin != NULL ) &&
                ( region != NULL ) )
            {
                CALL_LOGGING_ENTER(
                    "queue = %p, src_buffer = %p, dst_buffer = %p, src_origin = < %zu, %zu, %zu >, dst_origin = < %zu, %zu, %zu >, region = < %zu, %zu, %zu >%s",
                    command_queue,
                    src_buffer,
                    dst_buffer,
                    src_origin[0], src_origin[1], src_origin[2],
                    dst_origin[0], dst_origin[1], dst_origin[2],
                    region[0], region[1], region[2],
                    eventWaitListString.c_str() );
            }
            else
            {
                CALL_LOGGING_ENTER(
                    "queue = %p, src_buffer = %p, dst_buffer = %p%s",
                    command_queue,
                    src_buffer,
                    dst_buffer,
                    eventWaitListString.c_str() );
            }
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_BLOCKING( CL_FALSE, region ? region[0] * region[1] * region[2] : 0 );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueCopyBufferRect(
                command_queue,
                src_buffer,
                dst_buffer,
                src_origin,
                dst_origin,
                region,
                src_row_pitch,
                src_slice_pitch,
                dst_row_pitch,
                dst_slice_pitch,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueReadImage)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueReadImage )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            if( ( origin != NULL ) &&
                ( region != NULL ) )
            {
                CALL_LOGGING_ENTER(
                    "queue = %p, image = %p, %s, origin = < %zu, %zu, %zu >, region = < %zu, %zu, %zu >, ptr = %p%s",
                    command_queue,
                    image,
                    blocking_read ? "blocking" : "non-blocking",
                    origin[0], origin[1], origin[2],
                    region[0], region[1], region[2],
                    ptr,
                    eventWaitListString.c_str() );
            }
            else
            {
                CALL_LOGGING_ENTER(
                    "queue = %p, image = %p, %s, ptr = %p",
                    command_queue,
                    image,
                    blocking_read ? "blocking" : "non-blocking",
                    ptr,
                    eventWaitListString.c_str() );
            }
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_BLOCKING( blocking_read, 0 );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            ITT_ADD_PARAM_AS_METADATA( blocking_read );

            if( pIntercept->config().OverrideReadImage )
            {
                retVal = pIntercept->ReadImage(
                    command_queue,
                    image,
                    blocking_read,
                    origin,
                    region,
                    row_pitch,
                    slice_pitch,
                    ptr,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );
            }
            else
            {
                retVal = pIntercept->dispatch().clEnqueueReadImage(
                    command_queue,
                    image,
                    blocking_read,
                    origin,
                    region,
                    row_pitch,
                    slice_pitch,
                    ptr,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );
            }

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
            DEVICE_PERFORMANCE_TIMING_CHECK_CONDITIONAL( blocking_read );
            FLUSH_CHROME_TRACE_BUFFERING_CONDITIONAL( blocking_read );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueWriteImage)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueWriteImage )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER(
                "queue = %p, image = %p, %s, ptr = %p%s",
                command_queue,
                image,
                blocking_write ? "blocking" : "non-blocking",
                ptr,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_BLOCKING( blocking_write, 0 );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            ITT_ADD_PARAM_AS_METADATA( blocking_write );

            if( pIntercept->config().OverrideWriteImage )
            {
                retVal = pIntercept->WriteImage(
                    command_queue,
                    image,
                    blocking_write,
                    origin,
                    region,
                    input_row_pitch,
                    input_slice_pitch,
                    ptr,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );
            }
            else
            {
                retVal = pIntercept->dispatch().clEnqueueWriteImage(
                    command_queue,
                    image,
                    blocking_write,
                    origin,
                    region,
                    input_row_pitch,
                    input_slice_pitch,
                    ptr,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );
            }

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
            DEVICE_PERFORMANCE_TIMING_CHECK_CONDITIONAL( blocking_write );
            FLUSH_CHROME_TRACE_BUFFERING_CONDITIONAL( blocking_write );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueFillImage)(
    cl_command_queue command_queue,
    cl_mem image,
    const void* fill_color,
    const size_t* origin,
    const size_t* region,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueFillImage )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p, image = %p%s",
                command_queue,
                image,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueFillImage(
                command_queue,
                image,
                fill_color,
                origin,
                region,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END();
            DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueCopyImage)(
    cl_command_queue command_queue,
    cl_mem src_image,
    cl_mem dst_image,
    const size_t* src_origin,
    const size_t* dst_origin,
    const size_t* region,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueCopyImage )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p, src_image = %p, dst_image = %p%s",
                command_queue,
                src_image,
                dst_image,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            if( pIntercept->config().OverrideCopyImage )
            {
                retVal = pIntercept->CopyImage(
                    command_queue,
                    src_image,
                    dst_image,
                    src_origin,
                    dst_origin,
                    region,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );
            }
            else
            {
                retVal = pIntercept->dispatch().clEnqueueCopyImage(
                    command_queue,
                    src_image,
                    dst_image,
                    src_origin,
                    dst_origin,
                    region,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );
            }

            HOST_PERFORMANCE_TIMING_END();
            DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueCopyImageToBuffer)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueCopyImageToBuffer )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p, src_image = %p, dst_buffer = %p%s",
                command_queue,
                src_image,
                dst_buffer,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueCopyImageToBuffer(
                command_queue,
                src_image,
                dst_buffer,
                src_origin,
                region,
                dst_offset,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END();
            DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueCopyBufferToImage)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueCopyBufferToImage )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p, src_buffer = %p, dst_image = %p%s",
                command_queue,
                src_buffer,
                dst_image,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueCopyBufferToImage(
                command_queue,
                src_buffer,
                dst_image,
                src_offset,
                dst_origin,
                region,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END();
            DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY void* CL_API_CALL CLIRN(clEnqueueMapBuffer)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueMapBuffer )
    {
        void*   retVal = NULL;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            cl_uint map_count = 0;
            if( pIntercept->config().CallLogging )
            {
                map_count = 0;
                pIntercept->dispatch().clGetMemObjectInfo(
                    buffer,
                    CL_MEM_MAP_COUNT,
                    sizeof( map_count ),
                    &map_count,
                    NULL );
            }
            CALL_LOGGING_ENTER(
                "[ map count = %d ] queue = %p, buffer = %p, %s, map_flags = %s (%llX), offset = %zu, cb = %zu%s",
                map_count,
                command_queue,
                buffer,
                blocking_map ? "blocking" : "non-blocking",
                pIntercept->enumName().name_map_flags( map_flags ).c_str(),
                map_flags,
                offset,
                cb,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            CHECK_ERROR_INIT( errcode_ret );
            GET_TIMING_TAGS_MAP( blocking_map, map_flags, cb );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            ITT_ADD_PARAM_AS_METADATA( blocking_map );

            retVal = pIntercept->dispatch().clEnqueueMapBuffer(
                command_queue,
                buffer,
                blocking_map,
                map_flags,
                offset,
                cb,
                num_events_in_wait_list,
                event_wait_list,
                event,
                errcode_ret );

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            DUMP_BUFFER_AFTER_MAP( command_queue, buffer, blocking_map, map_flags, retVal, offset, cb );
            CHECK_ERROR( errcode_ret[0] );
            ADD_MAP_POINTER( retVal, map_flags, cb );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            if( pIntercept->config().CallLogging )
            {
                map_count = 0;
                pIntercept->dispatch().clGetMemObjectInfo(
                    buffer,
                    CL_MEM_MAP_COUNT,
                    sizeof( map_count ),
                    &map_count,
                    NULL );
            }
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( errcode_ret[0], event,
                "[ map count = %d ] returned %p",
                map_count,
                retVal );
            DEVICE_PERFORMANCE_TIMING_CHECK_CONDITIONAL( blocking_map );
            FLUSH_CHROME_TRACE_BUFFERING_CONDITIONAL( blocking_map );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY void* CL_API_CALL CLIRN(clEnqueueMapImage)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueMapImage )
    {
        void*   retVal = NULL;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            cl_uint map_count = 0;
            if( pIntercept->config().CallLogging )
            {
                map_count = 0;
                pIntercept->dispatch().clGetMemObjectInfo(
                    image,
                    CL_MEM_MAP_COUNT,
                    sizeof( map_count ),
                    &map_count,
                    NULL );
            }
            if( ( origin != NULL ) &&
                ( region != NULL ) )
            {
                CALL_LOGGING_ENTER(
                    "[ map count = %d ] queue = %p, image = %p, %s, map_flags = %s (%llX), origin = < %zu, %zu, %zu >, region = < %zu, %zu, %zu >%s",
                    map_count,
                    command_queue,
                    image,
                    blocking_map ? "blocking" : "non-blocking",
                    pIntercept->enumName().name_map_flags( map_flags ).c_str(),
                    map_flags,
                    origin[0], origin[1], origin[2],
                    region[0], region[1], region[2],
                    eventWaitListString.c_str() );
            }
            else
            {
                CALL_LOGGING_ENTER(
                    "[ map count = %d ] queue = %p, image = %p, %s, map_flags = %s (%llX)%s",
                    map_count,
                    command_queue,
                    image,
                    blocking_map ? "blocking" : "non-blocking",
                    pIntercept->enumName().name_map_flags( map_flags ).c_str(),
                    map_flags,
                    eventWaitListString.c_str() );
            }
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            CHECK_ERROR_INIT( errcode_ret );
            GET_TIMING_TAGS_MAP( blocking_map, map_flags, 0 );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            ITT_ADD_PARAM_AS_METADATA( blocking_map );

            retVal = pIntercept->dispatch().clEnqueueMapImage(
                command_queue,
                image,
                blocking_map,
                map_flags,
                origin,
                region,
                image_row_pitch,
                image_slice_pitch,
                num_events_in_wait_list,
                event_wait_list,
                event,
                errcode_ret );

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            if( pIntercept->config().CallLogging )
            {
                map_count = 0;
                pIntercept->dispatch().clGetMemObjectInfo(
                    image,
                    CL_MEM_MAP_COUNT,
                    sizeof( map_count ),
                    &map_count,
                    NULL );
            }
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( errcode_ret[0], event,
                "[ map count = %d ] returned %p",
                map_count,
                retVal );
            DEVICE_PERFORMANCE_TIMING_CHECK_CONDITIONAL( blocking_map );
            FLUSH_CHROME_TRACE_BUFFERING_CONDITIONAL( blocking_map );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueUnmapMemObject)(
    cl_command_queue command_queue,
    cl_mem memobj,
    void* mapped_ptr,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueUnmapMemObject )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        DUMP_BUFFER_BEFORE_UNMAP( memobj, command_queue );
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            cl_uint map_count = 0;
            if( pIntercept->config().CallLogging )
            {
                map_count = 0;
                pIntercept->dispatch().clGetMemObjectInfo(
                    memobj,
                    CL_MEM_MAP_COUNT,
                    sizeof( map_count ),
                    &map_count,
                    NULL );
            }
            CALL_LOGGING_ENTER(
                "[ map count = %d ] queue = %p, memobj = %p, mapped_ptr = %p%s",
                map_count,
                command_queue,
                memobj,
                mapped_ptr,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_UNMAP( mapped_ptr );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueUnmapMemObject(
                command_queue,
                memobj,
                mapped_ptr,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            REMOVE_MAP_PTR( mapped_ptr );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            if( pIntercept->config().CallLogging )
            {
                map_count = 0;
                pIntercept->dispatch().clGetMemObjectInfo(
                    memobj,
                    CL_MEM_MAP_COUNT,
                    sizeof( map_count ),
                    &map_count,
                    NULL );
            }
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event, "[ map count = %d ]", map_count );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueMigrateMemObjects)(
    cl_command_queue command_queue,
    cl_uint num_mem_objects,
    const cl_mem* mem_objects,
    cl_mem_migration_flags flags,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueMigrateMemObjects )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p, num_mem_objects = %u, flags = %s (%llX)%s",
                command_queue,
                num_mem_objects,
                pIntercept->enumName().name_mem_migration_flags( flags ).c_str(),
                flags,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueMigrateMemObjects(
                command_queue,
                num_mem_objects,
                mem_objects,
                flags,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END();
            DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueNDRangeKernel)(
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
    CLIntercept*    pIntercept = GetIntercept();

    // This works starting C++11
    // https://stackoverflow.com/questions/14106653/are-function-local-static-mutexes-thread-safe
    static std::mutex localMutex;
    std::unique_lock<std::mutex> lock(localMutex);

    // In case we want to dump a replayble kernel by kernel name, we only do this on the first enqueue
    static bool hasDumpedBufferByName = false;
    static bool hasDumpedValidationBufferByName = false;
    static bool hasDumpedImageByName = false;
    static bool hasDumpedValidationImageByName = false;

    if( pIntercept && pIntercept->dispatch().clEnqueueNDRangeKernel )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        DUMP_BUFFERS_BEFORE_ENQUEUE( kernel, command_queue );
        DUMP_REPLAYABLE_KERNEL(
            kernel,
            command_queue,
            work_dim,
            global_work_offset,
            global_work_size,
            local_work_size );
        DUMP_IMAGES_BEFORE_ENQUEUE( kernel, command_queue );
        CHECK_AUBCAPTURE_START_KERNEL(
            kernel,
            work_dim,
            global_work_size,
            local_work_size,
            command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            if( pIntercept->config().NullLocalWorkSize )
            {
                local_work_size = NULL;
            }
            pIntercept->overrideNullLocalWorkSize(
                work_dim,
                global_work_size,
                local_work_size );

            std::string argsString;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getEnqueueNDRangeKernelArgsString(
                    work_dim,
                    global_work_offset,
                    global_work_size,
                    local_work_size,
                    argsString );
                argsString += getFormattedEventWaitList(
                    pIntercept,
                    num_events_in_wait_list,
                    event_wait_list);
            }
            CALL_LOGGING_ENTER_KERNEL(
                kernel,
                "queue = %p, kernel = %p, %s",
                command_queue,
                kernel,
                argsString.c_str() );

            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_KERNEL(
                command_queue,
                kernel,
                work_dim,
                global_work_offset,
                global_work_size,
                local_work_size );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

//            ITT_ADD_PARAM_AS_METADATA(command_queue);
//            ITT_ADD_PARAM_AS_METADATA(kernel);
            ITT_ADD_PARAM_AS_METADATA(work_dim);
            ITT_ADD_ARRAY_PARAM_AS_METADATA(work_dim, global_work_offset);
            ITT_ADD_ARRAY_PARAM_AS_METADATA(work_dim, global_work_size);
            ITT_ADD_ARRAY_PARAM_AS_METADATA(work_dim, local_work_size);
            ITT_ADD_ARRAY_PARAM_AS_METADATA(num_events_in_wait_list, event_wait_list);

            if( pIntercept->config().Emulate_cl_intel_unified_shared_memory )
            {
                pIntercept->setUSMKernelExecInfo(
                    command_queue,
                    kernel );
            }

            retVal = CL_INVALID_OPERATION;

            if( ( retVal != CL_SUCCESS ) &&
                pIntercept->config().OverrideBuiltinKernels )
            {

                retVal = pIntercept->NDRangeBuiltinKernel(
                    command_queue,
                    kernel,
                    work_dim,
                    global_work_offset,
                    global_work_size,
                    local_work_size,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );
            }

            if( retVal != CL_SUCCESS )
            {
                retVal = pIntercept->dispatch().clEnqueueNDRangeKernel(
                    command_queue,
                    kernel,
                    work_dim,
                    global_work_offset,
                    global_work_size,
                    local_work_size,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );
            }

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        DUMP_BUFFERS_AFTER_ENQUEUE( kernel, command_queue );
        DUMP_IMAGES_AFTER_ENQUEUE( kernel, command_queue );
        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueTask)(
    cl_command_queue command_queue,
    cl_kernel kernel,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueTask )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START_KERNEL( kernel, 0, NULL, NULL, command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER_KERNEL(
                kernel,
                "queue = %p, kernel = %p%s",
                command_queue,
                kernel,
                eventWaitListString.c_str());
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_KERNEL( command_queue, kernel, 0, NULL, NULL, NULL );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueTask(
                command_queue,
                kernel,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueNativeKernel)(
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueNativeKernel )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            CALL_LOGGING_ENTER( "queue = %p",
                command_queue );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueNativeKernel(
                command_queue,
                user_func,
                args,
                cb_args,
                num_mem_objects,
                mem_list,
                args_mem_loc,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END();
            DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueMarker)(
    cl_command_queue command_queue,
    cl_event* event )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueMarker )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            CALL_LOGGING_ENTER( "queue = %p",
                command_queue );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueMarker(
                command_queue,
                event );

            HOST_PERFORMANCE_TIMING_END();
            DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueWaitForEvents)(
    cl_command_queue command_queue,
    cl_uint num_events,
    const cl_event* event_list )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueWaitForEvents )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            std::string eventWaitListString;
            if( pIntercept->config().CallLogging &&
                num_events )
            {
                std::string eventString;
                pIntercept->getEventListString(
                    num_events,
                    event_list,
                    eventString );
                eventWaitListString += ", event_list = ";
                eventWaitListString += eventString;
            }
            CALL_LOGGING_ENTER( "queue = %p%s",
                command_queue,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events, event_list, NULL );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueWaitForEvents(
                command_queue,
                num_events,
                event_list );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );
            DEVICE_PERFORMANCE_TIMING_CHECK();
            FLUSH_CHROME_TRACE_BUFFERING();
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueBarrier)(
    cl_command_queue command_queue )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueBarrier )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            CALL_LOGGING_ENTER( "queue = %p",
                command_queue );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueBarrier(
                command_queue );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        FLUSH_AFTER_ENQUEUE_BARRIER( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueMarkerWithWaitList)(
    cl_command_queue command_queue,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueMarkerWithWaitList )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p%s",
                command_queue,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueMarkerWithWaitList(
                command_queue,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueBarrierWithWaitList)(
    cl_command_queue command_queue,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueBarrierWithWaitList )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p%s",
                command_queue,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueBarrierWithWaitList(
                command_queue,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        FLUSH_AFTER_ENQUEUE_BARRIER( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
CL_API_ENTRY void* CL_API_CALL CLIRN(clGetExtensionFunctionAddress)(
    const char* func_name )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetExtensionFunctionAddress )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "func_name = %s", func_name ? func_name : "(NULL)" );
        HOST_PERFORMANCE_TIMING_START();

        void*   retVal = NULL;
        if( func_name != NULL )
        {
            // First, check to see if this is an extension we know about.
            if( retVal == NULL )
            {
                retVal = pIntercept->getExtensionFunctionAddress(
                    NULL,
                    func_name );
            }

            // If it's not, call into the dispatch table as usual.
            if( retVal == NULL )
            {
                retVal = pIntercept->dispatch().clGetExtensionFunctionAddress(
                    func_name );
            }
        }

        HOST_PERFORMANCE_TIMING_END();
        CALL_LOGGING_EXIT( CL_SUCCESS, "returned %p", retVal );

        return retVal;
    }
    else
    {
        return NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 1.2
CL_API_ENTRY void* CL_API_CALL CLIRN(clGetExtensionFunctionAddressForPlatform)(
    cl_platform_id platform,
    const char* func_name )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetExtensionFunctionAddressForPlatform )
    {
        GET_ENQUEUE_COUNTER();

        std::string platformInfo;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getPlatformInfoString(
                platform,
                platformInfo );
        }
        CALL_LOGGING_ENTER( "platform = %s, func_name = %s",
            platformInfo.c_str(),
            func_name ? func_name : "(NULL)" );
        HOST_PERFORMANCE_TIMING_START();

        void*   retVal = NULL;
        if( func_name != NULL )
        {
            // First, check to see if this is an extension we know about.
            if( retVal == NULL )
            {
                retVal = pIntercept->getExtensionFunctionAddress(
                    platform,
                    func_name );
            }

            // If it's not, call into the dispatch table as usual.
            if( retVal == NULL )
            {
                retVal = pIntercept->dispatch().clGetExtensionFunctionAddressForPlatform(
                    platform,
                    func_name );
            }
        }

        HOST_PERFORMANCE_TIMING_END();
        CALL_LOGGING_EXIT( CL_SUCCESS, "returned %p", retVal );

        return retVal;
    }
    else
    {
        return NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_semaphore
CL_API_ENTRY cl_semaphore_khr CL_API_CALL clCreateSemaphoreWithPropertiesKHR(
    cl_context context,
    const cl_semaphore_properties_khr* properties,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreateSemaphoreWithPropertiesKHR )
        {
            GET_ENQUEUE_COUNTER();

            std::string propsStr;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getSemaphorePropertiesString(
                    properties,
                    propsStr );
            }
            CALL_LOGGING_ENTER( "context = %p, properties = [ %s ]",
                context,
                propsStr.c_str() );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_semaphore_khr    retVal = dispatchX.clCreateSemaphoreWithPropertiesKHR(
                context,
                properties,
                errcode_ret );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            if( retVal != NULL )
            {
                pIntercept->addSemaphoreInfo(
                    retVal,
                    context );
            }

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_semaphore
CL_API_ENTRY cl_int CL_API_CALL clEnqueueWaitSemaphoresKHR(
    cl_command_queue queue,
    cl_uint num_sema_objects,
    const cl_semaphore_khr *sema_objects,
    const cl_semaphore_payload_khr *sema_payload_list,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(queue);
        if( dispatchX.clEnqueueWaitSemaphoresKHR )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                std::string semaphoreString;
                if( pIntercept->config().CallLogging &&
                    num_sema_objects )
                {
                    std::string str;
                    pIntercept->getSemaphoreListString(
                        num_sema_objects,
                        sema_objects,
                        str );
                    semaphoreString += ", sema_objects = ";
                    semaphoreString += str;
                }
                CALL_LOGGING_ENTER( "queue = %p%s",
                    queue,
                    semaphoreString.c_str() );
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueWaitSemaphoresKHR(
                    queue,
                    num_sema_objects,
                    sema_objects,
                    sema_payload_list,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( queue );
            CHECK_AUBCAPTURE_STOP( queue  );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_semaphore
CL_API_ENTRY cl_int CL_API_CALL clEnqueueSignalSemaphoresKHR(
    cl_command_queue queue,
    cl_uint num_sema_objects,
    const cl_semaphore_khr *sema_objects,
    const cl_semaphore_payload_khr *sema_payload_list,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(queue);
        if( dispatchX.clEnqueueSignalSemaphoresKHR )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                std::string semaphoreString;
                if( pIntercept->config().CallLogging &&
                    num_sema_objects )
                {
                    std::string str;
                    pIntercept->getSemaphoreListString(
                        num_sema_objects,
                        sema_objects,
                        str );
                    semaphoreString += ", sema_objects = ";
                    semaphoreString += str;
                }
                CALL_LOGGING_ENTER( "queue = %p%s",
                    queue,
                    semaphoreString.c_str() );
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueSignalSemaphoresKHR(
                    queue,
                    num_sema_objects,
                    sema_objects,
                    sema_payload_list,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( queue );
            CHECK_AUBCAPTURE_STOP( queue  );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_semaphore
CL_API_ENTRY cl_int CL_API_CALL clGetSemaphoreInfoKHR(
    const cl_semaphore_khr semaphore,
    cl_semaphore_info_khr param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(semaphore);
        if( dispatchX.clGetSemaphoreInfoKHR )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "semaphore = %p, param_name = %s (%08X)",
                semaphore,
                pIntercept->enumName().name( param_name ).c_str(),
                param_name );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetSemaphoreInfoKHR(
                semaphore,
                param_name,
                param_value_size,
                param_value,
                param_value_size_ret );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_semaphore
CL_API_ENTRY cl_int CL_API_CALL clRetainSemaphoreKHR(
    cl_semaphore_khr semaphore )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(semaphore);
        if( dispatchX.clRetainSemaphoreKHR )
        {
            GET_ENQUEUE_COUNTER();

            cl_uint ref_count =
                pIntercept->config().CallLogging ?
                pIntercept->getRefCount( semaphore ) : 0;
            CALL_LOGGING_ENTER( "[ ref count = %d ] semaphore = %p",
                ref_count,
                semaphore );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clRetainSemaphoreKHR(
                semaphore );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            ADD_OBJECT_RETAIN( semaphore );
            ref_count =
                pIntercept->config().CallLogging ?
                pIntercept->getRefCount( semaphore ) : 0;
            CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", ref_count );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_semaphore
CL_API_ENTRY cl_int CL_API_CALL clReleaseSemaphoreKHR(
    cl_semaphore_khr semaphore)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(semaphore);
        if( dispatchX.clReleaseSemaphoreKHR )
        {
            GET_ENQUEUE_COUNTER();

            cl_uint ref_count =
                pIntercept->config().CallLogging ?
                pIntercept->getRefCount( semaphore ) : 0;
            CALL_LOGGING_ENTER( "[ ref count = %d ] semaphore = %p",
                ref_count,
                semaphore );
            pIntercept->checkRemoveSemaphoreInfo( semaphore );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clReleaseSemaphoreKHR(
                semaphore );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            ADD_OBJECT_RELEASE( semaphore );
            CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", --ref_count );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_external_semaphore
CL_API_ENTRY cl_int CL_API_CALL clGetSemaphoreHandleForTypeKHR(
    cl_semaphore_khr semaphore,
    cl_device_id device,
    cl_external_semaphore_handle_type_khr handle_type,
    size_t handle_size,
    void* handle_ptr,
    size_t* handle_size_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(semaphore);
        if( dispatchX.clGetSemaphoreHandleForTypeKHR )
        {
            GET_ENQUEUE_COUNTER();

            std::string deviceInfo;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getDeviceInfoString(
                    1,
                    &device,
                    deviceInfo );
            }
            CALL_LOGGING_ENTER( "semaphore = %p, device = %s, handle_type = %s (%X)",
                semaphore,
                deviceInfo.c_str(),
                pIntercept->enumName().name( handle_type ).c_str(),
                handle_type );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetSemaphoreHandleForTypeKHR(
                semaphore,
                device,
                handle_type,
                handle_size,
                handle_ptr,
                handle_size_ret);

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_gl_sharing
CL_API_ENTRY cl_mem CL_API_CALL CLIRN(clCreateFromGLBuffer)(
    cl_context context,
    cl_mem_flags flags,
    cl_GLuint bufobj,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    // The cl_khr_gl_sharing APIs and especially clGetGLContextInfoKHR are a
    // special-case: they are extension functions but do not necessarily pass
    // a dispatchable object as their first argument and are implemented in
    // the ICD loader and called into via the ICD dispatch table.  This means
    // that we can install it into our core API dispatch table as well and
    // don't need to look it up per-platform.

    if( pIntercept && pIntercept->dispatch().clCreateFromGLBuffer )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER(
            "context = %p, "
            "flags = %s (%llX)",
            context,
            pIntercept->enumName().name_mem_flags( flags ).c_str(),
            flags );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_mem  retVal = pIntercept->dispatch().clCreateFromGLBuffer(
            context,
            flags,
            bufobj,
            errcode_ret);

        HOST_PERFORMANCE_TIMING_END();
        ADD_BUFFER( retVal );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_gl_sharing - OpenCL 1.2
CL_API_ENTRY cl_mem CL_API_CALL CLIRN(clCreateFromGLTexture)(
    cl_context context,
    cl_mem_flags flags,
    cl_GLenum target,
    cl_GLint miplevel,
    cl_GLuint texture,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    // The cl_khr_gl_sharing APIs and especially clGetGLContextInfoKHR are a
    // special-case: they are extension functions but do not necessarily pass
    // a dispatchable object as their first argument and are implemented in
    // the ICD loader and called into via the ICD dispatch table.  This means
    // that we can install it into our core API dispatch table as well and
    // don't need to look it up per-platform.

    if( pIntercept && pIntercept->dispatch().clCreateFromGLTexture )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER(
            "context = %p, "
            "flags = %s (%llX), "
            "texture_target = %s (%d), "
            "miplevel = %d, "
            "texture = %d",
            context,
            pIntercept->enumName().name_mem_flags( flags ).c_str(),
            flags,
            pIntercept->enumName().name_gl( target ).c_str(),
            target,
            miplevel,
            texture );

        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_mem  retVal = pIntercept->dispatch().clCreateFromGLTexture(
            context,
            flags,
            target,
            miplevel,
            texture,
            errcode_ret);

        HOST_PERFORMANCE_TIMING_END();
        ADD_IMAGE( retVal );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );

        pIntercept->logCL_GLTextureDetails( retVal, target, miplevel, texture );

        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_gl_sharing
CL_API_ENTRY cl_mem CL_API_CALL CLIRN(clCreateFromGLTexture2D)(
    cl_context context,
    cl_mem_flags flags,
    cl_GLenum target,
    cl_GLint miplevel,
    cl_GLuint texture,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    // The cl_khr_gl_sharing APIs and especially clGetGLContextInfoKHR are a
    // special-case: they are extension functions but do not necessarily pass
    // a dispatchable object as their first argument and are implemented in
    // the ICD loader and called into via the ICD dispatch table.  This means
    // that we can install it into our core API dispatch table as well and
    // don't need to look it up per-platform.

    if( pIntercept && pIntercept->dispatch().clCreateFromGLTexture2D )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER(
            "context = %p, "
            "flags = %s (%llX), "
            "texture_target = %s (%d), "
            "miplevel = %d, "
            "texture = %d",
            context,
            pIntercept->enumName().name_mem_flags( flags ).c_str(),
            flags,
            pIntercept->enumName().name_gl( target ).c_str(),
            target,
            miplevel,
            texture );

        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_mem  retVal = pIntercept->dispatch().clCreateFromGLTexture2D(
            context,
            flags,
            target,
            miplevel,
            texture,
            errcode_ret);

        HOST_PERFORMANCE_TIMING_END();
        ADD_IMAGE( retVal );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );

        pIntercept->logCL_GLTextureDetails( retVal, target, miplevel, texture );

        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_gl_sharing
CL_API_ENTRY cl_mem CL_API_CALL CLIRN(clCreateFromGLTexture3D)(
    cl_context context,
    cl_mem_flags flags,
    cl_GLenum target,
    cl_GLint miplevel,
    cl_GLuint texture,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    // The cl_khr_gl_sharing APIs and especially clGetGLContextInfoKHR are a
    // special-case: they are extension functions but do not necessarily pass
    // a dispatchable object as their first argument and are implemented in
    // the ICD loader and called into via the ICD dispatch table.  This means
    // that we can install it into our core API dispatch table as well and
    // don't need to look it up per-platform.

    if( pIntercept && pIntercept->dispatch().clCreateFromGLTexture3D )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER(
            "context = %p, "
            "flags = %s (%llX), "
            "texture_target = %s (%d), "
            "miplevel = %d, "
            "texture = %d",
            context,
            pIntercept->enumName().name_mem_flags( flags ).c_str(),
            flags,
            pIntercept->enumName().name_gl( target ).c_str(),
            target,
            miplevel,
            texture );

        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_mem  retVal = pIntercept->dispatch().clCreateFromGLTexture3D(
            context,
            flags,
            target,
            miplevel,
            texture,
            errcode_ret);

        HOST_PERFORMANCE_TIMING_END();
        ADD_IMAGE( retVal );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );

        pIntercept->logCL_GLTextureDetails( retVal, target, miplevel, texture );

        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_gl_sharing
CL_API_ENTRY cl_mem CL_API_CALL CLIRN(clCreateFromGLRenderbuffer)(
    cl_context context,
    cl_mem_flags flags,
    cl_GLuint renderbuffer,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    // The cl_khr_gl_sharing APIs and especially clGetGLContextInfoKHR are a
    // special-case: they are extension functions but do not necessarily pass
    // a dispatchable object as their first argument and are implemented in
    // the ICD loader and called into via the ICD dispatch table.  This means
    // that we can install it into our core API dispatch table as well and
    // don't need to look it up per-platform.

    if( pIntercept && pIntercept->dispatch().clCreateFromGLRenderbuffer )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER(
            "context = %p, "
            "flags = %s (%llX)",
            context,
            pIntercept->enumName().name_mem_flags( flags ).c_str(),
            flags );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_mem  retVal = pIntercept->dispatch().clCreateFromGLRenderbuffer(
            context,
            flags,
            renderbuffer,
            errcode_ret);

        HOST_PERFORMANCE_TIMING_END();
        ADD_IMAGE( retVal );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_gl_sharing
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetGLObjectInfo)(
    cl_mem memobj,
    cl_gl_object_type* gl_object_type,
    cl_GLuint* gl_object_name)
{
    CLIntercept*    pIntercept = GetIntercept();

    // The cl_khr_gl_sharing APIs and especially clGetGLContextInfoKHR are a
    // special-case: they are extension functions but do not necessarily pass
    // a dispatchable object as their first argument and are implemented in
    // the ICD loader and called into via the ICD dispatch table.  This means
    // that we can install it into our core API dispatch table as well and
    // don't need to look it up per-platform.

    if( pIntercept && pIntercept->dispatch().clGetGLObjectInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER();
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetGLObjectInfo(
            memobj,
            gl_object_type,
            gl_object_name);

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_gl_sharing
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetGLTextureInfo)(
    cl_mem memobj,
    cl_gl_texture_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    // The cl_khr_gl_sharing APIs and especially clGetGLContextInfoKHR are a
    // special-case: they are extension functions but do not necessarily pass
    // a dispatchable object as their first argument and are implemented in
    // the ICD loader and called into via the ICD dispatch table.  This means
    // that we can install it into our core API dispatch table as well and
    // don't need to look it up per-platform.

    if( pIntercept && pIntercept->dispatch().clGetGLTextureInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER();
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetGLTextureInfo(
            memobj,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret);

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_gl_sharing
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueAcquireGLObjects)(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    // The cl_khr_gl_sharing APIs and especially clGetGLContextInfoKHR are a
    // special-case: they are extension functions but do not necessarily pass
    // a dispatchable object as their first argument and are implemented in
    // the ICD loader and called into via the ICD dispatch table.  This means
    // that we can install it into our core API dispatch table as well and
    // don't need to look it up per-platform.

    if( pIntercept && pIntercept->dispatch().clEnqueueAcquireGLObjects )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p, num_objects = %u%s",
                command_queue,
                num_objects,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueAcquireGLObjects(
                command_queue,
                num_objects,
                mem_objects,
                num_events_in_wait_list,
                event_wait_list,
                event);

            HOST_PERFORMANCE_TIMING_END();
            DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_gl_sharing
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueReleaseGLObjects)(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    // The cl_khr_gl_sharing APIs and especially clGetGLContextInfoKHR are a
    // special-case: they are extension functions but do not necessarily pass
    // a dispatchable object as their first argument and are implemented in
    // the ICD loader and called into via the ICD dispatch table.  This means
    // that we can install it into our core API dispatch table as well and
    // don't need to look it up per-platform.

    if( pIntercept && pIntercept->dispatch().clEnqueueReleaseGLObjects )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p, num_objects = %u%s",
                command_queue,
                num_objects,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueReleaseGLObjects(
                command_queue,
                num_objects,
                mem_objects,
                num_events_in_wait_list,
                event_wait_list,
                event);

            HOST_PERFORMANCE_TIMING_END();
            DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT( retVal, event );
            DEVICE_PERFORMANCE_TIMING_CHECK();
            FLUSH_CHROME_TRACE_BUFFERING();
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CL_API_ENTRY void* CL_API_CALL CLIRN(clSVMAlloc) (
    cl_context context,
    cl_svm_mem_flags flags,
    size_t size,
    cl_uint alignment)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clSVMAlloc )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "context = %p, flags = %s (%llX), size = %zu, alignment = %u",
            context,
            pIntercept->enumName().name_svm_mem_flags( flags ).c_str(),
            flags,
            size,
            alignment );
        HOST_PERFORMANCE_TIMING_START();

        void*   retVal = pIntercept->dispatch().clSVMAlloc(
            context,
            flags,
            size,
            alignment );

        HOST_PERFORMANCE_TIMING_END();
        ADD_SVM_ALLOCATION( retVal, size );
        // There is no error code returned from clSVMAlloc(), so strictly
        // speaking we have no error to "check" here.  Still, we'll invent
        // one if clSVMAlloc() returned NULL, so something will get logged
        // if ErrorLogging is enabled.
        cl_int  errorCode = ( retVal != NULL ) ? CL_SUCCESS : CL_INVALID_OPERATION;
        CHECK_ERROR( errorCode );
        CALL_LOGGING_EXIT( errorCode, "returned %p", retVal );

        return retVal;
    }

    cl_int* errcode_ret = NULL;
    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CL_API_ENTRY void CL_API_CALL CLIRN(clSVMFree) (
    cl_context context,
    void* svm_pointer)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clSVMFree )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "context = %p, svm_pointer = %p",
            context,
            svm_pointer );
        HOST_PERFORMANCE_TIMING_START();

        pIntercept->dispatch().clSVMFree(
            context,
            svm_pointer );

        HOST_PERFORMANCE_TIMING_END();
        REMOVE_SVM_ALLOCATION( svm_pointer );
        CALL_LOGGING_EXIT( CL_SUCCESS );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueSVMFree) (
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueSVMFree )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p, num_svm_pointers = %u%s",
                command_queue,
                num_svm_pointers,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueSVMFree(
                command_queue,
                num_svm_pointers,
                svm_pointers,
                pfn_free_func,
                user_data,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END();
            DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueSVMMemcpy) (
    cl_command_queue command_queue,
    cl_bool blocking_copy,
    void* dst_ptr,
    const void* src_ptr,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueSVMMemcpy )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p, %s, dst_ptr = %p, src_ptr = %p, size = %zu%s",
                command_queue,
                blocking_copy ? "blocking" : "non-blocking",
                dst_ptr,
                src_ptr,
                size,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_BLOCKING( blocking_copy, size );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueSVMMemcpy(
                command_queue,
                blocking_copy,
                dst_ptr,
                src_ptr,
                size,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
            DEVICE_PERFORMANCE_TIMING_CHECK_CONDITIONAL( blocking_copy );
            FLUSH_CHROME_TRACE_BUFFERING_CONDITIONAL( blocking_copy );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueSVMMemFill) (
    cl_command_queue command_queue,
    void* svm_ptr,
    const void* pattern,
    size_t pattern_size,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueSVMMemFill )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p, svm_ptr = %p, pattern_size = %zu, size = %zu%s",
                command_queue,
                svm_ptr,
                pattern_size,
                size,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_BLOCKING( CL_FALSE, size );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueSVMMemFill(
                command_queue,
                svm_ptr,
                pattern,
                pattern_size,
                size,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueSVMMap) (
    cl_command_queue command_queue,
    cl_bool blocking_map,
    cl_map_flags map_flags,
    void* svm_ptr,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueSVMMap )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p, %s, map_flags = %s (%llX), svm_ptr = %p, size = %zu%s",
                command_queue,
                blocking_map ? "blocking" : "non-blocking",
                pIntercept->enumName().name_map_flags( map_flags ).c_str(),
                map_flags,
                svm_ptr,
                size,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            GET_TIMING_TAGS_MAP( blocking_map, map_flags, size );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueSVMMap(
                command_queue,
                blocking_map,
                map_flags,
                svm_ptr,
                size,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END_WITH_TAG();
            DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_MAP_POINTER( svm_ptr, map_flags, size );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
            DEVICE_PERFORMANCE_TIMING_CHECK_CONDITIONAL( blocking_map );
            FLUSH_CHROME_TRACE_BUFFERING_CONDITIONAL( blocking_map );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clEnqueueSVMUnmap) (
    cl_command_queue command_queue,
    void* svm_ptr,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueSVMUnmap )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p, svm_ptr = %p%s",
                command_queue,
                svm_ptr,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueSVMUnmap(
                command_queue,
                svm_ptr,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END();
            DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
            CHECK_ERROR( retVal );
            REMOVE_MAP_PTR( svm_ptr );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
        CHECK_AUBCAPTURE_STOP( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clSetKernelArgSVMPointer) (
    cl_kernel kernel,
    cl_uint arg_index,
    const void* arg_value)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clSetKernelArgSVMPointer )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER_KERNEL(
            kernel,
            "kernel = %p, index = %u, value = %p",
            kernel,
            arg_index,
            arg_value );
        SET_KERNEL_ARG_SVM_POINTER( kernel, arg_index, arg_value );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clSetKernelArgSVMPointer(
            kernel,
            arg_index,
            arg_value );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clSetKernelExecInfo) (
    cl_kernel kernel,
    cl_kernel_exec_info param_name,
    size_t param_value_size,
    const void* param_value)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clSetKernelExecInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER_KERNEL( kernel, "param_name = %s (%08X)",
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = CL_INVALID_OPERATION;

        if( pIntercept->config().Emulate_cl_intel_unified_shared_memory )
        {
            retVal = pIntercept->trackUSMKernelExecInfo(
                kernel,
                param_name,
                param_value_size,
                param_value );
        }

        if( retVal != CL_SUCCESS )
        {
            retVal = pIntercept->dispatch().clSetKernelExecInfo(
                kernel,
                param_name,
                param_value_size,
                param_value );
        }

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CL_API_ENTRY cl_mem CL_API_CALL CLIRN(clCreatePipe) (
    cl_context context,
    cl_mem_flags flags,
    cl_uint pipe_packet_size,
    cl_uint pipe_max_packets,
    const cl_pipe_properties* properties,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreatePipe )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "context = %p, flags = %s (%llX), pipe_packet_size = %u, pipe_max_packets = %u",
            context,
            pIntercept->enumName().name_mem_flags( flags ).c_str(),
            flags,
            pipe_packet_size,
            pipe_max_packets );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_mem  retVal = pIntercept->dispatch().clCreatePipe(
            context,
            flags,
            pipe_packet_size,
            pipe_max_packets,
            properties,
            errcode_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0] );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetPipeInfo) (
    cl_mem pipe,
    cl_pipe_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetPipeInfo )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER( "mem = %p, param_name = %s (%08X)",
            pipe,
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetPipeInfo(
            pipe,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CL_API_ENTRY cl_command_queue CL_API_CALL CLIRN(clCreateCommandQueueWithProperties) (
    cl_context context,
    cl_device_id device,
    const cl_queue_properties* properties,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateCommandQueueWithProperties )
    {
        GET_ENQUEUE_COUNTER();

        cl_queue_properties*    newProperties = NULL;
        cl_command_queue    retVal = NULL;

        std::string deviceInfo;
        std::string propsStr;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getDeviceInfoString(
                1,
                &device,
                deviceInfo );
            pIntercept->getCommandQueuePropertiesString(
                properties,
                propsStr );
        }
        CALL_LOGGING_ENTER( "context = %p, device = %s, properties = [ %s ]",
            context,
            deviceInfo.c_str(),
            propsStr.c_str() );
        DUMMY_COMMAND_QUEUE( context, device );
        CREATE_COMMAND_QUEUE_OVERRIDE_INIT( device, properties, newProperties );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

#if defined(USE_MDAPI)
        if( pIntercept->config().DevicePerfCounterEventBasedSampling )
        {
            if( ( retVal == NULL ) && newProperties )
            {
                retVal = pIntercept->createMDAPICommandQueue(
                    context,
                    device,
                    newProperties,
                    errcode_ret );
            }
            if( retVal == NULL )
            {
                retVal = pIntercept->createMDAPICommandQueue(
                    context,
                    device,
                    properties,
                    errcode_ret );
            }
        }
#endif

        if( ( retVal == NULL ) && newProperties )
        {
            retVal = pIntercept->dispatch().clCreateCommandQueueWithProperties(
                context,
                device,
                newProperties,
                errcode_ret );
        }
        if( retVal == NULL )
        {
            retVal = pIntercept->dispatch().clCreateCommandQueueWithProperties(
                context,
                device,
                properties,
                errcode_ret );
        }

        HOST_PERFORMANCE_TIMING_END();
        COMMAND_QUEUE_PROPERTIES_CLEANUP( newProperties );
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );
        ADD_QUEUE( context, retVal );
        QUEUE_INFO_LOGGING( device, retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_create_command_queue
// This function should stay in sync with clCreateCommandQueueWithProperties, above.
CL_API_ENTRY cl_command_queue CL_API_CALL clCreateCommandQueueWithPropertiesKHR(
    cl_context context,
    cl_device_id device,
    const cl_queue_properties_khr* properties,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(device);
        if( dispatchX.clCreateCommandQueueWithPropertiesKHR )
        {
            GET_ENQUEUE_COUNTER();

            cl_queue_properties*    newProperties = NULL;
            cl_command_queue    retVal = NULL;

            std::string deviceInfo;
            std::string propsStr;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getDeviceInfoString(
                    1,
                    &device,
                    deviceInfo );
                pIntercept->getCommandQueuePropertiesString(
                    properties,
                    propsStr );
            }
            CALL_LOGGING_ENTER( "context = %p, device = %s, properties = [ %s ]",
                context,
                deviceInfo.c_str(),
                propsStr.c_str() );
            DUMMY_COMMAND_QUEUE( context, device );
            CREATE_COMMAND_QUEUE_OVERRIDE_INIT( device, properties, newProperties );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

#if defined(USE_MDAPI)
            if( pIntercept->config().DevicePerfCounterEventBasedSampling )
            {
                if( ( retVal == NULL ) && newProperties )
                {
                    retVal = pIntercept->createMDAPICommandQueue(
                        context,
                        device,
                        newProperties,
                        errcode_ret );
                }
                if( retVal == NULL )
                {
                    retVal = pIntercept->createMDAPICommandQueue(
                        context,
                        device,
                        properties,
                        errcode_ret );
                }
            }
#endif

            if( ( retVal == NULL ) && newProperties )
            {
                retVal = dispatchX.clCreateCommandQueueWithPropertiesKHR(
                    context,
                    device,
                    newProperties,
                    errcode_ret );
            }
            if( retVal == NULL )
            {
                retVal = dispatchX.clCreateCommandQueueWithPropertiesKHR(
                    context,
                    device,
                    properties,
                    errcode_ret );
            }

            HOST_PERFORMANCE_TIMING_END();
            COMMAND_QUEUE_PROPERTIES_CLEANUP( newProperties );
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );
            ADD_QUEUE( context, retVal );
            QUEUE_INFO_LOGGING( device, retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.0
CL_API_ENTRY cl_sampler CL_API_CALL CLIRN(clCreateSamplerWithProperties) (
    cl_context context,
    const cl_sampler_properties* sampler_properties,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateSamplerWithProperties )
    {
        GET_ENQUEUE_COUNTER();

        std::string propsStr;
        if( pIntercept->config().CallLogging ||
            pIntercept->config().DumpReplayKernelEnqueue != -1 ||
            !pIntercept->config().DumpReplayKernelName.empty() )
        {
            pIntercept->getSamplerPropertiesString(
                sampler_properties,
                propsStr );
        }
        CALL_LOGGING_ENTER( "context = %p, properties = [ %s ]",
            context,
            propsStr.c_str() );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_sampler  retVal = pIntercept->dispatch().clCreateSamplerWithProperties(
            context,
            sampler_properties,
            errcode_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );
        ADD_SAMPLER( retVal, propsStr );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.1
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clSetDefaultDeviceCommandQueue) (
    cl_context context,
    cl_device_id device,
    cl_command_queue command_queue )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clSetDefaultDeviceCommandQueue )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER();
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clSetDefaultDeviceCommandQueue(
            context,
            device,
            command_queue );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.1
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetDeviceAndHostTimer) (
    cl_device_id device,
    cl_ulong* device_timestamp,
    cl_ulong* host_timestamp )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetDeviceAndHostTimer )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER();
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetDeviceAndHostTimer(
            device,
            device_timestamp,
            host_timestamp );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.1
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetHostTimer) (
    cl_device_id device,
    cl_ulong* host_timestamp )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetHostTimer )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER();
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetHostTimer(
            device,
            host_timestamp );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.1
CL_API_ENTRY cl_program CL_API_CALL CLIRN(clCreateProgramWithIL) (
    cl_context context,
    const void* il,
    size_t length,
    cl_int *errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCreateProgramWithIL )
    {
        GET_ENQUEUE_COUNTER();

        char*       injectedSPIRV = NULL;
        uint64_t    hash = 0;

        COMPUTE_SPIRV_HASH( length, il, hash );
        INJECT_PROGRAM_SPIRV( length, il, injectedSPIRV, hash );

        CALL_LOGGING_ENTER( "context = %p, length = %zu",
            context,
            length );
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_program  retVal = pIntercept->dispatch().clCreateProgramWithIL(
            context,
            il,
            length,
            errcode_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        DUMP_PROGRAM_SPIRV( retVal, length, il, hash );
        SAVE_PROGRAM_HASH( retVal, hash );
        DELETE_INJECTED_SPIRV( injectedSPIRV );

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_il_program
// This function should stay in sync with clCreateProgramWithIL, above.
CL_API_ENTRY cl_program CL_API_CALL clCreateProgramWithILKHR(
    cl_context context,
    const void* il,
    size_t length,
    cl_int *errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreateProgramWithILKHR )
        {
            GET_ENQUEUE_COUNTER();

            char*       injectedSPIRV = NULL;
            uint64_t    hash = 0;

            COMPUTE_SPIRV_HASH( length, il, hash );
            INJECT_PROGRAM_SPIRV( length, il, injectedSPIRV, hash );

            CALL_LOGGING_ENTER( "context = %p, length = %zu",
                context,
                length );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_program  retVal = dispatchX.clCreateProgramWithILKHR(
                context,
                il,
                length,
                errcode_ret );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            DUMP_PROGRAM_SPIRV( retVal, length, il, hash );
            SAVE_PROGRAM_HASH( retVal, hash );
            DELETE_INJECTED_SPIRV( injectedSPIRV );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.1
CL_API_ENTRY cl_kernel CL_API_CALL CLIRN(clCloneKernel) (
    cl_kernel source_kernel,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clCloneKernel )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER();
        CHECK_ERROR_INIT( errcode_ret );
        HOST_PERFORMANCE_TIMING_START();

        cl_kernel   retVal = pIntercept->dispatch().clCloneKernel(
            source_kernel,
            errcode_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( errcode_ret[0] );
        ADD_OBJECT_ALLOCATION( retVal );
        CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

        if( retVal != NULL )
        {
            pIntercept->addKernelInfo( retVal, source_kernel );
        }

        return retVal;
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.1
CL_API_ENTRY cl_int CL_API_CALL CLIRN(clGetKernelSubGroupInfo) (
    cl_kernel kernel,
    cl_device_id device,
    cl_kernel_sub_group_info param_name,
    size_t input_value_size,
    const void* input_value,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clGetKernelSubGroupInfo )
    {
        GET_ENQUEUE_COUNTER();

        std::string deviceInfo;
        if( pIntercept->config().CallLogging )
        {
            pIntercept->getDeviceInfoString(
                1,
                &device,
                deviceInfo );
        }
        CALL_LOGGING_ENTER_KERNEL( kernel, "device = %s, param_name = %s (%08X)",
            deviceInfo.c_str(),
            pIntercept->enumName().name( param_name ).c_str(),
            param_name );
        HOST_PERFORMANCE_TIMING_START();

        cl_int retVal = pIntercept->dispatch().clGetKernelSubGroupInfo(
            kernel,
            device,
            param_name,
            input_value_size,
            input_value,
            param_value_size,
            param_value,
            param_value_size_ret );

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_subgroups
// This function should stay in sync with clGetKernelSubGroupInfo, above.
CL_API_ENTRY cl_int CL_API_CALL clGetKernelSubGroupInfoKHR(
    cl_kernel kernel,
    cl_device_id device,
    cl_kernel_sub_group_info param_name,
    size_t input_value_size,
    const void* input_value,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(kernel);
        if( dispatchX.clGetKernelSubGroupInfoKHR )
        {
            GET_ENQUEUE_COUNTER();

            std::string deviceInfo;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getDeviceInfoString(
                    1,
                    &device,
                    deviceInfo );
            }
            CALL_LOGGING_ENTER_KERNEL( kernel, "device = %s, param_name = %s (%08X)",
                deviceInfo.c_str(),
                pIntercept->enumName().name( param_name ).c_str(),
                param_name );
            HOST_PERFORMANCE_TIMING_START();

            cl_int retVal = dispatchX.clGetKernelSubGroupInfoKHR(
                kernel,
                device,
                param_name,
                input_value_size,
                input_value,
                param_value_size,
                param_value,
                param_value_size_ret );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// OpenCL 2.1
CL_API_ENTRY cl_int CL_API_CALL clEnqueueSVMMigrateMem(
    cl_command_queue command_queue,
    cl_uint num_svm_pointers,
    const void** svm_pointers,
    const size_t* sizes,
    cl_mem_migration_flags flags,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept && pIntercept->dispatch().clEnqueueSVMMigrateMem )
    {
        cl_int  retVal = CL_SUCCESS;

        INCREMENT_ENQUEUE_COUNTER();
        CHECK_AUBCAPTURE_START( command_queue );

        if( pIntercept->config().NullEnqueue == false )
        {
            const std::string eventWaitListString = getFormattedEventWaitList(
                pIntercept,
                num_events_in_wait_list,
                event_wait_list);

            CALL_LOGGING_ENTER( "queue = %p, num_svm_pointers = %u, flags = %s (%llX)%s",
                command_queue,
                num_svm_pointers,
                pIntercept->enumName().name_mem_migration_flags( flags ).c_str(),
                flags,
                eventWaitListString.c_str() );
            CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
            DEVICE_PERFORMANCE_TIMING_START( event );
            HOST_PERFORMANCE_TIMING_START();

            retVal = pIntercept->dispatch().clEnqueueSVMMigrateMem(
                command_queue,
                num_svm_pointers,
                svm_pointers,
                sizes,
                flags,
                num_events_in_wait_list,
                event_wait_list,
                event );

            HOST_PERFORMANCE_TIMING_END();
            DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
            CHECK_ERROR( retVal );
            ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
            CALL_LOGGING_EXIT_EVENT( retVal, event );
            ADD_EVENT( event ? event[0] : NULL );
        }

        FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_external_memory
CL_API_ENTRY cl_int CL_API_CALL clEnqueueAcquireExternalMemObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_mem_objects,
    const cl_mem *mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_queue);
        if( dispatchX.clEnqueueAcquireExternalMemObjectsKHR )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( command_queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                CALL_LOGGING_ENTER();
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueAcquireExternalMemObjectsKHR(
                    command_queue,
                    num_mem_objects,
                    mem_objects,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
            CHECK_AUBCAPTURE_STOP( command_queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_external_memory
CL_API_ENTRY cl_int CL_API_CALL clEnqueueReleaseExternalMemObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_mem_objects,
    const cl_mem *mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_queue);
        if( dispatchX.clEnqueueReleaseExternalMemObjectsKHR )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( command_queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                CALL_LOGGING_ENTER();
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueReleaseExternalMemObjectsKHR(
                    command_queue,
                    num_mem_objects,
                    mem_objects,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                DEVICE_PERFORMANCE_TIMING_CHECK();
                FLUSH_CHROME_TRACE_BUFFERING();
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
            CHECK_AUBCAPTURE_STOP( command_queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_gl_sharing
CL_API_ENTRY cl_int CL_API_CALL clGetGLContextInfoKHR(
    const cl_context_properties *properties,
    cl_gl_context_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    // The cl_khr_gl_sharing APIs and especially clGetGLContextInfoKHR are a
    // special-case: they are extension functions but do not necessarily pass
    // a dispatchable object as their first argument and are implemented in
    // the ICD loader and called into via the ICD dispatch table.  This means
    // that we can install it into our core API dispatch table as well and
    // don't need to look it up per-platform.

    if( pIntercept && pIntercept->dispatch().clGetGLContextInfoKHR )
    {
        GET_ENQUEUE_COUNTER();
        CALL_LOGGING_ENTER();
        HOST_PERFORMANCE_TIMING_START();

        cl_int  retVal = pIntercept->dispatch().clGetGLContextInfoKHR(
            properties,
            param_name,
            param_value_size,
            param_value,
            param_value_size_ret);

        HOST_PERFORMANCE_TIMING_END();
        CHECK_ERROR( retVal );
        CALL_LOGGING_EXIT( retVal );

        return retVal;
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_gl_event
CL_API_ENTRY cl_event CL_API_CALL clCreateEventFromGLsyncKHR(
    cl_context context,
    cl_GLsync sync,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreateEventFromGLsyncKHR )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "context = %p",
                context );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_event    retVal = dispatchX.clCreateEventFromGLsyncKHR(
                context,
                sync,
                errcode_ret);

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

#if defined(_WIN32)

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_d3d10_sharing
CL_API_ENTRY cl_int CL_API_CALL clGetDeviceIDsFromD3D10KHR(
    cl_platform_id platform,
    cl_d3d10_device_source_khr d3d_device_source,
    void* d3d_object,
    cl_d3d10_device_set_khr d3d_device_set,
    cl_uint num_entries,
    cl_device_id* devices,
    cl_uint* num_devices)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(platform);
        if( dispatchX.clGetDeviceIDsFromD3D10KHR )
        {
            GET_ENQUEUE_COUNTER();

            std::string platformInfo;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getPlatformInfoString(
                    platform,
                    platformInfo );
            }
            CALL_LOGGING_ENTER( "platform = %s",
                platformInfo.c_str() );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetDeviceIDsFromD3D10KHR(
                platform,
                d3d_device_source,
                d3d_object,
                d3d_device_set,
                num_entries,
                devices,
                num_devices);

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_d3d10_sharing
CL_API_ENTRY cl_mem CL_API_CALL clCreateFromD3D10BufferKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D10Buffer* resource,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreateFromD3D10BufferKHR )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER(
                "context = %p, "
                "flags = %s (%llX)",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_mem  retVal = dispatchX.clCreateFromD3D10BufferKHR(
                context,
                flags,
                resource,
                errcode_ret);

            HOST_PERFORMANCE_TIMING_END();
            ADD_BUFFER( retVal );
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_d3d10_sharing
CL_API_ENTRY cl_mem CL_API_CALL clCreateFromD3D10Texture2DKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D10Texture2D* resource,
    UINT subresource,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreateFromD3D10Texture2DKHR )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER(
                "context = %p, "
                "flags = %s (%llX)",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_mem  retVal = dispatchX.clCreateFromD3D10Texture2DKHR(
                context,
                flags,
                resource,
                subresource,
                errcode_ret);

            HOST_PERFORMANCE_TIMING_END();
            ADD_IMAGE( retVal );
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_d3d10_sharing
CL_API_ENTRY cl_mem CL_API_CALL clCreateFromD3D10Texture3DKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D10Texture3D* resource,
    UINT subresource,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreateFromD3D10Texture3DKHR )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER(
                "context = %p, "
                "flags = %s (%llX)",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_mem  retVal = dispatchX.clCreateFromD3D10Texture3DKHR(
                context,
                flags,
                resource,
                subresource,
                errcode_ret);

            HOST_PERFORMANCE_TIMING_END();
            ADD_IMAGE( retVal );
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_d3d10_sharing
CL_API_ENTRY cl_int CL_API_CALL clEnqueueAcquireD3D10ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_queue);
        if( dispatchX.clEnqueueAcquireD3D10ObjectsKHR )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( command_queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                CALL_LOGGING_ENTER();
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueAcquireD3D10ObjectsKHR(
                    command_queue,
                    num_objects,
                    mem_objects,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
            CHECK_AUBCAPTURE_STOP( command_queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_d3d10_sharing
CL_API_ENTRY cl_int CL_API_CALL clEnqueueReleaseD3D10ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_queue);
        if( dispatchX.clEnqueueReleaseD3D10ObjectsKHR )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( command_queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                CALL_LOGGING_ENTER();
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueReleaseD3D10ObjectsKHR(
                    command_queue,
                    num_objects,
                    mem_objects,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                DEVICE_PERFORMANCE_TIMING_CHECK();
                FLUSH_CHROME_TRACE_BUFFERING();
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
            CHECK_AUBCAPTURE_STOP( command_queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_d3d11_sharing
CL_API_ENTRY cl_int CL_API_CALL clGetDeviceIDsFromD3D11KHR(
    cl_platform_id platform,
    cl_d3d11_device_source_khr d3d_device_source,
    void* d3d_object,
    cl_d3d11_device_set_khr d3d_device_set,
    cl_uint num_entries,
    cl_device_id* devices,
    cl_uint* num_devices)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(platform);
        if( dispatchX.clGetDeviceIDsFromD3D11KHR )
        {
            GET_ENQUEUE_COUNTER();

            std::string platformInfo;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getPlatformInfoString(
                    platform,
                    platformInfo );
            }
            CALL_LOGGING_ENTER( "platform = %s",
                platformInfo.c_str() )
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetDeviceIDsFromD3D11KHR(
                platform,
                d3d_device_source,
                d3d_object,
                d3d_device_set,
                num_entries,
                devices,
                num_devices);

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_d3d11_sharing
CL_API_ENTRY cl_mem CL_API_CALL clCreateFromD3D11BufferKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D11Buffer* resource,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreateFromD3D11BufferKHR )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER(
                "context = %p, "
                "flags = %s (%llX)",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_mem  retVal = dispatchX.clCreateFromD3D11BufferKHR(
                context,
                flags,
                resource,
                errcode_ret);

            HOST_PERFORMANCE_TIMING_END();
            ADD_BUFFER( retVal );
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_d3d11_sharing
CL_API_ENTRY cl_mem CL_API_CALL clCreateFromD3D11Texture2DKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D11Texture2D* resource,
    UINT subresource,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreateFromD3D11Texture2DKHR )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER(
                "context = %p, "
                "flags = %s (%llX)",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_mem  retVal = dispatchX.clCreateFromD3D11Texture2DKHR(
                context,
                flags,
                resource,
                subresource,
                errcode_ret);

            HOST_PERFORMANCE_TIMING_END();
            ADD_IMAGE( retVal );
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_d3d11_sharing
CL_API_ENTRY cl_mem CL_API_CALL clCreateFromD3D11Texture3DKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D11Texture3D* resource,
    UINT subresource,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreateFromD3D11Texture3DKHR )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER(
                "context = %p, "
                "flags = %s (%llX)",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_mem  retVal = dispatchX.clCreateFromD3D11Texture3DKHR(
                context,
                flags,
                resource,
                subresource,
                errcode_ret);

            HOST_PERFORMANCE_TIMING_END();
            ADD_IMAGE( retVal );
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_d3d11_sharing
CL_API_ENTRY cl_int CL_API_CALL clEnqueueAcquireD3D11ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_queue);
        if( dispatchX.clEnqueueAcquireD3D11ObjectsKHR )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( command_queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                CALL_LOGGING_ENTER();
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueAcquireD3D11ObjectsKHR(
                    command_queue,
                    num_objects,
                    mem_objects,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
            CHECK_AUBCAPTURE_STOP( command_queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_d3d11_sharing
CL_API_ENTRY cl_int CL_API_CALL clEnqueueReleaseD3D11ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_queue);
        if( dispatchX.clEnqueueReleaseD3D11ObjectsKHR )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( command_queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                CALL_LOGGING_ENTER();
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueReleaseD3D11ObjectsKHR(
                    command_queue,
                    num_objects,
                    mem_objects,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                DEVICE_PERFORMANCE_TIMING_CHECK();
                FLUSH_CHROME_TRACE_BUFFERING();
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
            CHECK_AUBCAPTURE_STOP( command_queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_dx9_media_sharing
CL_API_ENTRY cl_int CL_API_CALL clGetDeviceIDsFromDX9MediaAdapterKHR(
    cl_platform_id platform,
    cl_uint num_media_adapters,
    cl_dx9_media_adapter_type_khr* media_adapters_type,
    void* media_adapters,
    cl_dx9_media_adapter_set_khr media_adapter_set,
    cl_uint num_entries,
    cl_device_id* devices,
    cl_uint* num_devices)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(platform);
        if( dispatchX.clGetDeviceIDsFromDX9MediaAdapterKHR )
        {
            GET_ENQUEUE_COUNTER();

            std::string platformInfo;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getPlatformInfoString(
                    platform,
                    platformInfo );
            }
            CALL_LOGGING_ENTER( "platform = %s",
                platformInfo.c_str() )
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetDeviceIDsFromDX9MediaAdapterKHR(
                platform,
                num_media_adapters,
                media_adapters_type,
                media_adapters,
                media_adapter_set,
                num_entries,
                devices,
                num_devices);

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_dx9_media_sharing
CL_API_ENTRY cl_mem CL_API_CALL clCreateFromDX9MediaSurfaceKHR(
    cl_context context,
    cl_mem_flags flags,
    cl_dx9_media_adapter_type_khr adapter_type,
    void* surface_info,
    cl_uint plane,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreateFromDX9MediaSurfaceKHR )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER(
                "context = %p, "
                "flags = %s (%llX)",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_mem  retVal = dispatchX.clCreateFromDX9MediaSurfaceKHR(
                context,
                flags,
                adapter_type,
                surface_info,
                plane,
                errcode_ret);

            HOST_PERFORMANCE_TIMING_END();
            ADD_IMAGE( retVal );
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_dx9_media_sharing
CL_API_ENTRY cl_int CL_API_CALL clEnqueueAcquireDX9MediaSurfacesKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_queue);
        if( dispatchX.clEnqueueAcquireDX9MediaSurfacesKHR )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( command_queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                CALL_LOGGING_ENTER();
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueAcquireDX9MediaSurfacesKHR(
                    command_queue,
                    num_objects,
                    mem_objects,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
            CHECK_AUBCAPTURE_STOP( command_queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_dx9_media_sharing
CL_API_ENTRY cl_int CL_API_CALL clEnqueueReleaseDX9MediaSurfacesKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_queue);
        if( dispatchX.clEnqueueReleaseDX9MediaSurfacesKHR )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( command_queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                CALL_LOGGING_ENTER();
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueReleaseDX9MediaSurfacesKHR(
                    command_queue,
                    num_objects,
                    mem_objects,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                DEVICE_PERFORMANCE_TIMING_CHECK();
                FLUSH_CHROME_TRACE_BUFFERING();
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
            CHECK_AUBCAPTURE_STOP( command_queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}
#endif

#if defined(_WIN32)
///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_dx9_media_sharing Extension
CL_API_ENTRY cl_int CL_API_CALL clGetDeviceIDsFromDX9INTEL(
    cl_platform_id platform,
    cl_dx9_device_source_intel d3d_device_source,
    void *dx9_object,
    cl_dx9_device_set_intel d3d_device_set,
    cl_uint num_entries,
    cl_device_id* devices,
    cl_uint* num_devices )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(platform);
        if( dispatchX.clGetDeviceIDsFromDX9INTEL )
        {
            GET_ENQUEUE_COUNTER();

            std::string platformInfo;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getPlatformInfoString(
                    platform,
                    platformInfo );
            }
            CALL_LOGGING_ENTER( "platform = %s",
                platformInfo.c_str() )
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetDeviceIDsFromDX9INTEL(
                platform,
                d3d_device_source,
                dx9_object,
                d3d_device_set,
                num_entries,
                devices,
                num_devices);

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_dx9_media_sharing Extension
CL_API_ENTRY cl_mem CL_API_CALL clCreateFromDX9MediaSurfaceINTEL(
    cl_context context,
    cl_mem_flags flags,
    IDirect3DSurface9* resource,
    HANDLE sharedHandle,
    UINT plane,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreateFromDX9MediaSurfaceINTEL )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER(
                "context = %p, "
                "flags = %s (%llX)",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_mem  retVal = dispatchX.clCreateFromDX9MediaSurfaceINTEL(
                context,
                flags,
                resource,
                sharedHandle,
                plane,
                errcode_ret);

            HOST_PERFORMANCE_TIMING_END();
            ADD_IMAGE( retVal );
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_dx9_media_sharing Extension
CL_API_ENTRY cl_int CL_API_CALL clEnqueueAcquireDX9ObjectsINTEL(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_queue);
        if( dispatchX.clEnqueueAcquireDX9ObjectsINTEL )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( command_queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                CALL_LOGGING_ENTER();
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueAcquireDX9ObjectsINTEL(
                    command_queue,
                    num_objects,
                    mem_objects,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
            CHECK_AUBCAPTURE_STOP( command_queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_dx9_media_sharing Extension
CL_API_ENTRY cl_int CL_API_CALL clEnqueueReleaseDX9ObjectsINTEL(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_queue);
        if( dispatchX.clEnqueueReleaseDX9ObjectsINTEL )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( command_queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                CALL_LOGGING_ENTER();
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueReleaseDX9ObjectsINTEL(
                    command_queue,
                    num_objects,
                    mem_objects,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                DEVICE_PERFORMANCE_TIMING_CHECK();
                FLUSH_CHROME_TRACE_BUFFERING();
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
            CHECK_AUBCAPTURE_STOP( command_queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Unofficial MDAPI extension:
CL_API_ENTRY cl_command_queue CL_API_CALL clCreatePerfCountersCommandQueueINTEL(
    cl_context context,
    cl_device_id device,
    cl_command_queue_properties properties,
    cl_uint configuration,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreatePerfCountersCommandQueueINTEL )
        {
            GET_ENQUEUE_COUNTER();

            // We don't have to do this, since profiling must be enabled
            // for a perf counters command queue, but it doesn't hurt to
            // add it, either.
            if( pIntercept->config().DevicePerformanceTiming ||
                pIntercept->config().ITTPerformanceTiming ||
                pIntercept->config().ChromePerformanceTiming ||
                pIntercept->config().DevicePerfCounterEventBasedSampling )
            {
                properties |= (cl_command_queue_properties)CL_QUEUE_PROFILING_ENABLE;
            }

            std::string deviceInfo;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getDeviceInfoString(
                    1,
                    &device,
                    deviceInfo );
            }
            CALL_LOGGING_ENTER( "context = %p, device = %s, properties = %s (%llX), configuration = %u",
                context,
                deviceInfo.c_str(),
                pIntercept->enumName().name_command_queue_properties( properties ).c_str(),
                properties,
                configuration );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_command_queue    retVal = dispatchX.clCreatePerfCountersCommandQueueINTEL(
                context,
                device,
                properties,
                configuration,
                errcode_ret );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            ITT_REGISTER_COMMAND_QUEUE( retVal, true );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// Unofficial MDAPI extension:
CL_API_ENTRY cl_int CL_API_CALL clSetPerformanceConfigurationINTEL(
    cl_device_id    device,
    cl_uint         count,
    cl_uint*        offsets,
    cl_uint*        values )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(device);
        if( dispatchX.clSetPerformanceConfigurationINTEL )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER();
            HOST_PERFORMANCE_TIMING_START();

            cl_int retVal = dispatchX.clSetPerformanceConfigurationINTEL(
                device,
                count,
                offsets,
                values );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_suggested_local_work_size
CL_API_ENTRY cl_int CL_API_CALL clGetKernelSuggestedLocalWorkSizeKHR(
    cl_command_queue commandQueue,
    cl_kernel kernel,
    cl_uint workDim,
    const size_t *globalWorkOffset,
    const size_t *globalWorkSize,
    size_t *suggestedLocalWorkSize)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(commandQueue);
        if( dispatchX.clGetKernelSuggestedLocalWorkSizeKHR )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER_KERNEL(
                kernel,
                "queue = %p, kernel = %p",
                commandQueue,
                kernel );
            HOST_PERFORMANCE_TIMING_START();

            cl_int retVal = dispatchX.clGetKernelSuggestedLocalWorkSizeKHR(
                commandQueue,
                kernel,
                workDim,
                globalWorkOffset,
                globalWorkSize,
                suggestedLocalWorkSize );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_ext_image_requirements_info
CL_API_ENTRY cl_int CL_API_CALL clGetImageRequirementsInfoEXT(
    cl_context context,
    const cl_mem_properties* properties,
    cl_mem_flags flags,
    const cl_image_format* image_format,
    const cl_image_desc* image_desc,
    cl_image_requirements_info_ext param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clGetImageRequirementsInfoEXT )
        {
            GET_ENQUEUE_COUNTER();
            if( image_desc && image_format )
            {
                std::string propsStr;
                if( pIntercept->config().CallLogging )
                {
                    pIntercept->getMemPropertiesString(
                        properties,
                        propsStr );
                }
                CALL_LOGGING_ENTER(
                    "context = %p, "
                    "properties = [ %s ], "
                    "flags = %s (%llX), "
                    "format->channel_order = %s, "
                    "format->channel_data_type = %s, "
                    "desc->type = %s, "
                    "desc->width = %zu, "
                    "desc->height = %zu, "
                    "desc->depth = %zu, "
                    "desc->array_size = %zu, "
                    "desc->row_pitch = %zu, "
                    "desc->slice_pitch = %zu, "
                    "desc->num_mip_levels = %u, "
                    "desc->num_samples = %u, "
                    "desc->mem_object = %p, "
                    "param_name = %s (%08X)",
                    context,
                    propsStr.c_str(),
                    pIntercept->enumName().name_mem_flags( flags ).c_str(),
                    flags,
                    pIntercept->enumName().name( image_format->image_channel_order ).c_str(),
                    pIntercept->enumName().name( image_format->image_channel_data_type ).c_str(),
                    pIntercept->enumName().name( image_desc->image_type ).c_str(),
                    image_desc->image_width,
                    image_desc->image_height,
                    image_desc->image_depth,
                    image_desc->image_array_size,
                    image_desc->image_row_pitch,
                    image_desc->image_slice_pitch,
                    image_desc->num_mip_levels,
                    image_desc->num_samples,
                    image_desc->mem_object,
                    pIntercept->enumName().name( param_name ).c_str(),
                    param_name );
            }
            else
            {
                CALL_LOGGING_ENTER();
            }

            HOST_PERFORMANCE_TIMING_START();

            cl_int retVal = dispatchX.clGetImageRequirementsInfoEXT(
                context,
                properties,
                flags,
                image_format,
                image_desc,
                param_name,
                param_value_size,
                param_value,
                param_value_size_ret);

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// Unofficial cl_get_kernel_suggested_local_work_size extension:
// This function should stay in sync with clGetKernelSuggestedLocalWorkSizeKHR, above.
CL_API_ENTRY cl_int CL_API_CALL clGetKernelSuggestedLocalWorkSizeINTEL(
    cl_command_queue commandQueue,
    cl_kernel kernel,
    cl_uint workDim,
    const size_t *globalWorkOffset,
    const size_t *globalWorkSize,
    size_t *suggestedLocalWorkSize)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(commandQueue);
        if( dispatchX.clGetKernelSuggestedLocalWorkSizeINTEL )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER_KERNEL(
                kernel,
                "queue = %p, kernel = %p",
                commandQueue,
                kernel );
            HOST_PERFORMANCE_TIMING_START();

            cl_int retVal = dispatchX.clGetKernelSuggestedLocalWorkSizeINTEL(
                commandQueue,
                kernel,
                workDim,
                globalWorkOffset,
                globalWorkSize,
                suggestedLocalWorkSize );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_accelerator
CL_API_ENTRY cl_accelerator_intel CL_API_CALL clCreateAcceleratorINTEL(
    cl_context context,
    cl_accelerator_type_intel accelerator_type,
    size_t descriptor_size,
    const void* descriptor,
    cl_int* errcode_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreateAcceleratorINTEL )
        {
            GET_ENQUEUE_COUNTER();

            if( ( accelerator_type == CL_ACCELERATOR_TYPE_MOTION_ESTIMATION_INTEL ) &&
                ( descriptor_size >= sizeof( cl_motion_estimation_desc_intel ) ) )
            {
                cl_motion_estimation_desc_intel* desc =
                    (cl_motion_estimation_desc_intel*)descriptor;
                CALL_LOGGING_ENTER( "context = %p, motion_estimation_desc[ mb_block_type = %u, subpixel_mode = %u, sad_adjust_mode = %u, search_path_type = %u ]",
                    context,
                    desc->mb_block_type,
                    desc->subpixel_mode,
                    desc->sad_adjust_mode,
                    desc->search_path_type );
            }
            else
            {
                CALL_LOGGING_ENTER( "context = %p, accelerator_type = %u",
                    context,
                    accelerator_type );
            }
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_accelerator_intel retVal = dispatchX.clCreateAcceleratorINTEL(
                context,
                accelerator_type,
                descriptor_size,
                descriptor,
                errcode_ret);

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( errcode_ret[0] );
            //ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            if( retVal != NULL )
            {
                pIntercept->addAcceleratorInfo(
                    retVal,
                    context );
            }

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_accelerator
CL_API_ENTRY cl_int CL_API_CALL clGetAcceleratorInfoINTEL(
    cl_accelerator_intel accelerator,
    cl_accelerator_info_intel param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(accelerator);
        if( dispatchX.clGetAcceleratorInfoINTEL )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "param_name = %s (%X)",
                pIntercept->enumName().name( param_name ).c_str(),
                param_name );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetAcceleratorInfoINTEL(
                accelerator,
                param_name,
                param_value_size,
                param_value,
                param_value_size_ret );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_accelerator
CL_API_ENTRY cl_int CL_API_CALL clRetainAcceleratorINTEL(
    cl_accelerator_intel accelerator )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(accelerator);
        if( dispatchX.clRetainAcceleratorINTEL )
        {
            GET_ENQUEUE_COUNTER();

            cl_uint ref_count =
                pIntercept->config().CallLogging ?
                pIntercept->getRefCount( accelerator ) : 0;
            CALL_LOGGING_ENTER( "[ ref count = %d ] accelerator = %p",
                ref_count,
                accelerator );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clRetainAcceleratorINTEL(
                accelerator );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            ref_count =
                pIntercept->config().CallLogging ?
                pIntercept->getRefCount( accelerator ) : 0;
            CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", ref_count );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_accelerator
CL_API_ENTRY cl_int CL_API_CALL clReleaseAcceleratorINTEL(
    cl_accelerator_intel accelerator )
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(accelerator);
        if( dispatchX.clReleaseAcceleratorINTEL )
        {
            GET_ENQUEUE_COUNTER();

            cl_uint ref_count =
                pIntercept->config().CallLogging ?
                pIntercept->getRefCount( accelerator ) : 0;
            CALL_LOGGING_ENTER( "[ ref count = %d ] accelerator = %p",
                ref_count,
                accelerator );
            pIntercept->checkRemoveAcceleratorInfo( accelerator );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clReleaseAcceleratorINTEL(
                accelerator );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", --ref_count );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_va_api_media_sharing
CL_API_ENTRY cl_int CL_API_CALL clGetDeviceIDsFromVA_APIMediaAdapterINTEL(
    cl_platform_id platform,
    cl_va_api_device_source_intel media_adapter_type,
    void *media_adapter,
    cl_va_api_device_set_intel media_adapter_set,
    cl_uint num_entries,
    cl_device_id *devices,
    cl_uint *num_devices)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(platform);
        if( dispatchX.clGetDeviceIDsFromVA_APIMediaAdapterINTEL )
        {
            GET_ENQUEUE_COUNTER();

            std::string platformInfo;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getPlatformInfoString(
                    platform,
                    platformInfo );
            }
            CALL_LOGGING_ENTER( "platform = %s",
                platformInfo.c_str() )
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetDeviceIDsFromVA_APIMediaAdapterINTEL(
                platform,
                media_adapter_type,
                media_adapter,
                media_adapter_set,
                num_entries,
                devices,
                num_devices);

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_va_api_media_sharing
CL_API_ENTRY cl_mem CL_API_CALL clCreateFromVA_APIMediaSurfaceINTEL(
    cl_context context,
    cl_mem_flags flags,
    VASurfaceID *surface,
    cl_uint plane,
    cl_int *errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clCreateFromVA_APIMediaSurfaceINTEL )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER(
                "context = %p, "
                "flags = %s (%llX)",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_mem  retVal = dispatchX.clCreateFromVA_APIMediaSurfaceINTEL(
                context,
                flags,
                surface,
                plane,
                errcode_ret);

            HOST_PERFORMANCE_TIMING_END();
            ADD_IMAGE( retVal );
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_va_api_media_sharing
CL_API_ENTRY cl_int CL_API_CALL clEnqueueAcquireVA_APIMediaSurfacesINTEL(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem *mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_queue);
        if( dispatchX.clEnqueueAcquireVA_APIMediaSurfacesINTEL )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( command_queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                CALL_LOGGING_ENTER();
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueAcquireVA_APIMediaSurfacesINTEL(
                    command_queue,
                    num_objects,
                    mem_objects,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
            CHECK_AUBCAPTURE_STOP( command_queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_va_api_media_sharing
CL_API_ENTRY cl_int CL_API_CALL clEnqueueReleaseVA_APIMediaSurfacesINTEL(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem *mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_queue);
        if( dispatchX.clEnqueueReleaseVA_APIMediaSurfacesINTEL )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( command_queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                CALL_LOGGING_ENTER();
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueReleaseVA_APIMediaSurfacesINTEL(
                    command_queue,
                    num_objects,
                    mem_objects,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                DEVICE_PERFORMANCE_TIMING_CHECK();
                FLUSH_CHROME_TRACE_BUFFERING();
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
            CHECK_AUBCAPTURE_STOP( command_queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_sharing_format_query
CL_API_ENTRY cl_int CL_API_CALL clGetSupportedGLTextureFormatsINTEL(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_object_type image_type,
    cl_uint num_entries,
    cl_GLenum* gl_formats,
    cl_uint* num_texture_formats)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clGetSupportedGLTextureFormatsINTEL )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "context = %p, flags = %s (%llX), image_type = %s (%X)",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags,
                pIntercept->enumName().name( image_type ).c_str(),
                image_type );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetSupportedGLTextureFormatsINTEL(
                context,
                flags,
                image_type,
                num_entries,
                gl_formats,
                num_texture_formats);

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_sharing_format_query
CL_API_ENTRY cl_int CL_API_CALL clGetSupportedDX9MediaSurfaceFormatsINTEL(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_object_type image_type,
    cl_uint plane,
    cl_uint num_entries,
    D3DFORMAT* dx9_formats,
    cl_uint* num_surface_formats)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clGetSupportedDX9MediaSurfaceFormatsINTEL )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "context = %p, flags = %s (%llX), image_type = %s (%X), plane = %u",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags,
                pIntercept->enumName().name( image_type ).c_str(),
                image_type,
                plane );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetSupportedDX9MediaSurfaceFormatsINTEL(
                context,
                flags,
                image_type,
                plane,
                num_entries,
                dx9_formats,
                num_surface_formats);

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }
    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_sharing_format_query
CL_API_ENTRY cl_int CL_API_CALL clGetSupportedD3D10TextureFormatsINTEL(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_object_type image_type,
    cl_uint num_entries,
    DXGI_FORMAT* d3d10_formats,
    cl_uint* num_texture_formats)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clGetSupportedD3D10TextureFormatsINTEL )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "context = %p, flags = %s (%llX), image_type = %s (%X)",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags,
                pIntercept->enumName().name( image_type ).c_str(),
                image_type );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetSupportedD3D10TextureFormatsINTEL(
                context,
                flags,
                image_type,
                num_entries,
                d3d10_formats,
                num_texture_formats);

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }
    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_sharing_format_query
CL_API_ENTRY cl_int CL_API_CALL clGetSupportedD3D11TextureFormatsINTEL(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_object_type image_type,
    cl_uint plane,
    cl_uint num_entries,
    DXGI_FORMAT* d3d11_formats,
    cl_uint* num_texture_formats)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clGetSupportedD3D11TextureFormatsINTEL )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "context = %p, flags = %s (%llX), image_type = %s (%X), plane = %u",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags,
                pIntercept->enumName().name( image_type ).c_str(),
                image_type,
                plane );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetSupportedD3D11TextureFormatsINTEL(
                context,
                flags,
                image_type,
                plane,
                num_entries,
                d3d11_formats,
                num_texture_formats);

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }
    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_sharing_format_query
CL_API_ENTRY cl_int CL_API_CALL clGetSupportedVA_APIMediaSurfaceFormatsINTEL(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_object_type image_type,
    cl_uint plane,
    cl_uint num_entries,
    VAImageFormat* va_api_formats,
    cl_uint* num_surface_formats)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clGetSupportedVA_APIMediaSurfaceFormatsINTEL )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "context = %p, flags = %s (%llX), image_type = %s (%X), plane = %u",
                context,
                pIntercept->enumName().name_mem_flags( flags ).c_str(),
                flags,
                pIntercept->enumName().name( image_type ).c_str(),
                image_type,
                plane );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetSupportedVA_APIMediaSurfaceFormatsINTEL(
                context,
                flags,
                image_type,
                plane,
                num_entries,
                va_api_formats,
                num_surface_formats);

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }
    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
CL_API_ENTRY void* CL_API_CALL clHostMemAllocINTEL(
    cl_context context,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clHostMemAllocINTEL )
        {
            GET_ENQUEUE_COUNTER();

            cl_mem_properties_intel*    newProperties = NULL;
            void*   retVal = NULL;

            // TODO: Make properties string.
            CALL_LOGGING_ENTER( "context = %p, properties = %p, size = %zu, alignment = %u",
                context,
                properties,
                size,
                alignment );
            USM_ALLOC_OVERRIDE_INIT( properties, newProperties );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            if( ( retVal == NULL ) && newProperties )
            {
                retVal = dispatchX.clHostMemAllocINTEL(
                    context,
                    newProperties,
                    size,
                    alignment,
                    errcode_ret );
            }
            if( retVal == NULL )
            {
                retVal = dispatchX.clHostMemAllocINTEL(
                    context,
                    properties,
                    size,
                    alignment,
                    errcode_ret );
            }

            HOST_PERFORMANCE_TIMING_END();
            ADD_USM_ALLOCATION( retVal, size );
            USM_ALLOC_PROPERTIES_CLEANUP( newProperties );
            CHECK_ERROR( errcode_ret[0] );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
CL_API_ENTRY void* CL_API_CALL clDeviceMemAllocINTEL(
    cl_context context,
    cl_device_id device,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clDeviceMemAllocINTEL )
        {
            GET_ENQUEUE_COUNTER();

            cl_mem_properties_intel*    newProperties = NULL;
            void*   retVal = NULL;

            std::string deviceInfo;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getDeviceInfoString(
                    1,
                    &device,
                    deviceInfo );
            }
            // TODO: Make properties string.
            CALL_LOGGING_ENTER( "context = %p, device = %s, properties = %p, size = %zu, alignment = %u",
                context,
                deviceInfo.c_str(),
                properties,
                size,
                alignment );
            USM_ALLOC_OVERRIDE_INIT( properties, newProperties );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            if( ( retVal == NULL ) && newProperties )
            {
                retVal = dispatchX.clDeviceMemAllocINTEL(
                    context,
                    device,
                    newProperties,
                    size,
                    alignment,
                    errcode_ret );
            }
            if( retVal == NULL )
            {
                retVal = dispatchX.clDeviceMemAllocINTEL(
                    context,
                    device,
                    properties,
                    size,
                    alignment,
                    errcode_ret );
            }

            HOST_PERFORMANCE_TIMING_END();
            ADD_USM_ALLOCATION( retVal, size );
            USM_ALLOC_PROPERTIES_CLEANUP( newProperties );
            CHECK_ERROR( errcode_ret[0] );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
CL_API_ENTRY void* CL_API_CALL clSharedMemAllocINTEL(
    cl_context context,
    cl_device_id device,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clSharedMemAllocINTEL )
        {
            GET_ENQUEUE_COUNTER();

            cl_mem_properties_intel*    newProperties = NULL;
            void*   retVal = NULL;

            std::string deviceInfo;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getDeviceInfoString(
                    1,
                    &device,
                    deviceInfo );
            }
            // TODO: Make properties string.
            CALL_LOGGING_ENTER( "context = %p, device = %s, properties = %p, size = %zu, alignment = %u",
                context,
                deviceInfo.c_str(),
                properties,
                size,
                alignment );
            USM_ALLOC_OVERRIDE_INIT( properties, newProperties );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            if( ( retVal == NULL ) && newProperties )
            {
                retVal = dispatchX.clSharedMemAllocINTEL(
                    context,
                    device,
                    newProperties,
                    size,
                    alignment,
                    errcode_ret );
            }
            if( retVal == NULL )
            {
                retVal = dispatchX.clSharedMemAllocINTEL(
                    context,
                    device,
                    properties,
                    size,
                    alignment,
                    errcode_ret );
            }

            HOST_PERFORMANCE_TIMING_END();
            ADD_USM_ALLOCATION( retVal, size );
            USM_ALLOC_PROPERTIES_CLEANUP( newProperties );
            CHECK_ERROR( errcode_ret[0] );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
CL_API_ENTRY cl_int CL_API_CALL clMemFreeINTEL(
    cl_context context,
    void* ptr)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clMemFreeINTEL )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "context = %p, ptr = %p",
                context,
                ptr );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clMemFreeINTEL(
                context,
                ptr );

            HOST_PERFORMANCE_TIMING_END();
            REMOVE_USM_ALLOCATION( ptr );
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
CL_API_ENTRY cl_int CL_API_CALL
clMemBlockingFreeINTEL(
    cl_context context,
    void* ptr)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clMemBlockingFreeINTEL )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "context = %p, ptr = %p",
                context,
                ptr );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clMemBlockingFreeINTEL(
                context,
                ptr );

            HOST_PERFORMANCE_TIMING_END();
            REMOVE_USM_ALLOCATION( ptr );
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );
            DEVICE_PERFORMANCE_TIMING_CHECK();
            FLUSH_CHROME_TRACE_BUFFERING();

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
CL_API_ENTRY cl_int CL_API_CALL clGetMemAllocInfoINTEL(
    cl_context context,
    const void* ptr,
    cl_mem_info_intel param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(context);
        if( dispatchX.clGetMemAllocInfoINTEL )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "context = %p, ptr = %p, param_name = %s (%08X)",
                context,
                ptr,
                pIntercept->enumName().name( param_name ).c_str(),
                param_name );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetMemAllocInfoINTEL(
                context,
                ptr,
                param_name,
                param_value_size,
                param_value,
                param_value_size_ret );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
CL_API_ENTRY cl_int CL_API_CALL clSetKernelArgMemPointerINTEL(
    cl_kernel kernel,
    cl_uint arg_index,
    const void* arg_value)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(kernel);
        if( dispatchX.clSetKernelArgMemPointerINTEL )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER_KERNEL(
                kernel,
                "kernel = %p, index = %u, value = %p",
                kernel,
                arg_index,
                arg_value );
            CHECK_KERNEL_ARG_USM_POINTER( kernel, arg_value );
            SET_KERNEL_ARG_USM_POINTER( kernel, arg_index, arg_value );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clSetKernelArgMemPointerINTEL(
                kernel,
                arg_index,
                arg_value );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
CL_API_ENTRY cl_int CL_API_CALL clEnqueueMemsetINTEL(   // Deprecated
    cl_command_queue queue,
    void* dst_ptr,
    cl_int value,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(queue);
        if( dispatchX.clEnqueueMemsetINTEL )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                const std::string eventWaitListString = getFormattedEventWaitList(
                    pIntercept,
                    num_events_in_wait_list,
                    event_wait_list);

                CALL_LOGGING_ENTER( "queue = %p, dst_ptr = %p, value = %d, size = %zu%s",
                    queue,
                    dst_ptr,
                    value,
                    size,
                    eventWaitListString.c_str() );
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                GET_TIMING_TAGS_MEMFILL( queue, dst_ptr, size );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueMemsetINTEL(
                    queue,
                    dst_ptr,
                    value,
                    size,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );

                HOST_PERFORMANCE_TIMING_END_WITH_TAG();
                DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( queue );
            CHECK_AUBCAPTURE_STOP( queue  );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
CL_API_ENTRY cl_int CL_API_CALL clEnqueueMemFillINTEL(
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

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(queue);
        if( dispatchX.clEnqueueMemFillINTEL )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                const std::string eventWaitListString = getFormattedEventWaitList(
                    pIntercept,
                    num_events_in_wait_list,
                    event_wait_list);

                CALL_LOGGING_ENTER( "queue = %p, dst_ptr = %p, pattern_size = %zu, size = %zu%s",
                    queue,
                    dst_ptr,
                    pattern_size,
                    size,
                    eventWaitListString.c_str() );
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                GET_TIMING_TAGS_MEMFILL( queue, dst_ptr, size );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueMemFillINTEL(
                    queue,
                    dst_ptr,
                    pattern,
                    pattern_size,
                    size,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );

                HOST_PERFORMANCE_TIMING_END_WITH_TAG();
                DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( queue );
            CHECK_AUBCAPTURE_STOP( queue  );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
CL_API_ENTRY cl_int CL_API_CALL clEnqueueMemcpyINTEL(
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

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(queue);
        if( dispatchX.clEnqueueMemcpyINTEL )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                const std::string eventWaitListString = getFormattedEventWaitList(
                    pIntercept,
                    num_events_in_wait_list,
                    event_wait_list);

                CALL_LOGGING_ENTER( "queue = %p, %s, dst_ptr = %p, src_ptr = %p, size = %zu%s",
                    queue,
                    blocking ? "blocking" : "non-blocking",
                    dst_ptr,
                    src_ptr,
                    size,
                    eventWaitListString.c_str() );
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                GET_TIMING_TAGS_MEMCPY( queue, blocking, dst_ptr, src_ptr, size );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueMemcpyINTEL(
                    queue,
                    blocking,
                    dst_ptr,
                    src_ptr,
                    size,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );

                HOST_PERFORMANCE_TIMING_END_WITH_TAG();
                DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT_WITH_TAG( retVal, event );
                DEVICE_PERFORMANCE_TIMING_CHECK_CONDITIONAL( blocking );
                FLUSH_CHROME_TRACE_BUFFERING_CONDITIONAL( blocking );
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( queue );
            CHECK_AUBCAPTURE_STOP( queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
CL_API_ENTRY cl_int CL_API_CALL clEnqueueMigrateMemINTEL(
    cl_command_queue queue,
    const void* ptr,
    size_t size,
    cl_mem_migration_flags flags,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(queue);
        if( dispatchX.clEnqueueMigrateMemINTEL )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                const std::string eventWaitListString = getFormattedEventWaitList(
                    pIntercept,
                    num_events_in_wait_list,
                    event_wait_list);

                CALL_LOGGING_ENTER( "queue = %p, ptr = %p, size = %zu, flags = %s (%llX)%s",
                    queue,
                    ptr,
                    size,
                    pIntercept->enumName().name_mem_migration_flags( flags ).c_str(),
                    flags,
                    eventWaitListString.c_str() );
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueMigrateMemINTEL(
                    queue,
                    ptr,
                    size,
                    flags,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( queue );
            CHECK_AUBCAPTURE_STOP( queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_intel_unified_shared_memory
CL_API_ENTRY cl_int CL_API_CALL clEnqueueMemAdviseINTEL(
    cl_command_queue queue,
    const void* ptr,
    size_t size,
    cl_mem_advice_intel advice,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(queue);
        if( dispatchX.clEnqueueMemAdviseINTEL )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            CHECK_AUBCAPTURE_START( queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                const std::string eventWaitListString = getFormattedEventWaitList(
                    pIntercept,
                    num_events_in_wait_list,
                    event_wait_list);

                CALL_LOGGING_ENTER( "queue = %p, ptr = %p, size = %zu, advice = %s (%u)%s",
                    queue,
                    ptr,
                    size,
                    pIntercept->enumName().name(advice).c_str(),
                    advice,
                    eventWaitListString.c_str() );
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueMemAdviseINTEL(
                    queue,
                    ptr,
                    size,
                    advice,
                    num_events_in_wait_list,
                    event_wait_list,
                    event );

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( queue );
            CHECK_AUBCAPTURE_STOP( queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_command_buffer_khr CL_API_CALL clCreateCommandBufferKHR(
    cl_uint num_queues,
    const cl_command_queue* queues,
    const cl_command_buffer_properties_khr* properties,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        cl_command_queue queue = num_queues ? queues[0] : NULL;
        const auto& dispatchX = pIntercept->dispatchX(queue);
        if( dispatchX.clCreateCommandBufferKHR )
        {
            GET_ENQUEUE_COUNTER();

            std::string propsStr;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getCommandBufferPropertiesString(
                    properties,
                    propsStr );
            }
            CALL_LOGGING_ENTER( "num_queues = %u, properties = [ %s ]",
                num_queues,
                propsStr.c_str() );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_command_buffer_khr   retVal = dispatchX.clCreateCommandBufferKHR(
                num_queues,
                queues,
                properties,
                errcode_ret );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            if( retVal != NULL )
            {
                pIntercept->addCommandBufferInfo(
                    retVal,
                    queue );
            }

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clFinalizeCommandBufferKHR(
    cl_command_buffer_khr command_buffer)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clFinalizeCommandBufferKHR )
        {
            GET_ENQUEUE_COUNTER();

            CALL_LOGGING_ENTER( "command_buffer = %p",
                command_buffer );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clFinalizeCommandBufferKHR(
                command_buffer );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clRetainCommandBufferKHR(
    cl_command_buffer_khr command_buffer)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clRetainCommandBufferKHR )
        {
            GET_ENQUEUE_COUNTER();

            cl_uint ref_count =
                pIntercept->config().CallLogging ?
                pIntercept->getRefCount( command_buffer ) : 0;
            CALL_LOGGING_ENTER( "[ ref count = %d ] command_buffer = %p",
                ref_count,
                command_buffer );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clRetainCommandBufferKHR(
                command_buffer );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            ADD_OBJECT_RETAIN( command_buffer );
            ref_count =
                pIntercept->config().CallLogging ?
                pIntercept->getRefCount( command_buffer ) : 0;
            CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", ref_count );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clReleaseCommandBufferKHR(
    cl_command_buffer_khr command_buffer)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clReleaseCommandBufferKHR )
        {
            GET_ENQUEUE_COUNTER();

            cl_uint ref_count =
                pIntercept->config().CallLogging ?
                pIntercept->getRefCount( command_buffer ) : 0;
            CALL_LOGGING_ENTER( "[ ref count = %d ] command_buffer = %p",
                ref_count,
                command_buffer );
            pIntercept->checkRemoveCommandBufferInfo( command_buffer );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clReleaseCommandBufferKHR(
                command_buffer );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            ADD_OBJECT_RELEASE( command_buffer );
            CALL_LOGGING_EXIT( retVal, "[ ref count = %d ]", --ref_count );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clEnqueueCommandBufferKHR(
    cl_uint num_queues,
    cl_command_queue* queues,
    cl_command_buffer_khr command_buffer,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clEnqueueCommandBufferKHR )
        {
            cl_int  retVal = CL_SUCCESS;

            INCREMENT_ENQUEUE_COUNTER();
            COMMAND_BUFFER_GET_QUEUE( num_queues, queues, command_buffer );
            CHECK_AUBCAPTURE_START( command_queue );

            if( pIntercept->config().NullEnqueue == false )
            {
                const std::string eventWaitListString = getFormattedEventWaitList(
                    pIntercept,
                    num_events_in_wait_list,
                    event_wait_list);
                CALL_LOGGING_ENTER( "num_queues = %u, queues = %p, command_buffer = %p%s",
                    num_queues,
                    queues,
                    command_buffer,
                    eventWaitListString.c_str() );
                CHECK_EVENT_LIST( num_events_in_wait_list, event_wait_list, event );
                DEVICE_PERFORMANCE_TIMING_START( event );
                HOST_PERFORMANCE_TIMING_START();

                retVal = dispatchX.clEnqueueCommandBufferKHR(
                    num_queues,
                    queues,
                    command_buffer,
                    num_events_in_wait_list,
                    event_wait_list,
                    event);

                HOST_PERFORMANCE_TIMING_END();
                DEVICE_PERFORMANCE_TIMING_END( command_queue, event );
                CHECK_ERROR( retVal );
                ADD_OBJECT_ALLOCATION( event ? event[0] : NULL );
                CALL_LOGGING_EXIT_EVENT( retVal, event );
                ADD_EVENT( event ? event[0] : NULL );
            }

            FINISH_OR_FLUSH_AFTER_ENQUEUE( command_queue );
            CHECK_AUBCAPTURE_STOP( command_queue );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clCommandBarrierWithWaitListKHR(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clCommandBarrierWithWaitListKHR )
        {
            GET_ENQUEUE_COUNTER();

            CALL_LOGGING_ENTER(
                "command_buffer = %p, command_queue = %p",
                command_buffer,
                command_queue );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clCommandBarrierWithWaitListKHR(
                command_buffer,
                command_queue,
                num_sync_points_in_wait_list,
                sync_point_wait_list,
                sync_point,
                mutable_handle );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );
            ADD_MUTABLE_COMMAND( mutable_handle, command_buffer );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clCommandCopyBufferKHR(
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
    cl_mutable_command_khr* mutable_handle)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clCommandCopyBufferKHR )
        {
            GET_ENQUEUE_COUNTER();

            CALL_LOGGING_ENTER(
                "command_buffer = %p, command_queue = %p",
                command_buffer,
                command_queue );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clCommandCopyBufferKHR(
                command_buffer,
                command_queue,
                src_buffer,
                dst_buffer,
                src_offset,
                dst_offset,
                size,
                num_sync_points_in_wait_list,
                sync_point_wait_list,
                sync_point,
                mutable_handle );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );
            ADD_MUTABLE_COMMAND( mutable_handle, command_buffer );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clCommandCopyBufferRectKHR(
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
    cl_mutable_command_khr* mutable_handle)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clCommandCopyBufferRectKHR )
        {
            GET_ENQUEUE_COUNTER();

            CALL_LOGGING_ENTER(
                "command_buffer = %p, command_queue = %p",
                command_buffer,
                command_queue );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clCommandCopyBufferRectKHR(
                command_buffer,
                command_queue,
                src_buffer,
                dst_buffer,
                src_origin,
                dst_origin,
                region,
                src_row_pitch,
                src_slice_pitch,
                dst_row_pitch,
                dst_slice_pitch,
                num_sync_points_in_wait_list,
                sync_point_wait_list,
                sync_point,
                mutable_handle );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );
            ADD_MUTABLE_COMMAND( mutable_handle, command_buffer );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clCommandCopyBufferToImageKHR(
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
    cl_mutable_command_khr* mutable_handle)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clCommandCopyBufferToImageKHR )
        {
            GET_ENQUEUE_COUNTER();

            CALL_LOGGING_ENTER(
                "command_buffer = %p, command_queue = %p",
                command_buffer,
                command_queue );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clCommandCopyBufferToImageKHR(
                command_buffer,
                command_queue,
                src_buffer,
                dst_image,
                src_offset,
                dst_origin,
                region,
                num_sync_points_in_wait_list,
                sync_point_wait_list,
                sync_point,
                mutable_handle );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );
            ADD_MUTABLE_COMMAND( mutable_handle, command_buffer );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clCommandCopyImageKHR(
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
    cl_mutable_command_khr* mutable_handle)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clCommandCopyImageKHR )
        {
            GET_ENQUEUE_COUNTER();

            CALL_LOGGING_ENTER(
                "command_buffer = %p, command_queue = %p",
                command_buffer,
                command_queue );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clCommandCopyImageKHR(
                command_buffer,
                command_queue,
                src_image,
                dst_image,
                src_origin,
                dst_origin,
                region,
                num_sync_points_in_wait_list,
                sync_point_wait_list,
                sync_point,
                mutable_handle );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );
            ADD_MUTABLE_COMMAND( mutable_handle, command_buffer );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clCommandCopyImageToBufferKHR(
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
    cl_mutable_command_khr* mutable_handle)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clCommandCopyImageToBufferKHR )
        {
            GET_ENQUEUE_COUNTER();

            CALL_LOGGING_ENTER(
                "command_buffer = %p, command_queue = %p",
                command_buffer,
                command_queue );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clCommandCopyImageToBufferKHR(
                command_buffer,
                command_queue,
                src_image,
                dst_buffer,
                src_origin,
                region,
                dst_offset,
                num_sync_points_in_wait_list,
                sync_point_wait_list,
                sync_point,
                mutable_handle );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );
            ADD_MUTABLE_COMMAND( mutable_handle, command_buffer );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clCommandFillBufferKHR(
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
    cl_mutable_command_khr* mutable_handle)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clCommandFillBufferKHR )
        {
            GET_ENQUEUE_COUNTER();

            CALL_LOGGING_ENTER(
                "command_buffer = %p, command_queue = %p",
                command_buffer,
                command_queue );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clCommandFillBufferKHR(
                command_buffer,
                command_queue,
                buffer,
                pattern,
                pattern_size,
                offset,
                size,
                num_sync_points_in_wait_list,
                sync_point_wait_list,
                sync_point,
                mutable_handle );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );
            ADD_MUTABLE_COMMAND( mutable_handle, command_buffer );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clCommandFillImageKHR(
    cl_command_buffer_khr command_buffer,
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
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clCommandFillImageKHR )
        {
            GET_ENQUEUE_COUNTER();

            CALL_LOGGING_ENTER(
                "command_buffer = %p, command_queue = %p",
                command_buffer,
                command_queue );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clCommandFillImageKHR(
                command_buffer,
                command_queue,
                image,
                fill_color,
                origin,
                region,
                num_sync_points_in_wait_list,
                sync_point_wait_list,
                sync_point,
                mutable_handle );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );
            ADD_MUTABLE_COMMAND( mutable_handle, command_buffer );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clCommandSVMMemcpyKHR(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    void* dst_ptr,
    const void* src_ptr,
    size_t size,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clCommandSVMMemcpyKHR )
        {
            GET_ENQUEUE_COUNTER();

            CALL_LOGGING_ENTER(
                "command_buffer = %p, command_queue = %p, dst_ptr = %p, src_ptr = %p, size = %zu",
                command_buffer,
                command_queue,
                dst_ptr,
                src_ptr,
                size );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clCommandSVMMemcpyKHR(
                command_buffer,
                command_queue,
                dst_ptr,
                src_ptr,
                size,
                num_sync_points_in_wait_list,
                sync_point_wait_list,
                sync_point,
                mutable_handle );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );
            ADD_MUTABLE_COMMAND( mutable_handle, command_buffer );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clCommandSVMMemFillKHR(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    void* svm_ptr,
    const void* pattern,
    size_t pattern_size,
    size_t size,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clCommandSVMMemFillKHR )
        {
            GET_ENQUEUE_COUNTER();

            CALL_LOGGING_ENTER(
                "command_buffer = %p, command_queue = %p, svm_ptr = %p, pattern_size = %zu, size = %zu",
                command_buffer,
                command_queue,
                svm_ptr,
                pattern_size,
                size );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clCommandSVMMemFillKHR(
                command_buffer,
                command_queue,
                svm_ptr,
                pattern,
                pattern_size,
                size,
                num_sync_points_in_wait_list,
                sync_point_wait_list,
                sync_point,
                mutable_handle );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );
            ADD_MUTABLE_COMMAND( mutable_handle, command_buffer );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clCommandNDRangeKernelKHR(
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
    cl_mutable_command_khr* mutable_handle)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clCommandNDRangeKernelKHR )
        {
            GET_ENQUEUE_COUNTER();

            // TODO: Should NullLocalWorkSize or local work size overrides apply
            // here?

            std::string argsString;
            if( pIntercept->config().CallLogging )
            {
                pIntercept->getEnqueueNDRangeKernelArgsString(
                    work_dim,
                    global_work_offset,
                    global_work_size,
                    local_work_size,
                    argsString );
            }
            CALL_LOGGING_ENTER_KERNEL(
                kernel,
                "command_buffer = %p, queue = %p, kernel = %p, %s",
                command_buffer,
                command_queue,
                kernel,
                argsString.c_str() );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clCommandNDRangeKernelKHR(
                command_buffer,
                command_queue,
                properties,
                kernel,
                work_dim,
                global_work_offset,
                global_work_size,
                local_work_size,
                num_sync_points_in_wait_list,
                sync_point_wait_list,
                sync_point,
                mutable_handle );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );
            ADD_MUTABLE_COMMAND_NDRANGE( mutable_handle, command_buffer, work_dim );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer
CL_API_ENTRY cl_int CL_API_CALL clGetCommandBufferInfoKHR(
    cl_command_buffer_khr command_buffer,
    cl_command_buffer_info_khr param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clGetCommandBufferInfoKHR )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "command_buffer = %p, param_name = %s (%08X)",
                command_buffer,
                pIntercept->enumName().name( param_name ).c_str(),
                param_name );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetCommandBufferInfoKHR(
                command_buffer,
                param_name,
                param_value_size,
                param_value,
                param_value_size_ret );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer_multi_device
CL_API_ENTRY cl_command_buffer_khr CL_API_CALL clRemapCommandBufferKHR(
    cl_command_buffer_khr command_buffer,
    cl_bool automatic,
    cl_uint num_queues,
    const cl_command_queue* queues,
    cl_uint num_handles,
    const cl_mutable_command_khr* handles,
    cl_mutable_command_khr* handles_ret,
    cl_int* errcode_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        cl_command_queue queue = num_queues ? queues[0] : NULL;
        const auto& dispatchX = pIntercept->dispatchX(queue);
        if( dispatchX.clRemapCommandBufferKHR )
        {
            GET_ENQUEUE_COUNTER();

            CALL_LOGGING_ENTER( "command_buffer = %p, %s, num_queues = %u, num_handles = %u",
                command_buffer,
                automatic ? "automatic" : "non-automatic",
                num_queues,
                num_handles );
            CHECK_ERROR_INIT( errcode_ret );
            HOST_PERFORMANCE_TIMING_START();

            cl_command_buffer_khr   retVal = dispatchX.clRemapCommandBufferKHR(
                command_buffer,
                automatic,
                num_queues,
                queues,
                num_handles,
                handles,
                handles_ret,
                errcode_ret );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( errcode_ret[0] );
            ADD_OBJECT_ALLOCATION( retVal );
            CALL_LOGGING_EXIT( errcode_ret[0], "returned %p", retVal );

            if( retVal != NULL )
            {
                pIntercept->addCommandBufferInfo(
                    retVal,
                    queue );
            }

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(errcode_ret);
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer_mutable_dispatch
CL_API_ENTRY cl_int CL_API_CALL clUpdateMutableCommandsKHR(
    cl_command_buffer_khr command_buffer,
    const cl_mutable_base_config_khr* mutable_config)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command_buffer);
        if( dispatchX.clUpdateMutableCommandsKHR )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "command_buffer = %p, mutable_config = %p",
                command_buffer,
                mutable_config );
            if( pIntercept->config().CallLogging )
            {
                std::string configStr;
                pIntercept->getCommandBufferMutableConfigString(
                    mutable_config,
                    configStr );
                CALL_LOGGING_INFO("mutable_config %p: %s",
                    mutable_config,
                    configStr.c_str() );
            }
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clUpdateMutableCommandsKHR(
                command_buffer,
                mutable_config );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

///////////////////////////////////////////////////////////////////////////////
//
// cl_khr_command_buffer_mutable_dispatch
CL_API_ENTRY cl_int CL_API_CALL clGetMutableCommandInfoKHR(
    cl_mutable_command_khr command,
    cl_mutable_command_info_khr param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    CLIntercept*    pIntercept = GetIntercept();

    if( pIntercept )
    {
        const auto& dispatchX = pIntercept->dispatchX(command);
        if( dispatchX.clGetMutableCommandInfoKHR )
        {
            GET_ENQUEUE_COUNTER();
            CALL_LOGGING_ENTER( "command_buffer = %p, param_name = %s (%08X)",
                command,
                pIntercept->enumName().name( param_name ).c_str(),
                param_name );
            HOST_PERFORMANCE_TIMING_START();

            cl_int  retVal = dispatchX.clGetMutableCommandInfoKHR(
                command,
                param_name,
                param_value_size,
                param_value,
                param_value_size_ret );

            HOST_PERFORMANCE_TIMING_END();
            CHECK_ERROR( retVal );
            CALL_LOGGING_EXIT( retVal );

            return retVal;
        }
    }

    NULL_FUNCTION_POINTER_RETURN_ERROR();
}

#if defined(__APPLE__)
#include "OS/OS_mac_interpose.h"
#endif
