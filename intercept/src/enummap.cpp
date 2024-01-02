/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include "enummap.h"

#include "common.h"

/* if( _map.find( _enum ) != _map.end() ) fprintf(stderr, "Already found an entry for %08X (%d): new %s, old %s\n", _enum, _enum, #_enum, _map[ _enum ].c_str() ); \ */

#define ADD_ENUM_NAME( _map, _enum )                    \
{                                                       \
    CLI_ASSERT( _map.find( _enum ) == _map.end() );     \
    _map[ _enum ] = #_enum;                             \
}

CEnumNameMap::CEnumNameMap()
{
    /* Error Codes */
    ADD_ENUM_NAME( m_cl_int, CL_SUCCESS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NOT_FOUND );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NOT_AVAILABLE );
    ADD_ENUM_NAME( m_cl_int, CL_COMPILER_NOT_AVAILABLE );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_OBJECT_ALLOCATION_FAILURE );
    ADD_ENUM_NAME( m_cl_int, CL_OUT_OF_RESOURCES );
    ADD_ENUM_NAME( m_cl_int, CL_OUT_OF_HOST_MEMORY );
    ADD_ENUM_NAME( m_cl_int, CL_PROFILING_INFO_NOT_AVAILABLE );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_COPY_OVERLAP );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_FORMAT_MISMATCH );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_FORMAT_NOT_SUPPORTED );
    ADD_ENUM_NAME( m_cl_int, CL_BUILD_PROGRAM_FAILURE );
    ADD_ENUM_NAME( m_cl_int, CL_MAP_FAILURE );
    ADD_ENUM_NAME( m_cl_int, CL_MISALIGNED_SUB_BUFFER_OFFSET );
    ADD_ENUM_NAME( m_cl_int, CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST );
    ADD_ENUM_NAME( m_cl_int, CL_COMPILE_PROGRAM_FAILURE );
    ADD_ENUM_NAME( m_cl_int, CL_LINKER_NOT_AVAILABLE );
    ADD_ENUM_NAME( m_cl_int, CL_LINK_PROGRAM_FAILURE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_FAILED );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ARG_INFO_NOT_AVAILABLE );

    ADD_ENUM_NAME( m_cl_int, CL_INVALID_VALUE );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_DEVICE_TYPE );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_PLATFORM );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_DEVICE );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_CONTEXT );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_QUEUE_PROPERTIES );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_COMMAND_QUEUE );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_HOST_PTR );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_MEM_OBJECT );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_IMAGE_FORMAT_DESCRIPTOR );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_IMAGE_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_SAMPLER );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_BINARY );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_BUILD_OPTIONS );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_PROGRAM );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_PROGRAM_EXECUTABLE );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_KERNEL_NAME );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_KERNEL_DEFINITION );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_KERNEL );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_ARG_INDEX );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_ARG_VALUE );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_ARG_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_KERNEL_ARGS );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_WORK_DIMENSION );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_WORK_GROUP_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_WORK_ITEM_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_GLOBAL_OFFSET );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_EVENT_WAIT_LIST );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_EVENT );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_OPERATION );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_GL_OBJECT );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_BUFFER_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_MIP_LEVEL );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_GLOBAL_WORK_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_PROPERTY );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_IMAGE_DESCRIPTOR );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_COMPILER_OPTIONS );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_LINKER_OPTIONS );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_DEVICE_PARTITION_COUNT );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_PIPE_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_DEVICE_QUEUE );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_SPEC_ID );
    ADD_ENUM_NAME( m_cl_int, CL_MAX_SIZE_RESTRICTION_EXCEEDED );

    /* OpenCL Version */
    //CL_VERSION_1_0                              1
    //CL_VERSION_1_1                              1
    //CL_VERSION_1_2                              1
    //CL_VERSION_2_0                              1
    //CL_VERSION_2_1                              1
    //CL_VERSION_2_2                              1

    /* cl_bool */
    ADD_ENUM_NAME( m_cl_bool, CL_FALSE );
    ADD_ENUM_NAME( m_cl_bool, CL_TRUE );
    //CL_BLOCKING                                 CL_TRUE
    //CL_NON_BLOCKING                             CL_FALSE

    /* cl_platform_info */
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_PROFILE );
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_VERSION );
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_NAME );
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_VENDOR );
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_EXTENSIONS );
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_HOST_TIMER_RESOLUTION );
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_NUMERIC_VERSION );
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_EXTENSIONS_WITH_VERSION );

    /* cl_device_type - bitfield */
    ADD_ENUM_NAME( m_cl_device_type, CL_DEVICE_TYPE_DEFAULT );
    ADD_ENUM_NAME( m_cl_device_type, CL_DEVICE_TYPE_CPU );
    ADD_ENUM_NAME( m_cl_device_type, CL_DEVICE_TYPE_GPU );
    ADD_ENUM_NAME( m_cl_device_type, CL_DEVICE_TYPE_ACCELERATOR );
    ADD_ENUM_NAME( m_cl_device_type, CL_DEVICE_TYPE_CUSTOM );
    ADD_ENUM_NAME( m_cl_device_type, CL_DEVICE_TYPE_ALL );

    /* cl_device_info */
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_TYPE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_VENDOR_ID );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_COMPUTE_UNITS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_WORK_GROUP_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_WORK_ITEM_SIZES );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_CLOCK_FREQUENCY );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_ADDRESS_BITS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_READ_IMAGE_ARGS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_WRITE_IMAGE_ARGS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_MEM_ALLOC_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_IMAGE2D_MAX_WIDTH );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_IMAGE2D_MAX_HEIGHT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_IMAGE3D_MAX_WIDTH );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_IMAGE3D_MAX_HEIGHT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_IMAGE3D_MAX_DEPTH );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_IMAGE_SUPPORT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_PARAMETER_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_SAMPLERS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MEM_BASE_ADDR_ALIGN );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SINGLE_FP_CONFIG );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_GLOBAL_MEM_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_CONSTANT_ARGS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_LOCAL_MEM_TYPE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_LOCAL_MEM_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_ERROR_CORRECTION_SUPPORT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PROFILING_TIMER_RESOLUTION );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_ENDIAN_LITTLE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_AVAILABLE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_COMPILER_AVAILABLE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_EXECUTION_CAPABILITIES );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_QUEUE_PROPERTIES );
    // Same value as CL_DEVICE_QUEUE_PROPERTIES:
    //CL_DEVICE_QUEUE_ON_HOST_PROPERTIES          0x102A
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NAME );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_VENDOR );
    ADD_ENUM_NAME( m_cl_int, CL_DRIVER_VERSION );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PROFILE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_VERSION );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_EXTENSIONS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PLATFORM );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_DOUBLE_FP_CONFIG );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_HALF_FP_CONFIG );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_HOST_UNIFIED_MEMORY );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_OPENCL_C_VERSION );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_LINKER_AVAILABLE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_BUILT_IN_KERNELS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_IMAGE_MAX_BUFFER_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_IMAGE_MAX_ARRAY_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARENT_DEVICE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_MAX_SUB_DEVICES );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_PROPERTIES );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_AFFINITY_DOMAIN );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_TYPE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_REFERENCE_COUNT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PREFERRED_INTEROP_USER_SYNC );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PRINTF_BUFFER_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_IMAGE_PITCH_ALIGNMENT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_READ_WRITE_IMAGE_ARGS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_GLOBAL_VARIABLE_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_QUEUE_ON_DEVICE_PROPERTIES );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_QUEUE_ON_DEVICE_PREFERRED_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_QUEUE_ON_DEVICE_MAX_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_ON_DEVICE_QUEUES );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_ON_DEVICE_EVENTS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SVM_CAPABILITIES );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_GLOBAL_VARIABLE_PREFERRED_TOTAL_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_PIPE_ARGS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PIPE_MAX_ACTIVE_RESERVATIONS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PIPE_MAX_PACKET_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PREFERRED_PLATFORM_ATOMIC_ALIGNMENT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PREFERRED_GLOBAL_ATOMIC_ALIGNMENT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PREFERRED_LOCAL_ATOMIC_ALIGNMENT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_IL_VERSION );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_NUM_SUB_GROUPS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SUB_GROUP_INDEPENDENT_FORWARD_PROGRESS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NUMERIC_VERSION );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_EXTENSIONS_WITH_VERSION );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_ILS_WITH_VERSION );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_ATOMIC_MEMORY_CAPABILITIES );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_ATOMIC_FENCE_CAPABILITIES );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NON_UNIFORM_WORK_GROUP_SUPPORT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_OPENCL_C_ALL_VERSIONS );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_MULTIPLE );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_WORK_GROUP_COLLECTIVE_FUNCTIONS_SUPPORT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_GENERIC_ADDRESS_SPACE_SUPPORT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_OPENCL_C_FEATURES );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_DEVICE_ENQUEUE_CAPABILITIES );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PIPE_SUPPORT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_LATEST_CONFORMANCE_VERSION_PASSED );

    /* cl_device_fp_config - bitfield */
    ADD_ENUM_NAME( m_cl_device_fp_config, CL_FP_DENORM );
    ADD_ENUM_NAME( m_cl_device_fp_config, CL_FP_INF_NAN );
    ADD_ENUM_NAME( m_cl_device_fp_config, CL_FP_ROUND_TO_NEAREST );
    ADD_ENUM_NAME( m_cl_device_fp_config, CL_FP_ROUND_TO_ZERO );
    ADD_ENUM_NAME( m_cl_device_fp_config, CL_FP_ROUND_TO_INF );
    ADD_ENUM_NAME( m_cl_device_fp_config, CL_FP_FMA );
    ADD_ENUM_NAME( m_cl_device_fp_config, CL_FP_SOFT_FLOAT );
    ADD_ENUM_NAME( m_cl_device_fp_config, CL_FP_CORRECTLY_ROUNDED_DIVIDE_SQRT );

    /* cl_device_mem_cache_type */
    ADD_ENUM_NAME( m_cl_device_mem_cache_type, CL_NONE );
    ADD_ENUM_NAME( m_cl_device_mem_cache_type, CL_READ_ONLY_CACHE );
    ADD_ENUM_NAME( m_cl_device_mem_cache_type, CL_READ_WRITE_CACHE );

    /* cl_device_local_mem_type */
    ADD_ENUM_NAME( m_cl_device_local_mem_type, CL_LOCAL );
    ADD_ENUM_NAME( m_cl_device_local_mem_type, CL_GLOBAL );

    /* cl_device_exec_capabilities - bitfield */
    ADD_ENUM_NAME( m_cl_device_exec_capabilities, CL_EXEC_KERNEL  );
    ADD_ENUM_NAME( m_cl_device_exec_capabilities, CL_EXEC_NATIVE_KERNEL );

    /* cl_command_queue_properties - bitfield */
    ADD_ENUM_NAME( m_cl_command_queue_properties, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE );
    ADD_ENUM_NAME( m_cl_command_queue_properties, CL_QUEUE_PROFILING_ENABLE );
    ADD_ENUM_NAME( m_cl_command_queue_properties, CL_QUEUE_ON_DEVICE );
    ADD_ENUM_NAME( m_cl_command_queue_properties, CL_QUEUE_ON_DEVICE_DEFAULT );

    /* cl_context_info  */
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_REFERENCE_COUNT );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_DEVICES );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_PROPERTIES );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_NUM_DEVICES );

    /* cl_context_properties */
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_PLATFORM );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_INTEROP_USER_SYNC );

    /* cl_device_partition_property */
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_EQUALLY );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_BY_COUNTS );
    //CL_DEVICE_PARTITION_BY_COUNTS_LIST_END      0x0
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN );

    /* cl_device_affinity_domain */
    ADD_ENUM_NAME( m_cl_device_affinity_domain, CL_DEVICE_AFFINITY_DOMAIN_NUMA );
    ADD_ENUM_NAME( m_cl_device_affinity_domain, CL_DEVICE_AFFINITY_DOMAIN_L4_CACHE );
    ADD_ENUM_NAME( m_cl_device_affinity_domain, CL_DEVICE_AFFINITY_DOMAIN_L3_CACHE );
    ADD_ENUM_NAME( m_cl_device_affinity_domain, CL_DEVICE_AFFINITY_DOMAIN_L2_CACHE );
    ADD_ENUM_NAME( m_cl_device_affinity_domain, CL_DEVICE_AFFINITY_DOMAIN_L1_CACHE );
    ADD_ENUM_NAME( m_cl_device_affinity_domain, CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE );

    /* cl_device_svm_capabilities */
    ADD_ENUM_NAME( m_cl_device_svm_capabilities, CL_DEVICE_SVM_COARSE_GRAIN_BUFFER );
    ADD_ENUM_NAME( m_cl_device_svm_capabilities, CL_DEVICE_SVM_FINE_GRAIN_BUFFER );
    ADD_ENUM_NAME( m_cl_device_svm_capabilities, CL_DEVICE_SVM_FINE_GRAIN_SYSTEM );
    ADD_ENUM_NAME( m_cl_device_svm_capabilities, CL_DEVICE_SVM_ATOMICS );

    /* cl_command_queue_info */
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_CONTEXT );
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_DEVICE );
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_REFERENCE_COUNT );
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_PROPERTIES );
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_DEVICE_DEFAULT );
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_PROPERTIES_ARRAY );

    /* cl_mem_flags - bitfield */
    ADD_ENUM_NAME( m_cl_mem_flags, CL_MEM_READ_WRITE );
    ADD_ENUM_NAME( m_cl_mem_flags, CL_MEM_WRITE_ONLY );
    ADD_ENUM_NAME( m_cl_mem_flags, CL_MEM_READ_ONLY );
    ADD_ENUM_NAME( m_cl_mem_flags, CL_MEM_USE_HOST_PTR );
    ADD_ENUM_NAME( m_cl_mem_flags, CL_MEM_ALLOC_HOST_PTR );
    ADD_ENUM_NAME( m_cl_mem_flags, CL_MEM_COPY_HOST_PTR );
    // reserved                                         (1 << 6)
    ADD_ENUM_NAME( m_cl_mem_flags, CL_MEM_HOST_WRITE_ONLY );
    ADD_ENUM_NAME( m_cl_mem_flags, CL_MEM_HOST_READ_ONLY );
    ADD_ENUM_NAME( m_cl_mem_flags, CL_MEM_HOST_NO_ACCESS );
    ADD_ENUM_NAME( m_cl_mem_flags, CL_MEM_KERNEL_READ_AND_WRITE );

    /* cl_mem_migration_flags - bitfield */
    ADD_ENUM_NAME( m_cl_mem_migration_flags, CL_MIGRATE_MEM_OBJECT_HOST );
    ADD_ENUM_NAME( m_cl_mem_migration_flags, CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED );

    /* cl_channel_order */
    ADD_ENUM_NAME( m_cl_int, CL_R );
    ADD_ENUM_NAME( m_cl_int, CL_A );
    ADD_ENUM_NAME( m_cl_int, CL_RG );
    ADD_ENUM_NAME( m_cl_int, CL_RA );
    ADD_ENUM_NAME( m_cl_int, CL_RGB );
    ADD_ENUM_NAME( m_cl_int, CL_RGBA );
    ADD_ENUM_NAME( m_cl_int, CL_BGRA );
    ADD_ENUM_NAME( m_cl_int, CL_ARGB );
    ADD_ENUM_NAME( m_cl_int, CL_INTENSITY );
    ADD_ENUM_NAME( m_cl_int, CL_LUMINANCE );
    ADD_ENUM_NAME( m_cl_int, CL_Rx );
    ADD_ENUM_NAME( m_cl_int, CL_RGx );
    ADD_ENUM_NAME( m_cl_int, CL_RGBx );
    ADD_ENUM_NAME( m_cl_int, CL_DEPTH );
    ADD_ENUM_NAME( m_cl_int, CL_DEPTH_STENCIL );
    ADD_ENUM_NAME( m_cl_int, CL_sRGB );
    ADD_ENUM_NAME( m_cl_int, CL_sRGBx );
    ADD_ENUM_NAME( m_cl_int, CL_sRGBA );
    ADD_ENUM_NAME( m_cl_int, CL_sBGRA );
    ADD_ENUM_NAME( m_cl_int, CL_ABGR );

    /* cl_channel_type */
    ADD_ENUM_NAME( m_cl_int, CL_SNORM_INT8 );
    ADD_ENUM_NAME( m_cl_int, CL_SNORM_INT16 );
    ADD_ENUM_NAME( m_cl_int, CL_UNORM_INT8 );
    ADD_ENUM_NAME( m_cl_int, CL_UNORM_INT16 );
    ADD_ENUM_NAME( m_cl_int, CL_UNORM_SHORT_565 );
    ADD_ENUM_NAME( m_cl_int, CL_UNORM_SHORT_555 );
    ADD_ENUM_NAME( m_cl_int, CL_UNORM_INT_101010 );
    ADD_ENUM_NAME( m_cl_int, CL_SIGNED_INT8 );
    ADD_ENUM_NAME( m_cl_int, CL_SIGNED_INT16 );
    ADD_ENUM_NAME( m_cl_int, CL_SIGNED_INT32 );
    ADD_ENUM_NAME( m_cl_int, CL_UNSIGNED_INT8 );
    ADD_ENUM_NAME( m_cl_int, CL_UNSIGNED_INT16 );
    ADD_ENUM_NAME( m_cl_int, CL_UNSIGNED_INT32 );
    ADD_ENUM_NAME( m_cl_int, CL_HALF_FLOAT );
    ADD_ENUM_NAME( m_cl_int, CL_FLOAT );
    ADD_ENUM_NAME( m_cl_int, CL_UNORM_INT24 );
    ADD_ENUM_NAME( m_cl_int, CL_UNORM_INT_101010_2 );

    /* cl_mem_object_type */
    ADD_ENUM_NAME( m_cl_int, CL_MEM_OBJECT_BUFFER );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_OBJECT_IMAGE2D );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_OBJECT_IMAGE3D );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_OBJECT_IMAGE2D_ARRAY );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_OBJECT_IMAGE1D );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_OBJECT_IMAGE1D_ARRAY );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_OBJECT_IMAGE1D_BUFFER );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_OBJECT_PIPE );

    /* cl_mem_info */
    ADD_ENUM_NAME( m_cl_int, CL_MEM_TYPE );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_FLAGS );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_HOST_PTR );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_MAP_COUNT );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_REFERENCE_COUNT );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_CONTEXT );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ASSOCIATED_MEMOBJECT );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_OFFSET );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_USES_SVM_POINTER );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_PROPERTIES );

    /* cl_image_info */
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_FORMAT );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_ELEMENT_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_ROW_PITCH );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_SLICE_PITCH );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_WIDTH );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_HEIGHT );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_DEPTH );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_ARRAY_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_BUFFER );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_NUM_MIP_LEVELS );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_NUM_SAMPLES );

    /* cl_pipe_info */
    ADD_ENUM_NAME( m_cl_int, CL_PIPE_PACKET_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_PIPE_MAX_PACKETS );
    ADD_ENUM_NAME( m_cl_int, CL_PIPE_PROPERTIES );

    /* cl_addressing_mode */
    ADD_ENUM_NAME( m_cl_int, CL_ADDRESS_NONE );
    ADD_ENUM_NAME( m_cl_int, CL_ADDRESS_CLAMP_TO_EDGE );
    ADD_ENUM_NAME( m_cl_int, CL_ADDRESS_CLAMP );
    ADD_ENUM_NAME( m_cl_int, CL_ADDRESS_REPEAT );
    ADD_ENUM_NAME( m_cl_int, CL_ADDRESS_MIRRORED_REPEAT );

    /* cl_filter_mode */
    ADD_ENUM_NAME( m_cl_int, CL_FILTER_NEAREST );
    ADD_ENUM_NAME( m_cl_int, CL_FILTER_LINEAR );

    /* cl_sampler_info */
    ADD_ENUM_NAME( m_cl_int, CL_SAMPLER_REFERENCE_COUNT );
    ADD_ENUM_NAME( m_cl_int, CL_SAMPLER_CONTEXT );
    ADD_ENUM_NAME( m_cl_int, CL_SAMPLER_NORMALIZED_COORDS );
    ADD_ENUM_NAME( m_cl_int, CL_SAMPLER_ADDRESSING_MODE );
    ADD_ENUM_NAME( m_cl_int, CL_SAMPLER_FILTER_MODE );
    ADD_ENUM_NAME( m_cl_int, CL_SAMPLER_MIP_FILTER_MODE );
    ADD_ENUM_NAME( m_cl_int, CL_SAMPLER_LOD_MIN );
    ADD_ENUM_NAME( m_cl_int, CL_SAMPLER_LOD_MAX );
    ADD_ENUM_NAME( m_cl_int, CL_SAMPLER_PROPERTIES );

    /* cl_map_flags - bitfield */
    ADD_ENUM_NAME( m_cl_map_flags, CL_MAP_READ );
    ADD_ENUM_NAME( m_cl_map_flags, CL_MAP_WRITE );
    ADD_ENUM_NAME( m_cl_map_flags, CL_MAP_WRITE_INVALIDATE_REGION );

    /* cl_program_info */
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_REFERENCE_COUNT );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_CONTEXT );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_NUM_DEVICES );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_DEVICES );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_SOURCE );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_BINARY_SIZES );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_BINARIES );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_NUM_KERNELS );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_KERNEL_NAMES );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_IL );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_SCOPE_GLOBAL_CTORS_PRESENT );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_SCOPE_GLOBAL_DTORS_PRESENT );

    /* cl_program_build_info */
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_BUILD_STATUS );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_BUILD_OPTIONS );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_BUILD_LOG );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_BINARY_TYPE );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_BUILD_GLOBAL_VARIABLE_TOTAL_SIZE );

    /* cl_program_binary_type */
    ADD_ENUM_NAME( m_cl_program_binary_type, CL_PROGRAM_BINARY_TYPE_NONE );
    ADD_ENUM_NAME( m_cl_program_binary_type, CL_PROGRAM_BINARY_TYPE_COMPILED_OBJECT );
    ADD_ENUM_NAME( m_cl_program_binary_type, CL_PROGRAM_BINARY_TYPE_LIBRARY );
    ADD_ENUM_NAME( m_cl_program_binary_type, CL_PROGRAM_BINARY_TYPE_EXECUTABLE );

    /* cl_build_status */
    ADD_ENUM_NAME( m_cl_build_status, CL_BUILD_SUCCESS );
    ADD_ENUM_NAME( m_cl_build_status, CL_BUILD_NONE );
    ADD_ENUM_NAME( m_cl_build_status, CL_BUILD_ERROR );
    ADD_ENUM_NAME( m_cl_build_status, CL_BUILD_IN_PROGRESS );

    /* cl_kernel_info */
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_FUNCTION_NAME );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_NUM_ARGS );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_REFERENCE_COUNT );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_CONTEXT );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_PROGRAM );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ATTRIBUTES );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_MAX_NUM_SUB_GROUPS );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_COMPILE_NUM_SUB_GROUPS );

    /* cl_kernel_arg_info */
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ARG_ADDRESS_QUALIFIER );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ARG_ACCESS_QUALIFIER );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ARG_TYPE_NAME );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ARG_TYPE_QUALIFIER );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ARG_NAME );

    /* cl_kernel_arg_address_qualifier */
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ARG_ADDRESS_GLOBAL );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ARG_ADDRESS_LOCAL );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ARG_ADDRESS_CONSTANT );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ARG_ADDRESS_PRIVATE );

    /* cl_kernel_arg_access_qualifier */
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ARG_ACCESS_READ_ONLY );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ARG_ACCESS_WRITE_ONLY );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ARG_ACCESS_READ_WRITE );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_ARG_ACCESS_NONE );

    /* cl_kernel_arg_type_qualifer */
    ADD_ENUM_NAME( m_cl_kernel_arg_type_qualifier, CL_KERNEL_ARG_TYPE_NONE );
    ADD_ENUM_NAME( m_cl_kernel_arg_type_qualifier, CL_KERNEL_ARG_TYPE_CONST );
    ADD_ENUM_NAME( m_cl_kernel_arg_type_qualifier, CL_KERNEL_ARG_TYPE_RESTRICT );
    ADD_ENUM_NAME( m_cl_kernel_arg_type_qualifier, CL_KERNEL_ARG_TYPE_VOLATILE );
    ADD_ENUM_NAME( m_cl_kernel_arg_type_qualifier, CL_KERNEL_ARG_TYPE_PIPE );

    /* cl_kernel_work_group_info */
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_WORK_GROUP_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_COMPILE_WORK_GROUP_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_LOCAL_MEM_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_PRIVATE_MEM_SIZE );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_GLOBAL_WORK_SIZE );

    /* cl_kernel_sub_group_info */
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_LOCAL_SIZE_FOR_SUB_GROUP_COUNT );

    /* cl_kernel_exec_info */
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_EXEC_INFO_SVM_PTRS );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_EXEC_INFO_SVM_FINE_GRAIN_SYSTEM );

    /* cl_event_info  */
    ADD_ENUM_NAME( m_cl_int, CL_EVENT_COMMAND_QUEUE );
    ADD_ENUM_NAME( m_cl_int, CL_EVENT_COMMAND_TYPE );
    ADD_ENUM_NAME( m_cl_int, CL_EVENT_REFERENCE_COUNT );
    ADD_ENUM_NAME( m_cl_int, CL_EVENT_COMMAND_EXECUTION_STATUS );
    ADD_ENUM_NAME( m_cl_int, CL_EVENT_CONTEXT );

    /* cl_command_type */
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_NDRANGE_KERNEL );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_TASK );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_NATIVE_KERNEL );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_READ_BUFFER );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_WRITE_BUFFER );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_COPY_BUFFER );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_READ_IMAGE );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_WRITE_IMAGE );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_COPY_IMAGE );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_COPY_IMAGE_TO_BUFFER );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_COPY_BUFFER_TO_IMAGE );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_MAP_BUFFER );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_MAP_IMAGE );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_UNMAP_MEM_OBJECT );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_MARKER );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_ACQUIRE_GL_OBJECTS );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_RELEASE_GL_OBJECTS );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_READ_BUFFER_RECT );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_WRITE_BUFFER_RECT );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_COPY_BUFFER_RECT );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_USER );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_BARRIER );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_MIGRATE_MEM_OBJECTS );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_FILL_BUFFER );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_FILL_IMAGE );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_SVM_FREE );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_SVM_MEMCPY );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_SVM_MEMFILL );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_SVM_MAP );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_SVM_UNMAP );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_SVM_MIGRATE_MEM );

    /* command execution status */
    ADD_ENUM_NAME( m_cl_command_exec_status, CL_COMPLETE );
    ADD_ENUM_NAME( m_cl_command_exec_status, CL_RUNNING );
    ADD_ENUM_NAME( m_cl_command_exec_status, CL_SUBMITTED );
    ADD_ENUM_NAME( m_cl_command_exec_status, CL_QUEUED );

    /* cl_buffer_create_type  */
    ADD_ENUM_NAME( m_cl_int, CL_BUFFER_CREATE_TYPE_REGION );

    /* cl_profiling_info  */
    ADD_ENUM_NAME( m_cl_int, CL_PROFILING_COMMAND_QUEUED );
    ADD_ENUM_NAME( m_cl_int, CL_PROFILING_COMMAND_SUBMIT );
    ADD_ENUM_NAME( m_cl_int, CL_PROFILING_COMMAND_START );
    ADD_ENUM_NAME( m_cl_int, CL_PROFILING_COMMAND_END );
    ADD_ENUM_NAME( m_cl_int, CL_PROFILING_COMMAND_COMPLETE );

    /* cl_svm_mem_flags */
    ADD_ENUM_NAME( m_cl_svm_mem_flags, CL_MEM_READ_WRITE );
    ADD_ENUM_NAME( m_cl_svm_mem_flags, CL_MEM_WRITE_ONLY );
    ADD_ENUM_NAME( m_cl_svm_mem_flags, CL_MEM_READ_ONLY );
    ADD_ENUM_NAME( m_cl_svm_mem_flags, CL_MEM_SVM_FINE_GRAIN_BUFFER );
    ADD_ENUM_NAME( m_cl_svm_mem_flags, CL_MEM_SVM_ATOMICS );

    /* cl_device_atomic_capabilities */
    ADD_ENUM_NAME( m_cl_device_atomic_capabilities, CL_DEVICE_ATOMIC_ORDER_RELAXED );
    ADD_ENUM_NAME( m_cl_device_atomic_capabilities, CL_DEVICE_ATOMIC_ORDER_ACQ_REL );
    ADD_ENUM_NAME( m_cl_device_atomic_capabilities, CL_DEVICE_ATOMIC_ORDER_SEQ_CST );
    ADD_ENUM_NAME( m_cl_device_atomic_capabilities, CL_DEVICE_ATOMIC_SCOPE_WORK_ITEM );
    ADD_ENUM_NAME( m_cl_device_atomic_capabilities, CL_DEVICE_ATOMIC_SCOPE_WORK_GROUP );
    ADD_ENUM_NAME( m_cl_device_atomic_capabilities, CL_DEVICE_ATOMIC_SCOPE_DEVICE );
    ADD_ENUM_NAME( m_cl_device_atomic_capabilities, CL_DEVICE_ATOMIC_SCOPE_ALL_DEVICES );

    /* cl_device_device_enqueue_capabilities */
    ADD_ENUM_NAME( m_cl_device_device_enqueue_capabilities, CL_DEVICE_QUEUE_SUPPORTED );
    ADD_ENUM_NAME( m_cl_device_device_enqueue_capabilities, CL_DEVICE_QUEUE_REPLACEABLE_DEFAULT );

    // cl_gl_object_type
    ADD_ENUM_NAME( m_cl_int, CL_GL_OBJECT_BUFFER );
    ADD_ENUM_NAME( m_cl_int, CL_GL_OBJECT_TEXTURE2D );
    ADD_ENUM_NAME( m_cl_int, CL_GL_OBJECT_TEXTURE3D );
    ADD_ENUM_NAME( m_cl_int, CL_GL_OBJECT_RENDERBUFFER );
    ADD_ENUM_NAME( m_cl_int, CL_GL_OBJECT_TEXTURE2D_ARRAY );
    ADD_ENUM_NAME( m_cl_int, CL_GL_OBJECT_TEXTURE1D );
    ADD_ENUM_NAME( m_cl_int, CL_GL_OBJECT_TEXTURE1D_ARRAY );
    ADD_ENUM_NAME( m_cl_int, CL_GL_OBJECT_TEXTURE_BUFFER );

    // cl_gl_texture_info
    ADD_ENUM_NAME( m_cl_int, CL_GL_TEXTURE_TARGET );
    ADD_ENUM_NAME( m_cl_int, CL_GL_MIPMAP_LEVEL );
    ADD_ENUM_NAME( m_cl_int, CL_GL_NUM_SAMPLES );

    // Error Code
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR );

    // cl_gl_context_info
    ADD_ENUM_NAME( m_cl_int, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICES_FOR_GL_CONTEXT_KHR );

    // cl_context_properties
    ADD_ENUM_NAME( m_cl_int, CL_GL_CONTEXT_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_EGL_DISPLAY_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_GLX_DISPLAY_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_WGL_HDC_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_CGL_SHAREGROUP_KHR );

    // Extensions

#if defined(_WIN32)
    // cl_khr_d3d10_sharing
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_D3D10_DEVICE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_D3D10_RESOURCE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR );

    // cl_khr_d3d10_sharing
    ADD_ENUM_NAME( m_cl_int, CL_D3D10_DEVICE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_D3D10_DXGI_ADAPTER_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_PREFERRED_DEVICES_FOR_D3D10_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_ALL_DEVICES_FOR_D3D10_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_D3D10_DEVICE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_D3D10_PREFER_SHARED_RESOURCES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_D3D10_RESOURCE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_D3D10_SUBRESOURCE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_ACQUIRE_D3D10_OBJECTS_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_RELEASE_D3D10_OBJECTS_KHR );
#endif

#if defined(_WIN32)
    // cl_khr_d3d11_sharing
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_D3D11_DEVICE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_D3D11_RESOURCE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_D3D11_RESOURCE_ALREADY_ACQUIRED_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_D3D11_RESOURCE_NOT_ACQUIRED_KHR );

    // cl_khr_d3d11_sharing
    ADD_ENUM_NAME( m_cl_int, CL_D3D11_DEVICE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_D3D11_DXGI_ADAPTER_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_PREFERRED_DEVICES_FOR_D3D11_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_ALL_DEVICES_FOR_D3D11_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_D3D11_DEVICE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_D3D11_PREFER_SHARED_RESOURCES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_D3D11_RESOURCE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_D3D11_SUBRESOURCE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_ACQUIRE_D3D11_OBJECTS_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_RELEASE_D3D11_OBJECTS_KHR );
#endif

    // cl_khr_device_uuid
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_UUID_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_DRIVER_UUID_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_LUID_VALID_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_LUID_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NODE_MASK_KHR );

#if defined(_WIN32)
    // cl_khr_dx9_media_sharing
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_DX9_MEDIA_ADAPTER_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_DX9_MEDIA_SURFACE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_DX9_MEDIA_SURFACE_ALREADY_ACQUIRED_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_DX9_MEDIA_SURFACE_NOT_ACQUIRED_KHR );

    // cl_khr_dx9_media_sharing
    ADD_ENUM_NAME( m_cl_int, CL_ADAPTER_D3D9_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_ADAPTER_D3D9EX_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_ADAPTER_DXVA_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_PREFERRED_DEVICES_FOR_DX9_MEDIA_ADAPTER_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_ALL_DEVICES_FOR_DX9_MEDIA_ADAPTER_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_ADAPTER_D3D9_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_ADAPTER_D3D9EX_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_ADAPTER_DXVA_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_DX9_MEDIA_ADAPTER_TYPE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_DX9_MEDIA_SURFACE_INFO_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_DX9_MEDIA_PLANE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_ACQUIRE_DX9_MEDIA_SURFACES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_RELEASE_DX9_MEDIA_SURFACES_KHR );
#endif

    // cl_khr_command_buffer
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_COMMAND_BUFFER_CAPABILITIES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_COMMAND_BUFFER_REQUIRED_QUEUE_PROPERTIES_KHR );

    ADD_ENUM_NAME( m_cl_device_command_buffer_capabilities_khr, CL_COMMAND_BUFFER_CAPABILITY_KERNEL_PRINTF_KHR );
    ADD_ENUM_NAME( m_cl_device_command_buffer_capabilities_khr, CL_COMMAND_BUFFER_CAPABILITY_DEVICE_SIDE_ENQUEUE_KHR );
    ADD_ENUM_NAME( m_cl_device_command_buffer_capabilities_khr, CL_COMMAND_BUFFER_CAPABILITY_SIMULTANEOUS_USE_KHR );
    ADD_ENUM_NAME( m_cl_device_command_buffer_capabilities_khr, CL_COMMAND_BUFFER_CAPABILITY_OUT_OF_ORDER_KHR );

    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_BUFFER_FLAGS_KHR );

    ADD_ENUM_NAME( m_cl_command_buffer_flags_khr, CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR )

    ADD_ENUM_NAME( m_cl_int, CL_INVALID_COMMAND_BUFFER_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_SYNC_POINT_WAIT_LIST_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_INCOMPATIBLE_COMMAND_QUEUE_KHR );

    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_BUFFER_QUEUES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_BUFFER_NUM_QUEUES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_BUFFER_REFERENCE_COUNT_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_BUFFER_STATE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_BUFFER_PROPERTIES_ARRAY_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_BUFFER_CONTEXT_KHR );

    //CL_COMMAND_BUFFER_STATE_RECORDING_KHR               0
    //CL_COMMAND_BUFFER_STATE_EXECUTABLE_KHR              1
    //CL_COMMAND_BUFFER_STATE_PENDING_KHR                 2
    //CL_COMMAND_BUFFER_STATE_INVALID_KHR                 3

    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_COMMAND_BUFFER_KHR );

    // cl_khr_command_buffer_multi_device
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_COMMAND_BUFFER_CAPABILITIES_KHR );

    ADD_ENUM_NAME( m_cl_platform_command_buffer_capabilities_khr, CL_COMMAND_BUFFER_PLATFORM_UNIVERSAL_SYNC_KHR );
    ADD_ENUM_NAME( m_cl_platform_command_buffer_capabilities_khr, CL_COMMAND_BUFFER_PLATFORM_REMAP_QUEUES_KHR );
    ADD_ENUM_NAME( m_cl_platform_command_buffer_capabilities_khr, CL_COMMAND_BUFFER_PLATFORM_AUTOMATIC_REMAP_KHR );

    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_COMMAND_BUFFER_NUM_SYNC_DEVICES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_COMMAND_BUFFER_SYNC_DEVICES_KHR );

    ADD_ENUM_NAME( m_cl_device_command_buffer_capabilities_khr, CL_COMMAND_BUFFER_CAPABILITY_MULTIPLE_QUEUE_KHR );

    ADD_ENUM_NAME( m_cl_command_buffer_flags_khr, CL_COMMAND_BUFFER_DEVICE_SIDE_SYNC_KHR );

    // cl_khr_command_buffer_mutable_dispatch
    ADD_ENUM_NAME( m_cl_command_buffer_flags_khr, CL_COMMAND_BUFFER_MUTABLE_KHR );

    ADD_ENUM_NAME( m_cl_int, CL_INVALID_MUTABLE_COMMAND_KHR );

    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MUTABLE_DISPATCH_CAPABILITIES_KHR );

    ADD_ENUM_NAME( m_cl_int, CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR );
    ADD_ENUM_NAME( m_cl_mutable_dispatch_fields_khr, CL_MUTABLE_DISPATCH_GLOBAL_OFFSET_KHR );
    ADD_ENUM_NAME( m_cl_mutable_dispatch_fields_khr, CL_MUTABLE_DISPATCH_GLOBAL_SIZE_KHR );
    ADD_ENUM_NAME( m_cl_mutable_dispatch_fields_khr, CL_MUTABLE_DISPATCH_LOCAL_SIZE_KHR );
    ADD_ENUM_NAME( m_cl_mutable_dispatch_fields_khr, CL_MUTABLE_DISPATCH_ARGUMENTS_KHR );
    ADD_ENUM_NAME( m_cl_mutable_dispatch_fields_khr, CL_MUTABLE_DISPATCH_EXEC_INFO_KHR );

    ADD_ENUM_NAME( m_cl_int, CL_MUTABLE_COMMAND_COMMAND_QUEUE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_MUTABLE_COMMAND_COMMAND_BUFFER_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_MUTABLE_COMMAND_COMMAND_TYPE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_MUTABLE_DISPATCH_PROPERTIES_ARRAY_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_MUTABLE_DISPATCH_KERNEL_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_MUTABLE_DISPATCH_DIMENSIONS_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_MUTABLE_DISPATCH_GLOBAL_WORK_OFFSET_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_MUTABLE_DISPATCH_GLOBAL_WORK_SIZE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_MUTABLE_DISPATCH_LOCAL_WORK_SIZE_KHR );

    ADD_ENUM_NAME( m_cl_command_buffer_structure_type_khr, CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR );
    ADD_ENUM_NAME( m_cl_command_buffer_structure_type_khr, CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR );

    // cl_khr_extended_versioning extension
    // Most enums for this extension were added to OpenCL 3.0.
    //CL_PLATFORM_NUMERIC_VERSION_KHR                  0x0906
    //CL_PLATFORM_EXTENSIONS_WITH_VERSION_KHR          0x0907
    //CL_DEVICE_NUMERIC_VERSION_KHR                    0x105E
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_OPENCL_C_NUMERIC_VERSION_KHR );
    //CL_DEVICE_EXTENSIONS_WITH_VERSION_KHR            0x1060
    //CL_DEVICE_ILS_WITH_VERSION_KHR                   0x1061
    //CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION_KHR      0x1062

    // cl_khr_external_memory
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_HANDLE_LIST_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_ACQUIRE_EXTERNAL_MEM_OBJECTS_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_RELEASE_EXTERNAL_MEM_OBJECTS_KHR );

    // cl_khr_external_memory_dma_buf
    ADD_ENUM_NAME( m_cl_int, CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR );

    // cl_khr_external_memory_dx
    ADD_ENUM_NAME( m_cl_int, CL_EXTERNAL_MEMORY_HANDLE_D3D11_TEXTURE_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_EXTERNAL_MEMORY_HANDLE_D3D11_TEXTURE_KMT_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_EXTERNAL_MEMORY_HANDLE_D3D12_HEAP_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_EXTERNAL_MEMORY_HANDLE_D3D12_RESOURCE_KHR );

    // cl_khr_external_memory_opaque_fd
    ADD_ENUM_NAME( m_cl_int, CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR );

    // cl_khr_external_memory_win32
    ADD_ENUM_NAME( m_cl_int, CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KMT_KHR );

    // cl_khr_external_semaphore
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_SEMAPHORE_IMPORT_HANDLE_TYPES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SEMAPHORE_IMPORT_HANDLE_TYPES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR );

    // cl_khr_external_semaphore_dx_fence
    ADD_ENUM_NAME( m_cl_int, CL_SEMAPHORE_HANDLE_D3D12_FENCE_KHR );

    // cl_khr_external_semaphore_opaque_fd 1
    ADD_ENUM_NAME( m_cl_int, CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR );

    // cl_khr_external_semaphore_sync_fd
    ADD_ENUM_NAME( m_cl_int, CL_SEMAPHORE_HANDLE_SYNC_FD_KHR );

    // cl_khr_external_semaphore_win32
    ADD_ENUM_NAME( m_cl_int, CL_SEMAPHORE_HANDLE_OPAQUE_WIN32_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_SEMAPHORE_HANDLE_OPAQUE_WIN32_KMT_KHR );

    // cl_khr_gl_event
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_GL_FENCE_SYNC_OBJECT_KHR );

    // cl_khr_icd
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_ICD_SUFFIX_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_NOT_FOUND_KHR );

    // cl_khr_il_program
    // These enums are core in OpenCL 2.1.
    //CL_DEVICE_IL_VERSION_KHR                        0x105B
    //CL_PROGRAM_IL_KHR                               0x1169

    // cl_khr_initalize_memory
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_MEMORY_INITIALIZE_KHR );

    // cl_khr_integer_dot_product
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_INTEGER_DOT_PRODUCT_CAPABILITIES_KHR );

    // cl_khr_pci_bus_info extension
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PCI_BUS_INFO_KHR );

    // cl_khr_priority_hints extension
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_PRIORITY_KHR );

    // cl_khr_semaphore
    ADD_ENUM_NAME( m_cl_int, CL_PLATFORM_SEMAPHORE_TYPES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SEMAPHORE_TYPES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_SEMAPHORE_CONTEXT_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_SEMAPHORE_REFERENCE_COUNT_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_SEMAPHORE_PROPERTIES_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_SEMAPHORE_PAYLOAD_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_SEMAPHORE_TYPE_KHR );
    // Shared with cl_khr_external_memory:
    //CL_DEVICE_HANDLE_LIST_KHR                      0x2051
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_SEMAPHORE_WAIT_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_SEMAPHORE_SIGNAL_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_SEMAPHORE_KHR );

    ADD_ENUM_NAME( m_cl_semaphore_type_khr, CL_SEMAPHORE_TYPE_BINARY_KHR );

    // cl_khr_spir
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SPIR_VERSIONS );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_BINARY_TYPE_INTERMEDIATE );

    // cl_khr_subgroups
    // These enums were promoted to core in OpenCL 2.1.
    //CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE_KHR    0x2033
    //CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE_KHR       0x2034

    // cl_khr_terminate_context
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_TERMINATE_CAPABILITY_KHR );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_TERMINATE_KHR );

    // cl_khr_throttle_hints extension
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_THROTTLE_KHR );

    // cl_ext_atomic_counters
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_ATOMIC_COUNTERS_EXT );

    // cl_ext_cxx_for_opencl
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_CXX_FOR_OPENCL_NUMERIC_VERSION_EXT );

    // cl_ext_device_fission
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_EQUALLY_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_BY_COUNTS_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_BY_NAMES_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARENT_DEVICE_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_TYPES_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_AFFINITY_DOMAINS_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_REFERENCE_COUNT_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_STYLE_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PARTITION_FAILED_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_PARTITION_COUNT_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_PARTITION_NAME_EXT );

    // cl_ext_float_atomics
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SINGLE_FP_ATOMIC_CAPABILITIES_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_DOUBLE_FP_ATOMIC_CAPABILITIES_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_HALF_FP_ATOMIC_CAPABILITIES_EXT );

    // cl_ext_image_from_buffer
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_REQUIREMENTS_SLICE_PITCH_ALIGNMENT_EXT );

    // cl_ext_image_requirements_info
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_REQUIREMENTS_ROW_PITCH_ALIGNMENT_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_REQUIREMENTS_BASE_ADDRESS_ALIGNMENT_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_REQUIREMENTS_SIZE_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_REQUIREMENTS_MAX_WIDTH_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_REQUIREMENTS_MAX_HEIGHT_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_REQUIREMENTS_MAX_DEPTH_EXT );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_REQUIREMENTS_MAX_ARRAY_SIZE_EXT );

    // cl_altera_compiler_mode
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_COMPILER_MODE_ALTERA );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_PROGRAM_EXE_LIBRARY_ROOT_ALTERA );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_OFFLINE_DEVICE_ALTERA );

    // cl_altera_device_temperature
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_CORE_TEMPERATURE_ALTERA );

    // cl_amd_device_attribute_query
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_MAX_WORK_GROUP_SIZE_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PREFERRED_CONSTANT_BUFFER_SIZE_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PCIE_ID_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PROFILING_TIMER_OFFSET_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_TOPOLOGY_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_BOARD_NAME_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_GLOBAL_FREE_MEMORY_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SIMD_WIDTH_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SIMD_INSTRUCTION_WIDTH_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_WAVEFRONT_WIDTH_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_GLOBAL_MEM_CHANNELS_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_GLOBAL_MEM_CHANNEL_BANKS_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_GLOBAL_MEM_CHANNEL_BANK_WIDTH_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_LOCAL_MEM_SIZE_PER_COMPUTE_UNIT_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_LOCAL_MEM_BANKS_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_THREAD_TRACE_SUPPORTED_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_GFXIP_MAJOR_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_GFXIP_MINOR_AMD );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_AVAILABLE_ASYNC_QUEUES_AMD );

    // cl_amd_offline_devices
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_OFFLINE_DEVICES_AMD );

    // cl_arm_get_core_id
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_COMPUTE_UNITS_BITFIELD_ARM );

    // cl_arm_job_slot_selection
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_JOB_SLOTS_ARM );
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_JOB_SLOT_ARM );

    // cl_arm_printf extension
    ADD_ENUM_NAME( m_cl_int, CL_PRINTF_CALLBACK_ARM );
    ADD_ENUM_NAME( m_cl_int, CL_PRINTF_BUFFERSIZE_ARM );

    // cl_arm_scheduling_controls
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SCHEDULING_CONTROLS_CAPABILITIES_ARM );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_EXEC_INFO_WORKGROUP_BATCH_SIZE_ARM );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_EXEC_INFO_WORKGROUP_BATCH_SIZE_MODIFIER_ARM );
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_KERNEL_BATCHING_ARM );
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_DEFERRED_FLUSH_ARM );

    // cl_intel_accelerator
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_ACCELERATOR_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_ACCELERATOR_TYPE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_ACCELERATOR_DESC_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_ACCELERATOR_TYPE_NOT_SUPPORTED_INTEL );

    // cl_intel_accelerator
    ADD_ENUM_NAME( m_cl_int, CL_ACCELERATOR_DESCRIPTOR_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_ACCELERATOR_REFERENCE_COUNT_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_ACCELERATOR_CONTEXT_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_ACCELERATOR_TYPE_INTEL );

    // cl_intel_advanced_motion_estimation
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_ME_VERSION_INTEL );

    // cl_intel_command_queue_families
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_QUEUE_FAMILY_PROPERTIES_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_FAMILY_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_INDEX_INTEL );

    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_CREATE_SINGLE_QUEUE_EVENTS_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_CREATE_CROSS_QUEUE_EVENTS_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_SINGLE_QUEUE_EVENT_WAIT_LIST_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_CROSS_QUEUE_EVENT_WAIT_LIST_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_TRANSFER_BUFFER_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_TRANSFER_BUFFER_RECT_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_MAP_BUFFER_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_FILL_BUFFER_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_TRANSFER_IMAGE_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_MAP_IMAGE_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_FILL_IMAGE_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_TRANSFER_BUFFER_IMAGE_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_TRANSFER_IMAGE_BUFFER_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_MARKER_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_BARRIER_INTEL );
    ADD_ENUM_NAME( m_cl_command_queue_capabilities_intel, CL_QUEUE_CAPABILITY_KERNEL_INTEL );

    // cl_intel_device_attribute_query
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_IP_VERSION_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_ID_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NUM_SLICES_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NUM_SUB_SLICES_PER_SLICE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NUM_EUS_PER_SUB_SLICE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NUM_THREADS_PER_EU_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_FEATURE_CAPABILITIES_INTEL );

    // cl_intel_device_side_avc_motion_estimation (partial)
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_AVC_ME_VERSION_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_AVC_ME_SUPPORTS_TEXTURE_SAMPLER_USE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_AVC_ME_SUPPORTS_PREEMPTION_INTEL );

    // cl_intel_driver_diagnostics
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_SHOW_DIAGNOSTICS_INTEL );

#if defined(_WIN32)
    // cl_intel_dx9_media_sharing
    // These error codes are shared with cl_khr_dx9_media_sharing.
    //CL_INVALID_DX9_DEVICE_INTEL                   -1010
    //CL_INVALID_DX9_RESOURCE_INTEL                 -1011
    //CL_DX9_RESOURCE_ALREADY_ACQUIRED_INTEL        -1012
    //CL_DX9_RESOURCE_NOT_ACQUIRED_INTEL            -1013

    // cl_intel_dx9_media_sharing
    ADD_ENUM_NAME( m_cl_int, CL_D3D9_DEVICE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_D3D9EX_DEVICE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DXVA_DEVICE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_PREFERRED_DEVICES_FOR_DX9_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_ALL_DEVICES_FOR_DX9_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_D3D9_DEVICE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_D3D9EX_DEVICE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_DXVA_DEVICE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_DX9_RESOURCE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_DX9_SHARED_HANDLE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_DX9_PLANE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_ACQUIRE_DX9_OBJECTS_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_RELEASE_DX9_OBJECTS_INTEL );
#endif

    // cl_intel_egl_image_yuv
    ADD_ENUM_NAME( m_cl_int, CL_EGL_YUV_PLANE_INTEL );

    // cl_intel_mem_channel_property
    ADD_ENUM_NAME( m_cl_int, CL_MEM_CHANNEL_INTEL );

    // cl_intel_mem_force_host_memory
    ADD_ENUM_NAME( m_cl_mem_flags, CL_MEM_FORCE_HOST_MEMORY_INTEL );

    // cl_intel_motion_estimation

    //CL_ACCELERATOR_TYPE_MOTION_ESTIMATION_INTEL     0x0

    //CL_ME_MB_TYPE_16x16_INTEL                       0x0
    //CL_ME_MB_TYPE_8x8_INTEL                         0x1
    //CL_ME_MB_TYPE_4x4_INTEL                         0x2

    //CL_ME_SUBPIXEL_MODE_INTEGER_INTEL               0x0
    //CL_ME_SUBPIXEL_MODE_HPEL_INTEL                  0x1
    //CL_ME_SUBPIXEL_MODE_QPEL_INTEL                  0x2

    //CL_ME_SAD_ADJUST_MODE_NONE_INTEL                0x0
    //CL_ME_SAD_ADJUST_MODE_HAAR_INTEL                0x1

    //CL_ME_SEARCH_PATH_RADIUS_2_2_INTEL              0x0
    //CL_ME_SEARCH_PATH_RADIUS_4_4_INTEL              0x1
    //CL_ME_SEARCH_PATH_RADIUS_16_12_INTEL            0x5

    //CL_ME_CHROMA_INTRA_PREDICT_ENABLED_INTEL        0x1
    //CL_ME_LUMA_INTRA_PREDICT_ENABLED_INTEL          0x2

    //CL_ME_COST_PENALTY_NONE_INTEL                   0x0
    //CL_ME_COST_PENALTY_LOW_INTEL                    0x1
    //CL_ME_COST_PENALTY_NORMAL_INTEL                 0x2
    //CL_ME_COST_PENALTY_HIGH_INTEL                   0x3

    //CL_ME_COST_PRECISION_QPEL_INTEL                 0x0
    //CL_ME_COST_PRECISION_HPEL_INTEL                 0x1
    //CL_ME_COST_PRECISION_PEL_INTEL                  0x2
    //CL_ME_COST_PRECISION_DPEL_INTEL                 0x3

    //CL_ME_VERSION_LEGACY_INTEL                      0x0
    //CL_ME_VERSION_ADVANCED_VER_1_INTEL              0x1

    // cl_intel_packed_yuv

    ADD_ENUM_NAME( m_cl_int, CL_YUYV_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_UYVY_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_YVYU_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_VYUY_INTEL );

    // cl_intel_planar_yuv

    ADD_ENUM_NAME( m_cl_int, CL_NV12_INTEL );

    ADD_ENUM_NAME( m_cl_mem_flags, CL_MEM_NO_ACCESS_INTEL );
    ADD_ENUM_NAME( m_cl_mem_flags, CL_MEM_ACCESS_FLAGS_UNRESTRICTED_INTEL );

    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PLANAR_YUV_MAX_WIDTH_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PLANAR_YUV_MAX_HEIGHT_INTEL );

    // cl_intel_required_subgroup_size
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SUB_GROUP_SIZES_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_SPILL_MEM_SIZE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_COMPILE_SUB_GROUP_SIZE_INTEL );

    // cl_intel_simultaneous_sharing
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SIMULTANEOUS_INTEROPS_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_NUM_SIMULTANEOUS_INTEROPS_INTEL );

    // cl_intel_thread_local_exec
    ADD_ENUM_NAME( m_cl_command_queue_properties, CL_QUEUE_THREAD_LOCAL_EXEC_ENABLE_INTEL );

    // cl_intel_unified_shared_memory
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_DEVICE_MEM_CAPABILITIES_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_CROSS_DEVICE_SHARED_MEM_CAPABILITIES_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_SHARED_SYSTEM_MEM_CAPABILITIES_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ALLOC_FLAGS_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ALLOC_TYPE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ALLOC_BASE_PTR_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ALLOC_SIZE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ALLOC_DEVICE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ALLOC_INFO_TBD0_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ALLOC_INFO_TBD1_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_TYPE_UNKNOWN_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_TYPE_HOST_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_TYPE_DEVICE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_TYPE_SHARED_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ADVICE_TBD0_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ADVICE_TBD1_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ADVICE_TBD2_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ADVICE_TBD3_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ADVICE_TBD4_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ADVICE_TBD5_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ADVICE_TBD6_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ADVICE_TBD7_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_MEMFILL_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_MEMCPY_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_MIGRATEMEM_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_MEMADVISE_INTEL );

    ADD_ENUM_NAME( m_cl_device_unified_shared_memory_capabilities_intel, CL_UNIFIED_SHARED_MEMORY_ACCESS_INTEL );
    ADD_ENUM_NAME( m_cl_device_unified_shared_memory_capabilities_intel, CL_UNIFIED_SHARED_MEMORY_ATOMIC_ACCESS_INTEL );
    ADD_ENUM_NAME( m_cl_device_unified_shared_memory_capabilities_intel, CL_UNIFIED_SHARED_MEMORY_CONCURRENT_ACCESS_INTEL );
    ADD_ENUM_NAME( m_cl_device_unified_shared_memory_capabilities_intel, CL_UNIFIED_SHARED_MEMORY_CONCURRENT_ATOMIC_ACCESS_INTEL );

    ADD_ENUM_NAME( m_cl_mem_alloc_flags_intel, CL_MEM_ALLOC_WRITE_COMBINED_INTEL );

    // cl_intel_va_api_media_sharing

    ADD_ENUM_NAME( m_cl_int, CL_VA_API_DISPLAY_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_PREFERRED_DEVICES_FOR_VA_API_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_ALL_DEVICES_FOR_VA_API_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_VA_API_DISPLAY_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_VA_API_SURFACE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_VA_API_PLANE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_ACQUIRE_VA_API_MEDIA_SURFACES_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_COMMAND_RELEASE_VA_API_MEDIA_SURFACES_INTEL );

    ADD_ENUM_NAME( m_cl_int, CL_INVALID_VA_API_MEDIA_ADAPTER_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_INVALID_VA_API_MEDIA_SURFACE_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_VA_API_MEDIA_SURFACE_ALREADY_ACQUIRED_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_VA_API_MEDIA_SURFACE_NOT_ACQUIRED_INTEL );

    // cl_nv_create_buffer
    ADD_ENUM_NAME( m_cl_mem_flags_NV, CL_MEM_LOCATION_HOST_NV );
    ADD_ENUM_NAME( m_cl_mem_flags_NV, CL_MEM_PINNED_NV );

    // cl_nv_device_attribute_query
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_REGISTERS_PER_BLOCK_NV );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_WARP_SIZE_NV );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_GPU_OVERLAP_NV );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_KERNEL_EXEC_TIMEOUT_NV );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_INTEGRATED_MEMORY_NV );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_ATTRIBUTE_ASYNC_ENGINE_COUNT_NV );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PCI_BUS_ID_NV );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PCI_SLOT_ID_NV );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PCI_DOMAIN_ID_NV );

    // cl_qcom_ext_host_ptr extension
    ADD_ENUM_NAME( m_cl_mem_flags, CL_MEM_EXT_HOST_PTR_QCOM );

    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_EXT_MEM_PADDING_IN_BYTES_QCOM );
    ADD_ENUM_NAME( m_cl_int, CL_DEVICE_PAGE_SIZE_QCOM );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_ROW_ALIGNMENT_QCOM );
    ADD_ENUM_NAME( m_cl_int, CL_IMAGE_SLICE_ALIGNMENT_QCOM );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_HOST_UNCACHED_QCOM );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_HOST_WRITEBACK_QCOM );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_HOST_WRITETHROUGH_QCOM );
    ADD_ENUM_NAME( m_cl_int, CL_MEM_HOST_WRITE_COMBINING_QCOM );

    // cl_qcom_ion_host_ptr extension
    ADD_ENUM_NAME( m_cl_int, CL_MEM_ION_HOST_PTR_QCOM );

    // Unofficial MDAPI extension:
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_MDAPI_PROPERTIES_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_QUEUE_MDAPI_CONFIGURATION_INTEL );

    // Unofficial kernel profiling extension:
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_KERNEL_PROFILING_MODES_COUNT_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_CONTEXT_KERNEL_PROFILING_MODE_INFO_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_IL_SYMBOLS_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_BINARY_PROGRAM_INTEL );

    // Unofficial extension (for now) for VTune Debug Info:
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_DEBUG_INFO_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_PROGRAM_DEBUG_INFO_SIZES_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_BINARIES_INTEL );
    ADD_ENUM_NAME( m_cl_int, CL_KERNEL_BINARY_SIZES_INTEL );

#if !defined(__ANDROID__) && !defined(__APPLE__)
    // gl texture targets
    ADD_ENUM_NAME( m_GLenum, GL_TEXTURE_BUFFER );
    ADD_ENUM_NAME( m_GLenum, GL_TEXTURE_1D );
    ADD_ENUM_NAME( m_GLenum, GL_TEXTURE_1D_ARRAY );
    ADD_ENUM_NAME( m_GLenum, GL_TEXTURE_2D );
    ADD_ENUM_NAME( m_GLenum, GL_TEXTURE_2D_ARRAY );
    ADD_ENUM_NAME( m_GLenum, GL_TEXTURE_3D );
    ADD_ENUM_NAME( m_GLenum, GL_TEXTURE_CUBE_MAP_POSITIVE_X );
    ADD_ENUM_NAME( m_GLenum, GL_TEXTURE_CUBE_MAP_POSITIVE_Y );
    ADD_ENUM_NAME( m_GLenum, GL_TEXTURE_CUBE_MAP_POSITIVE_Z );
    ADD_ENUM_NAME( m_GLenum, GL_TEXTURE_CUBE_MAP_NEGATIVE_X );
    ADD_ENUM_NAME( m_GLenum, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y );
    ADD_ENUM_NAME( m_GLenum, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z );
    ADD_ENUM_NAME( m_GLenum, GL_TEXTURE_RECTANGLE );

    // gl texture formats
    ADD_ENUM_NAME( m_GLenum, GL_ALPHA );
    ADD_ENUM_NAME( m_GLenum, GL_RGB );
    ADD_ENUM_NAME( m_GLenum, GL_RGBA );
    ADD_ENUM_NAME( m_GLenum, GL_RGBA32F );
    ADD_ENUM_NAME( m_GLenum, GL_RGB32F );
    ADD_ENUM_NAME( m_GLenum, GL_RGBA16F );
    ADD_ENUM_NAME( m_GLenum, GL_RGB16F );
    ADD_ENUM_NAME( m_GLenum, GL_RGBA32UI );
    ADD_ENUM_NAME( m_GLenum, GL_RGB32UI );
    ADD_ENUM_NAME( m_GLenum, GL_RGBA16UI );
    ADD_ENUM_NAME( m_GLenum, GL_RGB16UI );
    ADD_ENUM_NAME( m_GLenum, GL_RGBA8UI );
    ADD_ENUM_NAME( m_GLenum, GL_RGB8UI );
    ADD_ENUM_NAME( m_GLenum, GL_RGBA32I );
    ADD_ENUM_NAME( m_GLenum, GL_RGB32I );
    ADD_ENUM_NAME( m_GLenum, GL_RGBA16I );
    ADD_ENUM_NAME( m_GLenum, GL_RGB16I );
    ADD_ENUM_NAME( m_GLenum, GL_RGBA8I );
    ADD_ENUM_NAME( m_GLenum, GL_RGB8I );
    ADD_ENUM_NAME( m_GLenum, GL_RG );
    ADD_ENUM_NAME( m_GLenum, GL_R8 );
    ADD_ENUM_NAME( m_GLenum, GL_R16 );
    ADD_ENUM_NAME( m_GLenum, GL_RG8 );
    ADD_ENUM_NAME( m_GLenum, GL_RG16 );
    ADD_ENUM_NAME( m_GLenum, GL_R16F );
    ADD_ENUM_NAME( m_GLenum, GL_R32F );
    ADD_ENUM_NAME( m_GLenum, GL_RG16F );
    ADD_ENUM_NAME( m_GLenum, GL_RG32F );
    ADD_ENUM_NAME( m_GLenum, GL_R8I );
    ADD_ENUM_NAME( m_GLenum, GL_R8UI );
    ADD_ENUM_NAME( m_GLenum, GL_R16I );
    ADD_ENUM_NAME( m_GLenum, GL_R16UI );
    ADD_ENUM_NAME( m_GLenum, GL_R32I );
    ADD_ENUM_NAME( m_GLenum, GL_R32UI );
    ADD_ENUM_NAME( m_GLenum, GL_RG8I );
    ADD_ENUM_NAME( m_GLenum, GL_RG8UI );
    ADD_ENUM_NAME( m_GLenum, GL_RG16I );
    ADD_ENUM_NAME( m_GLenum, GL_RG16UI );
    ADD_ENUM_NAME( m_GLenum, GL_RG32I );
    ADD_ENUM_NAME( m_GLenum, GL_RG32UI );
#endif
}
