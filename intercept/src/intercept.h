/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/
#pragma once

#include <atomic>
#include <chrono>
#include <cinttypes>
#include <fstream>
#include <list>
#include <vector>
#include <map>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <unordered_map>

#include <stdint.h>

#include "common.h"

#include "chrometracer.h"
#include "enummap.h"
#include "dispatch.h"
#include "objtracker.h"

#include "instrumentation.h"

#if defined(USE_MDAPI)
#include "MetricsDiscoveryHelper.h"
#endif

#if defined(_WIN32)

#elif defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)

#include <string.h>
#define strcpy_s( _dst, _size, _src )   strncpy( _dst, _src, _size )

#else
#error Unknown OS!
#endif

#include "OS/OS.h"

class CLIntercept
{
    struct Config;

public:
#if defined(CLINTERCEPT_HIGH_RESOLUTON_CLOCK)
    using clock = std::chrono::high_resolution_clock;
#else
    using clock = std::chrono::steady_clock;
#endif

    static bool Create( void* pGlobalData, CLIntercept*& pIntercept );
    static void Delete( CLIntercept*& pIntercept );

    void    report();

    void    callLoggingEnter(
                const char* functionName,
                const uint64_t enqueueCounter,
                const cl_kernel kernel );
    void    callLoggingEnter(
                const char* functionName,
                const uint64_t enqueueCounter,
                const cl_kernel kernel,
                const char* formatStr,
                ... );

    void    callLoggingInfo(
                const std::string& str );
    void    callLoggingInfo(
                const char* formatStr,
                ... );

    void    callLoggingExit(
                const char* functionName,
                const cl_int errorCode,
                const cl_event* event );
    void    callLoggingExit(
                const char* functionName,
                const cl_int errorCode,
                const cl_event* event,
                const char* formatStr,
                ... );

    void    cacheDeviceInfo(
                cl_device_id device );
    void    getDeviceIndexString(
                cl_device_id device,
                std::string& str );
    cl_int  getDeviceMajorMinorVersion(
                cl_device_id device,
                size_t& majorVersion,
                size_t& minorVersion ) const;
    bool    getMajorMinorVersionFromString(
                const char* prefix,
                const char* str,
                size_t& majorVersion,
                size_t& minorVersion ) const;
    bool    getDeviceIndex(
                cl_device_id device,
                cl_uint& platformIndex,
                cl_uint& deviceIndex ) const;
    bool    checkDeviceForExtension(
                cl_device_id device,
                const char* extensionName ) const;

    cl_int  allocateAndGetPlatformInfoString(
                cl_platform_id platform,
                cl_platform_info param_name,
                char*& param_value ) const;
    cl_int  allocateAndGetDeviceInfoString(
                cl_device_id device,
                cl_device_info param_name,
                char*& param_value ) const;
    cl_int  allocateAndGetKernelInfoString(
                cl_kernel kernel,
                cl_kernel_info param_name,
                char*& param_value ) const;
    cl_int  allocateAndGetProgramDeviceList(
                cl_program program,
                cl_uint& numDevices,
                cl_device_id*& deviceList ) const;
    cl_int  allocateAndGetKernelISABinary(
                cl_kernel kernel,
                cl_device_id device,
                size_t& kernelISABinarySize,
                char*& kernelISABinary ) const;

    void    getPlatformInfoString(
                cl_platform_id platform,
                std::string& str ) const;
    void    getDeviceInfoString(
                cl_uint num_devices,
                const cl_device_id* devices,
                std::string& str ) const;
    void    getDevicePartitionPropertiesString(
                const cl_device_partition_property* properties,
                std::string& str ) const;
    void    getEventListString(
                cl_uint num_events,
                const cl_event* event_list,
                std::string& str ) const;
    void    getSemaphoreListString(
                cl_uint num_semaphores,
                const cl_semaphore_khr* semaphore_list,
                std::string& str ) const;
    void    getContextPropertiesString(
                const cl_context_properties* properties,
                std::string& str ) const;
    void    getSamplerPropertiesString(
                const cl_sampler_properties* properties,
                std::string& str ) const;
    void    getCommandQueuePropertiesString(
                const cl_queue_properties* properties,
                std::string& str ) const;
    void    getMemPropertiesString(
                const cl_mem_properties* properties,
                std::string& str ) const;
    void    getSemaphorePropertiesString(
                const cl_semaphore_properties_khr* properties,
                std::string& str ) const;
    void    getCommandBufferPropertiesString(
                const cl_command_buffer_properties_khr* properties,
                std::string& str ) const;
    void    getCommandBufferMutableConfigString(
                const cl_mutable_base_config_khr* mutable_config,
                std::string& str ) const;
    void    getCreateKernelsInProgramRetString(
                cl_int retVal,
                cl_kernel* kernels,
                cl_uint* num_kernels_ret,
                std::string& str ) const;
    void    getKernelArgString(
                cl_uint arg_index,
                size_t arg_size,
                const void* arg_value,
                std::string& str ) const;
    void    getEnqueueNDRangeKernelArgsString(
                cl_uint work_dim,
                const size_t* global_work_offset,
                const size_t* global_work_size,
                const size_t* local_work_size,
                std::string& str ) const;
    void    getCreateSubBufferArgsString(
                cl_buffer_create_type createType,
                const void *createInfo,
                std::string& str ) const;

    void    logCLInfo();
    void    logBuild(
                clock::time_point buildTimeStart,
                const cl_program program,
                cl_uint num_devices,
                const cl_device_id* device_list );
    void    logError(
                const char* functionName,
                cl_int errorCode );
    void    logFlushOrFinishAfterEnqueueStart(
                const char* flushOrFinish,
                const char* functionName );
    void    logFlushOrFinishAfterEnqueueEnd(
                const char* flushOrFinish,
                const char* functionName,
                cl_int errorCode );
    void    logKernelInfo(
                const cl_kernel* kernels,
                cl_uint numKernels );
    void    logQueueInfo(
                const cl_device_id device,
                const cl_command_queue queue );

    void    logCL_GLTextureDetails( cl_mem image, cl_GLenum target, cl_GLint miplevel, cl_GLuint texture );

    cl_int  autoPartitionGetDeviceIDs(
        cl_platform_id platform,
        cl_device_type device_type,
        cl_uint num_entries,
        cl_device_id* devices,
        cl_uint* num_devices );

    struct SContextCallbackInfo
    {
        CLIntercept*        pIntercept;
        void (CL_CALLBACK*  pApplicationCallback)( const char*, const void*, size_t, void* );
        void*               pUserData;
    };
    static void CL_CALLBACK contextCallbackCaller(
                                const char*,
                                const void*,
                                size_t,
                                void* );
    void    contextCallback(
                const std::string& errinfo,
                const void* private_info,
                size_t cb );
    void    contextCallbackOverrideInit(
                const cl_context_properties* properties,
                void (CL_CALLBACK*& pCallback)( const char*, const void*, size_t, void* ),
                void*& pUserData,
                SContextCallbackInfo*& pContextCallbackInfo,
                cl_context_properties*& pLocalContextProperties );
    void    contextCallbackOverrideCleanup(
                const cl_context context,
                SContextCallbackInfo*& pContextCallbackInfo,
                cl_context_properties*& pLocalContextProperties );

    struct SEventCallbackInfo
    {
        CLIntercept*        pIntercept;
        void (CL_CALLBACK*  pApplicationCallback)( cl_event, cl_int, void* );
        void*               pUserData;
    };
    static void CL_CALLBACK eventCallbackCaller(
                                cl_event,
                                cl_int,
                                void* );
    void    eventCallback(
                cl_event event,
                cl_int status );

    void    overrideNullLocalWorkSize(
                const cl_uint work_dim,
                const size_t* global_work_size,
                const size_t*& local_work_size );

    void    combineProgramStrings(
                cl_uint& count,
                const char**& strings,
                const size_t*& lengths,
                char*& singleString ) const;

    void    incrementProgramCompileCount(
                const cl_program program );
    uint64_t computeHash(
                const void* ptr,
                size_t length );
    void    saveProgramHash(
                const cl_program program,
                uint64_t hash );
    void    saveProgramOptionsHash(
                const cl_program program,
                const char* options );

    bool    injectProgramSource(
                const uint64_t hash,
                cl_uint& count,
                const char**& strings,
                const size_t*& lengths,
                char*& singleString );
    bool    prependProgramSource(
                const uint64_t hash,
                cl_uint& count,
                const char**& strings,
                const size_t*& lengths,
                char*& singleString );
    bool    injectProgramSPIRV(
                const uint64_t hash,
                size_t& length,
                const void*& il,
                char*& injectedIL );
    bool    injectProgramOptions(
                const cl_program program,
                cl_bool isCompile,
                cl_bool isLink,
                char*& newOptions );
    bool    appendBuildOptions(
                const char* append,
                const char* options,
                char*& newOptions ) const;
    void    dumpProgramSourceScript(
                const cl_program program,
                const char* singleString );
    void    dumpProgramSource(
                const cl_program program,
                uint64_t hash,
                bool modified,
                const char* singleString );
    void    dumpInputProgramBinaries(
                const cl_program program,
                uint64_t hash,
                bool modified,
                cl_uint num_devices,
                const cl_device_id* device_list,
                const size_t* lengths,
                const unsigned char** binaries );
    void    dumpProgramSPIRV(
                const cl_program program,
                uint64_t hash,
                bool modified,
                const size_t length,
                const void* il );
    void    dumpProgramOptionsScript(
                const cl_program program,
                const char* options );
    void    dumpProgramOptions(
                const cl_program program,
                bool modified,
                cl_bool isCompile,
                cl_bool isLink,
                const char* options );
    void    dumpProgramBuildLog(
                const cl_program program,
                const cl_device_id device,
                const char* buildLog,
                const size_t buildLogSize );

    cl_program createProgramWithInjectionBinaries(
                uint64_t hash,
                cl_context context,
                cl_int* errcode_ret );
    void    dumpProgramBinary(
                const cl_program program );

    void    dumpKernelISABinaries(
                const cl_program program );

    cl_program createProgramWithInjectionSPIRV(
                uint64_t hash,
                cl_context context,
                cl_int* errcode_ret );
    void    autoCreateSPIRV(
                const cl_program program,
                const char* options );

    void    getTimingTagBlocking(
                const char* functionName,
                const cl_bool blocking,
                const size_t size,
                std::string& hostTag,
                std::string& deviceTag );
    void    getTimingTagsMap(
                const char* functionName,
                const cl_map_flags flags,
                const cl_bool blocking,
                const size_t size,
                std::string& hostTag,
                std::string& deviceTag );
    void    getTimingTagsUnmap(
                const char* functionName,
                const void* ptr,
                std::string& hostTag,
                std::string& deviceTag );
    void    getTimingTagsMemfill(
                const char* functionName,
                const cl_command_queue queue,
                const void* dst,
                const size_t size,
                std::string& hostTag,
                std::string& deviceTag );
    void    getTimingTagsMemcpy(
                const char* functionName,
                const cl_command_queue queue,
                const cl_bool blocking,
                const void* dst,
                const void* src,
                const size_t size,
                std::string& hostTag,
                std::string& deviceTag );
    void    getTimingTagsKernel(
                const cl_command_queue queue,
                const cl_kernel kernel,
                const cl_uint workDim,
                const size_t* gwo,
                const size_t* gws,
                const size_t* lws,
                std::string& hostTag,
                std::string& deviceTag );

    void    updateHostTimingStats(
                const char* functionName,
                const std::string& tag,
                clock::time_point start,
                clock::time_point end );

    void    modifyCommandQueueProperties(
                cl_command_queue_properties& props ) const;
    void    createCommandQueueProperties(
                cl_device_id device,
                cl_command_queue_properties props,
                cl_queue_properties*& pLocalQueueProperties ) const;
    void    createCommandQueuePropertiesOverride(
                cl_device_id device,
                const cl_queue_properties* properties,
                cl_queue_properties*& pLocalQueueProperties ) const;
    bool    checkHostPerformanceTimingEnqueueLimits(
                uint64_t enqueueCounter ) const;
    bool    checkDevicePerformanceTimingEnqueueLimits(
                uint64_t enqueueCounter ) const;
    void    dummyCommandQueue(
                cl_context context,
                cl_device_id device );

    void    addTimingEvent(
                const char* functionName,
                const uint64_t enqueueCounter,
                const clock::time_point queuedTime,
                const std::string& tag,
                const cl_command_queue queue,
                cl_event event );
    void    checkTimingEvents();

    cl_command_queue    getCommandBufferCommandQueue(
                cl_uint numQueues,
                cl_command_queue* queues,
                cl_command_buffer_khr cmdbuf );

    cl_command_queue    createCommandQueueWithProperties(
                cl_context context,
                cl_device_id device,
                const cl_queue_properties* properties,
                cl_int* errcode_ret );

    void    addSubDeviceInfo(
                cl_device_id parentDevice,
                const cl_device_id* subdevices,
                cl_uint numSubDevices );
    void    checkRemoveDeviceInfo(
                cl_device_id device );

    void    addKernelInfo(
                const cl_kernel kernel,
                const cl_program program,
                const std::string& kernelName );
    void    addKernelInfo(
                const cl_kernel* kernels,
                const cl_program program,
                cl_uint numKernels );
    void    addKernelInfo(
                const cl_kernel kernel,
                const cl_kernel source_kernel );
    void    checkRemoveKernelInfo(
                const cl_kernel kernel );

    void    addAcceleratorInfo(
                cl_accelerator_intel accelerator,
                cl_context context );
    void    checkRemoveAcceleratorInfo(
                cl_accelerator_intel accelerator );

    void    addSemaphoreInfo(
                cl_semaphore_khr semaphore,
                cl_context context );
    void    checkRemoveSemaphoreInfo(
                cl_semaphore_khr semaphore );

    void    addCommandBufferInfo(
                cl_command_buffer_khr cmdbuf,
                cl_command_queue queue);
    void    checkRemoveCommandBufferInfo(
                cl_command_buffer_khr cmdbuf );

    void    addMutableCommandInfo(
                cl_mutable_command_khr cmd,
                cl_command_buffer_khr cmdbuf,
                cl_uint dim );

    void    addSamplerString(
                cl_sampler sampler,
                const std::string& str );
    void    checkRemoveSamplerString(
                cl_sampler sampler );
    bool    checkGetSamplerString(
                size_t size,
                const void *arg_value,
                std::string& str ) const;

    void    addQueue(
                cl_context context,
                cl_command_queue queue );
    void    checkRemoveQueue(
                cl_command_queue queue );
    void    addEvent(
                cl_event event,
                uint64_t enqueueCounter );
    void    checkRemoveEvent(
                cl_event event );
    void    addBuffer(
                cl_mem buffer );
    void    addImage(
                cl_mem image );
    void    checkRemoveMemObj(
                cl_mem memobj );
    void    addSVMAllocation(
                void* svmPtr,
                size_t size );
    void    removeSVMAllocation(
                void* svmPtr );
    void    addUSMAllocation(
                void* usmPtr,
                size_t size );
    void    removeUSMAllocation(
                void* usmPtr );
    void    setKernelArg(
                cl_kernel kernel,
                cl_uint arg_index,
                cl_mem memobj );
    void    setKernelArg(
                cl_kernel kernel,
                cl_uint arg_index,
                const void* arg_value,
                size_t arg_size );
    void    setKernelArgSVMPointer(
                cl_kernel kernel,
                cl_uint arg_index,
                const void* arg );
    void    setKernelArgUSMPointer(
                cl_kernel kernel,
                cl_uint arg_index,
                const void* arg );
    void    dumpBuffersForKernel(
                const std::string& name,
                const uint64_t enqueueCounter,
                cl_kernel kernel,
                cl_command_queue command_queue,
                bool replay,
                bool byKernelName );
    void    dumpArgumentsForKernel(
                cl_kernel kernel,
                uint64_t enqueueCounter,
                bool byKernelName );
    void    dumpKernelSourceOrDeviceBinary(
                cl_kernel kernel,
                uint64_t enqueueCounter,
                bool byKernelName );
    void    dumpKernelInfo(
                cl_kernel kernel,
                uint64_t enqueueCounter,
                size_t work_dim,
                const size_t* global_work_offset,
                const size_t* global_work_size,
                const size_t* local_work_size,
                bool byKernelName );
    void    dumpImagesForKernel(
                const std::string& name,
                const uint64_t enqueueCounter,
                cl_kernel kernel,
                cl_command_queue command_queue,
                bool replay,
                bool byKernelName );

    void    dumpArgument(
                const uint64_t enqueueCounter,
                cl_kernel kernel,
                cl_int arg_index,
                size_t size,
                const void *pBuffer );
    void    dumpBuffer(
                const std::string& name,
                const uint64_t enqueueCounter,
                cl_mem memobj,
                cl_command_queue command_queue,
                void* ptr,
                size_t offset,
                size_t size );

    void    addMapPointer(
                const void* ptr,
                const cl_map_flags flags,
                const size_t size );
    void    removeMapPointer(
                const void* ptr );

    void    checkEventList(
                const char* functionName,
                cl_uint numEvents,
                const cl_event* eventList,
                cl_event* event );
    void    checkKernelArgUSMPointer(
                cl_kernel kernel,
                const void* arg );

    bool    checkRelaxAllocationLimitsSupport(
                cl_program program ) const;
    bool    checkRelaxAllocationLimitsSupport(
                cl_uint numDevices,
                const cl_device_id* deviceList ) const;
    void    usmAllocPropertiesOverride(
                const cl_mem_properties_intel* properties,
                cl_mem_properties_intel*& pLocalAllocProperties ) const;

    void    startAubCapture(
                const char* functionName,
                const uint64_t enqueueCounter,
                const cl_kernel kernel,
                const cl_uint workDim,
                const size_t* gws,
                const size_t* lws,
                cl_command_queue commandQueue );
    void    stopAubCapture(
                cl_command_queue commandQueue );

    void    initPrecompiledKernelOverrides(
                const cl_context context );
    void    initBuiltinKernelOverrides(
                const cl_context context );

    cl_int  writeStringToMemory(
                size_t param_value_size,
                const std::string& param,
                size_t* param_value_size_ret,
                char* pointer ) const;
    template< class T >
    cl_int  writeVectorToMemory(
                size_t param_value_size,
                const std::vector<T>& param,
                size_t* param_value_size_ret,
                T* pointer ) const;
    template< class T >
    cl_int  writeParamToMemory(
                size_t param_value_size,
                T param,
                size_t* param_value_size_ret,
                T* pointer ) const;

    bool    overrideGetPlatformInfo(
                cl_platform_id platform,
                cl_platform_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret,
                cl_int& errorCode ) const;
    bool    overrideGetDeviceInfo(
                cl_device_id device,
                cl_platform_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret,
                cl_int& errorCode ) const;

    cl_int  ReadBuffer(
                cl_command_queue commandQueue,
                cl_mem srcBuffer,
                cl_bool blockingRead,
                size_t srcOffset,
                size_t bytesToRead,
                void* dstPtr,
                cl_uint numEventsInWaitList,
                const cl_event* eventWaitList,
                cl_event* event );
    cl_int  WriteBuffer(
                cl_command_queue commandQueue,
                cl_mem dstBuffer,
                cl_bool blockingWrite,
                size_t dstOffset,
                size_t bytesToWrite,
                const void* srcPtr,
                cl_uint numEventsInWaitList,
                const cl_event* eventWaitList,
                cl_event* event );
    cl_int  CopyBuffer(
                cl_command_queue commandQueue,
                cl_mem srcBuffer,
                cl_mem dstBuffer,
                size_t srcOffset,
                size_t dstOffset,
                size_t bytesToCopy,
                cl_uint numEventsInWaitList,
                const cl_event* eventWaitList,
                cl_event* event );
    cl_int  CopyBufferHelper(
                cl_context context,
                cl_command_queue commandQueue,
                cl_mem srcBuffer,
                cl_mem dstBuffer,
                size_t srcOffset,
                size_t dstOffset,
                size_t bytesToCopy,
                cl_uint numEventsInWaitList,
                const cl_event* eventWaitList,
                cl_event* event );

    cl_int  ReadImage(
                cl_command_queue commandQueue,
                cl_mem srcImage,
                cl_bool blockingRead,
                const size_t* srcOrigin,
                const size_t* region,
                size_t dstRowPitch,
                size_t dstSlicePitch,
                void* dstPtr,
                cl_uint numEventsInWaitList,
                const cl_event* eventWaitList,
                cl_event* event );
    cl_int  WriteImage(
                cl_command_queue commandQueue,
                cl_mem dstImage,
                cl_bool blockingWrite,
                const size_t* dstOrigin,
                const size_t* region,
                size_t srcRowPitch,
                size_t srcSlicePitch,
                const void* srcPtr,
                cl_uint numEventsInWaitList,
                const cl_event* eventWaitList,
                cl_event* event );
    cl_int  CopyImage(
                cl_command_queue commandQueue,
                cl_mem srcImage,
                cl_mem dstImage,
                const size_t* srcOrigin,
                const size_t* dstOrigin,
                const size_t* region,
                cl_uint numEventsInWaitList,
                const cl_event* eventWaitList,
                cl_event* event );
    cl_int  CopyImageHelper(
                cl_context context,
                cl_command_queue commandQueue,
                cl_mem srcImage,
                cl_mem dstImage,
                const size_t* srcOrigin,
                const size_t* dstOrigin,
                const size_t* region,
                cl_uint numEventsInWaitList,
                const cl_event* eventWaitList,
                cl_event* event );

    cl_program createProgramWithBuiltinKernels(
                cl_context context );
    cl_kernel createBuiltinKernel(
                cl_program program,
                const std::string& kernel_name,
                cl_int* errcode_ret );
    cl_int  NDRangeBuiltinKernel(
                cl_command_queue commandQueue,
                cl_kernel kernel,
                cl_uint work_dim,
                const size_t* global_work_offset,
                const size_t* global_work_size,
                const size_t* local_work_size,
                cl_uint num_events_in_wait_list,
                const cl_event* event_wait_list,
                cl_event* event );

    void*   getExtensionFunctionAddress(
                cl_platform_id platform,
                const std::string& func_name );

#if defined(USE_MDAPI)
    void    initCustomPerfCounters();

    cl_command_queue    createMDAPICommandQueue(
                cl_context context,
                cl_device_id device,
                cl_command_queue_properties properties,
                cl_int* errcode_ret );
    cl_command_queue    createMDAPICommandQueue(
                cl_context context,
                cl_device_id device,
                const cl_queue_properties* properties,
                cl_int* errcode_ret );
#endif

    const cl_icd_dispatch&  dispatch() const;

    const CLdispatchX&  dispatchX( cl_accelerator_intel accelerator ) const;
    const CLdispatchX&  dispatchX( cl_command_queue queue ) const;
    const CLdispatchX&  dispatchX( cl_context context ) const;
    const CLdispatchX&  dispatchX( cl_device_id device ) const;
    const CLdispatchX&  dispatchX( cl_kernel kernel ) const;
    const CLdispatchX&  dispatchX( cl_mem memobj ) const;
    const CLdispatchX&  dispatchX( cl_platform_id platform ) const;
    const CLdispatchX&  dispatchX( cl_semaphore_khr semaphore ) const;
    const CLdispatchX&  dispatchX( cl_command_buffer_khr cmdbuf ) const;
    const CLdispatchX&  dispatchX( cl_mutable_command_khr cmd ) const;

    cl_platform_id  getPlatform( cl_accelerator_intel accelerator ) const;
    cl_platform_id  getPlatform( cl_command_queue queue ) const;
    cl_platform_id  getPlatform( cl_context context ) const;
    cl_platform_id  getPlatform( cl_device_id device ) const;
    cl_platform_id  getPlatform( cl_kernel kernel ) const;
    cl_platform_id  getPlatform( cl_mem memobj ) const;
    cl_platform_id  getPlatform( cl_semaphore_khr semaphore ) const;
    cl_platform_id  getPlatform( cl_command_buffer_khr cmdbuf ) const;
    cl_platform_id  getPlatform( cl_mutable_command_khr cmd ) const;

    cl_uint getRefCount( cl_accelerator_intel accelerator );
    cl_uint getRefCount( cl_command_queue queue ) const;
    cl_uint getRefCount( cl_context conest ) const;
    cl_uint getRefCount( cl_device_id device ) const;
    cl_uint getRefCount( cl_event event ) const;
    cl_uint getRefCount( cl_program program ) const;
    cl_uint getRefCount( cl_kernel kernel ) const;
    cl_uint getRefCount( cl_mem memobj ) const;
    cl_uint getRefCount( cl_sampler sampler ) const;
    cl_uint getRefCount( cl_semaphore_khr semaphore );
    cl_uint getRefCount( cl_command_buffer_khr cmdbuf );

    const OS::Services& OS() const;

    const CEnumNameMap& enumName() const;

    const Config&   config() const;

    uint64_t    getEnqueueCounter() const;
    uint64_t    incrementEnqueueCounter();

    CObjectTracker& objectTracker();

    bool    dumpBufferForKernel( const cl_kernel kernel );
    bool    dumpImagesForKernel( const cl_kernel kernel );
    bool    checkDumpBufferEnqueueLimits( uint64_t enqueueCounter ) const;
    bool    checkDumpImageEnqueueLimits( uint64_t enqueueCounter ) const;
    bool    checkDumpByCounter( uint64_t enqueueCounter ) const;
    bool    checkDumpByName ( cl_kernel kernel );

    bool    checkAubCaptureEnqueueLimits( uint64_t enqueueCounter ) const;
    bool    checkAubCaptureKernelSignature(
                const cl_kernel kernel,
                cl_uint workDim,
                const size_t* gws,
                const size_t* lws);

    unsigned int    getThreadNumber( uint64_t threadId );

    void    saveProgramNumber( const cl_program program );
    unsigned int    getProgramNumber() const;

    cl_device_type filterDeviceType( cl_device_type device_type ) const;

#if defined(USE_ITT)
    __itt_domain*   ittDomain() const;

    void    ittInit();

    void    ittCallLoggingEnter(
                const char* functionName,
                const cl_kernel kernel );
    void    ittCallLoggingExit();

    void    ittRegisterCommandQueue(
                cl_command_queue queue,
                bool supportsPerfCounters );
    void    ittReleaseCommandQueue(
                cl_command_queue );
    void    ittTraceEvent(
                const std::string& name,
                cl_event event,
                clock::time_point queuedTime,
                cl_ulong commandQueued,
                cl_ulong commandSubmit,
                cl_ulong commandStart,
                cl_ulong commandEnd );
#endif

    void    chromeCallLoggingExit(
                const char* functionName,
                const std::string& tag,
                bool includeId,
                const uint64_t enqueueCounter,
                clock::time_point start,
                clock::time_point end );
    void    chromeRegisterCommandQueue(
                cl_command_queue queue );
    void    chromeTraceEvent(
                const std::string& name,
                bool useProfilingDelta,
                int64_t profilingDeltaNS,
                uint64_t enqueueCounter,
                unsigned int queueNumber,
                clock::time_point queuedTime,
                cl_ulong commandQueued,
                cl_ulong commandSubmit,
                cl_ulong commandStart,
                cl_ulong commandEnd );
    void    flushChromeTraceBuffering();

    // USM Emulation:
    void*   emulatedHostMemAlloc(
                cl_context context,
                const cl_mem_properties_intel* properties,
                size_t size,
                cl_uint alignment,
                cl_int* errcode_ret);
    void*   emulatedDeviceMemAlloc(
                cl_context context,
                cl_device_id device,
                const cl_mem_properties_intel* properties,
                size_t size,
                cl_uint alignment,
                cl_int* errcode_ret);
    void*   emulatedSharedMemAlloc(
                cl_context context,
                cl_device_id device,
                const cl_mem_properties_intel* properties,
                size_t size,
                cl_uint alignment,
                cl_int* errcode_ret);
    cl_int  emulatedMemFree(
                cl_context context,
                const void* ptr );
    cl_int  emulatedGetMemAllocInfoINTEL(
                cl_context context,
                const void* ptr,
                cl_mem_info_intel param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret);

    cl_int  trackUSMKernelExecInfo(
                cl_kernel kernel,
                cl_kernel_exec_info param_name,
                size_t param_value_size,
                const void* param_value);
    cl_int  setUSMKernelExecInfo(
                cl_command_queue queue,
                cl_kernel kernel );

    cl_int  finishAll(
                cl_context conetxt );

    void saveSampler(
                cl_kernel kernel,
                cl_uint arg_index,
                std::string const& sampler );

private:
    static const char* sc_URL;
    static const char* sc_DumpDirectoryName;
    static const char* sc_ReportFileName;
    static const char* sc_LogFileName;
    static const char* sc_TraceFileName;
    static const char* sc_DumpPerfCountersFileNamePrefix;

#if defined(CLINTERCEPT_CMAKE)
    static const char* sc_GitDescribe;
    static const char* sc_GitRefSpec;
    static const char* sc_GitHash;
#endif

    CLIntercept( void* pGlobalData );
    ~CLIntercept();

    bool    init();
    void    log(const std::string& s);
    void    logf(const char* str, ...);

    void    logPlatformInfo( cl_platform_id platform );
    void    logDeviceInfo( cl_device_id device );

#if defined(_WIN32) || defined(__linux__) || defined(__FreeBSD__)
    bool    initDispatch( const std::string& libName );
#elif defined(__APPLE__)
    bool    initDispatch( void );
#else
#error Unknown OS!
#endif
    void    addShortKernelName(
                const std::string& kernelName );
    std::string getShortKernelName(
                    const cl_kernel kernel );
    std::string getShortKernelNameWithHash(
                    const cl_kernel kernel );

    void    getCallLoggingPrefix(
                std::string& str );

    void    writeReport(
                std::ostream& os );

    uint64_t        m_ProcessId;
    std::mutex      m_Mutex;

    typedef std::map< cl_platform_id, CLdispatchX > CLdispatchXMap;

    OS::Services    m_OS;
    cl_icd_dispatch m_Dispatch;
    CLdispatchXMap  m_DispatchX;
    CEnumNameMap    m_EnumNameMap;
    CObjectTracker  m_ObjectTracker;

    void*       m_OpenCLLibraryHandle;

    std::ofstream   m_InterceptLog;
    CChromeTracer   m_ChromeTrace;

    mutable char    m_StringBuffer[CLI_STRING_BUFFER_SIZE];

    bool        m_LoggedCLInfo;

    std::atomic<uint64_t>   m_EnqueueCounter;

    clock::time_point   m_StartTime;

    typedef std::map< uint64_t, unsigned int>   CThreadNumberMap;
    CThreadNumberMap    m_ThreadNumberMap;

    typedef std::map< cl_device_id, std::vector<cl_device_id> > CSubDeviceCacheMap;
    CSubDeviceCacheMap  m_SubDeviceCacheMap;

    unsigned int    m_EventsChromeTraced;

    unsigned int    m_ProgramNumber;

    // This defines a mapping between a sub-device handle and information
    // about the sub-device.

    struct SSubDeviceInfo
    {
        cl_device_id    ParentDevice;
        cl_uint         SubDeviceIndex;
    };

    typedef std::map< cl_device_id, SSubDeviceInfo >    CSubDeviceInfoMap;
    CSubDeviceInfoMap   m_SubDeviceInfoMap;

    // This defines a mapping between the program handle and information
    // about the program.

    struct SProgramInfo
    {
        unsigned int    ProgramNumber;
        unsigned int    CompileCount;

        uint64_t        ProgramHash;
        uint64_t        OptionsHash;
    };

    typedef std::map< cl_program, SProgramInfo >    CProgramInfoMap;
    CProgramInfoMap m_ProgramInfoMap;

    struct SHostTimingStats
    {
        SHostTimingStats() :
            NumberOfCalls(0),
            MinNS(ULLONG_MAX),
            MaxNS(0),
            TotalNS(0) {}

        uint64_t    NumberOfCalls;
        uint64_t    MinNS;
        uint64_t    MaxNS;
        uint64_t    TotalNS;
    };

    typedef std::unordered_map< std::string, SHostTimingStats > CHostTimingStatsMap;
    CHostTimingStatsMap  m_HostTimingStatsMap;

    // These structures define a mapping between a device ID handle and
    // properties of a device, for easier querying.

    struct SDeviceInfo
    {
        cl_device_id    ParentDevice;   // null for root devices
        cl_uint     PlatformIndex;      // zero for sub-devices
        cl_uint     DeviceIndex;

        cl_device_type  Type;

        std::string Name;
        std::string NameForReport;

        cl_uint     NumericVersion;

        cl_uint     NumComputeUnits;
        cl_uint     MaxClockFrequency;

        bool        HasDeviceAndHostTimer;
        int64_t     DeviceHostTimeDeltaNS;

        bool        Supports_cl_khr_create_command_queue;
        bool        Supports_cl_khr_subgroups;
    };

    typedef std::map< cl_device_id, SDeviceInfo >   CDeviceInfoMap;
    CDeviceInfoMap  m_DeviceInfoMap;

    // These structures define a mapping between a key and a device
    // timing record.  The key consists of a device ID and a string
    // identifier.  The string identifier usually consists of the
    // kernel name and possibly some extra information such as the
    // local and global work size, The kernel name could be the "real"
    // kernel name, or a kernel name ID, or is the OpenCL API name for
    // non-kernel device timing stats.

    struct SDeviceTimingStats
    {
        SDeviceTimingStats() :
            NumberOfCalls(0),
            MinNS(CL_ULONG_MAX),
            MaxNS(0),
            TotalNS(0) {}

        uint64_t    NumberOfCalls;
        cl_ulong    MinNS;
        cl_ulong    MaxNS;
        cl_ulong    TotalNS;
    };

    typedef std::unordered_map< std::string, SDeviceTimingStats >   CDeviceTimingStatsMap;
    typedef std::map< cl_device_id, CDeviceTimingStatsMap > CDeviceDeviceTimingStatsMap;
    CDeviceDeviceTimingStatsMap m_DeviceTimingStatsMap;

    // This defines a mapping between the kernel handle and information
    // about the kernel.

    struct SKernelInfo
    {
        std::string     KernelName;

        uint64_t        ProgramHash;
        uint64_t        OptionsHash;

        unsigned int    ProgramNumber;
        unsigned int    CompileCount;
    };

    typedef std::map< cl_kernel, SKernelInfo >  CKernelInfoMap;
    CKernelInfoMap  m_KernelInfoMap;

    // This defines a mapping between the "real" kernel name and a kernel
    // name ID.  Only kernels with names larger than a control variable
    // will be added to this map.

    unsigned int    m_KernelID;

    typedef std::unordered_map< std::string, std::string >  CLongKernelNameMap;
    CLongKernelNameMap  m_LongKernelNameMap;

    // This is a list of pending events that haven't been added to the
    // device timing stats map yet.

    struct SEventListNode
    {
        cl_device_id        Device;
        unsigned int        QueueNumber;
        std::string         Name;
        uint64_t            EnqueueCounter;
        clock::time_point   QueuedTime;
        bool                UseProfilingDelta;
        int64_t             ProfilingDeltaNS;
        cl_event            Event;
    };

    typedef std::list< SEventListNode > CEventList;
    CEventList  m_EventList;

#if defined(USE_MDAPI)
    MetricsDiscovery::MDHelper* m_pMDHelper;
    MetricsDiscovery::CMetricAggregations m_MetricAggregations;

    std::ofstream   m_MetricDump;

    void    getMDAPICountersFromStream( void );
    void    getMDAPICountersFromEvent(
                const std::string& name,
                const cl_event event );
    void    reportMDAPICounters(
                std::ostream& os );
#endif

    unsigned int    m_QueueNumber;

    typedef std::map< cl_command_queue, unsigned int >  CQueueNumberMap;
    CQueueNumberMap m_QueueNumberMap;

    typedef std::list< cl_command_queue >   CQueueList;
    typedef std::map< cl_context, CQueueList >  CContextQueuesMap;
    CContextQueuesMap   m_ContextQueuesMap;

    typedef std::map< cl_event, uint64_t >  CEventIdMap;
    CEventIdMap m_EventIdMap;

    unsigned int    m_MemAllocNumber;

    typedef std::map< const void*, unsigned int >   CMemAllocNumberMap;
    CMemAllocNumberMap  m_MemAllocNumberMap;

    typedef std::map< cl_sampler, std::string > CSamplerDataMap;
    CSamplerDataMap m_SamplerDataMap;

    typedef std::map< cl_mem, size_t >  CBufferInfoMap;
    CBufferInfoMap      m_BufferInfoMap;

    typedef std::map< const void*, size_t > CSVMAllocInfoMap;
    CSVMAllocInfoMap    m_SVMAllocInfoMap;

    typedef std::map< const void*, size_t > CUSMAllocInfoMap;
    CUSMAllocInfoMap    m_USMAllocInfoMap;

    struct SImageInfo
    {
        size_t  Region[3];
        size_t  ElementSize;
        cl_image_format Format;
        size_t RowPitch;
        size_t SlicePitch;
        cl_mem_object_type ImageType;
    };

    typedef std::map< cl_mem, SImageInfo >  CImageInfoMap;
    CImageInfoMap   m_ImageInfoMap;

    typedef std::map< cl_uint, const void* >        CKernelArgMemMap;
    typedef std::map< cl_kernel, CKernelArgMemMap > CKernelArgMap;
    CKernelArgMap   m_KernelArgMap;

    typedef std::map< cl_uint, std::vector<unsigned char>> CKernelArgVectorMemMap;
    typedef std::map< cl_kernel, CKernelArgVectorMemMap > CKernelArgVectorMap;
    CKernelArgVectorMap m_KernelArgVectorMap;

    typedef std::map< cl_uint, size_t> CKernelArgLocalMemMap;
    typedef std::map< cl_kernel, CKernelArgLocalMemMap > CKernelArgLocalMap;
    CKernelArgLocalMap m_KernelArgLocalMap;

    typedef std::map<cl_program, std::string> CSourceStringMap;
    CSourceStringMap m_SourceStringMap;

    typedef std::map<cl_uint, std::string> CSamplerArgMap;
    typedef std::map<cl_kernel, CSamplerArgMap> CSamplerKernelArgMap;
    CSamplerKernelArgMap m_samplerKernelArgMap;

    struct SMapPointerInfo
    {
        cl_map_flags    Flags;
        size_t          Size;
    };

    typedef std::map< const void*, SMapPointerInfo >    CMapPointerInfoMap;
    CMapPointerInfoMap  m_MapPointerInfoMap;

    bool    m_AubCaptureStarted;
    cl_uint m_AubCaptureKernelEnqueueSkipCounter;
    cl_uint m_AubCaptureKernelEnqueueCaptureCounter;

    typedef std::set<std::string>   CAubCaptureSet;
    CAubCaptureSet  m_AubCaptureSet;

    typedef std::map< cl_context, SContextCallbackInfo* >   CContextCallbackInfoMap;
    CContextCallbackInfoMap m_ContextCallbackInfoMap;

    struct SPrecompiledKernelOverrides
    {
        cl_program  Program;

        cl_kernel   Kernel_CopyBufferBytes;
        cl_kernel   Kernel_CopyBufferUInts;
        cl_kernel   Kernel_CopyBufferUInt4s;
        cl_kernel   Kernel_CopyBufferUInt16s;

        cl_kernel   Kernel_CopyImage2Dto2DFloat;
        cl_kernel   Kernel_CopyImage2Dto2DInt;
        cl_kernel   Kernel_CopyImage2Dto2DUInt;
    };

    typedef std::map< cl_context, SPrecompiledKernelOverrides* >    CPrecompiledKernelOverridesMap;
    CPrecompiledKernelOverridesMap  m_PrecompiledKernelOverridesMap;

    struct SBuiltinKernelOverrides
    {
        cl_program  Program;

        cl_kernel   Kernel_block_motion_estimate_intel;
    };

    typedef std::map< cl_context, SBuiltinKernelOverrides* >    CBuiltinKernelOverridesMap;
    CBuiltinKernelOverridesMap  m_BuiltinKernelOverridesMap;

    typedef std::map< cl_accelerator_intel, cl_platform_id >    CAcceleratorInfoMap;
    CAcceleratorInfoMap     m_AcceleratorInfoMap;

    typedef std::map< cl_semaphore_khr, cl_platform_id >    CSemaphoreInfoMap;
    CSemaphoreInfoMap   m_SemaphoreInfoMap;

    typedef std::map< cl_command_buffer_khr, cl_platform_id >   CCommandBufferInfoMap;
    CCommandBufferInfoMap   m_CommandBufferInfoMap;

    struct SMutableCommandInfo
    {
        cl_platform_id  Platform;
        cl_uint         WorkDim;
    };

    typedef std::map< cl_mutable_command_khr, SMutableCommandInfo >  CMutableCommandInfoMap;
    CMutableCommandInfoMap  m_MutableCommandInfoMap;

    typedef std::list< cl_mutable_command_khr > CMutableCommandList;
    typedef std::map< cl_command_buffer_khr, CMutableCommandList >  CCommandBufferMutableCommandsMap;
    CCommandBufferMutableCommandsMap    m_CommandBufferMutableCommandsMap;

    struct Config
    {
#define CLI_CONTROL( _type, _name, _init, _desc )   _type _name;
#include "controls.h"
#undef CLI_CONTROL
    } m_Config;

#if defined(USE_ITT)
    bool                m_ITTInitialized;

    __itt_domain*       m_ITTDomain;

    //__ittx_task_state*  m_ITTQueuedState;
    //__ittx_task_state*  m_ITTSubmittedState;
    //__ittx_task_state*  m_ITTExecutingState;

    //__itt_track_group*  m_ITTQueueTrackGroup;

    static void ITTAPI ittClockInfoCallback(
        __itt_clock_info* pClockInfo,
        void* pData );

    struct SITTQueueInfo
    {
        const CLIntercept*  pIntercept;
        bool                SupportsPerfCounters;

        __itt_track*        itt_track;
        __itt_clock_domain* itt_clock_domain;
        clock::time_point   CPUReferenceTime;
        cl_ulong            CLReferenceTime;
    };

    typedef std::map< cl_command_queue, SITTQueueInfo > CITTQueueInfoMap;
    CITTQueueInfoMap    m_ITTQueueInfoMap;
#endif

    // USM Emulation:
    struct SUSMAllocInfo
    {
        SUSMAllocInfo() :
            Type( CL_MEM_TYPE_UNKNOWN_INTEL ),
            Device( NULL ),
            BaseAddress( NULL ),
            Size( 0 ),
            Alignment( 0 ) {}

        cl_unified_shared_memory_type_intel Type;
        cl_device_id    Device;

        const void*     BaseAddress;
        size_t          Size;
        size_t          Alignment;
    };

    typedef std::map< const void*, SUSMAllocInfo >  CUSMAllocMap;
    typedef std::vector<const void*>    CUSMAllocVector;

    struct SUSMContextInfo
    {
        CUSMAllocMap    AllocMap;

        CUSMAllocVector HostAllocVector;
        CUSMAllocVector DeviceAllocVector;
        CUSMAllocVector SharedAllocVector;
        // Note: We could differentiate between device allocs for
        // specific devices, but we do not do this currently.
    };

    typedef std::map< cl_context, SUSMContextInfo > CUSMContextInfoMap;
    CUSMContextInfoMap  m_USMContextInfoMap;

    struct SUSMKernelInfo
    {
        SUSMKernelInfo() :
            IndirectHostAccess( false ),
            IndirectDeviceAccess( false ),
            IndirectSharedAccess( false ) {}

        bool    IndirectHostAccess;
        bool    IndirectDeviceAccess;
        bool    IndirectSharedAccess;

        std::vector<void*>  SVMPtrs;
        std::vector<void*>  USMPtrs;
    };

    typedef std::map< cl_kernel, SUSMKernelInfo >   CUSMKernelInfoMap;
    CUSMKernelInfoMap   m_USMKernelInfoMap;

    DISALLOW_COPY_AND_ASSIGN( CLIntercept );
};

///////////////////////////////////////////////////////////////////////////////
//
template< class T >
cl_int CLIntercept::writeVectorToMemory(
    size_t param_value_size,
    const std::vector<T>& param,
    size_t *param_value_size_ret,
    T* pointer ) const
{
    cl_int  errorCode = CL_SUCCESS;

    size_t  size = param.size() * sizeof(T);

    if( pointer != NULL )
    {
        if( param_value_size < size )
        {
            errorCode = CL_INVALID_VALUE;
        }
        else
        {
            CLI_MEMCPY(pointer, size, param.data(), size);
        }
    }

    if( param_value_size_ret != NULL )
    {
        *param_value_size_ret = size;
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
template< class T >
cl_int CLIntercept::writeParamToMemory(
    size_t param_value_size,
    T param,
    size_t *param_value_size_ret,
    T* pointer ) const
{
    cl_int  errorCode = CL_SUCCESS;

    if( pointer != NULL )
    {
        if( param_value_size < sizeof(param) )
        {
            errorCode = CL_INVALID_VALUE;
        }
        else
        {
            *pointer = param;
        }
    }

    if( param_value_size_ret != NULL )
    {
        *param_value_size_ret = sizeof(param);
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
inline const cl_icd_dispatch& CLIntercept::dispatch() const
{
    return m_Dispatch;
}

///////////////////////////////////////////////////////////////////////////////
//
inline const CLdispatchX& CLIntercept::dispatchX( cl_accelerator_intel accelerator ) const
{
    cl_platform_id  platform = getPlatform( accelerator );
    return dispatchX( platform );
}

inline const CLdispatchX& CLIntercept::dispatchX( cl_command_queue queue ) const
{
    cl_platform_id  platform = getPlatform(queue);
    return dispatchX( platform );
}

inline const CLdispatchX& CLIntercept::dispatchX( cl_context context ) const
{
    cl_platform_id  platform = getPlatform(context);
    return dispatchX( platform );
}

inline const CLdispatchX& CLIntercept::dispatchX( cl_device_id device ) const
{
    cl_platform_id  platform = getPlatform(device);
    return dispatchX( platform );
}

inline const CLdispatchX& CLIntercept::dispatchX( cl_kernel kernel ) const
{
    cl_platform_id  platform = getPlatform(kernel);
    return dispatchX( platform );
}

inline const CLdispatchX& CLIntercept::dispatchX( cl_mem memobj ) const
{
    cl_platform_id  platform = getPlatform(memobj);
    return dispatchX( platform );
}

inline const CLdispatchX& CLIntercept::dispatchX( cl_platform_id platform ) const
{
    CLdispatchXMap::const_iterator iter = m_DispatchX.find(platform);
    if( iter != m_DispatchX.end() )
    {
        return iter->second;
    }
    else
    {
        return m_DispatchX.at(NULL);
    }
}

inline const CLdispatchX& CLIntercept::dispatchX( cl_semaphore_khr semaphore ) const
{
    cl_platform_id  platform = getPlatform( semaphore );
    return dispatchX( platform );
}

inline const CLdispatchX& CLIntercept::dispatchX( cl_command_buffer_khr cmdbuf ) const
{
    cl_platform_id  platform = getPlatform( cmdbuf );
    return dispatchX( platform );
}

inline const CLdispatchX& CLIntercept::dispatchX( cl_mutable_command_khr cmd ) const
{
    cl_platform_id  platform = getPlatform( cmd );
    return dispatchX( platform );
}

///////////////////////////////////////////////////////////////////////////////
//
inline cl_platform_id CLIntercept::getPlatform( cl_accelerator_intel accelerator ) const
{
    CAcceleratorInfoMap::const_iterator iter = m_AcceleratorInfoMap.find(accelerator);
    if( iter != m_AcceleratorInfoMap.end() )
    {
        return iter->second;
    }
    else
    {
        return NULL;
    }
}

inline cl_platform_id CLIntercept::getPlatform( cl_command_queue queue ) const
{
    cl_device_id device = NULL;
    dispatch().clGetCommandQueueInfo(
        queue,
        CL_QUEUE_DEVICE,
        sizeof(device),
        &device,
        NULL );
    return getPlatform(device);
}

inline cl_platform_id CLIntercept::getPlatform( cl_context context ) const
{
    cl_uint numDevices = 0;
    dispatch().clGetContextInfo(
        context,
        CL_CONTEXT_NUM_DEVICES,
        sizeof(numDevices),
        &numDevices,
        NULL );

    if( numDevices == 1 )   // fast path, no dynamic allocation
    {
        cl_device_id    device = NULL;
        dispatch().clGetContextInfo(
            context,
            CL_CONTEXT_DEVICES,
            sizeof(cl_device_id),
            &device,
            NULL );
        return getPlatform(device);
    }
    else if( numDevices )   // slower path, dynamic allocation
    {
        std::vector<cl_device_id>   devices(numDevices);
        dispatch().clGetContextInfo(
            context,
            CL_CONTEXT_DEVICES,
            numDevices * sizeof(cl_device_id),
            devices.data(),
            NULL );
        return getPlatform(devices[0]);
    }

    return NULL;
}

inline cl_platform_id CLIntercept::getPlatform( cl_device_id device ) const
{
    cl_platform_id platform = NULL;
    dispatch().clGetDeviceInfo(
        device,
        CL_DEVICE_PLATFORM,
        sizeof(platform),
        &platform,
        NULL );
    return platform;
}

inline cl_platform_id CLIntercept::getPlatform( cl_kernel kernel ) const
{
    cl_context context = NULL;
    dispatch().clGetKernelInfo(
        kernel,
        CL_KERNEL_CONTEXT,
        sizeof(context),
        &context,
        NULL);
    return getPlatform(context);
}

inline cl_platform_id CLIntercept::getPlatform( cl_mem memobj ) const
{
    cl_context context = NULL;
    dispatch().clGetMemObjectInfo(
        memobj,
        CL_MEM_CONTEXT,
        sizeof(context),
        &context,
        NULL);
    return getPlatform(context);
}

inline cl_platform_id CLIntercept::getPlatform( cl_semaphore_khr semaphore ) const
{
    CSemaphoreInfoMap::const_iterator iter = m_SemaphoreInfoMap.find(semaphore);
    if( iter != m_SemaphoreInfoMap.end() )
    {
        return iter->second;
    }
    else
    {
        return NULL;
    }
}

inline cl_platform_id CLIntercept::getPlatform( cl_command_buffer_khr cmdbuf ) const
{
    CCommandBufferInfoMap::const_iterator iter = m_CommandBufferInfoMap.find(cmdbuf);
    if( iter != m_CommandBufferInfoMap.end() )
    {
        return iter->second;
    }
    else
    {
        return NULL;
    }
}

inline cl_platform_id CLIntercept::getPlatform( cl_mutable_command_khr cmd ) const
{
    CMutableCommandInfoMap::const_iterator iter = m_MutableCommandInfoMap.find(cmd);
    if( iter != m_MutableCommandInfoMap.end() )
    {
        return iter->second.Platform;
    }
    else
    {
        return NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
inline cl_uint CLIntercept::getRefCount( cl_accelerator_intel accelerator )
{
    cl_platform_id  platform = this->getPlatform(accelerator);
    if( dispatchX(platform).clGetAcceleratorInfoINTEL == NULL )
    {
        getExtensionFunctionAddress(
            platform,
            "clGetAcceleratorInfoINTEL" );
    }

    cl_uint refCount = 0;
    const auto& dispatchX = this->dispatchX(platform);
    if( dispatchX.clGetAcceleratorInfoINTEL )
    {
        dispatchX.clGetAcceleratorInfoINTEL(
            accelerator,
            CL_ACCELERATOR_REFERENCE_COUNT_INTEL,
            sizeof(refCount),
            &refCount,
            NULL );
    }
    return refCount;
}

inline cl_uint CLIntercept::getRefCount( cl_command_queue queue ) const
{
    cl_uint refCount = 0;
    dispatch().clGetCommandQueueInfo(
        queue,
        CL_QUEUE_REFERENCE_COUNT,
        sizeof(refCount),
        &refCount,
        NULL);
    return refCount;
}

inline cl_uint CLIntercept::getRefCount( cl_context context ) const
{
    cl_uint refCount = 0;
    dispatch().clGetContextInfo(
        context,
        CL_CONTEXT_REFERENCE_COUNT,
        sizeof(refCount),
        &refCount,
        NULL);
    return refCount;
}

inline cl_uint CLIntercept::getRefCount( cl_device_id device ) const
{
    cl_uint refCount = 0;
    dispatch().clGetDeviceInfo(
        device,
        CL_DEVICE_REFERENCE_COUNT,
        sizeof(refCount),
        &refCount,
        NULL);
    return refCount;
}

inline cl_uint CLIntercept::getRefCount( cl_event event ) const
{
    cl_uint refCount = 0;
    dispatch().clGetEventInfo(
        event,
        CL_EVENT_REFERENCE_COUNT,
        sizeof(refCount),
        &refCount,
        NULL);
    return refCount;
}

inline cl_uint CLIntercept::getRefCount( cl_program program ) const
{
    cl_uint refCount = 0;
    dispatch().clGetProgramInfo(
        program,
        CL_PROGRAM_REFERENCE_COUNT,
        sizeof(refCount),
        &refCount,
        NULL);
    return refCount;
}

inline cl_uint CLIntercept::getRefCount( cl_kernel kernel ) const
{
    cl_uint refCount = 0;
    dispatch().clGetKernelInfo(
        kernel,
        CL_KERNEL_REFERENCE_COUNT,
        sizeof(refCount),
        &refCount,
        NULL);
    return refCount;
}

inline cl_uint CLIntercept::getRefCount( cl_mem memobj ) const
{
    cl_uint refCount = 0;
    dispatch().clGetMemObjectInfo(
        memobj,
        CL_MEM_REFERENCE_COUNT,
        sizeof(refCount),
        &refCount,
        NULL);
    return refCount;
}

inline cl_uint CLIntercept::getRefCount( cl_sampler sampler ) const
{
    cl_uint refCount = 0;
    dispatch().clGetSamplerInfo(
        sampler,
        CL_SAMPLER_REFERENCE_COUNT,
        sizeof(refCount),
        &refCount,
        NULL);
    return refCount;
}

inline cl_uint CLIntercept::getRefCount( cl_semaphore_khr semaphore )
{
    cl_platform_id  platform = this->getPlatform(semaphore);
    if( dispatchX(platform).clGetSemaphoreInfoKHR == NULL )
    {
        getExtensionFunctionAddress(
            platform,
            "clGetSemaphoreInfoKHR" );
    }

    cl_uint refCount = 0;
    const auto& dispatchX = this->dispatchX(platform);
    if( dispatchX.clGetSemaphoreInfoKHR )
    {
        dispatchX.clGetSemaphoreInfoKHR(
            semaphore,
            CL_SEMAPHORE_REFERENCE_COUNT_KHR,
            sizeof(refCount),
            &refCount,
            NULL );
    }
    return refCount;
}

inline cl_uint CLIntercept::getRefCount( cl_command_buffer_khr cmdbuf )
{
    cl_platform_id  platform = this->getPlatform(cmdbuf);
    if( dispatchX(platform).clGetCommandBufferInfoKHR == NULL )
    {
        getExtensionFunctionAddress(
            platform,
            "clGetCommandBufferInfoKHR" );
    }

    cl_uint refCount = 0;
    const auto& dispatchX = this->dispatchX(platform);
    if( dispatchX.clGetCommandBufferInfoKHR )
    {
        dispatchX.clGetCommandBufferInfoKHR(
            cmdbuf,
            CL_COMMAND_BUFFER_REFERENCE_COUNT_KHR,
            sizeof(refCount),
            &refCount,
            NULL );
    }
    return refCount;
}

///////////////////////////////////////////////////////////////////////////////
//
inline const OS::Services& CLIntercept::OS() const
{
    return m_OS;
}

///////////////////////////////////////////////////////////////////////////////
//
inline const CEnumNameMap& CLIntercept::enumName() const
{
    return m_EnumNameMap;
}

///////////////////////////////////////////////////////////////////////////////
//
inline const CLIntercept::Config& CLIntercept::config() const
{
    return m_Config;
}

///////////////////////////////////////////////////////////////////////////////
//
inline uint64_t CLIntercept::getEnqueueCounter() const
{
    return m_EnqueueCounter.load(std::memory_order_relaxed);
}

inline uint64_t CLIntercept::incrementEnqueueCounter()
{
    uint64_t reportInterval = m_Config.ReportInterval;
    if( reportInterval != 0 )
    {
        uint64_t enqueueCounter = m_EnqueueCounter.load();
        if( enqueueCounter != 0 && enqueueCounter % reportInterval == 0 )
        {
            report();
        }
    }
    return m_EnqueueCounter.fetch_add(1, std::memory_order_relaxed);
}

#define GET_ENQUEUE_COUNTER()                                               \
    uint64_t enqueueCounter = pIntercept->getEnqueueCounter();

#define INCREMENT_ENQUEUE_COUNTER()                                         \
    uint64_t enqueueCounter = pIntercept->incrementEnqueueCounter();

///////////////////////////////////////////////////////////////////////////////
//
inline CObjectTracker& CLIntercept::objectTracker()
{
    return m_ObjectTracker;
}

#define ADD_OBJECT_ALLOCATION( _obj )                                       \
    if( pIntercept->config().LeakChecking )                                 \
    {                                                                       \
        pIntercept->objectTracker().AddAllocation(_obj);                    \
    }

#define ADD_OBJECT_RETAIN( _obj )                                           \
    if( pIntercept->config().LeakChecking )                                 \
    {                                                                       \
        pIntercept->objectTracker().AddRetain(_obj);                        \
    }

#define ADD_OBJECT_RELEASE( _obj )                                          \
    if( pIntercept->config().LeakChecking )                                 \
    {                                                                       \
        pIntercept->objectTracker().AddRelease(_obj);                       \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define LOG_CLINFO()                                                        \
    if( pIntercept->config().CLInfoLogging )                                \
    {                                                                       \
        pIntercept->logCLInfo();                                            \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define BUILD_LOGGING_INIT()                                                \
    CLIntercept::clock::time_point  buildTimeStart;                         \
    if( pIntercept->config().BuildLogging )                                 \
    {                                                                       \
        buildTimeStart = CLIntercept::clock::now();                         \
    }

#define BUILD_LOGGING( program, num_devices, device_list )                  \
    if( pIntercept->config().BuildLogging )                                 \
    {                                                                       \
        pIntercept->logBuild(                                               \
            buildTimeStart,                                                 \
            program,                                                        \
            num_devices,                                                    \
            device_list );                                                  \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define CALL_LOGGING_ENTER(...)                                             \
    if( pIntercept->config().CallLogging )                                  \
    {                                                                       \
        pIntercept->callLoggingEnter(                                       \
            __FUNCTION__, enqueueCounter, NULL, ##__VA_ARGS__ );            \
    }                                                                       \
    ITT_CALL_LOGGING_ENTER( NULL );

#define CALL_LOGGING_ENTER_KERNEL(kernel, ...)                              \
    if( pIntercept->config().CallLogging )                                  \
    {                                                                       \
        pIntercept->callLoggingEnter(                                       \
            __FUNCTION__, enqueueCounter, kernel, ##__VA_ARGS__ );          \
    }                                                                       \
    ITT_CALL_LOGGING_ENTER( kernel );

#define CALL_LOGGING_INFO(...)                                              \
    if( pIntercept->config().CallLogging )                                  \
    {                                                                       \
        pIntercept->callLoggingInfo( __VA_ARGS__ );                         \
    }                                                                       \

#define CALL_LOGGING_EXIT(errorCode, ...)                                   \
    if( pIntercept->config().CallLogging )                                  \
    {                                                                       \
        pIntercept->callLoggingExit(                                        \
            __FUNCTION__,                                                   \
            errorCode,                                                      \
            NULL,                                                           \
            ##__VA_ARGS__ );                                                \
    }                                                                       \
    if( pIntercept->config().ChromeCallLogging )                            \
    {                                                                       \
        pIntercept->chromeCallLoggingExit(                                  \
            __FUNCTION__,                                                   \
            "",                                                             \
            false,                                                          \
            0,                                                              \
            cpuStart,                                                       \
            cpuEnd );                                                       \
    }                                                                       \
    ITT_CALL_LOGGING_EXIT();

#define CALL_LOGGING_EXIT_EVENT(errorCode, event, ...)                      \
    if( pIntercept->config().CallLogging )                                  \
    {                                                                       \
        pIntercept->callLoggingExit(                                        \
            __FUNCTION__,                                                   \
            errorCode,                                                      \
            event,                                                          \
            ##__VA_ARGS__ );                                                \
    }                                                                       \
    if( pIntercept->config().ChromeCallLogging )                            \
    {                                                                       \
        pIntercept->chromeCallLoggingExit(                                  \
            __FUNCTION__,                                                   \
            "",                                                             \
            true,                                                           \
            enqueueCounter,                                                 \
            cpuStart,                                                       \
            cpuEnd );                                                       \
    }                                                                       \
    ITT_CALL_LOGGING_EXIT();

#define CALL_LOGGING_EXIT_EVENT_WITH_TAG(errorCode, _event, ...)            \
    if( pIntercept->config().CallLogging )                                  \
    {                                                                       \
        pIntercept->callLoggingExit(                                        \
            __FUNCTION__,                                                   \
            errorCode,                                                      \
            _event,                                                         \
            ##__VA_ARGS__ );                                                \
    }                                                                       \
    if( pIntercept->config().ChromeCallLogging )                            \
    {                                                                       \
        pIntercept->chromeCallLoggingExit(                                  \
            __FUNCTION__,                                                   \
            hostTag,                                                        \
            true,                                                           \
            enqueueCounter,                                                 \
            cpuStart,                                                       \
            cpuEnd );                                                       \
    }                                                                       \
    ITT_CALL_LOGGING_EXIT();

///////////////////////////////////////////////////////////////////////////////
//
#define CHECK_ERROR_INIT( pErrorCode )                                      \
    cl_int  localErrorCode = CL_SUCCESS;                                    \
    if( ( pIntercept->config().CallLogging ||                               \
          pIntercept->config().ErrorLogging ||                              \
          pIntercept->config().ErrorAssert ||                               \
          pIntercept->config().NoErrors ) &&                                \
        ( pErrorCode == NULL ) )                                            \
    {                                                                       \
        pErrorCode = &localErrorCode;                                       \
    }

#define CHECK_ERROR( errorCode )                                            \
    if( ( pIntercept->config().ErrorLogging ||                              \
          pIntercept->config().ErrorAssert ||                               \
          pIntercept->config().NoErrors ) &&                                \
        ( errorCode != CL_SUCCESS ) )                                       \
    {                                                                       \
        if( pIntercept->config().ErrorLogging )                             \
        {                                                                   \
            pIntercept->logError( __FUNCTION__, errorCode );                \
        }                                                                   \
        if( pIntercept->config().ErrorAssert )                              \
        {                                                                   \
            CLI_DEBUG_BREAK();                                              \
        }                                                                   \
        if( pIntercept->config().NoErrors )                                 \
        {                                                                   \
            errorCode = CL_SUCCESS;                                         \
        }                                                                   \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define CREATE_CONTEXT_OVERRIDE_INIT( _props, _func, _data, _newprops )     \
    CLIntercept::SContextCallbackInfo*  pContextCallbackInfo = NULL;        \
    if( pIntercept->config().NullContextCallback )                          \
    {                                                                       \
        _func = NULL;                                                       \
    }                                                                       \
    if( pIntercept->config().ContextCallbackLogging ||                      \
        pIntercept->config().ContextHintLevel )                             \
    {                                                                       \
        pIntercept->contextCallbackOverrideInit(                            \
            _props,                                                         \
            _func,                                                          \
            _data,                                                          \
            pContextCallbackInfo,                                           \
            _newprops );                                                    \
    }

#define CREATE_CONTEXT_OVERRIDE_CLEANUP( _context, _newprops )              \
    if( pIntercept->config().ContextCallbackLogging ||                      \
        pIntercept->config().ContextHintLevel )                             \
    {                                                                       \
        pIntercept->contextCallbackOverrideCleanup(                         \
            _context,                                                       \
            pContextCallbackInfo,                                           \
            _newprops );                                                    \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define EVENT_CALLBACK_OVERRIDE_INIT( _func, _data )                        \
    CLIntercept::SEventCallbackInfo*  pEventCallbackInfo = NULL;            \
    if( pIntercept->config().EventCallbackLogging )                         \
    {                                                                       \
        pEventCallbackInfo = new CLIntercept::SEventCallbackInfo;           \
        if( pEventCallbackInfo )                                            \
        {                                                                   \
            pEventCallbackInfo->pIntercept = pIntercept;                    \
            pEventCallbackInfo->pApplicationCallback = _func;               \
            pEventCallbackInfo->pUserData = _data;                          \
                                                                            \
            _func = CLIntercept::eventCallbackCaller;                       \
            _data = pEventCallbackInfo;                                     \
        }                                                                   \
    }

#define EVENT_CALLBACK_OVERRIDE_CLEANUP( _errCode )                         \
    if( pIntercept->config().EventCallbackLogging &&                        \
        _errCode != CL_SUCCESS )                                            \
    {                                                                       \
        delete pEventCallbackInfo;                                          \
        pEventCallbackInfo = NULL;                                          \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define FINISH_OR_FLUSH_AFTER_ENQUEUE( _command_queue )                     \
    if( pIntercept->config().FinishAfterEnqueue )                           \
    {                                                                       \
        {                                                                   \
            TOOL_OVERHEAD_TIMING_START();                                   \
            pIntercept->logFlushOrFinishAfterEnqueueStart(                  \
                "clFinish",                                                 \
                __FUNCTION__ );                                             \
            cl_int  e = pIntercept->dispatch().clFinish( _command_queue );  \
            pIntercept->logFlushOrFinishAfterEnqueueEnd(                    \
                "clFinish",                                                 \
                __FUNCTION__,                                               \
                e );                                                        \
            TOOL_OVERHEAD_TIMING_END( "(finish after enqueue)" );           \
        }                                                                   \
        {                                                                   \
            TOOL_OVERHEAD_TIMING_START();                                   \
            pIntercept->checkTimingEvents();                                \
            TOOL_OVERHEAD_TIMING_END( "(device timing overhead)" );         \
        }                                                                   \
    }                                                                       \
    else if( pIntercept->config().FlushAfterEnqueue )                       \
    {                                                                       \
        /*pIntercept->logFlushOrFinishAfterEnqueueStart(*/                  \
        /*    "clFlush",                                */                  \
        /*    __FUNCTION__ );                           */                  \
        /* cl_int  e = */ pIntercept->dispatch().clFlush( _command_queue ); \
        /*pIntercept->logFlushOrFinishAfterEnqueueEnd(*/                    \
        /*    "clFlush",                              */                    \
        /*    __FUNCTION__,                           */                    \
        /*    e );                                    */                    \
    }

#define FLUSH_AFTER_ENQUEUE_BARRIER( _command_queue )                       \
    if( pIntercept->config().FlushAfterEnqueueBarrier )                     \
    {                                                                       \
        /*pIntercept->logFlushOrFinishAfterEnqueueStart(*/                  \
        /*    "clFlush (for barrier)",                  */                  \
        /*    __FUNCTION__ );                           */                  \
        /* cl_int  e = */ pIntercept->dispatch().clFlush( _command_queue ); \
        /*pIntercept->logFlushOrFinishAfterEnqueueEnd(*/                    \
        /*    "clFlush (for barrier)",                */                    \
        /*    __FUNCTION__,                           */                    \
        /*    e );                                    */                    \
    }

///////////////////////////////////////////////////////////////////////////////
//
inline bool CLIntercept::dumpBufferForKernel( const cl_kernel kernel )
{
    // Note: This currently checks the long kernel name.
    // Should it be the short kernel name instead?
    return m_Config.DumpBuffersForKernel.empty() ||
        m_KernelInfoMap[ kernel ].KernelName == m_Config.DumpBuffersForKernel;
}

inline bool CLIntercept::dumpImagesForKernel( const cl_kernel kernel )
{
    // Note: This currently checks the long kernel name.
    // Should it be the short kernel name instead?
    return m_Config.DumpImagesForKernel.empty() ||
        m_KernelInfoMap[ kernel ].KernelName == m_Config.DumpImagesForKernel;
}

inline bool CLIntercept::checkDumpBufferEnqueueLimits(
    uint64_t enqueueCounter ) const
{
    return ( enqueueCounter >= m_Config.DumpBuffersMinEnqueue ) &&
           ( enqueueCounter <= m_Config.DumpBuffersMaxEnqueue );
}

inline bool CLIntercept::checkDumpImageEnqueueLimits(
    uint64_t enqueueCounter ) const
{
    return ( enqueueCounter >= m_Config.DumpImagesMinEnqueue ) &&
           ( enqueueCounter <= m_Config.DumpImagesMaxEnqueue );
}

inline bool CLIntercept::checkDumpByCounter( uint64_t enqueueCounter ) const
{
    return enqueueCounter == static_cast<uint64_t>(config().DumpReplayKernelEnqueue);
}

inline bool CLIntercept::checkDumpByName( cl_kernel kernel )
{
    return !config().DumpReplayKernelName.empty() &&
        getShortKernelName(kernel) == config().DumpReplayKernelName;
}

#define ADD_QUEUE( _context, _queue )                                       \
    if( _queue &&                                                           \
        ( pIntercept->config().ChromePerformanceTiming ||                   \
          pIntercept->config().Emulate_cl_intel_unified_shared_memory ) )   \
    {                                                                       \
        pIntercept->addQueue(                                               \
            _context,                                                       \
            _queue );                                                       \
        if( pIntercept->config().ChromePerformanceTiming )                  \
        {                                                                   \
            pIntercept->chromeRegisterCommandQueue( _queue );               \
        }                                                                   \
    }

#define REMOVE_QUEUE( _queue )                                              \
    if( _queue &&                                                           \
        ( pIntercept->config().ChromePerformanceTiming ||                   \
          pIntercept->config().Emulate_cl_intel_unified_shared_memory ) )   \
    {                                                                       \
        pIntercept->checkRemoveQueue( _queue );                             \
    }

#define ADD_EVENT( _event )                                                 \
    if( ( _event ) &&                                                       \
        ( pIntercept->config().ChromeCallLogging ||                         \
          pIntercept->config().ChromePerformanceTiming ) )                  \
    {                                                                       \
        pIntercept->addEvent( _event, enqueueCounter );                     \
    }

#define REMOVE_EVENT( _event )                                              \
    if( ( _event ) &&                                                       \
        ( pIntercept->config().ChromeCallLogging ||                         \
          pIntercept->config().ChromePerformanceTiming ) )                  \
    {                                                                       \
        pIntercept->checkRemoveEvent( _event );                             \
    }

#define ADD_BUFFER( _buffer )                                               \
    if( _buffer &&                                                          \
        ( pIntercept->config().DumpBuffersAfterCreate ||                    \
          pIntercept->config().DumpBuffersAfterMap ||                       \
          pIntercept->config().DumpBuffersBeforeUnmap ||                    \
          pIntercept->config().DumpBuffersBeforeEnqueue ||                  \
          pIntercept->config().DumpBuffersAfterEnqueue  ||                  \
          pIntercept->config().DumpReplayKernelEnqueue != -1 ||             \
          !pIntercept->config().DumpReplayKernelName.empty() ) )            \
    {                                                                       \
        pIntercept->addBuffer( _buffer );                                   \
    }

#define ADD_IMAGE( _image )                                                 \
    if( _image &&                                                           \
        ( pIntercept->config().DumpImagesBeforeEnqueue ||                   \
          pIntercept->config().DumpImagesAfterEnqueue ||                    \
          pIntercept->config().DumpReplayKernelEnqueue != -1 ||             \
          !pIntercept->config().DumpReplayKernelName.empty() ) )            \
    {                                                                       \
        pIntercept->addImage( _image );                                     \
    }

#define REMOVE_MEMOBJ( _memobj )                                            \
    if( _memobj &&                                                          \
        ( pIntercept->config().DumpBuffersAfterCreate ||                    \
          pIntercept->config().DumpBuffersAfterMap ||                       \
          pIntercept->config().DumpBuffersBeforeUnmap ||                    \
          pIntercept->config().DumpBuffersBeforeEnqueue ||                  \
          pIntercept->config().DumpBuffersAfterEnqueue ||                   \
          pIntercept->config().DumpImagesBeforeEnqueue ||                   \
          pIntercept->config().DumpImagesAfterEnqueue ||                    \
          pIntercept->config().DumpReplayKernelEnqueue != -1 ||             \
          !pIntercept->config().DumpReplayKernelName.empty() ) )            \
    {                                                                       \
        pIntercept->checkRemoveMemObj( _memobj );                           \
    }

#define ADD_SAMPLER( sampler, str )                                         \
    if( sampler &&                                                          \
        ( pIntercept->config().CallLogging ||                               \
          pIntercept->config().DumpReplayKernelEnqueue != -1 ||             \
          !pIntercept->config().DumpReplayKernelName.empty() ) )            \
    {                                                                       \
        pIntercept->addSamplerString( sampler, str );                       \
    }

#define REMOVE_SAMPLER( sampler )                                           \
    if( sampler &&                                                          \
        ( pIntercept->config().CallLogging ||                               \
          pIntercept->config().DumpReplayKernelEnqueue != -1 ||             \
          !pIntercept->config().DumpReplayKernelName.empty() ) )            \
    {                                                                       \
        pIntercept->checkRemoveSamplerString( sampler );                    \
    }

#define ADD_SVM_ALLOCATION( svmPtr, size )                                  \
    if( svmPtr &&                                                           \
        ( pIntercept->config().DumpBuffersBeforeEnqueue ||                  \
          pIntercept->config().DumpBuffersAfterEnqueue ||                   \
          pIntercept->config().DumpReplayKernelEnqueue != -1 ||             \
          !pIntercept->config().DumpReplayKernelName.empty() ) )            \
    {                                                                       \
        pIntercept->addSVMAllocation( svmPtr, size );                       \
    }

#define REMOVE_SVM_ALLOCATION( svmPtr )                                     \
    if( svmPtr &&                                                           \
        ( pIntercept->config().DumpBuffersBeforeEnqueue ||                  \
          pIntercept->config().DumpBuffersAfterEnqueue ||                   \
          pIntercept->config().DumpReplayKernelEnqueue != -1 ||             \
          !pIntercept->config().DumpReplayKernelName.empty() ) )            \
    {                                                                       \
        pIntercept->removeSVMAllocation( svmPtr );                          \
    }

#define ADD_USM_ALLOCATION( usmPtr, size )                                  \
    if( usmPtr &&                                                           \
        ( pIntercept->config().DumpBuffersBeforeEnqueue ||                  \
          pIntercept->config().DumpBuffersAfterEnqueue ||                   \
          pIntercept->config().DumpReplayKernelEnqueue != -1 ||             \
          !pIntercept->config().DumpReplayKernelName.empty() ) )            \
    {                                                                       \
        pIntercept->addUSMAllocation( usmPtr, size );                       \
    }

#define REMOVE_USM_ALLOCATION( usmPtr )                                     \
    if( usmPtr &&                                                           \
        ( pIntercept->config().DumpBuffersBeforeEnqueue ||                  \
          pIntercept->config().DumpBuffersAfterEnqueue ||                   \
          pIntercept->config().DumpReplayKernelEnqueue != -1 ||             \
          !pIntercept->config().DumpReplayKernelName.empty() ) )            \
    {                                                                       \
        pIntercept->removeUSMAllocation( usmPtr );                          \
    }

#define ADD_MUTABLE_COMMAND( pCmd, cmdbuf )                                 \
    if( pCmd && pCmd[0] )                                                   \
    {                                                                       \
        pIntercept->addMutableCommandInfo( pCmd[0], cmdbuf, 0 );            \
    }

#define ADD_MUTABLE_COMMAND_NDRANGE( pCmd, cmdbuf, workdim )                \
    if( pCmd && pCmd[0] )                                                   \
    {                                                                       \
        pIntercept->addMutableCommandInfo( pCmd[0], cmdbuf, workdim );      \
    }

#define SET_KERNEL_ARG( kernel, arg_index, arg_size, arg_value )            \
    if( pIntercept->config().DumpArgumentsOnSet &&                          \
        enqueueCounter >= pIntercept->config().DumpArgumentsOnSetMinEnqueue && \
        enqueueCounter <= pIntercept->config().DumpArgumentsOnSetMaxEnqueue ) \
    {                                                                       \
        pIntercept->dumpArgument(                                           \
            enqueueCounter, kernel, arg_index, arg_size, arg_value );       \
    }                                                                       \
    if( pIntercept->config().DumpBuffersBeforeEnqueue ||                    \
        pIntercept->config().DumpBuffersAfterEnqueue ||                     \
        pIntercept->config().DumpImagesBeforeEnqueue ||                     \
        pIntercept->config().DumpImagesAfterEnqueue ||                      \
        pIntercept->config().DumpReplayKernelEnqueue != -1 ||               \
        !pIntercept->config().DumpReplayKernelName.empty() )                \
    {                                                                       \
        if( (arg_value != NULL) && (arg_size == sizeof(cl_mem)) )           \
        {                                                                   \
            cl_mem* pMem = (cl_mem*)arg_value;                              \
            pIntercept->setKernelArg( kernel, arg_index, pMem[0] );         \
        }                                                                   \
        pIntercept->setKernelArg( kernel, arg_index, arg_value, arg_size ); \
    }

#define SET_KERNEL_ARG_SVM_POINTER( kernel, arg_index, arg_value )          \
    if( pIntercept->config().DumpBuffersBeforeEnqueue ||                    \
        pIntercept->config().DumpBuffersAfterEnqueue ||                     \
        pIntercept->config().DumpReplayKernelEnqueue != -1 ||               \
        !pIntercept->config().DumpReplayKernelName.empty() )                \
    {                                                                       \
        pIntercept->setKernelArgSVMPointer( kernel, arg_index, arg_value ); \
    }

#define SET_KERNEL_ARG_USM_POINTER( kernel, arg_index, arg_value )          \
    if( pIntercept->config().DumpBuffersBeforeEnqueue ||                    \
        pIntercept->config().DumpBuffersAfterEnqueue ||                     \
        pIntercept->config().DumpReplayKernelEnqueue != -1 ||               \
        !pIntercept->config().DumpReplayKernelName.empty() )                \
    {                                                                       \
        pIntercept->setKernelArgUSMPointer( kernel, arg_index, arg_value ); \
    }

#define INITIALIZE_BUFFER_CONTENTS_INIT( _flags, _size, _ptr )              \
    char*   zeroData = NULL;                                                \
    if( pIntercept->config().InitializeBuffers &&                           \
        _ptr == NULL &&                                                     \
        !( _flags & ( CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR ) ) )      \
    {                                                                       \
        zeroData = new char[ _size ];                                       \
        if( zeroData != NULL )                                              \
        {                                                                   \
            memset( zeroData, 0, _size );                                   \
            _ptr = zeroData;                                                \
            _flags |= (cl_mem_flags)CL_MEM_COPY_HOST_PTR;                   \
        }                                                                   \
    }

// Note: The cleanup setup currently does not reset the flags or host pointer.
// This mostly means that initialized buffers may be dumped after creation,
// whereas if the flags were reset then the dump buffer after create step
// would not be triggered.
#define INITIALIZE_BUFFER_CONTENTS_CLEANUP( _flags, _ptr )                  \
    if( zeroData != NULL )                                                  \
    {                                                                       \
        delete [] zeroData;                                                 \
        zeroData = NULL;                                                    \
    }

#define DUMP_BUFFER_AFTER_CREATE( memobj, flags, ptr, size )                \
    if( memobj &&                                                           \
        ( flags & ( CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR ) ) &&       \
        pIntercept->checkDumpBufferEnqueueLimits( enqueueCounter ) &&       \
        pIntercept->config().DumpBuffersAfterCreate )                       \
    {                                                                       \
        pIntercept->dumpBuffer(                                             \
            "Create", enqueueCounter, memobj, NULL, ptr, 0, size );         \
    }

#define DUMP_BUFFER_AFTER_MAP( command_queue, memobj, blocking_map, flags, ptr, offset, size ) \
    if( memobj &&                                                           \
        !( flags & CL_MAP_WRITE_INVALIDATE_REGION ) &&                      \
        pIntercept->checkDumpBufferEnqueueLimits( enqueueCounter ) &&       \
        pIntercept->config().DumpBuffersAfterMap )                          \
    {                                                                       \
        if( blocking_map == false )                                         \
        {                                                                   \
            pIntercept->dispatch().clFinish( command_queue );               \
        }                                                                   \
        pIntercept->dumpBuffer(                                             \
            "Map", enqueueCounter, memobj, NULL, ptr, offset, size );       \
    }

#define DUMP_BUFFER_BEFORE_UNMAP( memobj, command_queue)                    \
    if( memobj &&                                                           \
        command_queue &&                                                    \
        pIntercept->checkDumpBufferEnqueueLimits( enqueueCounter ) &&       \
        pIntercept->config().DumpBuffersBeforeUnmap )                       \
    {                                                                       \
        pIntercept->dumpBuffer(                                             \
            "Unmap", enqueueCounter, memobj, command_queue, NULL, 0, 0 );   \
    }

#define DUMP_BUFFERS_BEFORE_ENQUEUE( kernel, command_queue)                 \
    if( pIntercept->config().DumpBuffersBeforeEnqueue &&                    \
        pIntercept->checkDumpBufferEnqueueLimits( enqueueCounter ) &&       \
        pIntercept->dumpBufferForKernel( kernel ) )                         \
    {                                                                       \
        pIntercept->dumpBuffersForKernel(                                   \
            "Pre", enqueueCounter, kernel, command_queue, false, false );   \
    }

#define DUMP_BUFFERS_AFTER_ENQUEUE( kernel, command_queue )                 \
    if( ( pIntercept->config().DumpBuffersAfterEnqueue &&                   \
          pIntercept->checkDumpBufferEnqueueLimits( enqueueCounter ) &&     \
          pIntercept->dumpBufferForKernel( kernel ) ) ||                    \
        ( hasDumpedBufferByName && !hasDumpedValidationBufferByName ) )     \
    {                                                                       \
        hasDumpedValidationBufferByName = true;                             \
        pIntercept->dumpBuffersForKernel(                                   \
            "Post", enqueueCounter, kernel, command_queue, false,           \
             !pIntercept->config().DumpReplayKernelName.empty() );          \
    }

#define DUMP_REPLAYABLE_KERNEL( kernel, command_queue, work_dim, gws_offset, gws, lws ) \
    if ( pIntercept->checkDumpByCounter( enqueueCounter ) ||                \
        ( pIntercept->checkDumpByName( kernel ) &&                          \
          ( !hasDumpedBufferByName || !hasDumpedImageByName ) ) )           \
    {                                                                       \
        hasDumpedBufferByName = true;                                       \
        hasDumpedImageByName = true;                                        \
        pIntercept->dumpBuffersForKernel(                                   \
            "", enqueueCounter, kernel, command_queue, true,                \
            !pIntercept->config().DumpReplayKernelName.empty() );           \
        pIntercept->dumpImagesForKernel(                                    \
            "", enqueueCounter, kernel, command_queue, true,                \
            !pIntercept->config().DumpReplayKernelName.empty() );           \
        pIntercept->dumpKernelSourceOrDeviceBinary(                         \
            kernel, enqueueCounter,                                         \
            !pIntercept->config().DumpReplayKernelName.empty() );           \
        pIntercept->dumpKernelInfo(                                         \
            kernel, enqueueCounter, work_dim, gws_offset, gws, lws,         \
            !pIntercept->config().DumpReplayKernelName.empty() );           \
        pIntercept->dumpArgumentsForKernel(                                 \
            kernel, enqueueCounter,                                         \
            !pIntercept->config().DumpReplayKernelName.empty() );           \
    }

#define DUMP_IMAGES_BEFORE_ENQUEUE( kernel, command_queue )                 \
    if( pIntercept->config().DumpImagesBeforeEnqueue &&                     \
        pIntercept->checkDumpImageEnqueueLimits( enqueueCounter ) &&        \
        pIntercept->dumpImagesForKernel( kernel ) )                         \
    {                                                                       \
        pIntercept->dumpImagesForKernel(                                    \
            "Pre", enqueueCounter, kernel, command_queue, false, false );   \
    }

#define DUMP_IMAGES_AFTER_ENQUEUE( kernel, command_queue )                  \
    if( ( pIntercept->config().DumpImagesAfterEnqueue &&                    \
          pIntercept->checkDumpImageEnqueueLimits( enqueueCounter )  &&     \
          pIntercept->dumpImagesForKernel( kernel ) ) ||                    \
          ( hasDumpedImageByName && !hasDumpedValidationImageByName ) )     \
    {                                                                       \
        hasDumpedValidationImageByName = true;                              \
        pIntercept->dumpImagesForKernel(                                    \
            "Post", enqueueCounter, kernel, command_queue, false,           \
            !pIntercept->config().DumpReplayKernelName.empty() );           \
    }

#define ADD_MAP_POINTER( _ptr, _flags, _sz )                                \
    if( _ptr &&                                                             \
        ( pIntercept->config().ChromeCallLogging ||                         \
          pIntercept->config().HostPerformanceTiming ||                     \
          pIntercept->config().DevicePerformanceTiming ||                   \
          pIntercept->config().ITTPerformanceTiming ||                      \
          pIntercept->config().ChromePerformanceTiming ||                   \
          pIntercept->config().DevicePerfCounterEventBasedSampling ) )      \
    {                                                                       \
        pIntercept->addMapPointer( _ptr, _flags, _sz );                     \
    }

#define REMOVE_MAP_PTR( _ptr )                                              \
    if( _ptr &&                                                             \
        ( pIntercept->config().ChromeCallLogging ||                         \
          pIntercept->config().HostPerformanceTiming ||                     \
          pIntercept->config().DevicePerformanceTiming ||                   \
          pIntercept->config().ITTPerformanceTiming ||                      \
          pIntercept->config().ChromePerformanceTiming ||                   \
          pIntercept->config().DevicePerfCounterEventBasedSampling ) )      \
    {                                                                       \
        pIntercept->removeMapPointer( _ptr );                               \
    }

///////////////////////////////////////////////////////////////////////////////
//
inline bool CLIntercept::checkAubCaptureEnqueueLimits(
    uint64_t enqueueCounter ) const
{
    return ( enqueueCounter >= m_Config.AubCaptureMinEnqueue ) &&
           ( enqueueCounter <= m_Config.AubCaptureMaxEnqueue );
}

// Note: We do not individually aub capture non-kernel enqueues at the moment.
#define CHECK_AUBCAPTURE_START( command_queue )                             \
    if( pIntercept->config().AubCapture &&                                  \
        pIntercept->checkAubCaptureEnqueueLimits( enqueueCounter ) &&       \
        !pIntercept->config().AubCaptureIndividualEnqueues )                \
    {                                                                       \
        pIntercept->startAubCapture(                                        \
            __FUNCTION__, enqueueCounter,                                   \
            NULL, 0, NULL, NULL, command_queue );                           \
    }

#define CHECK_AUBCAPTURE_START_KERNEL( kernel, wd, gws, lws, command_queue )\
    if( pIntercept->config().AubCapture &&                                  \
        pIntercept->checkAubCaptureEnqueueLimits( enqueueCounter ) &&       \
        pIntercept->checkAubCaptureKernelSignature( kernel, wd, gws, lws ) )\
    {                                                                       \
        pIntercept->startAubCapture(                                        \
            __FUNCTION__, enqueueCounter,                                   \
            kernel, wd, gws, lws, command_queue );                          \
    }

#define CHECK_AUBCAPTURE_STOP( command_queue )                              \
    if( pIntercept->config().AubCapture &&                                  \
        ( pIntercept->config().AubCaptureIndividualEnqueues ||              \
          !pIntercept->checkAubCaptureEnqueueLimits( enqueueCounter ) ) )   \
    {                                                                       \
        pIntercept->stopAubCapture( command_queue );                        \
    }

///////////////////////////////////////////////////////////////////////////////
//

// Shared:

#define SAVE_PROGRAM_HASH( program, hash )                                  \
    if( pIntercept->config().BuildLogging ||                                \
        pIntercept->config().KernelNameHashTracking ||                      \
        pIntercept->config().DumpProgramSource ||                           \
        pIntercept->config().DumpInputProgramBinaries ||                    \
        pIntercept->config().DumpProgramBinaries ||                         \
        pIntercept->config().DumpProgramSPIRV ||                            \
        pIntercept->config().DumpProgramBuildLogs ||                        \
        pIntercept->config().DumpKernelISABinaries ||                       \
        pIntercept->config().InjectProgramSource ||                         \
        pIntercept->config().AutoCreateSPIRV ||                             \
        pIntercept->config().AubCaptureUniqueKernels )                      \
    {                                                                       \
        pIntercept->saveProgramHash( program, hash );                       \
    }

#define SAVE_PROGRAM_OPTIONS_HASH( program, options )                       \
    if( pIntercept->config().BuildLogging ||                                \
        pIntercept->config().KernelNameHashTracking ||                      \
        pIntercept->config().DumpProgramSource ||                           \
        pIntercept->config().DumpInputProgramBinaries ||                    \
        pIntercept->config().DumpProgramBinaries ||                         \
        pIntercept->config().DumpProgramSPIRV ||                            \
        pIntercept->config().DumpProgramBuildLogs ||                        \
        pIntercept->config().DumpKernelISABinaries ||                       \
        pIntercept->config().InjectProgramSource ||                         \
        pIntercept->config().AutoCreateSPIRV ||                             \
        pIntercept->config().AubCaptureUniqueKernels )                      \
    {                                                                       \
        pIntercept->saveProgramOptionsHash( program, options );             \
    }

#define DUMP_PROGRAM_OPTIONS( program, options, isCompile, isLink )         \
    if( pIntercept->config().DumpProgramSource ||                           \
        pIntercept->config().DumpInputProgramBinaries ||                    \
        pIntercept->config().DumpProgramBinaries ||                         \
        pIntercept->config().DumpProgramSPIRV )                             \
    {                                                                       \
        pIntercept->dumpProgramOptions(                                     \
            program,                                                        \
            modified,                                                       \
            isCompile,                                                      \
            isLink,                                                         \
            options );                                                      \
    }                                                                       \
    else if( ( modified == false ) &&                                       \
             ( pIntercept->config().SimpleDumpProgramSource ||              \
               pIntercept->config().DumpProgramSourceScript ) )             \
    {                                                                       \
        pIntercept->dumpProgramOptionsScript( program, options );           \
    }

#define INCREMENT_PROGRAM_COMPILE_COUNT( _program )                         \
    if( _program &&                                                         \
        ( pIntercept->config().BuildLogging ||                              \
          pIntercept->config().KernelNameHashTracking ||                    \
          pIntercept->config().InjectProgramSource ||                       \
          pIntercept->config().DumpProgramSourceScript ||                   \
          pIntercept->config().DumpProgramSource ||                         \
          pIntercept->config().DumpProgramBinaries ||                       \
          pIntercept->config().DumpProgramSPIRV ||                          \
          pIntercept->config().DumpProgramBuildLogs ||                      \
          pIntercept->config().DumpKernelISABinaries ||                     \
          pIntercept->config().AutoCreateSPIRV ||                           \
          pIntercept->config().AubCaptureUniqueKernels ) )                  \
    {                                                                       \
        pIntercept->incrementProgramCompileCount( _program );               \
    }

#define PROGRAM_OPTIONS_CLEANUP( newOptions )                               \
    delete [] newOptions;                                                   \
    newOptions = NULL;

// Called from clCreateProgramWithSource:

#define CREATE_COMBINED_PROGRAM_STRING( count, strings, lengths, singleString, hash ) \
    if( pIntercept->config().BuildLogging ||                                \
        pIntercept->config().KernelNameHashTracking ||                      \
        pIntercept->config().SimpleDumpProgramSource ||                     \
        pIntercept->config().DumpProgramSourceScript ||                     \
        pIntercept->config().DumpProgramSource ||                           \
        pIntercept->config().DumpProgramBinaries ||                         \
        pIntercept->config().DumpProgramSPIRV ||                            \
        pIntercept->config().DumpProgramBuildLogs ||                        \
        pIntercept->config().DumpKernelISABinaries ||                       \
        pIntercept->config().InjectProgramSource ||                         \
        pIntercept->config().InjectProgramBinaries ||                       \
        pIntercept->config().PrependProgramSource ||                        \
        pIntercept->config().AutoCreateSPIRV ||                             \
        pIntercept->config().AubCaptureUniqueKernels ||                     \
        pIntercept->config().DumpReplayKernelEnqueue != -1 ||               \
        !pIntercept->config().DumpReplayKernelName.empty() )                \
    {                                                                       \
        pIntercept->combineProgramStrings(                                  \
            count,                                                          \
            strings,                                                        \
            lengths,                                                        \
            singleString );                                                 \
        hash = pIntercept->computeHash(                                     \
            singleString,                                                   \
            strlen( singleString ) );                                       \
    }

#define INJECT_PROGRAM_SOURCE( count, strings, lengths, singleString, hash ) \
    bool    injected = false;                                               \
    if( pIntercept->config().InjectProgramSource )                          \
    {                                                                       \
        injected = pIntercept->injectProgramSource(                         \
            hash,                                                           \
            count,                                                          \
            strings,                                                        \
            lengths,                                                        \
            singleString );                                                 \
    }

#define PREPEND_PROGRAM_SOURCE( count, strings, lengths, singleString, hash ) \
    if( pIntercept->config().PrependProgramSource )                         \
    {                                                                       \
        injected |= pIntercept->prependProgramSource(                       \
            hash,                                                           \
            count,                                                          \
            strings,                                                        \
            lengths,                                                        \
            singleString );                                                 \
    }

#define DUMP_PROGRAM_SOURCE( program, singleString, hash )                  \
    if( pIntercept->config().DumpProgramSource ||                           \
        pIntercept->config().AutoCreateSPIRV )                              \
    {                                                                       \
        pIntercept->dumpProgramSource(                                      \
            program,                                                        \
            hash,                                                           \
            injected,                                                       \
            singleString );                                                 \
    }                                                                       \
    else if( ( injected == false ) &&                                       \
             ( pIntercept->config().SimpleDumpProgramSource ||              \
               pIntercept->config().DumpProgramSourceScript ) )             \
    {                                                                       \
        pIntercept->dumpProgramSourceScript( program, singleString );       \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        pIntercept->saveProgramNumber( program );                           \
    }

#define DELETE_COMBINED_PROGRAM_STRING( singleString )                      \
    delete [] singleString;                                                 \
    singleString = NULL;

// Called from clCreateProgramWithBinary:

// Note: This does not currently combine program binaries before computing
// the hash.  This will work fine for single-device binaries, but may be
// incomplete or incorrect for multi-device binaries.
// Note: This checks for more than just dumping input program binaries and
// program binaries so we have a hash when we dump program options, also.
#define COMPUTE_BINARY_HASH( _num, _lengths, _binaries, _hash )             \
    if( _lengths && _binaries &&                                            \
        ( pIntercept->config().DumpProgramSource ||                         \
          pIntercept->config().DumpInputProgramBinaries ||                  \
          pIntercept->config().DumpProgramBinaries ||                       \
          pIntercept->config().DumpProgramSPIRV ) )                         \
    {                                                                       \
        _hash = pIntercept->computeHash(                                    \
            _binaries[0],                                                   \
            _lengths[0] );                                                  \
    }

#define DUMP_INPUT_PROGRAM_BINARIES( _program, _num, _devs, _lengths, _binaries, _hash )    \
    if( pIntercept->config().DumpInputProgramBinaries )                     \
    {                                                                       \
        pIntercept->dumpInputProgramBinaries(                               \
            _program,                                                       \
            _hash,                                                          \
            false, /* modified */                                           \
            _num,                                                           \
            _devs,                                                          \
            _lengths,                                                       \
            _binaries );                                                    \
    }

// Called from clCreateProgramWithIL:

#define COMPUTE_SPIRV_HASH( _length, _il, _hash )                           \
    if( _length && _il && pIntercept->config().DumpProgramSPIRV )           \
    {                                                                       \
        _hash = pIntercept->computeHash(                                    \
            _il,                                                            \
            _length );                                                      \
    }

#define INJECT_PROGRAM_SPIRV( _length, _il, _injectedSPIRV, _hash )         \
    bool    injected = false;                                               \
    if( pIntercept->config().InjectProgramSPIRV )                           \
    {                                                                       \
        injected = pIntercept->injectProgramSPIRV(                          \
            _hash,                                                          \
            _length,                                                        \
            _il,                                                            \
            _injectedSPIRV );                                               \
    }

#define DUMP_PROGRAM_SPIRV( program, length, il, hash )                     \
    if( pIntercept->config().DumpProgramSPIRV )                             \
    {                                                                       \
        pIntercept->dumpProgramSPIRV( program, hash, injected, length, il );\
    }                                                                       \
    else                                                                    \
    {                                                                       \
        pIntercept->saveProgramNumber( program );                           \
    }

#define DELETE_INJECTED_SPIRV( _injectedSPIRV )                             \
    delete [] _injectedSPIRV;                                               \
    _injectedSPIRV = NULL;

// Called from clLinkProgram:

#define PROGRAM_LINK_OPTIONS_OVERRIDE_INIT( numDevices, deviceList, options, newOptions ) \
    bool    modified = false;                                               \
    if( !pIntercept->config().AppendLinkOptions.empty() )                   \
    {                                                                       \
        modified |= pIntercept->appendBuildOptions(                         \
            pIntercept->config().AppendLinkOptions.c_str(),                 \
            options,                                                        \
            newOptions );                                                   \
    }                                                                       \
    if( pIntercept->config().RelaxAllocationLimits &&                       \
        pIntercept->checkRelaxAllocationLimitsSupport( numDevices, deviceList ) )\
    {                                                                       \
        modified |= pIntercept->appendBuildOptions(                         \
            "-cl-intel-greater-than-4GB-buffer-required",                   \
            options,                                                        \
            newOptions );                                                   \
    }

#define SAVE_PROGRAM_NUMBER( program )                                      \
    pIntercept->saveProgramNumber( program );

// Called from clCompileProgram and clBuildProgram:

#define PROGRAM_OPTIONS_OVERRIDE_INIT( _program, _options, _newOptions, _isCompile ) \
    bool    modified = false;                                               \
    if( pIntercept->config().InjectProgramSource )                          \
    {                                                                       \
        modified |= pIntercept->injectProgramOptions(                       \
            _program,                                                       \
            _isCompile,                                                     \
            false, /* isLink */                                             \
            _newOptions );                                                  \
    }                                                                       \
    if( !pIntercept->config().AppendBuildOptions.empty() )                  \
    {                                                                       \
        modified |= pIntercept->appendBuildOptions(                         \
            pIntercept->config().AppendBuildOptions.c_str(),                \
            _options,                                                       \
            _newOptions );                                                  \
    }                                                                       \
    if( pIntercept->config().RelaxAllocationLimits &&                       \
        pIntercept->checkRelaxAllocationLimitsSupport( program ) )          \
    {                                                                       \
        modified |= pIntercept->appendBuildOptions(                         \
            "-cl-intel-greater-than-4GB-buffer-required",                   \
            _options,                                                       \
            _newOptions );                                                  \
    }

#define DUMP_OUTPUT_PROGRAM_BINARIES( program )                             \
    if( pIntercept->config().DumpProgramBinaries )                          \
    {                                                                       \
        pIntercept->dumpProgramBinary( program );                           \
    }

#define DUMP_KERNEL_ISA_BINARIES( program )                                 \
    if( pIntercept->config().DumpKernelISABinaries )                        \
    {                                                                       \
        pIntercept->dumpKernelISABinaries( program );                       \
    }

#define AUTO_CREATE_SPIRV( _program, _options )                             \
    if( _program && pIntercept->config().AutoCreateSPIRV )                  \
    {                                                                       \
        pIntercept->autoCreateSPIRV( _program, _options );                  \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define INIT_PRECOMPILED_KERNEL_OVERRIDES( context )                        \
    if( ( context != NULL ) &&                                              \
        ( pIntercept->config().OverrideReadBuffer ||                        \
          pIntercept->config().OverrideWriteBuffer ||                       \
          pIntercept->config().OverrideCopyBuffer ||                        \
          pIntercept->config().OverrideReadImage ||                         \
          pIntercept->config().OverrideWriteImage ||                        \
          pIntercept->config().OverrideCopyImage ) )                        \
    {                                                                       \
        pIntercept->initPrecompiledKernelOverrides( context );              \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define INIT_BUILTIN_KERNEL_OVERRIDES( context )                            \
    if( ( context != NULL ) &&                                              \
        pIntercept->config().OverrideBuiltinKernels )                       \
    {                                                                       \
        pIntercept->initBuiltinKernelOverrides( context );                  \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define GET_TIMING_TAGS_BLOCKING( _blocking, _sz )                          \
    std::string hostTag, deviceTag;                                         \
    if( pIntercept->config().ChromeCallLogging ||                           \
        ( pIntercept->config().HostPerformanceTiming &&                     \
          pIntercept->checkHostPerformanceTimingEnqueueLimits( enqueueCounter ) ) ||\
        ( ( pIntercept->config().DevicePerformanceTiming ||                   \
            pIntercept->config().ITTPerformanceTiming ||                      \
            pIntercept->config().ChromePerformanceTiming ||                   \
            pIntercept->config().DevicePerfCounterEventBasedSampling ) &&     \
          pIntercept->checkDevicePerformanceTimingEnqueueLimits( enqueueCounter ) ) )\
    {                                                                       \
        pIntercept->getTimingTagBlocking(                                   \
            __FUNCTION__,                                                   \
            _blocking,                                                      \
            _sz,                                                            \
            hostTag,                                                        \
            deviceTag );                                                    \
    }

#define GET_TIMING_TAGS_MAP( _blocking_map, _map_flags, _sz )               \
    std::string hostTag, deviceTag;                                         \
    if( pIntercept->config().ChromeCallLogging ||                           \
        ( pIntercept->config().HostPerformanceTiming &&                     \
          pIntercept->checkHostPerformanceTimingEnqueueLimits( enqueueCounter ) ) ||\
        ( ( pIntercept->config().DevicePerformanceTiming ||                   \
            pIntercept->config().ITTPerformanceTiming ||                      \
            pIntercept->config().ChromePerformanceTiming ||                   \
            pIntercept->config().DevicePerfCounterEventBasedSampling ) &&     \
          pIntercept->checkDevicePerformanceTimingEnqueueLimits( enqueueCounter ) ) )\
    {                                                                       \
        pIntercept->getTimingTagsMap(                                       \
            __FUNCTION__,                                                   \
            _map_flags,                                                     \
            _blocking_map,                                                  \
            _sz,                                                            \
            hostTag,                                                        \
            deviceTag );                                                    \
    }

#define GET_TIMING_TAGS_UNMAP( _ptr )                                       \
    std::string hostTag, deviceTag;                                         \
    if( pIntercept->config().ChromeCallLogging ||                           \
        ( pIntercept->config().HostPerformanceTiming &&                     \
          pIntercept->checkHostPerformanceTimingEnqueueLimits( enqueueCounter ) ) ||\
        ( ( pIntercept->config().DevicePerformanceTiming ||                   \
            pIntercept->config().ITTPerformanceTiming ||                      \
            pIntercept->config().ChromePerformanceTiming ||                   \
            pIntercept->config().DevicePerfCounterEventBasedSampling ) &&     \
          pIntercept->checkDevicePerformanceTimingEnqueueLimits( enqueueCounter ) ) )\
    {                                                                       \
        pIntercept->getTimingTagsUnmap(                                     \
            __FUNCTION__,                                                   \
            _ptr,                                                           \
            hostTag,                                                        \
            deviceTag );                                                    \
    }

#define GET_TIMING_TAGS_MEMFILL( _queue, _dst_ptr, _sz )                    \
    std::string hostTag, deviceTag;                                         \
    if( pIntercept->config().ChromeCallLogging ||                           \
        ( pIntercept->config().HostPerformanceTiming &&                     \
          pIntercept->checkHostPerformanceTimingEnqueueLimits( enqueueCounter ) ) ||\
        ( ( pIntercept->config().DevicePerformanceTiming ||                   \
            pIntercept->config().ITTPerformanceTiming ||                      \
            pIntercept->config().ChromePerformanceTiming ||                   \
            pIntercept->config().DevicePerfCounterEventBasedSampling ) &&     \
          pIntercept->checkDevicePerformanceTimingEnqueueLimits( enqueueCounter ) ) )\
    {                                                                       \
        pIntercept->getTimingTagsMemfill(                                   \
            __FUNCTION__,                                                   \
            _queue,                                                         \
            _dst_ptr,                                                       \
            _sz,                                                            \
            hostTag,                                                        \
            deviceTag );                                                    \
    }

#define GET_TIMING_TAGS_MEMCPY( _queue, _blocking, _dst_ptr, _src_ptr, _sz )\
    std::string hostTag, deviceTag;                                         \
    if( pIntercept->config().ChromeCallLogging ||                           \
        ( pIntercept->config().HostPerformanceTiming &&                     \
          pIntercept->checkHostPerformanceTimingEnqueueLimits( enqueueCounter ) ) ||\
        ( ( pIntercept->config().DevicePerformanceTiming ||                   \
            pIntercept->config().ITTPerformanceTiming ||                      \
            pIntercept->config().ChromePerformanceTiming ||                   \
            pIntercept->config().DevicePerfCounterEventBasedSampling ) &&     \
          pIntercept->checkDevicePerformanceTimingEnqueueLimits( enqueueCounter ) ) )\
    {                                                                       \
        pIntercept->getTimingTagsMemcpy(                                    \
            __FUNCTION__,                                                   \
            _queue,                                                         \
            _blocking,                                                      \
            _dst_ptr,                                                       \
            _src_ptr,                                                       \
            _sz,                                                            \
            hostTag,                                                        \
            deviceTag );                                                    \
    }

#define GET_TIMING_TAGS_KERNEL( _queue, _kernel, _dim, _gwo, _gws, _lws )   \
    std::string hostTag, deviceTag;                                         \
    if( pIntercept->config().ChromeCallLogging ||                           \
        ( pIntercept->config().HostPerformanceTiming &&                     \
          pIntercept->checkHostPerformanceTimingEnqueueLimits( enqueueCounter ) ) ||\
        ( ( pIntercept->config().DevicePerformanceTiming ||                   \
            pIntercept->config().ITTPerformanceTiming ||                      \
            pIntercept->config().ChromePerformanceTiming ||                   \
            pIntercept->config().DevicePerfCounterEventBasedSampling ) &&     \
          pIntercept->checkDevicePerformanceTimingEnqueueLimits( enqueueCounter ) ) )\
    {                                                                       \
        pIntercept->getTimingTagsKernel(                                    \
            _queue,                                                         \
            _kernel,                                                        \
            _dim,                                                           \
            _gwo,                                                           \
            _gws,                                                           \
            _lws,                                                           \
            hostTag,                                                        \
            deviceTag );                                                    \
    }

///////////////////////////////////////////////////////////////////////////////
//
inline bool CLIntercept::checkHostPerformanceTimingEnqueueLimits(
    uint64_t enqueueCounter ) const
{
    return ( enqueueCounter >= m_Config.HostPerformanceTimingMinEnqueue ) &&
           ( enqueueCounter <= m_Config.HostPerformanceTimingMaxEnqueue );
}

#define HOST_PERFORMANCE_TIMING_START()                                     \
    CLIntercept::clock::time_point   cpuStart, cpuEnd;                      \
    if( pIntercept->config().HostPerformanceTiming ||                       \
        pIntercept->config().ChromeCallLogging )                            \
    {                                                                       \
        cpuStart = CLIntercept::clock::now();                               \
    }

#define HOST_PERFORMANCE_TIMING_END()                                       \
    if( pIntercept->config().HostPerformanceTiming ||                       \
        pIntercept->config().ChromeCallLogging )                            \
    {                                                                       \
        cpuEnd = CLIntercept::clock::now();                                 \
        if( pIntercept->config().HostPerformanceTiming &&                   \
            pIntercept->checkHostPerformanceTimingEnqueueLimits( enqueueCounter ) )\
        {                                                                   \
            pIntercept->updateHostTimingStats(                              \
                __FUNCTION__,                                               \
                "",                                                         \
                cpuStart,                                                   \
                cpuEnd );                                                   \
        }                                                                   \
    }

#define HOST_PERFORMANCE_TIMING_END_WITH_TAG()                              \
    if( pIntercept->config().HostPerformanceTiming ||                       \
        pIntercept->config().ChromeCallLogging )                            \
    {                                                                       \
        cpuEnd = CLIntercept::clock::now();                                 \
        if( pIntercept->config().HostPerformanceTiming &&                   \
            pIntercept->checkHostPerformanceTimingEnqueueLimits( enqueueCounter ) )\
        {                                                                   \
            pIntercept->updateHostTimingStats(                              \
                __FUNCTION__,                                               \
                hostTag,                                                    \
                cpuStart,                                                   \
                cpuEnd );                                                   \
        }                                                                   \
    }

#define TOOL_OVERHEAD_TIMING_START()                                        \
    CLIntercept::clock::time_point   toolStart, toolEnd;                    \
    if( pIntercept->config().ToolOverheadTiming &&                          \
        ( pIntercept->config().HostPerformanceTiming ||                     \
          pIntercept->config().ChromeCallLogging ) )                        \
    {                                                                       \
        toolStart = CLIntercept::clock::now();                              \
    }

#define TOOL_OVERHEAD_TIMING_END( _tag )                                    \
    if( pIntercept->config().ToolOverheadTiming &&                          \
        ( pIntercept->config().HostPerformanceTiming ||                     \
          pIntercept->config().ChromeCallLogging ) )                        \
    {                                                                       \
        toolEnd = CLIntercept::clock::now();                                \
        if( pIntercept->config().HostPerformanceTiming &&                   \
            pIntercept->checkHostPerformanceTimingEnqueueLimits( enqueueCounter ) )\
        {                                                                   \
            pIntercept->updateHostTimingStats(                              \
                _tag,                                                       \
                "",                                                         \
                toolStart,                                                  \
                toolEnd );                                                  \
        }                                                                   \
        if( pIntercept->config().ChromeCallLogging )                        \
        {                                                                   \
            pIntercept->chromeCallLoggingExit(                              \
                _tag,                                                       \
                "",                                                         \
                false,                                                      \
                0,                                                          \
                toolStart,                                                  \
                toolEnd );                                                  \
        }                                                                   \
    }

///////////////////////////////////////////////////////////////////////////////
//
inline bool CLIntercept::checkDevicePerformanceTimingEnqueueLimits(
    uint64_t enqueueCounter ) const
{
    return ( enqueueCounter >= m_Config.DevicePerformanceTimingMinEnqueue ) &&
           ( enqueueCounter <= m_Config.DevicePerformanceTimingMaxEnqueue );
}

#define CREATE_COMMAND_QUEUE_PROPERTIES( _device, _props, _newprops )       \
    if( pIntercept->config().DefaultQueuePriorityHint ||                    \
        pIntercept->config().DefaultQueueThrottleHint )                     \
    {                                                                       \
        pIntercept->createCommandQueueProperties(                           \
            _device,                                                        \
            _props,                                                         \
            _newprops );                                                    \
    }

#define CREATE_COMMAND_QUEUE_OVERRIDE_INIT( _device, _props, _newprops )    \
    if( pIntercept->config().DevicePerformanceTiming ||                     \
        pIntercept->config().ITTPerformanceTiming ||                        \
        pIntercept->config().ChromePerformanceTiming ||                     \
        pIntercept->config().DevicePerfCounterEventBasedSampling ||         \
        pIntercept->config().InOrderQueue ||                                \
        pIntercept->config().NoProfilingQueue ||                            \
        pIntercept->config().DefaultQueuePriorityHint ||                    \
        pIntercept->config().DefaultQueueThrottleHint )                     \
    {                                                                       \
        pIntercept->createCommandQueuePropertiesOverride(                   \
            _device,                                                        \
            _props,                                                         \
            _newprops );                                                    \
    }

#define COMMAND_QUEUE_PROPERTIES_CLEANUP( _newprops )                       \
    delete [] _newprops;                                                    \
    _newprops = NULL;

#define QUEUE_INFO_LOGGING( _device, _queue )                               \
    if( pIntercept->config().QueueInfoLogging && ( _queue != NULL ) )       \
    {                                                                       \
        pIntercept->logQueueInfo(                                           \
            _device,                                                        \
            _queue );                                                       \
    }

#define DUMMY_COMMAND_QUEUE( _context, _device )                            \
    if( pIntercept->config().DummyOutOfOrderQueue )                         \
    {                                                                       \
        pIntercept->dummyCommandQueue( _context, _device );                 \
    }

#define DEVICE_PERFORMANCE_TIMING_START( pEvent )                           \
    CLIntercept::clock::time_point   queuedTime;                            \
    cl_event    local_event = NULL;                                         \
    bool        isLocalEvent = false;                                       \
    if( pIntercept->config().DevicePerformanceTiming ||                     \
        pIntercept->config().ITTPerformanceTiming ||                        \
        pIntercept->config().ChromePerformanceTiming ||                     \
        pIntercept->config().DevicePerfCounterEventBasedSampling )          \
    {                                                                       \
        queuedTime = CLIntercept::clock::now();                             \
        if( pEvent == NULL )                                                \
        {                                                                   \
            pEvent = &local_event;                                          \
            isLocalEvent = true;                                            \
        }                                                                   \
    }

#define DEVICE_PERFORMANCE_TIMING_END( queue, pEvent )                      \
    if( ( pIntercept->config().DevicePerformanceTiming ||                   \
          pIntercept->config().ITTPerformanceTiming ||                      \
          pIntercept->config().ChromePerformanceTiming ||                   \
          pIntercept->config().DevicePerfCounterEventBasedSampling ) &&     \
        ( pEvent != NULL ) )                                                \
    {                                                                       \
        if( pIntercept->checkDevicePerformanceTimingEnqueueLimits( enqueueCounter ) &&\
            ( !pIntercept->config().DevicePerformanceTimingSkipUnmap ||     \
              std::string(__FUNCTION__) != "clEnqueueUnmapMemObject" ) )    \
        {                                                                   \
            /*TOOL_OVERHEAD_TIMING_START();*/                               \
            pIntercept->addTimingEvent(                                     \
                __FUNCTION__,                                               \
                enqueueCounter,                                             \
                queuedTime,                                                 \
                "",                                                         \
                queue,                                                      \
                pEvent[0] );                                                \
            /*TOOL_OVERHEAD_TIMING_END( "(timing event overhead)" );*/      \
        }                                                                   \
        if( isLocalEvent )                                                  \
        {                                                                   \
            pIntercept->dispatch().clReleaseEvent( pEvent[0] );             \
            pEvent = NULL;                                                  \
        }                                                                   \
    }

#define DEVICE_PERFORMANCE_TIMING_END_WITH_TAG( queue, pEvent )             \
    if( ( pIntercept->config().DevicePerformanceTiming ||                   \
          pIntercept->config().ITTPerformanceTiming ||                      \
          pIntercept->config().ChromePerformanceTiming ||                   \
          pIntercept->config().DevicePerfCounterEventBasedSampling ) &&     \
        ( pEvent != NULL ) )                                                \
    {                                                                       \
        if( pIntercept->checkDevicePerformanceTimingEnqueueLimits( enqueueCounter ) )\
        {                                                                   \
            /*TOOL_OVERHEAD_TIMING_START();*/                               \
            pIntercept->addTimingEvent(                                     \
                __FUNCTION__,                                               \
                enqueueCounter,                                             \
                queuedTime,                                                 \
                deviceTag,                                                  \
                queue,                                                      \
                pEvent[0] );                                                \
            /*TOOL_OVERHEAD_TIMING_END( "(timing event overhead)" );*/      \
        }                                                                   \
        if( isLocalEvent )                                                  \
        {                                                                   \
            pIntercept->dispatch().clReleaseEvent( pEvent[0] );             \
            pEvent = NULL;                                                  \
        }                                                                   \
    }

#define DEVICE_PERFORMANCE_TIMING_CHECK()                                   \
    if( pIntercept->config().DevicePerformanceTiming ||                     \
        pIntercept->config().ITTPerformanceTiming ||                        \
        pIntercept->config().ChromePerformanceTiming ||                     \
        pIntercept->config().DevicePerfCounterEventBasedSampling ||         \
        pIntercept->config().DevicePerfCounterTimeBasedSampling )           \
    {                                                                       \
        TOOL_OVERHEAD_TIMING_START();                                       \
        pIntercept->checkTimingEvents();                                    \
        TOOL_OVERHEAD_TIMING_END( "(device timing overhead)" );             \
    }

#define DEVICE_PERFORMANCE_TIMING_CHECK_CONDITIONAL( _condition )           \
    if( ( _condition ) &&                                                   \
        ( pIntercept->config().DevicePerformanceTiming ||                   \
          pIntercept->config().ITTPerformanceTiming ||                      \
          pIntercept->config().ChromePerformanceTiming ||                   \
          pIntercept->config().DevicePerfCounterEventBasedSampling ||       \
          pIntercept->config().DevicePerfCounterTimeBasedSampling ) )       \
    {                                                                       \
        TOOL_OVERHEAD_TIMING_START();                                       \
        pIntercept->checkTimingEvents();                                    \
        TOOL_OVERHEAD_TIMING_END( "(device timing overhead)" );             \
    }

///////////////////////////////////////////////////////////////////////////////
//
inline void CLIntercept::flushChromeTraceBuffering()
{
    m_ChromeTrace.flush();
}

#define FLUSH_CHROME_TRACE_BUFFERING()                                      \
    if( pIntercept->config().ChromeTraceBufferSize &&                       \
        pIntercept->config().ChromeTraceBufferingBlockingCallFlush &&       \
        ( pIntercept->config().ChromeCallLogging ||                         \
          pIntercept->config().ChromePerformanceTiming ) )                  \
    {                                                                       \
        TOOL_OVERHEAD_TIMING_START();                                       \
        pIntercept->flushChromeTraceBuffering();                            \
        TOOL_OVERHEAD_TIMING_END( "(chrome trace flush overhead)" );        \
    }

#define FLUSH_CHROME_TRACE_BUFFERING_CONDITIONAL( _condition )              \
    if( ( _condition ) &&                                                   \
        pIntercept->config().ChromeTraceBufferSize &&                       \
        pIntercept->config().ChromeTraceBufferingBlockingCallFlush &&       \
        ( pIntercept->config().ChromeCallLogging ||                         \
          pIntercept->config().ChromePerformanceTiming ) )                  \
    {                                                                       \
        TOOL_OVERHEAD_TIMING_START();                                       \
        pIntercept->flushChromeTraceBuffering();                            \
        TOOL_OVERHEAD_TIMING_END( "(chrome trace flush overhead)" );        \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define COMMAND_BUFFER_GET_QUEUE( _numQueues, _queues, _cmdbuf )            \
    cl_command_queue command_queue =                                        \
        pIntercept->getCommandBufferCommandQueue(                           \
            _numQueues,                                                     \
            _queues,                                                        \
            _cmdbuf );

///////////////////////////////////////////////////////////////////////////////
//
#define CHECK_EVENT_LIST( _numEvents, _eventList, _event )                  \
    if( pIntercept->config().EventChecking )                                \
    {                                                                       \
        pIntercept->checkEventList(                                         \
            __FUNCTION__,                                                   \
            _numEvents,                                                     \
            _eventList,                                                     \
            _event );                                                       \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define CHECK_KERNEL_ARG_USM_POINTER( _kernel, _arg )                       \
    if( pIntercept->config().USMChecking )                                  \
    {                                                                       \
        pIntercept->checkKernelArgUSMPointer(                               \
            _kernel,                                                        \
            _arg );                                                         \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define USM_ALLOC_OVERRIDE_INIT( _props, _newprops )                        \
    if( pIntercept->config().RelaxAllocationLimits )                        \
    {                                                                       \
        pIntercept->usmAllocPropertiesOverride(                             \
            _props,                                                         \
            _newprops );                                                    \
    }

#define USM_ALLOC_PROPERTIES_CLEANUP( _newprops )                           \
    delete [] _newprops;                                                    \
    _newprops = NULL;

///////////////////////////////////////////////////////////////////////////////
//
inline std::string CLIntercept::getShortKernelName(
    const cl_kernel kernel )
{
    const std::string& realKernelName = m_KernelInfoMap[ kernel ].KernelName;

    CLongKernelNameMap::const_iterator i = m_LongKernelNameMap.find( realKernelName );

    const std::string& shortKernelName =
        ( i != m_LongKernelNameMap.end() ) ?
        i->second :
        realKernelName;

    CLI_ASSERT( shortKernelName.length() <= m_Config.LongKernelNameCutoff );

    return shortKernelName;
}

///////////////////////////////////////////////////////////////////////////////
//
inline std::string CLIntercept::getShortKernelNameWithHash(
    const cl_kernel kernel )
{
    std::string name = getShortKernelName( kernel );

    if( config().KernelNameHashTracking )
    {
        const SKernelInfo& kernelInfo = m_KernelInfoMap[ kernel ];

        char    hashString[256] = "";
        if( config().OmitProgramNumber )
        {
            CLI_SPRINTF( hashString, 256, "$%08X_%04u_%08X",
                (unsigned int)kernelInfo.ProgramHash,
                kernelInfo.CompileCount,
                (unsigned int)kernelInfo.OptionsHash );
        }
        else
        {
            CLI_SPRINTF( hashString, 256, "$%04u_%08X_%04u_%08X",
                kernelInfo.ProgramNumber,
                (unsigned int)kernelInfo.ProgramHash,
                kernelInfo.CompileCount,
                (unsigned int)kernelInfo.OptionsHash );
        }

        name += hashString;
    }

    return name;
}

///////////////////////////////////////////////////////////////////////////////
//
inline unsigned int CLIntercept::getThreadNumber( uint64_t threadId )
{
    CThreadNumberMap::const_iterator iter = m_ThreadNumberMap.find( threadId );
    unsigned int    threadNumber = 0;

    if( iter != m_ThreadNumberMap.end() )
    {
        threadNumber = iter->second;
    }
    else
    {
        threadNumber = (unsigned int)m_ThreadNumberMap.size();
        m_ThreadNumberMap[ threadId ] = threadNumber;

        if( m_Config.ChromeCallLogging )
        {
            m_ChromeTrace.addThreadMetadata( threadId, threadNumber );
        }
    }

    return threadNumber;
}

///////////////////////////////////////////////////////////////////////////////
//
inline void CLIntercept::saveProgramNumber( const cl_program program )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    SProgramInfo&   programInfo = m_ProgramInfoMap[ program ];
    programInfo.ProgramNumber = m_ProgramNumber;
    programInfo.CompileCount = 0;

    m_ProgramNumber++;
}

inline unsigned int CLIntercept::getProgramNumber() const
{
    return m_ProgramNumber;
}

///////////////////////////////////////////////////////////////////////////////
//
inline cl_device_type CLIntercept::filterDeviceType( cl_device_type device_type ) const
{
    if( m_Config.DeviceType & device_type )
    {
        device_type = CL_DEVICE_TYPE_ALL;
    }
    device_type &= (cl_device_type)m_Config.DeviceTypeFilter;
    return device_type;
}

#if defined(USE_ITT)

///////////////////////////////////////////////////////////////////////////////
//
inline __itt_domain* CLIntercept::ittDomain() const
{
    return m_ITTDomain;
}

#endif

///////////////////////////////////////////////////////////////////////////////
//
inline void CLIntercept::logCL_GLTextureDetails( cl_mem image, cl_GLenum target, cl_GLint miplevel, cl_GLuint texture )
{
    CLIntercept* pIntercept = this;

    cl_image_format cl_format = { 0 };
    size_t cl_elementSize = 0;
    size_t cl_rowPitch = 0;
    size_t cl_slicePitch = 0;
    size_t cl_width = 0;
    size_t cl_height = 0;
    size_t cl_depth = 0;

    cl_int subErrorCode = CL_SUCCESS;

    if( subErrorCode == CL_SUCCESS )
    {
        subErrorCode = dispatch().clGetImageInfo(
            image,
            CL_IMAGE_FORMAT,
            sizeof(cl_format),
            &cl_format,
            NULL);
    }

    if( subErrorCode == CL_SUCCESS )
    {
        subErrorCode = dispatch().clGetImageInfo(
            image,
            CL_IMAGE_ELEMENT_SIZE,
            sizeof(cl_elementSize),
            &cl_elementSize,
            NULL);
    }

    if( subErrorCode == CL_SUCCESS )
    {
        subErrorCode = dispatch().clGetImageInfo(
            image,
            CL_IMAGE_ROW_PITCH,
            sizeof(cl_rowPitch),
            &cl_rowPitch,
            NULL);
    }

    if( subErrorCode == CL_SUCCESS )
    {
        subErrorCode = dispatch().clGetImageInfo(
            image,
            CL_IMAGE_SLICE_PITCH,
            sizeof(cl_slicePitch),
            &cl_slicePitch,
            NULL);
    }

    if( subErrorCode == CL_SUCCESS )
    {
        subErrorCode = dispatch().clGetImageInfo(
            image,
            CL_IMAGE_WIDTH,
            sizeof(cl_width),
            &cl_width,
            NULL);
    }

    if( subErrorCode == CL_SUCCESS )
    {
        subErrorCode = dispatch().clGetImageInfo(
            image,
            CL_IMAGE_HEIGHT,
            sizeof(cl_height),
            &cl_height,
            NULL);
    }

    if( subErrorCode == CL_SUCCESS )
    {
        subErrorCode = dispatch().clGetImageInfo(
            image,
            CL_IMAGE_DEPTH,
            sizeof(cl_depth),
            &cl_depth,
            NULL);
    }

    CALL_LOGGING_INFO(
        "CL Channel Order = %s, "
        "CL Channel Data Type = %s, "
        "CL Row Pitch = %zu, "
        "CL Slice Pitch = %zu, "
        "CL Width = %zu, "
        "CL Height = %zu, "
        "CL Depth = %zu, ",
        enumName().name( cl_format.image_channel_order ).c_str(),
        enumName().name( cl_format.image_channel_data_type ).c_str(),
        cl_rowPitch,
        cl_slicePitch,
        cl_width,
        cl_height,
        cl_depth );

    // OpenGL.lib is not linked into CLIntercept - the OpenGL calls are performed by using GetProcAddress,
    // which is only available on windows.
#ifdef _WIN32
    HMODULE glModule = GetModuleHandle( "Opengl32.dll" );

    if( glModule != NULL )
    {
        FARPROC ptrGetTexLevel = GetProcAddress( glModule, "glGetTexLevelParameteriv" );
        FARPROC ptrGetIntegerv = GetProcAddress( glModule, "glGetIntegerv" );
        FARPROC ptrBindTexture = GetProcAddress( glModule, "glBindTexture" );
        FARPROC ptrGetError    = GetProcAddress( glModule, "glGetError" );

        if( ( ptrGetTexLevel != NULL ) &&
            ( ptrGetIntegerv != NULL ) &&
            ( ptrBindTexture != NULL ) &&
            ( ptrGetError != NULL ) )
        {
            PFNGLGETTEXLEVELPARAMETERIVPROC _glGetTexLevel = (PFNGLGETTEXLEVELPARAMETERIVPROC)ptrGetTexLevel;
            PFNGLGETINTEGERVPROC _glGetInteger = (PFNGLGETINTEGERVPROC)ptrGetIntegerv;
            PFNGLBINDTEXTUREPROC _glBindTexture = (PFNGLBINDTEXTUREPROC)ptrBindTexture;
            PFNGLGETERRORPROC _glGetError = (PFNGLGETERRORPROC)ptrGetError;

            GLenum gl_error = _glGetError();

            if( gl_error == GL_FALSE )
            {
                GLint restoreTextureId = 0;

                // Save the currently bound texture - we need to to rebind a different
                // texture to query it.
                switch( target )
                {
                case GL_TEXTURE_1D:
                    _glGetInteger( GL_TEXTURE_BINDING_1D, &restoreTextureId );
                    break;
                case GL_TEXTURE_1D_ARRAY:
                    _glGetInteger( GL_TEXTURE_BINDING_1D_ARRAY, &restoreTextureId );
                    break;
                case GL_TEXTURE_2D:
                    _glGetInteger( GL_TEXTURE_BINDING_2D, &restoreTextureId );
                    break;
                case GL_TEXTURE_2D_ARRAY:
                    _glGetInteger( GL_TEXTURE_BINDING_2D_ARRAY, &restoreTextureId );
                    break;
                case GL_TEXTURE_3D:
                    _glGetInteger( GL_TEXTURE_BINDING_3D, &restoreTextureId );
                    break;
                case GL_TEXTURE_CUBE_MAP:
                    _glGetInteger( GL_TEXTURE_BINDING_CUBE_MAP, &restoreTextureId );
                    break;
                case GL_TEXTURE_BUFFER:
                    _glGetInteger( GL_TEXTURE_BINDING_BUFFER, &restoreTextureId );
                    break;
                default:
                    // unexpected texture type
                    gl_error = GL_TRUE;
                    break;
                }

                if( gl_error == GL_FALSE )
                {
                    GLint gl_width = 0;
                    GLint gl_height = 0;
                    GLint gl_depth = 0;
                    GLint gl_internal_format = 0;
                    GLint gl_buffer_size = 0;
                    GLint gl_buffer_offset = 0;
                    GLint gl_compressed_texture = GL_FALSE;

                    // Bind the texture we want to query
                    _glBindTexture( target, texture );
                    gl_error = _glGetError();


                    if( gl_error == GL_FALSE )
                    {
                        _glGetTexLevel(
                            target,
                            miplevel > 0 ? miplevel : 0,
                            GL_TEXTURE_INTERNAL_FORMAT,
                            &gl_internal_format );
                        gl_error = _glGetError();
                    }

                    if( gl_error == GL_FALSE )
                    {
                        _glGetTexLevel(
                            target,
                            miplevel > 0 ? miplevel : 0,
                            GL_TEXTURE_WIDTH,
                            &gl_width );
                        gl_error = _glGetError();
                    }

                    if( gl_error == GL_FALSE )
                    {
                        _glGetTexLevel(
                            target,
                            miplevel > 0 ? miplevel : 0,
                            GL_TEXTURE_HEIGHT,
                            &gl_height );
                        gl_error = _glGetError();
                    }

                    if( gl_error == GL_FALSE )
                    {
                        _glGetTexLevel(
                            target,
                            miplevel > 0 ? miplevel : 0,
                            GL_TEXTURE_DEPTH,
                            &gl_depth );
                        gl_error = _glGetError();
                    }

                    if( gl_error == GL_FALSE )
                    {
                        _glGetTexLevel(
                            target,
                            miplevel > 0 ? miplevel : 0,
                            GL_TEXTURE_BUFFER_SIZE,
                            &gl_buffer_size );
                        gl_error = _glGetError();
                    }

                    if( gl_error == GL_FALSE )
                    {
                        _glGetTexLevel(
                            target,
                            miplevel > 0 ? miplevel : 0,
                            GL_TEXTURE_BUFFER_OFFSET,
                            &gl_buffer_offset );
                        gl_error = _glGetError();
                    }

                    // restore original bound texture
                    _glBindTexture( target, restoreTextureId );
                    gl_error = _glGetError();

                    CALL_LOGGING_INFO(
                        "GL Internal Format = %s (%d), "
                        "GL Width = %d, "
                        "GL Height = %d, "
                        "GL Depth = %d, "
                        "GL Buffer Size = %d, "
                        "GL Buffer Offset = %d ",
                        enumName().name_gl( gl_internal_format ).c_str(),
                        gl_internal_format,
                        gl_width,
                        gl_height,
                        gl_depth,
                        gl_buffer_size,
                        gl_buffer_offset );
                }
            }
        }
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
extern CLIntercept* g_pIntercept;

inline CLIntercept* GetIntercept()
{
    return g_pIntercept;
}

#define NULL_FUNCTION_POINTER_RETURN_ERROR()                                \
    CLI_ASSERT(0);                                                          \
    return CL_INVALID_OPERATION;

#define NULL_FUNCTION_POINTER_SET_ERROR_RETURN_NULL(pErrorCode)             \
    CLI_ASSERT(0);                                                          \
    if( pErrorCode )                                                        \
    {                                                                       \
        pErrorCode[0] = CL_INVALID_OPERATION;                               \
    }                                                                       \
    return NULL;
