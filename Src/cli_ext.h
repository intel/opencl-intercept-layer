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

#pragma once

// cl_khr_gl_sharing
extern CL_API_ENTRY
cl_int CL_API_CALL clGetGLContextInfoKHR(
    const cl_context_properties *properties,
    cl_gl_context_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret);

// cl_khr_gl_event
#define CL_COMMAND_GL_FENCE_SYNC_OBJECT_KHR     0x200D

// cl_khr_gl_event
extern CL_API_ENTRY
cl_event CL_API_CALL clCreateEventFromGLsyncKHR(
    cl_context context,
    cl_GLsync sync,
    cl_int* errcode_ret);

#if defined(_WIN32)

// Minimal set of types for cl_khr_d3d10_sharing.
// Don't include cl_d3d10.h here because we don't want a dependency on d3d10.h.
typedef cl_uint cl_d3d10_device_source_khr;
typedef cl_uint cl_d3d10_device_set_khr;
class ID3D10Buffer;
class ID3D10Texture2D;
class ID3D10Texture3D;

// cl_khr_d3d10_sharing
extern CL_API_ENTRY
cl_int CL_API_CALL clGetDeviceIDsFromD3D10KHR(
    cl_platform_id platform,
    cl_d3d10_device_source_khr d3d_device_source,
    void* d3d_object,
    cl_d3d10_device_set_khr d3d_device_set,
    cl_uint num_entries,
    cl_device_id* devices,
    cl_uint* num_devices);

// cl_khr_d3d10_sharing
extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromD3D10BufferKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D10Buffer* resource,
    cl_int* errcode_ret);

// cl_khr_d3d10_sharing
extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromD3D10Texture2DKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D10Texture2D* resource,
    UINT subresource,
    cl_int* errcode_ret);

// cl_khr_d3d10_sharing
extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromD3D10Texture3DKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D10Texture3D* resource,
    UINT subresource,
    cl_int* errcode_ret);

// cl_khr_d3d10_sharing
extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueAcquireD3D10ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

// cl_khr_d3d10_sharing
extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueReleaseD3D10ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

// Minimal set of types for cl_khr_d3d11_sharing.
// Don't include cl_d3d11.h here because we don't want a dependency on d3d10.h.
typedef cl_uint cl_d3d11_device_source_khr;
typedef cl_uint cl_d3d11_device_set_khr;
class ID3D11Buffer;
class ID3D11Texture2D;
class ID3D11Texture3D;

// cl_khr_d3d11_sharing
extern CL_API_ENTRY
cl_int CL_API_CALL clGetDeviceIDsFromD3D11KHR(
    cl_platform_id platform,
    cl_d3d11_device_source_khr d3d_device_source,
    void* d3d_object,
    cl_d3d11_device_set_khr d3d_device_set,
    cl_uint num_entries,
    cl_device_id* devices,
    cl_uint* num_devices);

// cl_khr_d3d11_sharing
extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromD3D11BufferKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D11Buffer* resource,
    cl_int* errcode_ret);

// cl_khr_d3d11_sharing
extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromD3D11Texture2DKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D11Texture2D* resource,
    UINT subresource,
    cl_int* errcode_ret);

// cl_khr_d3d11_sharing
extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromD3D11Texture3DKHR(
    cl_context context,
    cl_mem_flags flags,
    ID3D11Texture3D* resource,
    UINT subresource,
    cl_int* errcode_ret);

// cl_khr_d3d11_sharing
extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueAcquireD3D11ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

// cl_khr_d3d11_sharing
extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueReleaseD3D11ObjectsKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

// Minimal set of types for cl_khr_dx9_media_sharing.
// Don't include cl_d3d9.h here because we don't want a dependency on d3d9.h.
typedef cl_uint cl_dx9_media_adapter_type_khr;
typedef cl_uint cl_dx9_media_adapter_set_khr;
typedef cl_uint cl_dx9_media_adapter_type_khr;
class IDirect3DSurface9;

// cl_khr_dx9_media_sharing
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

// cl_khr_dx9_media_sharing
extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromDX9MediaSurfaceKHR(
    cl_context context,
    cl_mem_flags flags,
    cl_dx9_media_adapter_type_khr adapter_type,
    void* surface_info,
    cl_uint plane,
    cl_int* errcode_ret);

// cl_khr_dx9_media_sharing
extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueAcquireDX9MediaSurfacesKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

// cl_khr_dx9_media_sharing
extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueReleaseDX9MediaSurfacesKHR(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event);

// Minimal set of types for cl_intel_d3d9_media_sharing.
// Don't include cl_d3d9.h here because we don't want a dependency on d3d9.h.
typedef cl_uint cl_dx9_device_source_intel;
typedef cl_uint cl_dx9_device_set_intel;
class IDirect3DSurface9;

// cl_intel_dx9_media_sharing
extern CL_API_ENTRY
cl_int CL_API_CALL clGetDeviceIDsFromDX9INTEL(
    cl_platform_id platform,
    cl_dx9_device_source_intel d3d_device_source,
    void *dx9_object,
    cl_dx9_device_set_intel d3d_device_set,
    cl_uint num_entries,
    cl_device_id* devices,
    cl_uint* num_devices );

// cl_intel_dx9_media_sharing
extern CL_API_ENTRY
cl_mem CL_API_CALL clCreateFromDX9MediaSurfaceINTEL(
    cl_context context,
    cl_mem_flags flags,
    IDirect3DSurface9* resource,
    HANDLE sharedHandle,
    UINT plane,
    cl_int* errcode_ret );

// cl_intel_dx9_media_sharing
extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueAcquireDX9ObjectsINTEL(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event );

// cl_intel_dx9_media_sharing
extern CL_API_ENTRY
cl_int CL_API_CALL clEnqueueReleaseDX9ObjectsINTEL(
    cl_command_queue command_queue,
    cl_uint num_objects,
    const cl_mem* mem_objects,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event );

#endif

// cl_khr_il_program
#define CL_DEVICE_IL_VERSION_KHR                        0x105B
#define CL_PROGRAM_IL_KHR                               0x1169
extern CL_API_ENTRY
cl_program CL_API_CALL clCreateProgramWithILKHR(
    cl_context context,
    const void* il,
    size_t length,
    cl_int* errcode_ret );

// cl_khr_subgroups
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

// cl_khr_create_command_queue
typedef cl_bitfield cl_queue_properties_khr;
extern CL_API_ENTRY
cl_command_queue CL_API_CALL clCreateCommandQueueWithPropertiesKHR(
    cl_context context,
    cl_device_id device,
    const cl_queue_properties_khr* properties,
    cl_int* errcode_ret);

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

// Unofficial kernel profiling extension:
#define CL_CONTEXT_KERNEL_PROFILING_MODES_COUNT_INTEL   0x407A
#define CL_CONTEXT_KERNEL_PROFILING_MODE_INFO_INTEL     0x407B
#define CL_KERNEL_IL_SYMBOLS_INTEL                      0x407C
#define CL_KERNEL_BINARY_PROGRAM_INTEL                  0x407D

// Unofficial VTune Debug Info extension:
#define CL_PROGRAM_DEBUG_INFO_INTEL                     0x4100
#define CL_PROGRAM_DEBUG_INFO_SIZES_INTEL               0x4101
#define CL_KERNEL_BINARIES_INTEL                        0x4102
#define CL_KERNEL_BINARY_SIZES_INTEL                    0x4103

// VME

typedef struct _cl_accelerator_intel*     cl_accelerator_intel;
typedef cl_uint                           cl_accelerator_type_intel;
typedef cl_uint                           cl_accelerator_info_intel;

// Error Codes
#define CL_INVALID_ACCELERATOR_INTEL                    -1094
#define CL_INVALID_ACCELERATOR_TYPE_INTEL               -1095
#define CL_INVALID_ACCELERATOR_DESC_INTEL               -1096
#define CL_ACCELERATOR_TYPE_NOT_SUPPORTED_INTEL         -1097

// cl_device_info
#define CL_DEVICE_ME_VERSION_INTEL                      0x407E
#define CL_DEVICE_TRANSFORM_MASK_MAX_WIDTH_INTEL        0x409C
#define CL_DEVICE_TRANSFORM_MASK_MAX_HEIGHT_INTEL       0x409D
#define CL_DEVICE_TRANSFORM_FILTER_MAX_WIDTH_INTEL      0x409E
#define CL_DEVICE_TRANSFORM_FILTER_MAX_HEIGHT_INTEL     0x409F

// cl_accelerator_type_intel
#define CL_ACCELERATOR_TYPE_MOTION_ESTIMATION_INTEL     0x0

// cl_accelerator_info_intel
#define CL_ACCELERATOR_DESCRIPTOR_INTEL                 0x4090
#define CL_ACCELERATOR_REFERENCE_COUNT_INTEL            0x4091
#define CL_ACCELERATOR_CONTEXT_INTEL                    0x4092
#define CL_ACCELERATOR_TYPE_INTEL                       0x4093

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

#define CL_ME_VERSION_LEGACY_INTEL                      0x0
#define CL_ME_VERSION_ADVANCED_VER_1_INTEL              0x1

typedef struct _cl_motion_estimation_desc_intel {
    cl_uint mb_block_type;
    cl_uint subpixel_mode;
    cl_uint sad_adjust_mode;
    cl_uint search_path_type;
} cl_motion_estimation_desc_intel;

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

// cl_intel_egl_image_yuv
#define CL_EGL_YUV_PLANE_INTEL                      0x4107

// cl_intel_simultaneous_sharing
#define CL_DEVICE_SIMULTANEOUS_INTEROPS_INTEL       0x4104
#define CL_DEVICE_NUM_SIMULTANEOUS_INTEROPS_INTEL   0x4105

// cl_intel_thread_local_exec
#define CL_QUEUE_THREAD_LOCAL_EXEC_ENABLE_INTEL     (((cl_bitfield)1) << 31)

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

// cl_intel_packed_yuv
#define CL_YUYV_INTEL                               0x4076
#define CL_UYVY_INTEL                               0x4077
#define CL_YVYU_INTEL                               0x4078
#define CL_VYUY_INTEL                               0x4079

// cl_intel_planar_yuv

// cl_channel_order
#define CL_NV12_INTEL                               0x410E

// cl_mem_flags
#define CL_MEM_NO_ACCESS_INTEL                      (1 << 24)
#define CL_MEM_ACCESS_FLAGS_UNRESTRICTED_INTEL      (1 << 25)

// cl_device_info
#define CL_DEVICE_PLANAR_YUV_MAX_WIDTH_INTEL        0x417E
#define CL_DEVICE_PLANAR_YUV_MAX_HEIGHT_INTEL       0x417F

// cl_intel_required_subgroup_size
#define CL_DEVICE_SUB_GROUP_SIZES_INTEL             0x4108
#define CL_KERNEL_SPILL_MEM_SIZE_INTEL              0x4109
#define CL_KERNEL_COMPILE_SUB_GROUP_SIZE_INTEL      0x410A

// cl_intel_driver_diagnostics
#define CL_CONTEXT_SHOW_DIAGNOSTICS_INTEL           0x4106

// cl_intelx_video_enhancement
// This is the base-functionality VEBox extension.
// Note: These are preview enum names and values!

// cl_device_info
#define CL_DEVICE_VE_VERSION_INTEL                      0x4160
#define CL_DEVICE_VE_ENGINE_COUNT_INTEL                 0x4161

// cl_queue_properties / cl_command_queue_info
#define CL_QUEUE_VE_ENABLE_INTEL                        0x4162

// attribute_ids for cl_vebox_attrib_desc_intel
#define CL_VE_ACCELERATOR_ATTRIB_DENOISE_INTEL          0x4163
#define CL_VE_ACCELERATOR_ATTRIB_DEINTERLACE_INTEL      0x4164
#define CL_VE_ACCELERATOR_ATTRIB_HOT_PIXEL_CORR_INTEL   0x4165

// cl_accelerator_info_intel
#define CL_VE_ACCELERATOR_HISTOGRAMS_INTEL              0x4166
#define CL_VE_ACCELERATOR_STATISTICS_INTEL              0x4167
#define CL_VE_ACCELERATOR_STMM_INPUT_INTEL              0x4168
#define CL_VE_ACCELERATOR_STMM_OUTPUT_INTEL             0x4169

// cl_intelx_ve_color_pipeline
// Note: These are preview enum names and values!

// cl_device_info
#define CL_DEVICE_VE_COLOR_PIPE_VERSION_INTEL           0x416A

// attribute_ids for cl_vebox_attrib_desc_intel
#define CL_VE_ACCELERATOR_ATTRIB_STD_STE_INTEL          0x416B
#define CL_VE_ACCELERATOR_ATTRIB_GAMUT_COMP_INTEL       0x416C
#define CL_VE_ACCELERATOR_ATTRIB_GECC_INTEL             0x416D
#define CL_VE_ACCELERATOR_ATTRIB_ACE_INTEL              0x416E
#define CL_VE_ACCELERATOR_ATTRIB_ACE_ADV_INTEL          0x416F
#define CL_VE_ACCELERATOR_ATTRIB_TCC_INTEL              0x4170
#define CL_VE_ACCELERATOR_ATTRIB_PROC_AMP_INTEL         0x4171
#define CL_VE_ACCELERATOR_ATTRIB_BACK_END_CSC_INTEL     0x4172
#define CL_VE_ACCELERATOR_ATTRIB_AOI_ALPHA_INTEL        0x4173
#define CL_VE_ACCELERATOR_ATTRIB_CCM_INTEL              0x4174
#define CL_VE_ACCELERATOR_ATTRIB_FWD_GAMMA_CORRECT_INTEL 0x4175
#define CL_VE_ACCELERATOR_ATTRIB_FRONT_END_CSC_INTEL    0x4176

// cl_intelx_ve_camera_pipeline
// Note, these are preview enum names and values!

// cl_device_info
#define CL_DEVICE_VE_CAMERA_PIPE_VERSION_INTEL          0x4177

// attribute_ids for cl_vebox_attrib_desc_intel
#define CL_VE_ACCELERATOR_ATTRIB_BLACK_LEVEL_CORR_INTEL 0x4178
#define CL_VE_ACCELERATOR_ATTRIB_DEMOSAIC_INTEL         0x4179
#define CL_VE_ACCELERATOR_ATTRIB_WHITE_BALANCE_CORR_INTEL 0x417A
#define CL_VE_ACCELERATOR_ATTRIB_VIGNETTE_INTEL         0x417B

// HEVC PAK
// Note, this extension is still in development!

// cl_device_info
#define CL_DEVICE_PAK_VERSION_INTEL                     0x4180
#define CL_DEVICE_PAK_AVAILABLE_CODECS_INTEL            0x4181

// cl_queue_properties / cl_command_queue_info
#define CL_QUEUE_PAK_ENABLE_INTEL                       0x4189

// cl_accelerator_info_intel
#define CL_PAK_CTU_COUNT_INTEL                          0x4182
#define CL_PAK_CTU_WIDTH_INTEL                          0x4183
#define CL_PAK_CTU_HEIGHT_INTEL                         0x4184
#define CL_PAK_MAX_INTRA_DEPTH_INTEL                    0x4185
#define CL_PAK_MAX_INTER_DEPTH_INTEL                    0x4186
#define CL_PAK_NUM_CUS_PER_CTU_INTEL                    0x4187
#define CL_PAK_MV_BUFFER_SIZE_INTEL                     0x4188

// Error Codes
// These are currently all mapped to CL_INVALID_VALUE.
// Need official error code assignment.
#define CL_INVALID_PAK_CTU_SIZE_INTEL                   CL_INVALID_VALUE
#define CL_INVALID_PAK_TU_SIZE_INTEL                    CL_INVALID_VALUE
#define CL_INVALID_PAK_TU_INTRA_DEPTH_INTEL             CL_INVALID_VALUE
#define CL_INVALID_PAK_TU_INTER_DEPTH_INTEL             CL_INVALID_VALUE
#define CL_INVALID_PAK_BITRATE_RANGE_INTEL              CL_INVALID_VALUE
#define CL_INVALID_PAK_INSERTION_INTEL                  CL_INVALID_VALUE
#define CL_INVALID_PAK_CTU_POSITION_INTEL               CL_INVALID_VALUE
#define CL_INVALID_PAK_REFERENCE_IMAGE_INDEX_INTEL      CL_INVALID_VALUE

// Altera Extensions:

// cl_altera_device_temperature
#define CL_DEVICE_CORE_TEMPERATURE_ALTERA               0x40F3

// cl_altera_compiler_mode
#define CL_CONTEXT_COMPILER_MODE_ALTERA                 0x40F0
#define CL_CONTEXT_PROGRAM_EXE_LIBRARY_ROOT_ALTERA      0x40F1
#define CL_CONTEXT_OFFLINE_DEVICE_ALTERA                0x40F2

// These are from the Khronos cl_ext.h:

// cl_khr_icd
#define CL_PLATFORM_ICD_SUFFIX_KHR                  0x0920
#define CL_PLATFORM_NOT_FOUND_KHR                   -1001

// cl_khr_initalize_memory
#define CL_CONTEXT_MEMORY_INITIALIZE_KHR            0x2030

// cl_khr_terminate_context
#define CL_DEVICE_TERMINATE_CAPABILITY_KHR          0x2031
#define CL_CONTEXT_TERMINATE_KHR                    0x2032

// cl_khr_spir
#define CL_DEVICE_SPIR_VERSIONS                     0x40E0
#define CL_PROGRAM_BINARY_TYPE_INTERMEDIATE         0x40E1

// cl_khr_subgroups
#define CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE_KHR    0x2033
#define CL_KERNEL_SUB_GROUP_COUNT_FOR_NDRANGE_KHR   0x2034

// cl_nv_device_attribute_query
#define CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV       0x4000
#define CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV       0x4001
#define CL_DEVICE_REGISTERS_PER_BLOCK_NV            0x4002
#define CL_DEVICE_WARP_SIZE_NV                      0x4003
#define CL_DEVICE_GPU_OVERLAP_NV                    0x4004
#define CL_DEVICE_KERNEL_EXEC_TIMEOUT_NV            0x4005
#define CL_DEVICE_INTEGRATED_MEMORY_NV              0x4006

// cl_ext_atomic_counters
#define CL_DEVICE_MAX_ATOMIC_COUNTERS_EXT           0x4032

// cl_amd_device_attribute_query
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

// cl_amd_offline_devices
#define CL_CONTEXT_OFFLINE_DEVICES_AMD              0x403F

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

// cl_qcom_ion_host_ptr
#define CL_MEM_ION_HOST_PTR_QCOM                    0x40A8

// cl_arm_printf extension
#define CL_PRINTF_CALLBACK_ARM                      0x40B0
#define CL_PRINTF_BUFFERSIZE_ARM                    0x40B1
