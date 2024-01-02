/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
// cl_khr_command_buffer

// Note: This implements the provisional extension v0.9.2.

typedef cl_bitfield         cl_device_command_buffer_capabilities_khr;
typedef struct _cl_command_buffer_khr* cl_command_buffer_khr;
typedef cl_uint             cl_sync_point_khr;
typedef cl_uint             cl_command_buffer_info_khr;
typedef cl_uint             cl_command_buffer_state_khr;
typedef cl_properties       cl_command_buffer_properties_khr;
typedef cl_bitfield         cl_command_buffer_flags_khr;
typedef cl_properties       cl_ndrange_kernel_command_properties_khr;
typedef struct _cl_mutable_command_khr* cl_mutable_command_khr;

#define CL_DEVICE_COMMAND_BUFFER_CAPABILITIES_KHR           0x12A9
#define CL_DEVICE_COMMAND_BUFFER_REQUIRED_QUEUE_PROPERTIES_KHR 0x12AA

#define CL_COMMAND_BUFFER_CAPABILITY_KERNEL_PRINTF_KHR      (1 << 0)
#define CL_COMMAND_BUFFER_CAPABILITY_DEVICE_SIDE_ENQUEUE_KHR (1 << 1)
#define CL_COMMAND_BUFFER_CAPABILITY_SIMULTANEOUS_USE_KHR   (1 << 2)
#define CL_COMMAND_BUFFER_CAPABILITY_OUT_OF_ORDER_KHR       (1 << 3)

#define CL_COMMAND_BUFFER_FLAGS_KHR                         0x1293
#define CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR              (1 << 0)

#define CL_INVALID_COMMAND_BUFFER_KHR                       -1138
#define CL_INVALID_SYNC_POINT_WAIT_LIST_KHR                 -1139
#define CL_INCOMPATIBLE_COMMAND_QUEUE_KHR                   -1140

#define CL_COMMAND_BUFFER_QUEUES_KHR                        0x1294
#define CL_COMMAND_BUFFER_NUM_QUEUES_KHR                    0x1295
#define CL_COMMAND_BUFFER_REFERENCE_COUNT_KHR               0x1296
#define CL_COMMAND_BUFFER_STATE_KHR                         0x1297
#define CL_COMMAND_BUFFER_PROPERTIES_ARRAY_KHR              0x1298
#define CL_COMMAND_BUFFER_CONTEXT_KHR                       0x1299

#define CL_COMMAND_BUFFER_STATE_RECORDING_KHR               0
#define CL_COMMAND_BUFFER_STATE_EXECUTABLE_KHR              1
#define CL_COMMAND_BUFFER_STATE_PENDING_KHR                 2
#define CL_COMMAND_BUFFER_STATE_INVALID_KHR                 3

#define CL_COMMAND_COMMAND_BUFFER_KHR                       0x12A8

extern CL_API_ENTRY cl_command_buffer_khr CL_API_CALL
clCreateCommandBufferKHR(
    cl_uint num_queues,
    const cl_command_queue* queues,
    const cl_command_buffer_properties_khr* properties,
    cl_int* errcode_ret) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clFinalizeCommandBufferKHR(
    cl_command_buffer_khr command_buffer) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clRetainCommandBufferKHR(
    cl_command_buffer_khr command_buffer) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clReleaseCommandBufferKHR(
    cl_command_buffer_khr command_buffer) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clEnqueueCommandBufferKHR(
    cl_uint num_queues,
    cl_command_queue* queues,
    cl_command_buffer_khr command_buffer,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clCommandBarrierWithWaitListKHR(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clCommandCopyBufferKHR(
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
    cl_mutable_command_khr* mutable_handle) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clCommandCopyBufferRectKHR(
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
    cl_mutable_command_khr* mutable_handle) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clCommandCopyBufferToImageKHR(
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
    cl_mutable_command_khr* mutable_handle) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clCommandCopyImageKHR(
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
    cl_mutable_command_khr* mutable_handle) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clCommandCopyImageToBufferKHR(
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
    cl_mutable_command_khr* mutable_handle) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clCommandFillBufferKHR(
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
    cl_mutable_command_khr* mutable_handle) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clCommandFillImageKHR(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    cl_mem image,
    const void* fill_color,
    const size_t* origin,
    const size_t* region,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clCommandSVMMemcpyKHR(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    void* dst_ptr,
    const void* src_ptr,
    size_t size,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle);

extern CL_API_ENTRY cl_int CL_API_CALL
clCommandSVMMemFillKHR(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    void* svm_ptr,
    const void* pattern,
    size_t pattern_size,
    size_t size,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr* sync_point_wait_list,
    cl_sync_point_khr* sync_point,
    cl_mutable_command_khr* mutable_handle);

extern CL_API_ENTRY cl_int CL_API_CALL
clCommandNDRangeKernelKHR(
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
    cl_mutable_command_khr* mutable_handle) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetCommandBufferInfoKHR(
    cl_command_buffer_khr command_buffer,
    cl_command_buffer_info_khr param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret) ;

///////////////////////////////////////////////////////////////////////////////
// cl_khr_command_buffer_multi_device

// Note: This implements the provisional extension v0.9.0.

typedef cl_bitfield         cl_platform_command_buffer_capabilities_khr;

#define CL_PLATFORM_COMMAND_BUFFER_CAPABILITIES_KHR         0x0908
#define CL_COMMAND_BUFFER_PLATFORM_UNIVERSAL_SYNC_KHR       (1 << 0)
#define CL_COMMAND_BUFFER_PLATFORM_REMAP_QUEUES_KHR         (1 << 1)
#define CL_COMMAND_BUFFER_PLATFORM_AUTOMATIC_REMAP_KHR      (1 << 2)

#define CL_DEVICE_COMMAND_BUFFER_NUM_SYNC_DEVICES_KHR       0x12AB
#define CL_DEVICE_COMMAND_BUFFER_SYNC_DEVICES_KHR           0x12AC

#define CL_COMMAND_BUFFER_CAPABILITY_MULTIPLE_QUEUE_KHR     (1 << 4)

#define CL_COMMAND_BUFFER_DEVICE_SIDE_SYNC_KHR              (1 << 2)

extern CL_API_ENTRY cl_command_buffer_khr CL_API_CALL
clRemapCommandBufferKHR(
    cl_command_buffer_khr command_buffer,
    cl_bool automatic,
    cl_uint num_queues,
    const cl_command_queue* queues,
    cl_uint num_handles,
    const cl_mutable_command_khr* handles,
    cl_mutable_command_khr* handles_ret,
    cl_int* errcode_ret) ;

///////////////////////////////////////////////////////////////////////////////
// cl_khr_command_buffer_mutable_dispatch

// Note: This implements the provisional extension v0.9.0.

typedef cl_uint             cl_command_buffer_structure_type_khr;
typedef cl_bitfield         cl_mutable_dispatch_fields_khr;
typedef cl_uint             cl_mutable_command_info_khr;
typedef struct _cl_mutable_dispatch_arg_khr {
    cl_uint arg_index;
    size_t arg_size;
    const void* arg_value;
} cl_mutable_dispatch_arg_khr;
typedef struct _cl_mutable_dispatch_exec_info_khr {
    cl_uint param_name;
    size_t param_value_size;
    const void* param_value;
} cl_mutable_dispatch_exec_info_khr;
typedef struct _cl_mutable_dispatch_config_khr {
    cl_command_buffer_structure_type_khr type;
    const void* next;
    cl_mutable_command_khr command;
    cl_uint num_args;
    cl_uint num_svm_args;
    cl_uint num_exec_infos;
    cl_uint work_dim;
    const cl_mutable_dispatch_arg_khr* arg_list;
    const cl_mutable_dispatch_arg_khr* arg_svm_list;
    const cl_mutable_dispatch_exec_info_khr* exec_info_list;
    const size_t* global_work_offset;
    const size_t* global_work_size;
    const size_t* local_work_size;
} cl_mutable_dispatch_config_khr;
typedef struct _cl_mutable_base_config_khr {
    cl_command_buffer_structure_type_khr type;
    const void* next;
    cl_uint num_mutable_dispatch;
    const cl_mutable_dispatch_config_khr* mutable_dispatch_list;
} cl_mutable_base_config_khr;

#define CL_COMMAND_BUFFER_MUTABLE_KHR                       (1 << 1)

#define CL_INVALID_MUTABLE_COMMAND_KHR                      -1141

#define CL_DEVICE_MUTABLE_DISPATCH_CAPABILITIES_KHR         0x12B0

#define CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR            0x12B1
#define CL_MUTABLE_DISPATCH_GLOBAL_OFFSET_KHR               (1 << 0)
#define CL_MUTABLE_DISPATCH_GLOBAL_SIZE_KHR                 (1 << 1)
#define CL_MUTABLE_DISPATCH_LOCAL_SIZE_KHR                  (1 << 2)
#define CL_MUTABLE_DISPATCH_ARGUMENTS_KHR                   (1 << 3)
#define CL_MUTABLE_DISPATCH_EXEC_INFO_KHR                   (1 << 4)

#define CL_MUTABLE_COMMAND_COMMAND_QUEUE_KHR                0x12A0
#define CL_MUTABLE_COMMAND_COMMAND_BUFFER_KHR               0x12A1
#define CL_MUTABLE_COMMAND_COMMAND_TYPE_KHR                 0x12AD
#define CL_MUTABLE_DISPATCH_PROPERTIES_ARRAY_KHR            0x12A2
#define CL_MUTABLE_DISPATCH_KERNEL_KHR                      0x12A3
#define CL_MUTABLE_DISPATCH_DIMENSIONS_KHR                  0x12A4
#define CL_MUTABLE_DISPATCH_GLOBAL_WORK_OFFSET_KHR          0x12A5
#define CL_MUTABLE_DISPATCH_GLOBAL_WORK_SIZE_KHR            0x12A6
#define CL_MUTABLE_DISPATCH_LOCAL_WORK_SIZE_KHR             0x12A7

#define CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR           0
#define CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR       1

extern CL_API_ENTRY cl_int CL_API_CALL
clUpdateMutableCommandsKHR(
    cl_command_buffer_khr command_buffer,
    const cl_mutable_base_config_khr* mutable_config) ;

extern CL_API_ENTRY cl_int CL_API_CALL
clGetMutableCommandInfoKHR(
    cl_mutable_command_khr command,
    cl_mutable_command_info_khr param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret) ;

///////////////////////////////////////////////////////////////////////////////
// cl_khr_create_command_queue

typedef cl_properties cl_queue_properties_khr;

extern CL_API_ENTRY
cl_command_queue CL_API_CALL clCreateCommandQueueWithPropertiesKHR(
    cl_context context,
    cl_device_id device,
    const cl_queue_properties_khr* properties,
    cl_int* errcode_ret);

///////////////////////////////////////////////////////////////////////////////
// cl_khr_d3d10_sharing

#if defined(_WIN32)

// Minimal set of types for cl_khr_d3d10_sharing.
// Don't include cl_d3d10.h here because we don't want a dependency on d3d10.h.
typedef cl_uint cl_d3d10_device_source_khr;
typedef cl_uint cl_d3d10_device_set_khr;
class ID3D10Buffer;
class ID3D10Texture2D;
class ID3D10Texture3D;

#define CL_INVALID_D3D10_DEVICE_KHR                  -1002
#define CL_INVALID_D3D10_RESOURCE_KHR                -1003
#define CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR       -1004
#define CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR           -1005

#define CL_D3D10_DEVICE_KHR                          0x4010
#define CL_D3D10_DXGI_ADAPTER_KHR                    0x4011
#define CL_PREFERRED_DEVICES_FOR_D3D10_KHR           0x4012
#define CL_ALL_DEVICES_FOR_D3D10_KHR                 0x4013
#define CL_CONTEXT_D3D10_DEVICE_KHR                  0x4014
#define CL_CONTEXT_D3D10_PREFER_SHARED_RESOURCES_KHR 0x402C
#define CL_MEM_D3D10_RESOURCE_KHR                    0x4015
#define CL_IMAGE_D3D10_SUBRESOURCE_KHR               0x4016
#define CL_COMMAND_ACQUIRE_D3D10_OBJECTS_KHR         0x4017
#define CL_COMMAND_RELEASE_D3D10_OBJECTS_KHR         0x4018

extern CL_API_ENTRY
cl_int CL_API_CALL clGetDeviceIDsFromD3D10KHR(
    cl_platform_id platform,
    cl_d3d10_device_source_khr d3d_device_source,
    void* d3d_object,
    cl_d3d10_device_set_khr d3d_device_set,
    cl_uint num_entries,
    cl_device_id* devices,
    cl_uint* num_devices);

extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromD3D10BufferKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D10Buffer* resource,
    cl_int* errcode_ret);

extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromD3D10Texture2DKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D10Texture2D* resource,
    UINT subresource,
    cl_int* errcode_ret);

extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromD3D10Texture3DKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D10Texture3D* resource,
    UINT subresource,
    cl_int* errcode_ret);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueAcquireD3D10ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueReleaseD3D10ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

#endif

///////////////////////////////////////////////////////////////////////////////
// cl_khr_d3d11_sharing

#if defined(_WIN32)

// Minimal set of types for cl_khr_d3d11_sharing.
// Don't include cl_d3d11.h here because we don't want a dependency on d3d10.h.
typedef cl_uint cl_d3d11_device_source_khr;
typedef cl_uint cl_d3d11_device_set_khr;
class ID3D11Buffer;
class ID3D11Texture2D;
class ID3D11Texture3D;

#define CL_INVALID_D3D11_DEVICE_KHR                  -1006
#define CL_INVALID_D3D11_RESOURCE_KHR                -1007
#define CL_D3D11_RESOURCE_ALREADY_ACQUIRED_KHR       -1008
#define CL_D3D11_RESOURCE_NOT_ACQUIRED_KHR           -1009

#define CL_D3D11_DEVICE_KHR                          0x4019
#define CL_D3D11_DXGI_ADAPTER_KHR                    0x401A
#define CL_PREFERRED_DEVICES_FOR_D3D11_KHR           0x401B
#define CL_ALL_DEVICES_FOR_D3D11_KHR                 0x401C
#define CL_CONTEXT_D3D11_DEVICE_KHR                  0x401D
#define CL_CONTEXT_D3D11_PREFER_SHARED_RESOURCES_KHR 0x402D
#define CL_MEM_D3D11_RESOURCE_KHR                    0x401E
#define CL_IMAGE_D3D11_SUBRESOURCE_KHR               0x401F
#define CL_COMMAND_ACQUIRE_D3D11_OBJECTS_KHR         0x4020
#define CL_COMMAND_RELEASE_D3D11_OBJECTS_KHR         0x4021

extern CL_API_ENTRY
cl_int CL_API_CALL clGetDeviceIDsFromD3D11KHR(
    cl_platform_id platform,
    cl_d3d11_device_source_khr d3d_device_source,
    void* d3d_object,
    cl_d3d11_device_set_khr d3d_device_set,
    cl_uint num_entries,
    cl_device_id* devices,
    cl_uint* num_devices);

extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromD3D11BufferKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D11Buffer* resource,
    cl_int* errcode_ret);

extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromD3D11Texture2DKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D11Texture2D* resource,
    UINT subresource,
    cl_int* errcode_ret);

extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromD3D11Texture3DKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D11Texture3D* resource,
    UINT subresource,
    cl_int* errcode_ret);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueAcquireD3D11ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueReleaseD3D11ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

#endif

///////////////////////////////////////////////////////////////////////////////
// cl_khr_device_uuid

#define CL_DEVICE_UUID_KHR                                  0x106A
#define CL_DRIVER_UUID_KHR                                  0x106B
#define CL_DEVICE_LUID_VALID_KHR                            0x106C
#define CL_DEVICE_LUID_KHR                                  0x106D
#define CL_DEVICE_NODE_MASK_KHR                             0x106E

///////////////////////////////////////////////////////////////////////////////
// cl_khr_dx9_media_sharing

#if defined(_WIN32)

// Minimal set of types for cl_khr_dx9_media_sharing.
// Don't include cl_d3d9.h here because we don't want a dependency on d3d9.h.
typedef cl_uint cl_dx9_media_adapter_type_khr;
typedef cl_uint cl_dx9_media_adapter_set_khr;
typedef cl_uint cl_dx9_media_adapter_type_khr;
class IDirect3DSurface9;

#define CL_INVALID_DX9_MEDIA_ADAPTER_KHR                -1010
#define CL_INVALID_DX9_MEDIA_SURFACE_KHR                -1011
#define CL_DX9_MEDIA_SURFACE_ALREADY_ACQUIRED_KHR       -1012
#define CL_DX9_MEDIA_SURFACE_NOT_ACQUIRED_KHR           -1013

#define CL_ADAPTER_D3D9_KHR                              0x2020
#define CL_ADAPTER_D3D9EX_KHR                            0x2021
#define CL_ADAPTER_DXVA_KHR                              0x2022
#define CL_PREFERRED_DEVICES_FOR_DX9_MEDIA_ADAPTER_KHR   0x2023
#define CL_ALL_DEVICES_FOR_DX9_MEDIA_ADAPTER_KHR         0x2024
#define CL_CONTEXT_ADAPTER_D3D9_KHR                      0x2025
#define CL_CONTEXT_ADAPTER_D3D9EX_KHR                    0x2026
#define CL_CONTEXT_ADAPTER_DXVA_KHR                      0x2027
#define CL_MEM_DX9_MEDIA_ADAPTER_TYPE_KHR                0x2028
#define CL_MEM_DX9_MEDIA_SURFACE_INFO_KHR                0x2029
#define CL_IMAGE_DX9_MEDIA_PLANE_KHR                     0x202A
#define CL_COMMAND_ACQUIRE_DX9_MEDIA_SURFACES_KHR        0x202B
#define CL_COMMAND_RELEASE_DX9_MEDIA_SURFACES_KHR        0x202C

extern CL_API_ENTRY
cl_int CL_API_CALL clGetDeviceIDsFromDX9MediaAdapterKHR(
    cl_platform_id platform,
    cl_uint num_media_adapters,
    cl_dx9_media_adapter_type_khr* media_adapters_type,
    void* media_adapters,
    cl_dx9_media_adapter_set_khr media_adapter_set,
    cl_uint num_entries,
    cl_device_id* devices,
    cl_uint* num_devices);

extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromDX9MediaSurfaceKHR(
    cl_context context,
    cl_mem_flags flags,
    cl_dx9_media_adapter_type_khr adapter_type,
    void* surface_info,
    cl_uint plane,
    cl_int* errcode_ret);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueAcquireDX9MediaSurfacesKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueReleaseDX9MediaSurfacesKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

#endif

///////////////////////////////////////////////////////////////////////////////
// cl_khr_extended_versioning

#define CL_PLATFORM_NUMERIC_VERSION_KHR                  0x0906
#define CL_PLATFORM_EXTENSIONS_WITH_VERSION_KHR          0x0907
#define CL_DEVICE_NUMERIC_VERSION_KHR                    0x105E
#define CL_DEVICE_OPENCL_C_NUMERIC_VERSION_KHR           0x105F
#define CL_DEVICE_EXTENSIONS_WITH_VERSION_KHR            0x1060
#define CL_DEVICE_ILS_WITH_VERSION_KHR                   0x1061
#define CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION_KHR      0x1062

#define CL_VERSION_MAJOR_BITS_KHR (10)
#define CL_VERSION_MINOR_BITS_KHR (10)
#define CL_VERSION_PATCH_BITS_KHR (12)

#define CL_VERSION_MAJOR_MASK_KHR ((1 << CL_VERSION_MAJOR_BITS_KHR) - 1)
#define CL_VERSION_MINOR_MASK_KHR ((1 << CL_VERSION_MINOR_BITS_KHR) - 1)
#define CL_VERSION_PATCH_MASK_KHR ((1 << CL_VERSION_PATCH_BITS_KHR) - 1)

#define CL_VERSION_MAJOR_KHR(version) ((version) >> (CL_VERSION_MINOR_BITS_KHR + CL_VERSION_PATCH_BITS_KHR))
#define CL_VERSION_MINOR_KHR(version) (((version) >> CL_VERSION_PATCH_BITS_KHR) & CL_VERSION_MINOR_MASK_KHR)
#define CL_VERSION_PATCH_KHR(version) ((version) & CL_VERSION_PATCH_MASK_KHR)

#define CL_MAKE_VERSION_KHR(major, minor, patch) \
    ((((major) & CL_VERSION_MAJOR_MASK_KHR) << (CL_VERSION_MINOR_BITS_KHR + CL_VERSION_PATCH_BITS_KHR)) | \
    (((minor) &  CL_VERSION_MINOR_MASK_KHR) << CL_VERSION_PATCH_BITS_KHR) | \
    ((patch) & CL_VERSION_PATCH_MASK_KHR))

typedef cl_uint cl_version_khr;

#define CL_NAME_VERSION_MAX_NAME_SIZE_KHR 64

typedef struct _cl_name_version_khr
{
    cl_version_khr version;
    char name[CL_NAME_VERSION_MAX_NAME_SIZE_KHR];
} cl_name_version_khr;

///////////////////////////////////////////////////////////////////////////////
// cl_khr_external_memory

// Note: This implements the provisional extension v0.9.0.

typedef cl_uint             cl_external_memory_handle_type_khr;

#define CL_PLATFORM_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR      0x2044

#define CL_DEVICE_EXTERNAL_MEMORY_IMPORT_HANDLE_TYPES_KHR        0x204F

#define CL_DEVICE_HANDLE_LIST_KHR                                0x2051
#define CL_DEVICE_HANDLE_LIST_END_KHR                            0

#define CL_COMMAND_ACQUIRE_EXTERNAL_MEM_OBJECTS_KHR              0x2047
#define CL_COMMAND_RELEASE_EXTERNAL_MEM_OBJECTS_KHR              0x2048

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueAcquireExternalMemObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_mem_objects,
    const cl_mem *mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueReleaseExternalMemObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_mem_objects,
    const cl_mem *mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event);

// cl_khr_external_memory_dma_buf
#define CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR           0x2067

// cl_khr_external_memory_dx
#define CL_EXTERNAL_MEMORY_HANDLE_D3D11_TEXTURE_KHR     0x2063
#define CL_EXTERNAL_MEMORY_HANDLE_D3D11_TEXTURE_KMT_KHR 0x2064
#define CL_EXTERNAL_MEMORY_HANDLE_D3D12_HEAP_KHR        0x2065
#define CL_EXTERNAL_MEMORY_HANDLE_D3D12_RESOURCE_KHR    0x2066

// cl_khr_external_memory_opaque_fd
#define CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR         0x2060

// cl_khr_external_memory_win32
#define CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR      0x2061
#define CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KMT_KHR  0x2062

///////////////////////////////////////////////////////////////////////////////
// cl_khr_external_semaphore

typedef struct _cl_semaphore_khr * cl_semaphore_khr;
typedef cl_uint             cl_external_semaphore_handle_type_khr;

#define CL_PLATFORM_SEMAPHORE_IMPORT_HANDLE_TYPES_KHR       0x2037
#define CL_PLATFORM_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR       0x2038

#define CL_DEVICE_SEMAPHORE_IMPORT_HANDLE_TYPES_KHR         0x204D
#define CL_DEVICE_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR         0x204E

#define CL_SEMAPHORE_EXPORT_HANDLE_TYPES_KHR                0x203F
#define CL_SEMAPHORE_EXPORT_HANDLE_TYPES_LIST_END_KHR       0

extern CL_API_ENTRY cl_int CL_API_CALL
clGetSemaphoreHandleForTypeKHR(
    cl_semaphore_khr semaphore,
    cl_device_id device,
    cl_external_semaphore_handle_type_khr handle_type,
    size_t handle_size,
    void* handle_ptr,
    size_t* handle_size_ret);

// cl_khr_external_semaphore_dx_fence
#define CL_SEMAPHORE_HANDLE_D3D12_FENCE_KHR                 0x2059

// cl_khr_external_semaphore_opaque_fd
#define CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR                   0x2055

// cl_khr_external_semaphore_sync_fd
#define CL_SEMAPHORE_HANDLE_SYNC_FD_KHR                     0x2058

// cl_khr_external_semaphore_win32
#define CL_SEMAPHORE_HANDLE_OPAQUE_WIN32_KHR                0x2056
#define CL_SEMAPHORE_HANDLE_OPAQUE_WIN32_KMT_KHR            0x2057

///////////////////////////////////////////////////////////////////////////////
// cl_khr_fp16

#define CL_DEVICE_HALF_FP_CONFIG                         0x1033

///////////////////////////////////////////////////////////////////////////////
// cl_khr_gl_event

#define CL_COMMAND_GL_FENCE_SYNC_OBJECT_KHR     0x200D

extern CL_API_ENTRY
cl_event CL_API_CALL clCreateEventFromGLsyncKHR(
    cl_context context,
    cl_GLsync sync,
    cl_int* errcode_ret);

///////////////////////////////////////////////////////////////////////////////
// cl_khr_gl_sharing

extern CL_API_ENTRY
cl_int CL_API_CALL clGetGLContextInfoKHR(
    const cl_context_properties *properties,
    cl_gl_context_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret);

///////////////////////////////////////////////////////////////////////////////
// cl_khr_icd

#define CL_PLATFORM_ICD_SUFFIX_KHR                  0x0920
#define CL_PLATFORM_NOT_FOUND_KHR                   -1001

///////////////////////////////////////////////////////////////////////////////
// cl_khr_il_program

#define CL_DEVICE_IL_VERSION_KHR                        0x105B
#define CL_PROGRAM_IL_KHR                               0x1169

extern CL_API_ENTRY
cl_program CL_API_CALL clCreateProgramWithILKHR(
    cl_context context,
    const void* il,
    size_t length,
    cl_int* errcode_ret );

///////////////////////////////////////////////////////////////////////////////
// cl_khr_initialize_memory

#define CL_CONTEXT_MEMORY_INITIALIZE_KHR            0x2030

///////////////////////////////////////////////////////////////////////////////
// cl_khr_integer_dot_product

typedef cl_bitfield         cl_device_integer_dot_product_capabilities_khr;

#define CL_DEVICE_INTEGER_DOT_PRODUCT_INPUT_4x8BIT_PACKED_KHR (1 << 0)
#define CL_DEVICE_INTEGER_DOT_PRODUCT_INPUT_4x8BIT_KHR      (1 << 1)

#define CL_DEVICE_INTEGER_DOT_PRODUCT_CAPABILITIES_KHR      0x1073

///////////////////////////////////////////////////////////////////////////////
// cl_khr_pci_bus_info

typedef struct _cl_device_pci_bus_info_khr {
    cl_uint pci_domain;
    cl_uint pci_bus;
    cl_uint pci_device;
    cl_uint pci_function;
} cl_device_pci_bus_info_khr;

#define CL_DEVICE_PCI_BUS_INFO_KHR                  0x410F

///////////////////////////////////////////////////////////////////////////////
// cl_khr_priority_hints

#define CL_QUEUE_PRIORITY_KHR 0x1096
#define CL_QUEUE_PRIORITY_HIGH_KHR (1<<0)
#define CL_QUEUE_PRIORITY_MED_KHR (1<<1)
#define CL_QUEUE_PRIORITY_LOW_KHR (1<<2)

///////////////////////////////////////////////////////////////////////////////
// cl_khr_semaphore

// Note: This implements the provisional extension v0.9.0.

// Shared with cl_khr_external_semaphore:
//typedef struct _cl_semaphore_khr* cl_semaphore_khr;
typedef cl_properties cl_semaphore_properties_khr;
typedef cl_uint cl_semaphore_info_khr;
typedef cl_uint cl_semaphore_type_khr;
typedef cl_ulong cl_semaphore_payload_khr;

#define CL_SEMAPHORE_TYPE_BINARY_KHR                1

#define CL_PLATFORM_SEMAPHORE_TYPES_KHR                          0x2036
#define CL_DEVICE_SEMAPHORE_TYPES_KHR                            0x204C
#define CL_SEMAPHORE_CONTEXT_KHR                                 0x2039
#define CL_SEMAPHORE_REFERENCE_COUNT_KHR                         0x203A
#define CL_SEMAPHORE_PROPERTIES_KHR                              0x203B
#define CL_SEMAPHORE_PAYLOAD_KHR                                 0x203C
#define CL_SEMAPHORE_TYPE_KHR                                    0x203D

// Shared with cl_khr_external_memory:
//#define CL_DEVICE_HANDLE_LIST_KHR                              0x2051
//#define CL_DEVICE_HANDLE_LIST_END_KHR                          0

#define CL_COMMAND_SEMAPHORE_WAIT_KHR                            0x2042
#define CL_COMMAND_SEMAPHORE_SIGNAL_KHR                          0x2043

#define CL_INVALID_SEMAPHORE_KHR                                 -1142

extern CL_API_ENTRY
cl_semaphore_khr CL_API_CALL clCreateSemaphoreWithPropertiesKHR(
    cl_context context,
    const cl_semaphore_properties_khr* sema_props,
    cl_int* errcode_ret);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueWaitSemaphoresKHR(
    cl_command_queue command_queue,
    cl_uint num_sema_objects,
    const cl_semaphore_khr* sema_objects,
    const cl_semaphore_payload_khr* sema_payload_list,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueSignalSemaphoresKHR(
    cl_command_queue command_queue,
    cl_uint num_sema_objects,
    const cl_semaphore_khr* sema_objects,
    const cl_semaphore_payload_khr* sema_payload_list,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

extern CL_API_ENTRY
cl_int CL_API_CALL clGetSemaphoreInfoKHR(
    cl_semaphore_khr semaphore,
    cl_semaphore_info_khr param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret);

extern CL_API_ENTRY
cl_int CL_API_CALL clRetainSemaphoreKHR(
    cl_semaphore_khr semaphore);

extern CL_API_ENTRY
cl_int CL_API_CALL clReleaseSemaphoreKHR(
    cl_semaphore_khr semaphore);

///////////////////////////////////////////////////////////////////////////////
// cl_khr_spir

#define CL_DEVICE_SPIR_VERSIONS                     0x40E0
#define CL_PROGRAM_BINARY_TYPE_INTERMEDIATE         0x40E1

///////////////////////////////////////////////////////////////////////////////
// cl_khr_subgroups

#define CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE_KHR    0x2033
#define CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE_KHR   0x2034

typedef cl_uint  cl_kernel_sub_group_info;

extern CL_API_ENTRY
cl_int CL_API_CALL clGetKernelSubGroupInfoKHR(
    cl_kernel kernel,
    cl_device_id device,
    cl_kernel_sub_group_info param_name,
    size_t input_value_size,
    const void* input_value,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret);

///////////////////////////////////////////////////////////////////////////////
// cl_khr_suggested_local_work_size

extern CL_API_ENTRY
cl_int CL_API_CALL clGetKernelSuggestedLocalWorkSizeKHR(
    cl_command_queue command_queue,
    cl_kernel kernel,
    cl_uint work_dim,
    const size_t* global_work_offset,
    const size_t* global_work_size,
    size_t* suggested_local_work_size);

///////////////////////////////////////////////////////////////////////////////
// cl_khr_terminate_context

#define CL_DEVICE_TERMINATE_CAPABILITY_KHR          0x2031
#define CL_CONTEXT_TERMINATE_KHR                    0x2032

///////////////////////////////////////////////////////////////////////////////
// cl_khr_throttle_hints

#define CL_QUEUE_THROTTLE_KHR 0x1097
#define CL_QUEUE_THROTTLE_HIGH_KHR (1<<0)
#define CL_QUEUE_THROTTLE_MED_KHR (1<<1)
#define CL_QUEUE_THROTTLE_LOW_KHR (1<<2)

///////////////////////////////////////////////////////////////////////////////
// cl_ext_atomic_counters

#define CL_DEVICE_MAX_ATOMIC_COUNTERS_EXT           0x4032

///////////////////////////////////////////////////////////////////////////////
// cl_ext_cxx_for_opencl

#define CL_DEVICE_CXX_FOR_OPENCL_NUMERIC_VERSION_EXT 0x4230

///////////////////////////////////////////////////////////////////////////////
// cl_ext_device_fission

#define CL_DEVICE_PARTITION_EQUALLY_EXT             0x4050
#define CL_DEVICE_PARTITION_BY_COUNTS_EXT           0x4051
#define CL_DEVICE_PARTITION_BY_NAMES_EXT            0x4052
#define CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN_EXT  0x4053
#define CL_DEVICE_PARENT_DEVICE_EXT                 0x4054
#define CL_DEVICE_PARTITION_TYPES_EXT               0x4055
#define CL_DEVICE_AFFINITY_DOMAINS_EXT              0x4056
#define CL_DEVICE_REFERENCE_COUNT_EXT               0x4057
#define CL_DEVICE_PARTITION_STYLE_EXT               0x4058

#define CL_DEVICE_PARTITION_FAILED_EXT              -1057
#define CL_INVALID_PARTITION_COUNT_EXT              -1058
#define CL_INVALID_PARTITION_NAME_EXT               -1059

#define CL_AFFINITY_DOMAIN_L1_CACHE_EXT             0x1
#define CL_AFFINITY_DOMAIN_L2_CACHE_EXT             0x2
#define CL_AFFINITY_DOMAIN_L3_CACHE_EXT             0x3
#define CL_AFFINITY_DOMAIN_L4_CACHE_EXT             0x4
#define CL_AFFINITY_DOMAIN_NUMA_EXT                 0x10
#define CL_AFFINITY_DOMAIN_NEXT_FISSIONABLE_EXT     0x100

#define CL_PARTITION_BY_COUNTS_LIST_END_EXT         0x0
#define CL_PARTITION_BY_NAMES_LIST_END_EXT          -1

///////////////////////////////////////////////////////////////////////////////
// cl_ext_float_atomics

typedef cl_bitfield         cl_device_fp_atomic_capabilities_ext;

#define CL_DEVICE_GLOBAL_FP_ATOMIC_LOAD_STORE_EXT       (1 << 0)
#define CL_DEVICE_GLOBAL_FP_ATOMIC_ADD_EXT              (1 << 1)
#define CL_DEVICE_GLOBAL_FP_ATOMIC_MIN_MAX_EXT          (1 << 2)
#define CL_DEVICE_LOCAL_FP_ATOMIC_LOAD_STORE_EXT        (1 << 16)
#define CL_DEVICE_LOCAL_FP_ATOMIC_ADD_EXT               (1 << 17)
#define CL_DEVICE_LOCAL_FP_ATOMIC_MIN_MAX_EXT           (1 << 18)

#define CL_DEVICE_SINGLE_FP_ATOMIC_CAPABILITIES_EXT     0x4231
#define CL_DEVICE_DOUBLE_FP_ATOMIC_CAPABILITIES_EXT     0x4232
#define CL_DEVICE_HALF_FP_ATOMIC_CAPABILITIES_EXT       0x4233

///////////////////////////////////////////////////////////////////////////////
// cl_ext_image_from_buffer

#define CL_IMAGE_REQUIREMENTS_SLICE_PITCH_ALIGNMENT_EXT 0x1291

///////////////////////////////////////////////////////////////////////////////
// cl_ext_image_requirements_info

typedef cl_uint cl_image_requirements_info_ext;

#define CL_IMAGE_REQUIREMENTS_ROW_PITCH_ALIGNMENT_EXT   0x1290
#define CL_IMAGE_REQUIREMENTS_BASE_ADDRESS_ALIGNMENT_EXT 0x1292
#define CL_IMAGE_REQUIREMENTS_SIZE_EXT                  0x12B2
#define CL_IMAGE_REQUIREMENTS_MAX_WIDTH_EXT             0x12B3
#define CL_IMAGE_REQUIREMENTS_MAX_HEIGHT_EXT            0x12B4
#define CL_IMAGE_REQUIREMENTS_MAX_DEPTH_EXT             0x12B5
#define CL_IMAGE_REQUIREMENTS_MAX_ARRAY_SIZE_EXT        0x12B6

extern CL_API_ENTRY
cl_int CL_API_CALL clGetImageRequirementsInfoEXT(
    cl_context context,
    const cl_mem_properties* properties,
    cl_mem_flags flags,
    const cl_image_format* image_format,
    const cl_image_desc* image_desc,
    cl_image_requirements_info_ext param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret);

///////////////////////////////////////////////////////////////////////////////
// cl_altera_compiler_mode

#define CL_CONTEXT_COMPILER_MODE_ALTERA                 0x40F0
#define CL_CONTEXT_PROGRAM_EXE_LIBRARY_ROOT_ALTERA      0x40F1
#define CL_CONTEXT_OFFLINE_DEVICE_ALTERA                0x40F2

///////////////////////////////////////////////////////////////////////////////
// cl_altera_device_temperature

#define CL_DEVICE_CORE_TEMPERATURE_ALTERA               0x40F3

///////////////////////////////////////////////////////////////////////////////
// cl_amd_device_attribute_query

#define CL_DEVICE_PREFERRED_WORK_GROUP_SIZE_AMD     0x4030
#define CL_DEVICE_MAX_WORK_GROUP_SIZE_AMD           0x4031
#define CL_DEVICE_PREFERRED_CONSTANT_BUFFER_SIZE_AMD    0x4033
#define CL_DEVICE_PCIE_ID_AMD                       0x4034
#define CL_DEVICE_PROFILING_TIMER_OFFSET_AMD        0x4036
#define CL_DEVICE_TOPOLOGY_AMD                      0x4037
#define CL_DEVICE_BOARD_NAME_AMD                    0x4038
#define CL_DEVICE_GLOBAL_FREE_MEMORY_AMD            0x4039
#define CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD         0x4040
#define CL_DEVICE_SIMD_WIDTH_AMD                    0x4041
#define CL_DEVICE_SIMD_INSTRUCTION_WIDTH_AMD        0x4042
#define CL_DEVICE_WAVEFRONT_WIDTH_AMD               0x4043
#define CL_DEVICE_GLOBAL_MEM_CHANNELS_AMD           0x4044
#define CL_DEVICE_GLOBAL_MEM_CHANNEL_BANKS_AMD      0x4045
#define CL_DEVICE_GLOBAL_MEM_CHANNEL_BANK_WIDTH_AMD 0x4046
#define CL_DEVICE_LOCAL_MEM_SIZE_PER_COMPUTE_UNIT_AMD   0x4047
#define CL_DEVICE_LOCAL_MEM_BANKS_AMD               0x4048
#define CL_DEVICE_THREAD_TRACE_SUPPORTED_AMD        0x4049
#define CL_DEVICE_GFXIP_MAJOR_AMD                   0x404A
#define CL_DEVICE_GFXIP_MINOR_AMD                   0x404B
#define CL_DEVICE_AVAILABLE_ASYNC_QUEUES_AMD        0x404C

///////////////////////////////////////////////////////////////////////////////
// cl_amd_offline_devices

#define CL_CONTEXT_OFFLINE_DEVICES_AMD              0x403F

///////////////////////////////////////////////////////////////////////////////
// cl_arm_get_core_id

#define CL_DEVICE_COMPUTE_UNITS_BITFIELD_ARM        0x40BF

///////////////////////////////////////////////////////////////////////////////
// cl_arm_job_slot_selection

#define CL_DEVICE_JOB_SLOTS_ARM                     0x41E0
#define CL_QUEUE_JOB_SLOT_ARM                       0x41E1

///////////////////////////////////////////////////////////////////////////////
// cl_arm_printf

#define CL_PRINTF_CALLBACK_ARM                      0x40B0
#define CL_PRINTF_BUFFERSIZE_ARM                    0x40B1

///////////////////////////////////////////////////////////////////////////////
// cl_arm_scheduling_controls

#define CL_DEVICE_SCHEDULING_CONTROLS_CAPABILITIES_ARM          0x41E4
#define CL_DEVICE_SCHEDULING_KERNEL_BATCHING_ARM                (1 << 0)
#define CL_DEVICE_SCHEDULING_WORKGROUP_BATCH_SIZE_ARM           (1 << 1)
#define CL_DEVICE_SCHEDULING_WORKGROUP_BATCH_SIZE_MODIFIER_ARM  (1 << 2)
#define CL_DEVICE_SCHEDULING_DEFERRED_FLUSH_ARM                 (1 << 3)
#define CL_KERNEL_EXEC_INFO_WORKGROUP_BATCH_SIZE_ARM            0x41E5
#define CL_KERNEL_EXEC_INFO_WORKGROUP_BATCH_SIZE_MODIFIER_ARM   0x41E6
#define CL_QUEUE_KERNEL_BATCHING_ARM                            0x41E7
#define CL_QUEUE_DEFERRED_FLUSH_ARM                             0x41EC

///////////////////////////////////////////////////////////////////////////////
// cl_intel_accelerator

typedef struct _cl_accelerator_intel*     cl_accelerator_intel;
typedef cl_uint                           cl_accelerator_type_intel;
typedef cl_uint                           cl_accelerator_info_intel;

// Error Codes
#define CL_INVALID_ACCELERATOR_INTEL                    -1094
#define CL_INVALID_ACCELERATOR_TYPE_INTEL               -1095
#define CL_INVALID_ACCELERATOR_DESC_INTEL               -1096
#define CL_ACCELERATOR_TYPE_NOT_SUPPORTED_INTEL         -1097

// cl_accelerator_info_intel
#define CL_ACCELERATOR_DESCRIPTOR_INTEL                 0x4090
#define CL_ACCELERATOR_REFERENCE_COUNT_INTEL            0x4091
#define CL_ACCELERATOR_CONTEXT_INTEL                    0x4092
#define CL_ACCELERATOR_TYPE_INTEL                       0x4093

extern CL_API_ENTRY
cl_accelerator_intel CL_API_CALL clCreateAcceleratorINTEL(
    cl_context context,
    cl_accelerator_type_intel accelerator_type,
    size_t descriptor_size,
    const void* descriptor,
    cl_int* errcode_ret );

extern CL_API_ENTRY
cl_int CL_API_CALL clGetAcceleratorInfoINTEL(
    cl_accelerator_intel accelerator,
    cl_accelerator_info_intel param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret );

extern CL_API_ENTRY
cl_int CL_API_CALL clRetainAcceleratorINTEL(
    cl_accelerator_intel accelerator );

extern CL_API_ENTRY
cl_int CL_API_CALL clReleaseAcceleratorINTEL(
    cl_accelerator_intel accelerator );

///////////////////////////////////////////////////////////////////////////////
// cl_intel_advanced_motion_estimation

// cl_device_info
#define CL_DEVICE_ME_VERSION_INTEL                      0x407E

#define CL_ME_VERSION_LEGACY_INTEL                      0x0
#define CL_ME_VERSION_ADVANCED_VER_1_INTEL              0x1

///////////////////////////////////////////////////////////////////////////////
// cl_intel_command_queue_families
typedef cl_bitfield         cl_command_queue_capabilities_intel;

#define CL_QUEUE_FAMILY_MAX_NAME_SIZE_INTEL                 64
typedef struct _cl_queue_family_properties_intel {
    cl_command_queue_properties properties;
    cl_command_queue_capabilities_intel capabilities;
    cl_uint count;
    char name[CL_QUEUE_FAMILY_MAX_NAME_SIZE_INTEL];
} cl_queue_family_properties_intel;

#define CL_DEVICE_QUEUE_FAMILY_PROPERTIES_INTEL             0x418B
#define CL_QUEUE_FAMILY_INTEL                               0x418C
#define CL_QUEUE_INDEX_INTEL                                0x418D

#define CL_QUEUE_DEFAULT_CAPABILITIES_INTEL                 0
#define CL_QUEUE_CAPABILITY_CREATE_SINGLE_QUEUE_EVENTS_INTEL (1 << 0)
#define CL_QUEUE_CAPABILITY_CREATE_CROSS_QUEUE_EVENTS_INTEL (1 << 1)
#define CL_QUEUE_CAPABILITY_SINGLE_QUEUE_EVENT_WAIT_LIST_INTEL (1 << 2)
#define CL_QUEUE_CAPABILITY_CROSS_QUEUE_EVENT_WAIT_LIST_INTEL (1 << 3)
#define CL_QUEUE_CAPABILITY_TRANSFER_BUFFER_INTEL           (1 << 8)
#define CL_QUEUE_CAPABILITY_TRANSFER_BUFFER_RECT_INTEL      (1 << 9)
#define CL_QUEUE_CAPABILITY_MAP_BUFFER_INTEL                (1 << 10)
#define CL_QUEUE_CAPABILITY_FILL_BUFFER_INTEL               (1 << 11)
#define CL_QUEUE_CAPABILITY_TRANSFER_IMAGE_INTEL            (1 << 12)
#define CL_QUEUE_CAPABILITY_MAP_IMAGE_INTEL                 (1 << 13)
#define CL_QUEUE_CAPABILITY_FILL_IMAGE_INTEL                (1 << 14)
#define CL_QUEUE_CAPABILITY_TRANSFER_BUFFER_IMAGE_INTEL     (1 << 15)
#define CL_QUEUE_CAPABILITY_TRANSFER_IMAGE_BUFFER_INTEL     (1 << 16)
#define CL_QUEUE_CAPABILITY_MARKER_INTEL                    (1 << 24)
#define CL_QUEUE_CAPABILITY_BARRIER_INTEL                   (1 << 25)
#define CL_QUEUE_CAPABILITY_KERNEL_INTEL                    (1 << 26)

///////////////////////////////////////////////////////////////////////////////
// cl_intel_device_attribute_query
typedef cl_bitfield         cl_device_feature_capabilities_intel;

#define CL_DEVICE_FEATURE_FLAG_DP4A_INTEL                   (1 << 0)
#define CL_DEVICE_FEATURE_FLAG_DPAS_INTEL                   (1 << 1)

#define CL_DEVICE_IP_VERSION_INTEL                          0x4250
#define CL_DEVICE_ID_INTEL                                  0x4251
#define CL_DEVICE_NUM_SLICES_INTEL                          0x4252
#define CL_DEVICE_NUM_SUB_SLICES_PER_SLICE_INTEL            0x4253
#define CL_DEVICE_NUM_EUS_PER_SUB_SLICE_INTEL               0x4254
#define CL_DEVICE_NUM_THREADS_PER_EU_INTEL                  0x4255
#define CL_DEVICE_FEATURE_CAPABILITIES_INTEL                0x4256

///////////////////////////////////////////////////////////////////////////////
// cl_intel_device_side_avc_motion_estimation (partial)

#define CL_DEVICE_AVC_ME_VERSION_INTEL                      0x410B
#define CL_DEVICE_AVC_ME_SUPPORTS_TEXTURE_SAMPLER_USE_INTEL 0x410C
#define CL_DEVICE_AVC_ME_SUPPORTS_PREEMPTION_INTEL          0x410D

///////////////////////////////////////////////////////////////////////////////
// cl_intel_driver_diagnostics

#define CL_CONTEXT_SHOW_DIAGNOSTICS_INTEL           0x4106

///////////////////////////////////////////////////////////////////////////////
// cl_intel_d3d9_media_sharing

#if defined(_WIN32)

// Minimal set of types for cl_khr_dx9_media_sharing.
// Don't include cl_d3d9.h here because we don't want a dependency on d3d9.h.
typedef cl_uint cl_dx9_device_source_intel;
typedef cl_uint cl_dx9_device_set_intel;
class IDirect3DSurface9;

// These error codes are shared with cl_khr_dx9_media_sharing.
#define CL_INVALID_DX9_DEVICE_INTEL                   -1010
#define CL_INVALID_DX9_RESOURCE_INTEL                 -1011
#define CL_DX9_RESOURCE_ALREADY_ACQUIRED_INTEL        -1012
#define CL_DX9_RESOURCE_NOT_ACQUIRED_INTEL            -1013

#define CL_D3D9_DEVICE_INTEL                          0x4022
#define CL_D3D9EX_DEVICE_INTEL                        0x4070
#define CL_DXVA_DEVICE_INTEL                          0x4071
#define CL_PREFERRED_DEVICES_FOR_DX9_INTEL            0x4024
#define CL_ALL_DEVICES_FOR_DX9_INTEL                  0x4025
#define CL_CONTEXT_D3D9_DEVICE_INTEL                  0x4026
#define CL_CONTEXT_D3D9EX_DEVICE_INTEL                0x4072
#define CL_CONTEXT_DXVA_DEVICE_INTEL                  0x4073
#define CL_MEM_DX9_RESOURCE_INTEL                     0x4027
#define CL_MEM_DX9_SHARED_HANDLE_INTEL                0x4074
#define CL_IMAGE_DX9_PLANE_INTEL                      0x4075
#define CL_COMMAND_ACQUIRE_DX9_OBJECTS_INTEL          0x402A
#define CL_COMMAND_RELEASE_DX9_OBJECTS_INTEL          0x402B

extern CL_API_ENTRY
cl_int CL_API_CALL clGetDeviceIDsFromDX9INTEL(
    cl_platform_id platform,
    cl_dx9_device_source_intel d3d_device_source,
    void *dx9_object,
    cl_dx9_device_set_intel d3d_device_set,
    cl_uint num_entries,
    cl_device_id* devices,
    cl_uint* num_devices );

extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromDX9MediaSurfaceINTEL(
    cl_context context,
    cl_mem_flags flags,
    IDirect3DSurface9* resource,
    HANDLE sharedHandle,
    UINT plane,
    cl_int* errcode_ret );

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueAcquireDX9ObjectsINTEL(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event );

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueReleaseDX9ObjectsINTEL(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event );

#endif

///////////////////////////////////////////////////////////////////////////////
// cl_intel_egl_image_yuv

#define CL_EGL_YUV_PLANE_INTEL                      0x4107

///////////////////////////////////////////////////////////////////////////////
// cl_intel_mem_channel_property

#define CL_MEM_CHANNEL_INTEL                        0x4213

///////////////////////////////////////////////////////////////////////////////
// cl_intel_mem_force_host_memory

#define CL_MEM_FORCE_HOST_MEMORY_INTEL              (1 << 20)

///////////////////////////////////////////////////////////////////////////////
// cl_intel_motion_estimation

// cl_accelerator_type_intel
#define CL_ACCELERATOR_TYPE_MOTION_ESTIMATION_INTEL     0x0

// cl_motion_detect_desc_intel flags
#define CL_ME_MB_TYPE_16x16_INTEL                       0x0
#define CL_ME_MB_TYPE_8x8_INTEL                         0x1
#define CL_ME_MB_TYPE_4x4_INTEL                         0x2

#define CL_ME_SUBPIXEL_MODE_INTEGER_INTEL               0x0
#define CL_ME_SUBPIXEL_MODE_HPEL_INTEL                  0x1
#define CL_ME_SUBPIXEL_MODE_QPEL_INTEL                  0x2

#define CL_ME_SAD_ADJUST_MODE_NONE_INTEL                0x0
#define CL_ME_SAD_ADJUST_MODE_HAAR_INTEL                0x1

#define CL_ME_SEARCH_PATH_RADIUS_2_2_INTEL              0x0
#define CL_ME_SEARCH_PATH_RADIUS_4_4_INTEL              0x1
#define CL_ME_SEARCH_PATH_RADIUS_16_12_INTEL            0x5

#define CL_ME_CHROMA_INTRA_PREDICT_ENABLED_INTEL        0x1
#define CL_ME_LUMA_INTRA_PREDICT_ENABLED_INTEL          0x2

#define CL_ME_COST_PENALTY_NONE_INTEL                   0x0
#define CL_ME_COST_PENALTY_LOW_INTEL                    0x1
#define CL_ME_COST_PENALTY_NORMAL_INTEL                 0x2
#define CL_ME_COST_PENALTY_HIGH_INTEL                   0x3

#define CL_ME_COST_PRECISION_QPEL_INTEL                 0x0
#define CL_ME_COST_PRECISION_HPEL_INTEL                 0x1
#define CL_ME_COST_PRECISION_PEL_INTEL                  0x2
#define CL_ME_COST_PRECISION_DPEL_INTEL                 0x3

typedef struct _cl_motion_estimation_desc_intel {
    cl_uint mb_block_type;
    cl_uint subpixel_mode;
    cl_uint sad_adjust_mode;
    cl_uint search_path_type;
} cl_motion_estimation_desc_intel;

///////////////////////////////////////////////////////////////////////////////
// cl_intel_packed_yuv

#define CL_YUYV_INTEL                               0x4076
#define CL_UYVY_INTEL                               0x4077
#define CL_YVYU_INTEL                               0x4078
#define CL_VYUY_INTEL                               0x4079

///////////////////////////////////////////////////////////////////////////////
// cl_intel_planar_yuv

// cl_channel_order
#define CL_NV12_INTEL                               0x410E

// cl_mem_flags
#define CL_MEM_NO_ACCESS_INTEL                      (1 << 24)
#define CL_MEM_ACCESS_FLAGS_UNRESTRICTED_INTEL      (1 << 25)

// cl_device_info
#define CL_DEVICE_PLANAR_YUV_MAX_WIDTH_INTEL        0x417E
#define CL_DEVICE_PLANAR_YUV_MAX_HEIGHT_INTEL       0x417F

///////////////////////////////////////////////////////////////////////////////
// cl_intel_required_subgroup_size

#define CL_DEVICE_SUB_GROUP_SIZES_INTEL             0x4108
#define CL_KERNEL_SPILL_MEM_SIZE_INTEL              0x4109
#define CL_KERNEL_COMPILE_SUB_GROUP_SIZE_INTEL      0x410A

///////////////////////////////////////////////////////////////////////////////
// cl_intel_sharing_format_query

// Minimal set of types for cl_intel_sharing_format_query.
typedef void D3DFORMAT;
typedef void DXGI_FORMAT;
typedef void VAImageFormat;

extern CL_API_ENTRY
cl_int CL_API_CALL clGetSupportedGLTextureFormatsINTEL(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_object_type image_type,
    cl_uint num_entries,
    cl_GLenum* gl_formats,
    cl_uint* num_texture_formats);

extern CL_API_ENTRY
cl_int CL_API_CALL clGetSupportedDX9MediaSurfaceFormatsINTEL(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_object_type image_type,
    cl_uint plane,
    cl_uint num_entries,
    D3DFORMAT* dx9_formats,
    cl_uint* num_surface_formats);

extern CL_API_ENTRY
cl_int CL_API_CALL clGetSupportedD3D10TextureFormatsINTEL(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_object_type image_type,
    cl_uint num_entries,
    DXGI_FORMAT* d3d10_formats,
    cl_uint* num_texture_formats);

extern CL_API_ENTRY
cl_int CL_API_CALL clGetSupportedD3D11TextureFormatsINTEL(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_object_type image_type,
    cl_uint plane,
    cl_uint num_entries,
    DXGI_FORMAT* d3d11_formats,
    cl_uint* num_texture_formats);

extern CL_API_ENTRY
cl_int CL_API_CALL clGetSupportedVA_APIMediaSurfaceFormatsINTEL(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_object_type image_type,
    cl_uint plane,
    cl_uint num_entries,
    VAImageFormat* va_api_formats,
    cl_uint* num_surface_formats);

///////////////////////////////////////////////////////////////////////////////
// cl_intel_simultaneous_sharing

#define CL_DEVICE_SIMULTANEOUS_INTEROPS_INTEL       0x4104
#define CL_DEVICE_NUM_SIMULTANEOUS_INTEROPS_INTEL   0x4105

///////////////////////////////////////////////////////////////////////////////
// cl_intel_thread_local_exec

#define CL_QUEUE_THREAD_LOCAL_EXEC_ENABLE_INTEL     (((cl_bitfield)1) << 31)

///////////////////////////////////////////////////////////////////////////////
// cl_intel_unified_shared_memory POC

// These enums are in sync with revision Q of the USM spec.

// cl_device_info
#define CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL                   0x4190
#define CL_DEVICE_DEVICE_MEM_CAPABILITIES_INTEL                 0x4191
#define CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL   0x4192
#define CL_DEVICE_CROSS_DEVICE_SHARED_MEM_CAPABILITIES_INTEL    0x4193
#define CL_DEVICE_SHARED_SYSTEM_MEM_CAPABILITIES_INTEL          0x4194

typedef cl_bitfield cl_device_unified_shared_memory_capabilities_intel;

// cl_unified_shared_memory_capabilities_intel - bitfield
#define CL_UNIFIED_SHARED_MEMORY_ACCESS_INTEL                   (1 << 0)
#define CL_UNIFIED_SHARED_MEMORY_ATOMIC_ACCESS_INTEL            (1 << 1)
#define CL_UNIFIED_SHARED_MEMORY_CONCURRENT_ACCESS_INTEL        (1 << 2)
#define CL_UNIFIED_SHARED_MEMORY_CONCURRENT_ATOMIC_ACCESS_INTEL (1 << 3)

typedef cl_properties cl_mem_properties_intel;

// cl_mem_properties_intel
#define CL_MEM_ALLOC_FLAGS_INTEL        0x4195

typedef cl_bitfield cl_mem_alloc_flags_intel;

// cl_mem_alloc_flags_intel - bitfield
#define CL_MEM_ALLOC_WRITE_COMBINED_INTEL               (1 << 0)

typedef cl_uint cl_mem_info_intel;

// cl_mem_alloc_info_intel
#define CL_MEM_ALLOC_TYPE_INTEL         0x419A
#define CL_MEM_ALLOC_BASE_PTR_INTEL     0x419B
#define CL_MEM_ALLOC_SIZE_INTEL         0x419C
#define CL_MEM_ALLOC_DEVICE_INTEL       0x419D
/* CL_MEM_ALLOC_FLAGS_INTEL - defined above */
#define CL_MEM_ALLOC_INFO_TBD0_INTEL    0x419E  /* reserved for future */
#define CL_MEM_ALLOC_INFO_TBD1_INTEL    0x419F  /* reserved for future */

typedef cl_uint cl_unified_shared_memory_type_intel;

// cl_unified_shared_memory_type_intel
#define CL_MEM_TYPE_UNKNOWN_INTEL       0x4196
#define CL_MEM_TYPE_HOST_INTEL          0x4197
#define CL_MEM_TYPE_DEVICE_INTEL        0x4198
#define CL_MEM_TYPE_SHARED_INTEL        0x4199

typedef cl_uint cl_mem_advice_intel;

// cl_mem_advice_intel
#define CL_MEM_ADVICE_TBD0_INTEL        0x4208  /* reserved for future */
#define CL_MEM_ADVICE_TBD1_INTEL        0x4209  /* reserved for future */
#define CL_MEM_ADVICE_TBD2_INTEL        0x420A  /* reserved for future */
#define CL_MEM_ADVICE_TBD3_INTEL        0x420B  /* reserved for future */
#define CL_MEM_ADVICE_TBD4_INTEL        0x420C  /* reserved for future */
#define CL_MEM_ADVICE_TBD5_INTEL        0x420D  /* reserved for future */
#define CL_MEM_ADVICE_TBD6_INTEL        0x420E  /* reserved for future */
#define CL_MEM_ADVICE_TBD7_INTEL        0x420F  /* reserved for future */

// cl_kernel_exec_info
#define CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL      0x4200
#define CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL    0x4201
#define CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL    0x4202
#define CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL                  0x4203

// cl_command_type
#define CL_COMMAND_MEMFILL_INTEL        0x4204
#define CL_COMMAND_MEMCPY_INTEL         0x4205
#define CL_COMMAND_MIGRATEMEM_INTEL     0x4206
#define CL_COMMAND_MEMADVISE_INTEL      0x4207

extern CL_API_ENTRY
void* CL_API_CALL clHostMemAllocINTEL(
    cl_context context,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret);

extern CL_API_ENTRY
void* CL_API_CALL clDeviceMemAllocINTEL(
    cl_context context,
    cl_device_id device,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret);

extern CL_API_ENTRY
void* CL_API_CALL clSharedMemAllocINTEL(
    cl_context context,
    cl_device_id device,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret);

extern CL_API_ENTRY
cl_int CL_API_CALL clMemFreeINTEL(
    cl_context context,
    void* ptr);

extern CL_API_ENTRY
cl_int CL_API_CALL clMemBlockingFreeINTEL(
    cl_context context,
    void* ptr);

extern CL_API_ENTRY
cl_int CL_API_CALL clGetMemAllocInfoINTEL(
    cl_context context,
    const void* ptr,
    cl_mem_info_intel param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret);

extern CL_API_ENTRY
cl_int CL_API_CALL clSetKernelArgMemPointerINTEL(
    cl_kernel kernel,
    cl_uint arg_index,
    const void* arg_value);

// Memset has been deprecated and replaced by Memfill.
// This function can eventually be removed.
extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueMemsetINTEL(       // Deprecated
    cl_command_queue command_queue,
    void* dst_ptr,
    cl_int value,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueMemFillINTEL(
    cl_command_queue command_queue,
    void* dst_ptr,
    const void* pattern,
    size_t pattern_size,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueMemcpyINTEL(
    cl_command_queue command_queue,
    cl_bool blocking,
    void* dst_ptr,
    const void* src_ptr,
    size_t size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueMigrateMemINTEL(
    cl_command_queue command_queue,
    const void* ptr,
    size_t size,
    cl_mem_migration_flags flags,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueMemAdviseINTEL(
    cl_command_queue command_queue,
    const void* ptr,
    size_t size,
    cl_mem_advice_intel advice,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

///////////////////////////////////////////////////////////////////////////////
// cl_intel_va_api_media_sharing

#define CL_VA_API_DISPLAY_INTEL                     0x4094
#define CL_PREFERRED_DEVICES_FOR_VA_API_INTEL       0x4095
#define CL_ALL_DEVICES_FOR_VA_API_INTEL             0x4096
#define CL_CONTEXT_VA_API_DISPLAY_INTEL             0x4097
#define CL_MEM_VA_API_SURFACE_INTEL                 0x4098
#define CL_IMAGE_VA_API_PLANE_INTEL                 0x4099
#define CL_COMMAND_ACQUIRE_VA_API_MEDIA_SURFACES_INTEL 0x409A
#define CL_COMMAND_RELEASE_VA_API_MEDIA_SURFACES_INTEL 0x409B

#define CL_INVALID_VA_API_MEDIA_ADAPTER_INTEL       -1098
#define CL_INVALID_VA_API_MEDIA_SURFACE_INTEL       -1099
#define CL_VA_API_MEDIA_SURFACE_ALREADY_ACQUIRED_INTEL -1100
#define CL_VA_API_MEDIA_SURFACE_NOT_ACQUIRED_INTEL  -1101

// Minimal set of types for cl_intel_va_api_media_sharing.
typedef cl_uint cl_va_api_device_source_intel;
typedef cl_uint cl_va_api_device_set_intel;
struct VASurfaceID;

extern CL_API_ENTRY
cl_int CL_API_CALL clGetDeviceIDsFromVA_APIMediaAdapterINTEL(
    cl_platform_id platform,
    cl_va_api_device_source_intel media_adapter_type,
    void *media_adapter,
    cl_va_api_device_set_intel media_adapter_set,
    cl_uint num_entries,
    cl_device_id *devices,
    cl_uint *num_devices);

extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromVA_APIMediaSurfaceINTEL(
    cl_context context,
    cl_mem_flags flags,
    VASurfaceID *surface,
    cl_uint plane,
    cl_int *errcode_ret);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueAcquireVA_APIMediaSurfacesINTEL(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem *mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event);

extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueReleaseVA_APIMediaSurfacesINTEL(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem *mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event);

///////////////////////////////////////////////////////////////////////////////
// cl_nv_create_buffer

typedef cl_bitfield         cl_mem_flags_NV;

extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateBufferNV(
    cl_context context,
    cl_mem_flags flags,
    cl_mem_flags_NV flags_NV,
    size_t size,
    void* host_ptr,
    cl_int* errcode_ret);

#define CL_MEM_LOCATION_HOST_NV                     (1 << 0)
#define CL_MEM_PINNED_NV                            (1 << 1)

///////////////////////////////////////////////////////////////////////////////
// cl_nv_device_attribute_query

#define CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV       0x4000
#define CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV       0x4001
#define CL_DEVICE_REGISTERS_PER_BLOCK_NV            0x4002
#define CL_DEVICE_WARP_SIZE_NV                      0x4003
#define CL_DEVICE_GPU_OVERLAP_NV                    0x4004
#define CL_DEVICE_KERNEL_EXEC_TIMEOUT_NV            0x4005
#define CL_DEVICE_INTEGRATED_MEMORY_NV              0x4006
#define CL_DEVICE_ATTRIBUTE_ASYNC_ENGINE_COUNT_NV   0x4007
#define CL_DEVICE_PCI_BUS_ID_NV                     0x4008
#define CL_DEVICE_PCI_SLOT_ID_NV                    0x4009
#define CL_DEVICE_PCI_DOMAIN_ID_NV                  0x400A

///////////////////////////////////////////////////////////////////////////////
// cl_qcom_ext_host_ptr

#define CL_MEM_EXT_HOST_PTR_QCOM                    (1 << 29)

#define CL_DEVICE_EXT_MEM_PADDING_IN_BYTES_QCOM     0x40A0
#define CL_DEVICE_PAGE_SIZE_QCOM                    0x40A1
#define CL_IMAGE_ROW_ALIGNMENT_QCOM                 0x40A2
#define CL_IMAGE_SLICE_ALIGNMENT_QCOM               0x40A3
#define CL_MEM_HOST_UNCACHED_QCOM                   0x40A4
#define CL_MEM_HOST_WRITEBACK_QCOM                  0x40A5
#define CL_MEM_HOST_WRITETHROUGH_QCOM               0x40A6
#define CL_MEM_HOST_WRITE_COMBINING_QCOM            0x40A7

///////////////////////////////////////////////////////////////////////////////
// cl_qcom_ion_host_ptr

#define CL_MEM_ION_HOST_PTR_QCOM                    0x40A8

///////////////////////////////////////////////////////////////////////////////
// Unofficial MDAPI extension:

extern CL_API_ENTRY
cl_command_queue CL_API_CALL clCreatePerfCountersCommandQueueINTEL(
    cl_context context,
    cl_device_id device,
    cl_command_queue_properties properties,
    cl_uint configuration,
    cl_int* errcode_ret);

extern CL_API_ENTRY
cl_int CL_API_CALL clSetPerformanceConfigurationINTEL(
    cl_device_id    device,
    cl_uint         count,
    cl_uint*        offsets,
    cl_uint*        values );

#define CL_QUEUE_MDAPI_PROPERTIES_INTEL             0x425E
#define CL_QUEUE_MDAPI_CONFIGURATION_INTEL          0x425F

#define CL_QUEUE_MDAPI_ENABLE_INTEL                 (1 << 0)

///////////////////////////////////////////////////////////////////////////////
// Unofficial kernel profiling extension:

#define CL_CONTEXT_KERNEL_PROFILING_MODES_COUNT_INTEL   0x407A
#define CL_CONTEXT_KERNEL_PROFILING_MODE_INFO_INTEL     0x407B
#define CL_KERNEL_IL_SYMBOLS_INTEL                      0x407C
#define CL_KERNEL_BINARY_PROGRAM_INTEL                  0x407D

///////////////////////////////////////////////////////////////////////////////
// Unofficial VTune Debug Info extension:

#define CL_PROGRAM_DEBUG_INFO_INTEL                     0x4100
#define CL_PROGRAM_DEBUG_INFO_SIZES_INTEL               0x4101
#define CL_KERNEL_BINARIES_INTEL                        0x4102
#define CL_KERNEL_BINARY_SIZES_INTEL                    0x4103

///////////////////////////////////////////////////////////////////////////////
// Unofficial cl_get_kernel_suggested_local_work_size extension:

extern CL_API_ENTRY
cl_int CL_API_CALL clGetKernelSuggestedLocalWorkSizeINTEL(
    cl_command_queue commandQueue,
    cl_kernel kernel,
    cl_uint workDim,
    const size_t *globalWorkOffset,
    const size_t *globalWorkSize,
    size_t *suggestedLocalWorkSize);

///////////////////////////////////////////////////////////////////////////////
// Unofficial cl_intel_maximum_registers extension:

#define CL_KERNEL_REGISTER_COUNT_INTEL 0x425B

#ifdef __cplusplus
}
#endif
