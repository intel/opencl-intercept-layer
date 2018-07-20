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

#include <fstream>
#include <list>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <queue>

#include <stdint.h>

#include "common.h"
#include "enummap.h"
#include "dispatch.h"
#include "objtracker.h"

#include "instrumentation.h"

#if defined(USE_MDAPI)
#include "MetricsDiscoveryHelper.h"
#endif

#if defined(_WIN32)

#elif defined(__linux__) || defined(__APPLE__)

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
    static bool Create( void* pGlobalData, CLIntercept*& pIntercept );
    static void Delete( CLIntercept*& pIntercept );

    void    report();

    void    callLoggingEnter(
                const std::string& functionName,
                const cl_kernel kernel );
    void    callLoggingEnter(
                const std::string& functionName,
                const cl_kernel kernel,
                const char* formatStr,
                ... );

    void    callLoggingInfo(
                const std::string& str );
    void    callLoggingInfo(
                const char* formatStr,
                ... );

    void    callLoggingExit(
                const std::string& functionName,
                const cl_kernel kernel,
                const cl_event* event );
    void    callLoggingExit(
                const std::string& functionName,
                const cl_kernel kernel,
                const cl_event* event,
                const char* formatStr,
                ... );

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
    void    getEventListString(
                cl_uint num_events,
                const cl_event* event_list,
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
                uint64_t buildTimeStart,
                const cl_program program,
                cl_uint num_devices,
                const cl_device_id* device_list );
    void    logError(
                const std::string& functionName,
                cl_int errorCode );
    void    logFlushOrFinishAfterEnqueueStart(
                const std::string& flushOrFinish,
                const std::string& functionName );
    void    logFlushOrFinishAfterEnqueueEnd(
                const std::string& flushOrFinish,
                const std::string& functionName,
                cl_int errorCode );
    void    logPreferredWorkGroupSizeMultiple(
                const cl_kernel* kernels,
                cl_uint numKernels );

    void    logCL_GLTextureDetails( cl_mem image, cl_GLenum target, cl_GLint miplevel, cl_GLuint texture );

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

    void    incrementEnqueueCounter();
    uint64_t getEnqueueCounter();

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
    uint64_t hashString(
                const char* singleString,
                size_t length );
    void    saveProgramHash(
                const cl_program program,
                uint64_t hash );

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
                const char*& options,
                char*& newOptions );
    bool    appendBuildOptions(
                const char*& options,
                char*& newOptions );
    void    dumpProgramSourceScript(
                const cl_program program,
                const char* singleString );
    void    dumpProgramSource(
                uint64_t hash,
                const cl_program program,
                const char* singleString );
    void    dumpInputProgramBinaries(
                uint64_t hash,
                const cl_program program,
                cl_uint num_devices,
                const cl_device_id* device_list,
                const size_t* lengths,
                const unsigned char** binaries );
    void    dumpProgramSPIRV(
                uint64_t hash,
                const cl_program program,
                const size_t length,
                const void* il );
    void    dumpProgramOptionsScript(
                const cl_program program,
                const char* options );
    void    dumpProgramOptions(
                const cl_program program,
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

    void    updateHostTimingStats(
                const std::string& functionName ,
                const cl_kernel kernel,
                uint64_t start,
                uint64_t end );

    void    modifyCommandQueueProperties(
                cl_command_queue_properties& props ) const;
    void    createCommandQueueOverrideInit(
                const cl_queue_properties* properties,
                cl_queue_properties*& pLocalQueueProperties ) const;
    void    createCommandQueueOverrideCleanup(
                cl_queue_properties*& pLocalQueueProperties ) const;
    void    addTimingEvent(
                const std::string& functionName,
                const uint64_t queuedTime,
                const cl_kernel kernel,
                const cl_uint workDim,
                const size_t* gwo,
                const size_t* gws,
                const size_t* lws,
                cl_event event );
    void    checkTimingEvents();

    void    addKernelName(
                const cl_kernel kernel,
                const std::string& kernelName );

    void    addKernelNames(
                cl_kernel* kernels,
                cl_uint numKernels );

    void    removeKernel(
                const cl_kernel kernel );

    void    addBuffer(
                cl_mem buffer );
    void    addSampler(
                cl_sampler sampler,
                const std::string& str );
    void    removeSampler(
                cl_sampler sampler );
    bool    getSampler(
                size_t size,
                const void *arg_value,
                std::string& str ) const;
    void    addImage(
                cl_mem image );
    void    removeMemObj(
                cl_mem memobj );
    void    addSVMAllocation(
                void* svmPtr,
                size_t size );
    void    removeSVMAllocation(
                void* svmPtr );
    void    setKernelArg(
                cl_kernel kernel,
                cl_uint arg_index,
                cl_mem memobj );
    void    setKernelArgSVMPointer(
                cl_kernel kernel,
                cl_uint arg_index,
                const void* arg );
    void    dumpBuffersForKernel(
                const std::string& name,
                cl_kernel kernel,
                cl_command_queue command_queue );
    void    dumpImagesForKernel(
                const std::string& name,
                cl_kernel kernel,
                cl_command_queue command_queue );
    void    dumpBuffer(
                const std::string& name,
                cl_mem memobj,
                cl_command_queue command_queue,
                void* ptr,
                size_t offset,
                size_t size );
    void    dumpArgument(
                cl_kernel kernel,
                cl_int arg_index,
                size_t size,
                const void *pBuffer );

    void    checkEventList(
                const std::string& functionName,
                cl_uint numEvents,
                const cl_event* eventList );

    void    startAubCapture(
                const std::string& functionName,
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
    cl_int  writeParamToMemory(
                size_t param_value_size,
                T param,
                size_t* param_value_size_ret,
                T* pointer ) const;

    bool    overrideGetPlatformInfo(
                cl_platform_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret,
                cl_int& errorCode );
    bool    overrideGetDeviceInfo(
                cl_device_id device,
                cl_platform_info param_name,
                size_t param_value_size,
                void* param_value,
                size_t* param_value_size_ret,
                cl_int& errorCode );

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

    void    SIMDSurveyCreateProgramFromSource(
                const cl_program program,
                cl_context context,
                cl_uint count,
                const char** strings,
                const size_t* lengths );
    // TODO?
    // SIMDSurveyCreateProgramWithBinary
    // SIMDSurveyCreateProgramWithIL
    void    SIMDSurveyBuildProgram(
                const cl_program program,
                cl_uint numDevices,
                const cl_device_id* deviceList,
                const char* options );
    void    SIMDSurveyCreateKernel(
                const cl_program program,
                const cl_kernel kernel,
                const std::string& kernelName );
    // TODO?
    // SIMDSurveyCreateKernelsInProgram();
    // SIMDSurveyCloneKernel();
    void    SIMDSurveySetKernelArg(
                cl_kernel kernel,
                cl_uint argIndex,
                size_t argSize,
                const void* argValue );
    void    SIMDSurveyNDRangeKernel(
                cl_kernel& kernel );

    void*   getExtensionFunctionAddress(
                cl_platform_id platform,
                const std::string& func_name ) const;

#if defined(USE_MDAPI)
    GTDI_CONFIGURATION_SET initCustomPerfCounters(
                const std::string& setName );
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

    const OS::Services& OS() const;
    const CLdispatch&   dispatch() const;
    const CEnumNameMap& enumName() const;

    const Config&   config() const;

    CObjectTracker& objectTracker();

    bool    callLogging() const;

    bool    nullEnqueue() const;

    bool    dumpBufferForKernel( const cl_kernel kernel );
    bool    dumpImagesForKernel( const cl_kernel kernel );
    bool    checkDumpBufferEnqueueLimits() const;
    bool    checkDumpImageEnqueueLimits() const;

    bool    checkAubCaptureEnqueueLimits() const;
    bool    checkAubCaptureKernelSignature(
                const cl_kernel kernel,
                cl_uint workDim,
                const size_t* gws,
                const size_t* lws);

    void    saveProgramNumber( const cl_program program );
    unsigned int getProgramNumber() const;

    cl_device_type filterDeviceType( cl_device_type device_type ) const;

#if defined(USE_ITT)
    __itt_domain*   ittDomain() const;

    void    ittInit();

    void    ittCallLoggingEnter(
                const std::string& functionName,
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
                uint64_t queuedTime );
#endif

    void    chromeCallLoggingExit(
                const std::string& functionName,
                const cl_kernel kernel,
                uint64_t start,
                uint64_t end );
    void    chromeRegisterCommandQueue(
                cl_command_queue queue );
    void    chromeTraceEvent(
                const std::string& name,
                cl_event event,
                uint64_t queuedTime );

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

#if defined(_WIN32) || defined(__linux__)
    bool    initDispatch( const std::string& dllName );
#elif defined(__APPLE__)
    bool    initDispatch( void );
#else
#error Unknown OS!
#endif

    std::string getKernelName(
                  const cl_kernel kernel );

    void    getCallLoggingPrefix(
                std::string& str );

    void    writeReport(
                std::ostream& os );

    OS::Services    m_OS;
    CLdispatch      m_Dispatch;
    CEnumNameMap    m_EnumNameMap;
    CObjectTracker  m_ObjectTracker;

    void*       m_OpenCLLibraryHandle;

    std::ofstream   m_InterceptLog;
    std::ofstream   m_InterceptTrace;

    bool        m_LoggedCLInfo;

    uint64_t    m_EnqueueCounter;
    uint64_t    m_StartTime;

    typedef std::map< uint64_t, unsigned int>   CThreadNumberMap;
    CThreadNumberMap    m_ThreadNumberMap;

    unsigned int    m_ProgramNumber;

    typedef std::map< const cl_program, unsigned int >  CProgramNumberMap;
    CProgramNumberMap   m_ProgramNumberMap;

    typedef std::map< const cl_program, uint64_t >  CProgramHashMap;
    CProgramHashMap m_ProgramHashMap;

    typedef std::map< unsigned int, unsigned int > CProgramNumberCompileCountMap;
    CProgramNumberCompileCountMap   m_ProgramNumberCompileCountMap;

    struct SCpuTimingStats
    {
        uint64_t    NumberOfCalls;
        uint64_t    MinTicks;
        uint64_t    MaxTicks;
        uint64_t    TotalTicks;
    };

    typedef std::map< std::string, SCpuTimingStats* > CCpuTimingStatsMap;
    CCpuTimingStatsMap  m_CpuTimingStatsMap;

    struct SDeviceTimingStats
    {
        std::string KernelId;
        uint64_t    NumberOfCalls;
        cl_ulong    MinNS;
        cl_ulong    MaxNS;
        cl_ulong    TotalNS;
    };

    typedef std::map< std::string, SDeviceTimingStats* >   CDeviceTimingStatsMap;
    CDeviceTimingStatsMap   m_DeviceTimingStatsMap;

    struct kernelNameInfo
    {
      std::string kernelId;
      std::string kernelName;
    };
    typedef std::map< const cl_kernel, kernelNameInfo >    CKernelNameMap;
    CKernelNameMap  m_KernelNameMap;
    int m_kernelId;
    uint m_maxKernelLength;

    struct SEventListNode
    {
        std::string FunctionName;
        std::string KernelName;
        std::string KernelId;
        uint64_t    QueuedTime;
        cl_kernel   Kernel;
        cl_event    Event;
    };

    typedef std::list< SEventListNode* > CEventList;
    CEventList  m_EventList;

#if defined(USE_MDAPI)
    TimingProfile m_DeviceTimingProfile;

    typedef std::pair< std::string, char* > CMDDataEntry;
    typedef std::queue< CMDDataEntry* > CMDDataList;
    CMDDataList m_MDDataList;

    void    saveMDAPICounters(
                const std::string& name,
                const cl_event event );
    void    reportMDAPICounters(
                std::ostream& os );
#endif

    unsigned int    m_MemAllocNumber;

    typedef std::map< const void*, unsigned int >   CMemAllocNumberMap;
    CMemAllocNumberMap  m_MemAllocNumberMap;

    typedef std::map< cl_sampler, std::string > CSamplerDataMap;
    CSamplerDataMap m_SamplerDataMap;

    typedef std::map< const cl_mem, size_t >   CBufferInfoMap;
    CBufferInfoMap      m_BufferInfoMap;

    typedef std::map< const void*, size_t >    CSVMAllocInfoMap;
    CSVMAllocInfoMap    m_SVMAllocInfoMap;

    struct SImageInfo
    {
        size_t  Region[3];
        size_t  ElementSize;
    };

    typedef std::map< const cl_mem, SImageInfo >    CImageInfoMap;
    CImageInfoMap   m_ImageInfoMap;

    typedef std::map< cl_uint, const void* >                CKernelArgMemMap;
    typedef std::map< const cl_kernel, CKernelArgMemMap >   CKernelArgMap;
    CKernelArgMap   m_KernelArgMap;

    bool    m_AubCaptureStarted;
    cl_uint m_AubCaptureKernelEnqueueSkipCounter;
    cl_uint m_AubCaptureKernelEnqueueCaptureCounter;

    typedef std::set<std::string>   CAubCaptureSet;
    CAubCaptureSet  m_AubCaptureSet;

    typedef std::map< const cl_context, SContextCallbackInfo* >  CContextCallbackInfoMap;
    CContextCallbackInfoMap m_ContextCallbackInfoMap;

    typedef std::map< const cl_event, SEventCallbackInfo* > CEventCallbackInfoMap;
    CEventCallbackInfoMap   m_EventCallbackInfoMap;

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

    typedef std::map< const cl_context, SPrecompiledKernelOverrides* >  CPrecompiledKernelOverridesMap;
    CPrecompiledKernelOverridesMap  m_PrecompiledKernelOverridesMap;

    struct SBuiltinKernelOverrides
    {
        cl_program  Program;

        cl_kernel   Kernel_block_motion_estimate_intel;
    };

    typedef std::map< const cl_context, SBuiltinKernelOverrides* >  CBuiltinKernelOverridesMap;
    CBuiltinKernelOverridesMap  m_BuiltinKernelOverridesMap;

    struct SSIMDSurveyProgram
    {
        cl_program  SIMD8Program;
        cl_program  SIMD16Program;
        cl_program  SIMD32Program;
    };
    struct SSIMDSurveyKernel
    {
        cl_kernel   SIMD8Kernel;
        cl_kernel   SIMD16Kernel;
        cl_kernel   SIMD32Kernel;

        cl_ulong    SIMD8ExecutionTimeNS;
        cl_ulong    SIMD16ExecutionTimeNS;
        cl_ulong    SIMD32ExecutionTimeNS;

        uint32_t    ExecutionNumber;
    };

    typedef std::map< const cl_program, SSIMDSurveyProgram* >   CSIMDSurveyProgramMap;
    CSIMDSurveyProgramMap   m_SIMDSurveyProgramMap;

    typedef std::map< const cl_kernel, SSIMDSurveyKernel* > CSIMDSurveyKernelMap;
    CSIMDSurveyKernelMap    m_SIMDSurveyKernelMap;

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
        uint64_t            CPUReferenceTime;
        cl_ulong            CLReferenceTime;
    };

    typedef std::map< cl_command_queue, SITTQueueInfo* > CITTQueueInfoMap;
    CITTQueueInfoMap    m_ITTQueueInfoMap;
#endif

    DISALLOW_COPY_AND_ASSIGN( CLIntercept );
};

///////////////////////////////////////////////////////////////////////////////
//
inline const CLdispatch& CLIntercept::dispatch() const
{
    return m_Dispatch;
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
    uint64_t    buildTimeStart = 0;                                         \
    if( pIntercept->config().BuildLogging )                                 \
    {                                                                       \
        buildTimeStart = pIntercept->OS().GetTimer();                       \
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
inline bool CLIntercept::callLogging() const
{
    return m_Config.CallLogging;
}

#define CALL_LOGGING_ENTER(...)                                             \
    if( pIntercept->config().CallLogging )                                  \
    {                                                                       \
        pIntercept->callLoggingEnter( __FUNCTION__, NULL, ##__VA_ARGS__ );  \
    }                                                                       \
    ITT_CALL_LOGGING_ENTER( NULL );

#define CALL_LOGGING_ENTER_KERNEL(kernel, ...)                              \
    if( pIntercept->config().CallLogging )                                  \
    {                                                                       \
        pIntercept->callLoggingEnter( __FUNCTION__, kernel, ##__VA_ARGS__ );\
    }                                                                       \
    ITT_CALL_LOGGING_ENTER( kernel );

#define CALL_LOGGING_INFO(...)                                              \
    if( pIntercept->config().CallLogging )                                  \
    {                                                                       \
        pIntercept->callLoggingInfo( __VA_ARGS__ );                         \
    }                                                                       \

#define CALL_LOGGING_EXIT(...)                                                  \
    if( pIntercept->config().CallLogging )                                      \
    {                                                                           \
        pIntercept->callLoggingExit( __FUNCTION__, NULL, NULL, ##__VA_ARGS__ ); \
    }                                                                           \
    ITT_CALL_LOGGING_EXIT();

#define CALL_LOGGING_EXIT_EVENT(event, ...)                                     \
    if( pIntercept->config().CallLogging )                                      \
    {                                                                           \
        pIntercept->callLoggingExit( __FUNCTION__, NULL, event, ##__VA_ARGS__ );\
    }                                                                           \
    ITT_CALL_LOGGING_EXIT();

///////////////////////////////////////////////////////////////////////////////
//
#define CHECK_ERROR_INIT( pErrorCode )                                      \
    cl_int  localErrorCode = CL_SUCCESS;                                    \
    if( ( pIntercept->config().ErrorLogging ||                              \
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
    pIntercept->incrementEnqueueCounter();                                  \
    if( pIntercept->config().FinishAfterEnqueue )                           \
    {                                                                       \
        pIntercept->logFlushOrFinishAfterEnqueueStart(                      \
            "clFinish",                                                     \
            __FUNCTION__ );                                                 \
        cl_int  e = pIntercept->dispatch().clFinish( _command_queue );      \
        pIntercept->logFlushOrFinishAfterEnqueueEnd(                        \
            "clFinish",                                                     \
            __FUNCTION__,                                                   \
            e );                                                            \
        pIntercept->checkTimingEvents();                                    \
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
inline bool CLIntercept::nullEnqueue() const
{
    return m_Config.NullEnqueue;
}

///////////////////////////////////////////////////////////////////////////////
//
inline bool CLIntercept::dumpBufferForKernel( const cl_kernel kernel )
{
    return m_Config.DumpBuffersForKernel.empty() ||
        m_KernelNameMap[ kernel ].kernelName == m_Config.DumpBuffersForKernel;
}

inline bool CLIntercept::dumpImagesForKernel( const cl_kernel kernel )
{
    return m_Config.DumpImagesForKernel.empty() ||
        m_KernelNameMap[ kernel ].kernelName == m_Config.DumpImagesForKernel;
}

inline bool CLIntercept::checkDumpBufferEnqueueLimits() const
{
    return ( m_EnqueueCounter >= m_Config.DumpBuffersMinEnqueue ) &&
           ( m_EnqueueCounter <= m_Config.DumpBuffersMaxEnqueue );
}

inline bool CLIntercept::checkDumpImageEnqueueLimits() const
{
    return ( m_EnqueueCounter >= m_Config.DumpImagesMinEnqueue ) &&
           ( m_EnqueueCounter <= m_Config.DumpImagesMaxEnqueue );
}

#define ADD_BUFFER( buffer )                                                \
    if( buffer &&                                                           \
        ( pIntercept->config().DumpBuffersAfterCreate ||                    \
          pIntercept->config().DumpBuffersAfterMap ||                       \
          pIntercept->config().DumpBuffersBeforeUnmap ||                    \
          pIntercept->config().DumpBuffersBeforeEnqueue ||                  \
          pIntercept->config().DumpBuffersAfterEnqueue ) )                  \
    {                                                                       \
        pIntercept->addBuffer( buffer );                                    \
    }

#define ADD_IMAGE( image )                                                  \
    if( image &&                                                            \
        ( pIntercept->config().DumpImagesBeforeEnqueue ||                   \
          pIntercept->config().DumpImagesAfterEnqueue ) )                   \
    {                                                                       \
        pIntercept->addImage( image );                                      \
    }

#define REMOVE_MEMOBJ( memobj )                                             \
    if( memobj &&                                                           \
        ( pIntercept->config().DumpBuffersAfterCreate ||                    \
          pIntercept->config().DumpBuffersAfterMap ||                       \
          pIntercept->config().DumpBuffersBeforeUnmap ||                    \
          pIntercept->config().DumpBuffersBeforeEnqueue ||                  \
          pIntercept->config().DumpBuffersAfterEnqueue ||                   \
          pIntercept->config().DumpImagesBeforeEnqueue ||                   \
          pIntercept->config().DumpImagesAfterEnqueue ) )                   \
    {                                                                       \
        pIntercept->removeMemObj( memobj );                                 \
    }

#define ADD_SAMPLER( sampler, str )                                         \
    if( sampler &&                                                          \
        pIntercept->config().CallLogging )                                         \
    {                                                                       \
        pIntercept->addSampler( sampler, str );                             \
    }

#define REMOVE_SAMPLER( sampler )                                           \
    if( sampler &&                                                          \
        pIntercept->config().CallLogging )                                         \
    {                                                                       \
        pIntercept->removeSampler( sampler );                               \
    }

#define ADD_SVM_ALLOCATION( svmPtr, size )                                  \
    if( svmPtr &&                                                           \
        ( pIntercept->config().DumpBuffersBeforeEnqueue ||                  \
          pIntercept->config().DumpBuffersAfterEnqueue ) )                  \
    {                                                                       \
        pIntercept->addSVMAllocation( svmPtr, size );                       \
    }

#define REMOVE_SVM_ALLOCATION( svmPtr )                                     \
    if( svmPtr &&                                                           \
        ( pIntercept->config().DumpBuffersBeforeEnqueue ||                  \
          pIntercept->config().DumpBuffersAfterEnqueue ) )                  \
    {                                                                       \
        pIntercept->removeSVMAllocation( svmPtr );                          \
    }

#define SET_KERNEL_ARG( kernel, arg_index, arg_size, arg_value )            \
    if( ( pIntercept->config().DumpBuffersBeforeEnqueue ||                  \
          pIntercept->config().DumpBuffersAfterEnqueue ||                   \
          pIntercept->config().DumpImagesBeforeEnqueue ||                   \
          pIntercept->config().DumpImagesAfterEnqueue ) &&                  \
        ( arg_value != NULL ) &&                                            \
        ( arg_size == sizeof(cl_mem) ) )                                    \
    {                                                                       \
        cl_mem* pMem = (cl_mem*)arg_value;                                  \
        pIntercept->setKernelArg( kernel, arg_index, pMem[0] );             \
    }

#define SET_KERNEL_ARG_SVM_POINTER( kernel, arg_index, arg_value )          \
    if( pIntercept->config().DumpBuffersBeforeEnqueue ||                    \
        pIntercept->config().DumpBuffersAfterEnqueue )                      \
    {                                                                       \
        pIntercept->setKernelArgSVMPointer( kernel, arg_index, arg_value ); \
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
        pIntercept->checkDumpBufferEnqueueLimits() &&                       \
        pIntercept->config().DumpBuffersAfterCreate )                       \
    {                                                                       \
        pIntercept->dumpBuffer( "Create", memobj, NULL, ptr, 0, size );     \
    }

#define DUMP_BUFFER_AFTER_MAP( command_queue, memobj, blocking_map, flags, ptr, offset, size ) \
    if( memobj &&                                                           \
        !( flags & CL_MAP_WRITE_INVALIDATE_REGION ) &&                      \
        pIntercept->checkDumpBufferEnqueueLimits() &&                       \
        pIntercept->config().DumpBuffersAfterMap )                          \
    {                                                                       \
        if( blocking_map == false )                                         \
        {                                                                   \
            pIntercept->dispatch().clFinish( command_queue );               \
        }                                                                   \
        pIntercept->dumpBuffer( "Map", memobj, NULL, ptr, offset, size );   \
    }

#define DUMP_BUFFER_BEFORE_UNMAP( memobj, command_queue)                    \
    if( memobj &&                                                           \
        command_queue &&                                                    \
        pIntercept->checkDumpBufferEnqueueLimits() &&                       \
        pIntercept->config().DumpBuffersBeforeUnmap )                       \
    {                                                                       \
        pIntercept->dumpBuffer( "Unmap", memobj, command_queue, NULL, 0, 0 );\
    }

#define DUMP_BUFFERS_BEFORE_ENQUEUE( kernel, command_queue )                \
    if( pIntercept->checkDumpBufferEnqueueLimits() &&                       \
        pIntercept->config().DumpBuffersBeforeEnqueue &&                    \
        pIntercept->dumpBufferForKernel( kernel ) )                         \
    {                                                                       \
        pIntercept->dumpBuffersForKernel( "Pre", kernel, command_queue );   \
    }

#define DUMP_BUFFERS_AFTER_ENQUEUE( kernel, command_queue )                 \
    if( pIntercept->checkDumpBufferEnqueueLimits() &&                       \
        pIntercept->config().DumpBuffersAfterEnqueue &&                     \
        pIntercept->dumpBufferForKernel( kernel ) )                         \
    {                                                                       \
        pIntercept->dumpBuffersForKernel( "Post", kernel, command_queue );  \
    }

#define DUMP_IMAGES_BEFORE_ENQUEUE( kernel, command_queue )                 \
    if( pIntercept->checkDumpImageEnqueueLimits() &&                        \
        pIntercept->config().DumpImagesBeforeEnqueue &&                     \
        pIntercept->dumpImagesForKernel( kernel ) )                         \
    {                                                                       \
        pIntercept->dumpImagesForKernel( "Pre", kernel, command_queue );    \
    }

#define DUMP_IMAGES_AFTER_ENQUEUE( kernel, command_queue )                  \
    if( pIntercept->checkDumpImageEnqueueLimits() &&                        \
        pIntercept->config().DumpImagesAfterEnqueue &&                      \
        pIntercept->dumpImagesForKernel( kernel ) )                         \
    {                                                                       \
        pIntercept->dumpImagesForKernel( "Post", kernel, command_queue );   \
    }

///////////////////////////////////////////////////////////////////////////////
//
inline bool CLIntercept::checkAubCaptureEnqueueLimits() const
{
    return ( m_EnqueueCounter >= m_Config.AubCaptureMinEnqueue ) &&
           ( m_EnqueueCounter <= m_Config.AubCaptureMaxEnqueue );
}

// Note: We do not individually aub capture non-kernel enqueues at the moment.
#define CHECK_AUBCAPTURE_START( command_queue )                             \
    if( pIntercept->config().AubCapture &&                                  \
        pIntercept->checkAubCaptureEnqueueLimits() &&                       \
        !pIntercept->config().AubCaptureIndividualEnqueues )                \
    {                                                                       \
        pIntercept->startAubCapture(                                        \
            __FUNCTION__, NULL, 0, NULL, NULL, command_queue );             \
    }

#define CHECK_AUBCAPTURE_START_KERNEL( kernel, wd, gws, lws, command_queue )\
    if( pIntercept->config().AubCapture &&                                  \
        pIntercept->checkAubCaptureEnqueueLimits() &&                       \
        pIntercept->checkAubCaptureKernelSignature( kernel, wd, gws, lws ) )\
    {                                                                       \
        pIntercept->startAubCapture(                                        \
            __FUNCTION__, kernel, wd, gws, lws, command_queue );            \
    }

#define CHECK_AUBCAPTURE_STOP( command_queue )                              \
    if( pIntercept->config().AubCapture &&                                  \
        ( pIntercept->config().AubCaptureIndividualEnqueues ||              \
          !pIntercept->checkAubCaptureEnqueueLimits() ) )                   \
    {                                                                       \
        pIntercept->stopAubCapture( command_queue );                        \
    }

///////////////////////////////////////////////////////////////////////////////
//

// Shared:

#define SAVE_PROGRAM_HASH( program, hash )                                  \
    if( pIntercept->config().DevicePerformanceTimeHashTracking ||           \
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

// Called from clCreateProgramWithSource:

#define CREATE_COMBINED_PROGRAM_STRING( count, strings, lengths, singleString, hash ) \
    if( pIntercept->config().DevicePerformanceTimeHashTracking ||           \
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
        pIntercept->config().AubCaptureUniqueKernels )                      \
    {                                                                       \
        pIntercept->combineProgramStrings(                                  \
            count,                                                          \
            strings,                                                        \
            lengths,                                                        \
            singleString );                                                 \
        hash = pIntercept->hashString(                                      \
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
    if( ( injected == false ) &&                                            \
        ( pIntercept->config().DumpProgramSource ||                         \
          pIntercept->config().AutoCreateSPIRV ) )                          \
    {                                                                       \
        pIntercept->dumpProgramSource( hash, program, singleString );       \
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
#define COMPUTE_BINARY_HASH( _num, _lengths, _binaries, _hash )             \
    if( _lengths && _binaries &&                                            \
        ( pIntercept->config().DumpInputProgramBinaries ||                  \
          pIntercept->config().DumpProgramBinaries ) )                      \
    {                                                                       \
        _hash = pIntercept->hashString(                                     \
            (const char*)_binaries[0],                                      \
            _lengths[0] );                                                  \
    }

#define DUMP_INPUT_PROGRAM_BINARIES( _program, _num, _devs, _lengths, _binaries, _hash )    \
    if( pIntercept->config().DumpInputProgramBinaries )                     \
    {                                                                       \
        pIntercept->dumpInputProgramBinaries(                               \
            _hash,                                                          \
            _program,                                                       \
            _num,                                                           \
            _devs,                                                          \
            _lengths,                                                       \
            _binaries );                                                    \
    }

// Called from clCreateProgramWithIL:

#define COMPUTE_SPIRV_HASH( _length, _il, _hash )                           \
    if( _length && _il && pIntercept->config().DumpProgramSPIRV )           \
    {                                                                       \
        _hash = pIntercept->hashString(                                     \
            (const char*)_il,                                               \
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
    if( ( injected == false ) &&                                            \
        pIntercept->config().DumpProgramSPIRV )                             \
    {                                                                       \
        pIntercept->dumpProgramSPIRV( hash, program, length, il );          \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        pIntercept->saveProgramNumber( program );                           \
    }

#define DELETE_INJECTED_SPIRV( _injectedSPIRV )                             \
    delete [] _injectedSPIRV;                                               \
    _injectedSPIRV = NULL;

// Called from clBuildProgram:

#define MODIFY_PROGRAM_OPTIONS( program, options, newOptions )              \
    bool    modified = false;                                               \
    if( pIntercept->config().InjectProgramSource )                          \
    {                                                                       \
        modified |= pIntercept->injectProgramOptions(                       \
            program,                                                        \
            options,                                                        \
            newOptions );                                                   \
    }                                                                       \
    if( !pIntercept->config().AppendBuildOptions.empty() )                  \
    {                                                                       \
        modified |= pIntercept->appendBuildOptions(                         \
            options,                                                        \
            newOptions );                                                   \
    }

#define DUMP_PROGRAM_OPTIONS( program, options )                            \
    if( ( modified == false ) &&                                            \
        ( pIntercept->config().DumpProgramSource ||                         \
          pIntercept->config().DumpProgramBinaries ||                       \
          pIntercept->config().DumpProgramSPIRV ) )                         \
    {                                                                       \
        pIntercept->dumpProgramOptions( program, options );                 \
    }                                                                       \
    else if( ( modified == false ) &&                                       \
             ( pIntercept->config().SimpleDumpProgramSource ||              \
               pIntercept->config().DumpProgramSourceScript ) )             \
    {                                                                       \
        pIntercept->dumpProgramOptionsScript( program, options );           \
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

#define INCREMENT_PROGRAM_COMPILE_COUNT( _program )                         \
    if( _program &&                                                         \
        ( pIntercept->config().BuildLogging ||                              \
          pIntercept->config().DevicePerformanceTimeHashTracking ||         \
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

#define DELETE_MODIFIED_OPTIONS( newOptions )                               \
    delete [] newOptions;                                                   \
    newOptions = NULL;

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
#define CPU_PERFORMANCE_TIMING_START()                                      \
    uint64_t    cpuStart = 0, cpuEnd = 0;                                   \
    if( pIntercept->config().HostPerformanceTiming ||                       \
        pIntercept->config().ChromeCallLogging )                            \
    {                                                                       \
        cpuStart = pIntercept->OS().GetTimer();                             \
    }

#define CPU_PERFORMANCE_TIMING_END()                                        \
    if( pIntercept->config().HostPerformanceTiming ||                       \
        pIntercept->config().ChromeCallLogging )                            \
    {                                                                       \
        cpuEnd = pIntercept->OS().GetTimer();                               \
        if( pIntercept->config().HostPerformanceTiming )                    \
        {                                                                   \
            pIntercept->updateHostTimingStats(                              \
                __FUNCTION__,                                               \
                NULL,                                                       \
                cpuStart,                                                   \
                cpuEnd );                                                   \
        }                                                                   \
        if( pIntercept->config().ChromeCallLogging )                        \
        {                                                                   \
            pIntercept->chromeCallLoggingExit(                              \
                __FUNCTION__,                                               \
                NULL,                                                       \
                cpuStart,                                                   \
                cpuEnd );                                                   \
        }                                                                   \
    }

#define CPU_PERFORMANCE_TIMING_END_KERNEL( _kernel )                        \
    if( pIntercept->config().HostPerformanceTiming ||                       \
        pIntercept->config().ChromeCallLogging )                            \
    {                                                                       \
        cpuEnd = pIntercept->OS().GetTimer();                               \
        if( pIntercept->config().HostPerformanceTiming )                    \
        {                                                                   \
            pIntercept->updateHostTimingStats(                              \
                __FUNCTION__,                                               \
                _kernel,                                                    \
                cpuStart,                                                   \
                cpuEnd );                                                   \
        }                                                                   \
        if( pIntercept->config().ChromeCallLogging )                        \
        {                                                                   \
            pIntercept->chromeCallLoggingExit(                              \
                __FUNCTION__,                                               \
                _kernel,                                                    \
                cpuStart,                                                   \
                cpuEnd );                                                   \
        }                                                                   \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define CREATE_COMMAND_QUEUE_OVERRIDE_INIT( _props, _newprops )             \
    if( pIntercept->config().DevicePerformanceTiming ||                     \
        pIntercept->config().ITTPerformanceTiming ||                        \
        pIntercept->config().ChromePerformanceTiming ||                     \
        pIntercept->config().SIMDSurvey ||                                  \
        !pIntercept->config().DevicePerfCounterCustom.empty() ||            \
        pIntercept->config().InOrderQueue )                                 \
    {                                                                       \
        pIntercept->createCommandQueueOverrideInit(                         \
            _props,                                                         \
            _newprops );                                                    \
    }

#define CREATE_COMMAND_QUEUE_OVERRIDE_CLEANUP( _queue, _newprops )          \
    if( pIntercept->config().DevicePerformanceTiming ||                     \
        pIntercept->config().ITTPerformanceTiming ||                        \
        pIntercept->config().ChromePerformanceTiming ||                     \
        pIntercept->config().SIMDSurvey ||                                  \
        !pIntercept->config().DevicePerfCounterCustom.empty() ||            \
        pIntercept->config().InOrderQueue )                                 \
    {                                                                       \
        pIntercept->createCommandQueueOverrideCleanup(                      \
            _newprops );                                                    \
    }

#define DEVICE_PERFORMANCE_TIMING_START( pEvent )                           \
    uint64_t    queuedTime = 0;                                             \
    cl_event    local_event = NULL;                                         \
    bool        retainAppEvent = true;                                      \
    if( pIntercept->config().DevicePerformanceTiming ||                     \
        pIntercept->config().ITTPerformanceTiming ||                        \
        pIntercept->config().ChromePerformanceTiming ||                     \
        pIntercept->config().SIMDSurvey ||                                  \
        !pIntercept->config().DevicePerfCounterCustom.empty() )             \
    {                                                                       \
        queuedTime = pIntercept->OS().GetTimer();                           \
        if( pEvent == NULL )                                                \
        {                                                                   \
            pEvent = &local_event;                                          \
            retainAppEvent = false;                                         \
        }                                                                   \
    }

#define DEVICE_PERFORMANCE_TIMING_END( pEvent )                             \
    if( ( pIntercept->config().DevicePerformanceTiming ||                   \
          pIntercept->config().ITTPerformanceTiming ||                      \
          pIntercept->config().ChromePerformanceTiming ||                   \
          pIntercept->config().SIMDSurvey ||                                \
          !pIntercept->config().DevicePerfCounterCustom.empty() ) &&        \
        ( pEvent != NULL ) )                                                \
    {                                                                       \
        if( pIntercept->config().DevicePerformanceTimingSkipUnmap &&        \
            std::string(__FUNCTION__) == "clEnqueueUnmapMemObject" )        \
        {                                                                   \
            if( retainAppEvent == false )                                   \
            {                                                               \
                pIntercept->dispatch().clReleaseEvent( pEvent[0] );         \
                pEvent = NULL;                                              \
            }                                                               \
        }                                                                   \
        else                                                                \
        {                                                                   \
            pIntercept->addTimingEvent(                                     \
                __FUNCTION__,                                               \
                queuedTime,                                                 \
                NULL,                                                       \
                0, NULL, NULL, NULL,                                        \
                pEvent[0] );                                                \
            if( retainAppEvent )                                            \
            {                                                               \
                pIntercept->dispatch().clRetainEvent( pEvent[0] );          \
            }                                                               \
            else                                                            \
            {                                                               \
                pEvent = NULL;                                              \
            }                                                               \
        }                                                                   \
    }

#define DEVICE_PERFORMANCE_TIMING_END_KERNEL( pEvent, kernel, wd, gwo, gws, lws )\
    if( ( pIntercept->config().DevicePerformanceTiming ||                   \
          pIntercept->config().ITTPerformanceTiming ||                      \
          pIntercept->config().ChromePerformanceTiming ||                   \
          pIntercept->config().SIMDSurvey ||                                \
          !pIntercept->config().DevicePerfCounterCustom.empty() ) &&        \
        ( pEvent != NULL ) )                                                \
    {                                                                       \
        pIntercept->addTimingEvent(                                         \
            __FUNCTION__,                                                   \
            queuedTime,                                                     \
            kernel,                                                         \
            wd, gwo, gws, lws,                                              \
            pEvent[0] );                                                    \
        if( retainAppEvent )                                                \
        {                                                                   \
            pIntercept->dispatch().clRetainEvent( pEvent[0] );              \
        }                                                                   \
        else                                                                \
        {                                                                   \
            pEvent = NULL;                                                  \
        }                                                                   \
    }

#define DEVICE_PERFORMANCE_TIMING_CHECK()                                   \
    if( pIntercept->config().DevicePerformanceTiming ||                     \
        pIntercept->config().ITTPerformanceTiming ||                        \
        pIntercept->config().ChromePerformanceTiming ||                     \
        pIntercept->config().SIMDSurvey ||                                  \
        !pIntercept->config().DevicePerfCounterCustom.empty() )             \
    {                                                                       \
        pIntercept->checkTimingEvents();                                    \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define SIMD_SURVEY_CREATE_PROGRAM_FROM_SOURCE( _program, _context, _count, _strings, _lengths )    \
    if( pIntercept->config().SIMDSurvey &&                                  \
        _program != NULL )                                                  \
    {                                                                       \
        pIntercept->SIMDSurveyCreateProgramFromSource(                      \
            _program,                                                       \
            _context,                                                       \
            _count,                                                         \
            _strings,                                                       \
            _lengths );                                                     \
    }

#define SIMD_SURVEY_BUILD_PROGRAM( _program, _numDevices, _deviceList, _options )   \
    if( pIntercept->config().SIMDSurvey &&                                  \
        _program != NULL )                                                  \
    {                                                                       \
        pIntercept->SIMDSurveyBuildProgram(                                 \
            _program,                                                       \
            _numDevices,                                                    \
            _deviceList,                                                    \
            _options );                                                     \
    }

#define SIMD_SURVEY_CREATE_KERNEL( _program, _kernel, _name )               \
    if( pIntercept->config().SIMDSurvey &&                                  \
        _kernel != NULL )                                                   \
    {                                                                       \
        pIntercept->SIMDSurveyCreateKernel(                                 \
            _program,                                                       \
            _kernel,                                                        \
            _name );                                                        \
    }

#define SIMD_SURVEY_SET_KERNEL_ARG( _kernel, _argIndex, _argSize, _argValue )   \
    if( pIntercept->config().SIMDSurvey )                                   \
    {                                                                       \
        pIntercept->SIMDSurveySetKernelArg(                                 \
            _kernel,                                                        \
            _argIndex,                                                      \
            _argSize,                                                       \
            _argValue );                                                    \
    }

#define SIMD_SURVEY_NDRANGE_KERNEL( _kernel )                               \
    if( pIntercept->config().SIMDSurvey )                                   \
    {                                                                       \
        pIntercept->SIMDSurveyNDRangeKernel( _kernel );                     \
    }

///////////////////////////////////////////////////////////////////////////////
//
#define CHECK_EVENT_LIST( _numEvents, _eventList )                          \
    if( pIntercept->config().EventChecking )                                \
    {                                                                       \
        pIntercept->checkEventList( __FUNCTION__, _numEvents, _eventList ); \
    }

///////////////////////////////////////////////////////////////////////////////
//
inline void CLIntercept::saveProgramNumber( const cl_program program )
{
    m_OS.EnterCriticalSection();

    m_ProgramNumberMap[ program ] = m_ProgramNumber;
    m_ProgramNumber++;

    m_OS.LeaveCriticalSection();
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

///////////////////////////////////////////////////////////////////////////////
//
#if defined(USE_ITT)

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
        "CL Row Pitch = %d, "
        "CL Slice Pitch = %d, "
        "CL Width = %d, "
        "CL Height = %d, "
        "CL Depth = %d, ",
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
