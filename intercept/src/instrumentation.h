/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#if defined(USE_ITT)

#include <string>

#define INTEL_ITTNOTIFY_API_PRIVATE
#include <ittnotify.h>

/*****************************************************************************\
TASK METADATA:
\*****************************************************************************/
template<typename T>
struct MapToITTType                             { enum { value = __itt_metadata_unknown }; };

template<>
struct MapToITTType<cl_ulong>                   { enum { value = __itt_metadata_u64     }; };
template<>
struct MapToITTType<cl_long>                    { enum { value = __itt_metadata_s64     }; };
template<>
struct MapToITTType<cl_uint>                    { enum { value = __itt_metadata_u32     }; };
template<>
struct MapToITTType<cl_int>                     { enum { value = __itt_metadata_s32     }; };
template<>
struct MapToITTType<cl_ushort>                  { enum { value = __itt_metadata_u16     }; };
template<>
struct MapToITTType<cl_short>                   { enum { value = __itt_metadata_s16     }; };
template<>
struct MapToITTType<cl_float>                   { enum { value = __itt_metadata_float   }; };
template<>
struct MapToITTType<cl_double>                  { enum { value = __itt_metadata_double  }; };

// TODO: Is there a standard preprocessor define that can tell us pointer size?
#if defined(_WIN64) || defined(__LP64__)

CLI_C_ASSERT( sizeof(void*) == 8 );
template<typename T>
struct MapToITTType<T*>                         { enum { value = __itt_metadata_u64     }; };

#else

CLI_C_ASSERT( sizeof(void*) == 4 );
template<typename T>
struct MapToITTType<T*>                         { enum { value = __itt_metadata_u32     }; };

#endif

template<typename T>
inline void add_task_metadata(
    __itt_domain* domain,
    const std::string& name,
    const T value )
{
    __itt_string_handle* itt_string_handle = __itt_string_handle_create(name.c_str());
    __itt_metadata_type metadataType = (__itt_metadata_type)MapToITTType<T>::value;

    __itt_metadata_add_with_scope(domain, __itt_scope_task, itt_string_handle, metadataType, 1, (void*)&value);
}

template<>
inline void add_task_metadata<const cl_image_format*>(
    __itt_domain* domain,
    const std::string& name,
    const cl_image_format* value )
{
    if( value )
    {
        std::string fieldName;

        fieldName = name + ".image_channel_data_type";
        add_task_metadata(domain, fieldName.c_str(), value->image_channel_data_type);

        fieldName = name + ".image_channel_order";
        add_task_metadata(domain, fieldName.c_str(), value->image_channel_order);
    }
}

template<typename T>
inline void add_task_metadata_array(
    __itt_domain* domain,
    const std::string& name,
    const size_t count,
    const T* values )
{
    if( values )
    {
        __itt_string_handle* itt_string_handle = __itt_string_handle_create(name.c_str());
        __itt_metadata_type metadataType = (__itt_metadata_type)MapToITTType<T>::value;

        __itt_metadata_add_with_scope(domain, __itt_scope_task, itt_string_handle, metadataType, count, (void*)values);
    }
}

#define ITT_CALL_LOGGING_ENTER(_kernel)                                                         \
    if( pIntercept->config().ITTCallLogging )                                                   \
    {                                                                                           \
        pIntercept->ittInit();                                                                  \
        pIntercept->ittCallLoggingEnter( __FUNCTION__, _kernel );                               \
    }

#define ITT_CALL_LOGGING_EXIT()                                                                 \
    if( pIntercept->config().ITTCallLogging )                                                   \
    {                                                                                           \
        pIntercept->ittInit();                                                                  \
        pIntercept->ittCallLoggingExit();                                                       \
    }

#define ITT_ADD_PARAM_AS_METADATA(_param)                                                       \
    if( pIntercept->config().ITTCallLogging )                                                   \
    {                                                                                           \
        pIntercept->ittInit();                                                                  \
        __itt_domain* itt_domain = pIntercept->ittDomain();                                     \
        add_task_metadata( itt_domain, #_param, _param );                                       \
    }

#define ITT_ADD_ARRAY_PARAM_AS_METADATA(_count, _param)                                         \
    if( pIntercept->config().ITTCallLogging )                                                   \
    {                                                                                           \
        pIntercept->ittInit();                                                                  \
        __itt_domain* itt_domain = pIntercept->ittDomain();                                     \
        add_task_metadata_array( itt_domain, #_param, _count, _param );                         \
    }

#define ITT_REGISTER_COMMAND_QUEUE(_queue, _perfCounters)                                       \
    if( pIntercept->config().ITTPerformanceTiming )                                             \
    {                                                                                           \
        pIntercept->ittInit();                                                                  \
        pIntercept->ittRegisterCommandQueue( _queue, _perfCounters );                           \
    }

#define ITT_RELEASE_COMMAND_QUEUE(_queue)                                                       \
    if( pIntercept->config().ITTPerformanceTiming )                                             \
    {                                                                                           \
        pIntercept->ittInit();                                                                  \
        pIntercept->ittReleaseCommandQueue( _queue );                                           \
    }

#else

#define ITT_CALL_LOGGING_ENTER(_kernel)
#define ITT_CALL_LOGGING_EXIT()
#define ITT_ADD_PARAM_AS_METADATA(_param)
#define ITT_ADD_ARRAY_PARAM_AS_METADATA(_count, _param)
#define ITT_REGISTER_COMMAND_QUEUE(_queue, _perfCounters)
#define ITT_RELEASE_COMMAND_QUEUE(_queue)

#endif
