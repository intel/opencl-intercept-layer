/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include <algorithm>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdarg.h>
#include <sstream>
#include <time.h>       // strdate

#include "common.h"
#include "demangle.h"
#include "emulate.h"
#include "intercept.h"

/*****************************************************************************\

Inline Function:
    Hash

Description:
    Calculates hash from sequence of 32-bit values.

    Jenkins 96-bit mixing function with 32-bit feedback-loop and 64-bit state.

    All magic values are DWORDs of SHA2-256 mixing data:
    0x428a2f98 0x71374491 0xb5c0fbcf 0xe9b5dba5
    0x3956c25b 0x59f111f1 0x923f82a4 0xab1c5ed5

    From: http://www.burtleburtle.net/bob/c/lookup2.c

    lookup2.c, by Bob Jenkins, December 1996, Public Domain.
    hash(), hash2(), hash3, and mix() are externally useful functions.
    Routines to test the hash are included if SELF_TEST is defined.
    You can use this free for any purpose.  It has no warranty.

\*****************************************************************************/
#define HASH_JENKINS_MIX(a,b,c) \
{ \
    a -= b; a -= c; a ^= (c>>13); \
    b -= c; b -= a; b ^= (a<<8); \
    c -= a; c -= b; c ^= (b>>13); \
    a -= b; a -= c; a ^= (c>>12);  \
    b -= c; b -= a; b ^= (a<<16); \
    c -= a; c -= b; c ^= (b>>5); \
    a -= b; a -= c; a ^= (c>>3);  \
    b -= c; b -= a; b ^= (a<<10); \
    c -= a; c -= b; c ^= (b>>15); \
}
static inline uint64_t Hash(
    const void* ptr,
    size_t count )
{
    unsigned int    a = 0x428a2f98, hi = 0x71374491, lo = 0xb5c0fbcf;

    const uint32_t* dwData = reinterpret_cast<const uint32_t*>(ptr);
    size_t dwCount = count / sizeof(uint32_t);
    while( dwCount-- )
    {
        a ^= *(dwData++);
        HASH_JENKINS_MIX( a, hi, lo );
    }

    size_t extra = count % sizeof(uint32_t);
    if( extra != 0 )
    {
        uint32_t extraValue = 0;

        const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
        data += count - extra;

        for( size_t i = 0; i < extra; i++) {
            extraValue += *(data++) << (i * 8);
        }

        a ^= extraValue;
        HASH_JENKINS_MIX( a, hi, lo );
    }

    return (((uint64_t)hi)<<32)|lo;
}
#undef HASH_JENKINS_MIX

const char* CLIntercept::sc_URL = "https://github.com/intel/opencl-intercept-layer";
const char* CLIntercept::sc_DumpDirectoryName = "CLIntercept_Dump";
const char* CLIntercept::sc_ReportFileName = "clintercept_report.txt";
const char* CLIntercept::sc_LogFileName = "clintercept_log.txt";
const char* CLIntercept::sc_DumpPerfCountersFileNamePrefix = "clintercept_perfcounter";
const char* CLIntercept::sc_TraceFileName = "clintercept_trace.json";

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::Create( void* pGlobalData, CLIntercept*& pIntercept )
{
    bool    success = false;

    pIntercept = new CLIntercept( pGlobalData );
    if( pIntercept )
    {
        success = pIntercept->init();
        if( success == false )
        {
            Delete( pIntercept );
        }
    }

    return success;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::Delete( CLIntercept*& pIntercept )
{
    delete pIntercept;
    pIntercept = NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
CLIntercept::CLIntercept( void* pGlobalData )
    : m_OS( pGlobalData )
{
    m_ProcessId = m_OS.GetProcessID();

    m_Dispatch = {0};
    m_DispatchX[NULL] = {0};

    m_OpenCLLibraryHandle = NULL;

    m_LoggedCLInfo = false;

    m_EnqueueCounter.store(0, std::memory_order::memory_order_relaxed);

    m_EventsChromeTraced = 0;
    m_ProgramNumber = 0;
    m_KernelID = 0;

#if defined(USE_MDAPI)
    m_pMDHelper = NULL;
#endif

    m_QueueNumber = 0;
    m_MemAllocNumber = 0;

    m_AubCaptureStarted = false;
    m_AubCaptureKernelEnqueueSkipCounter = 0;
    m_AubCaptureKernelEnqueueCaptureCounter = 0;

#define CLI_CONTROL( _type, _name, _init, _desc )   m_Config . _name = _init;
#include "controls.h"
#undef CLI_CONTROL

#if defined(USE_ITT)
    m_ITTInitialized = false;

    m_ITTDomain = NULL;

    //m_ITTQueuedState = NULL;
    //m_ITTSubmittedState = NULL;
    //m_ITTExecutingState = NULL;

    //m_ITTQueueTrackGroup = NULL;
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
CLIntercept::~CLIntercept()
{
    stopAubCapture( NULL );
    report();

    std::lock_guard<std::mutex> lock(m_Mutex);

    log( "CLIntercept is shutting down...\n" );

    // Set the dispatch to the dummy dispatch.  The destructor is called
    // as the process is terminating.  We don't know when each DLL gets
    // unloaded, so it's not safe to call into any OpenCL functions in
    // our destructor.  Setting to the dummy dispatch ensures that no
    // OpenCL functions get called.  Note that this means we do potentially
    // leave some events, kernels, or programs un-released, but since
    // the process is terminating, that's probably OK.
    m_Dispatch = {0};

#if defined(USE_MDAPI)
    if( m_pMDHelper )
    {
        if( config().DevicePerfCounterTimeBasedSampling )
        {
            m_pMDHelper->CloseStream();
        }

        MetricsDiscovery::MDHelper::Delete( m_pMDHelper );
    }
#endif

    if( m_OpenCLLibraryHandle != NULL )
    {
        OS().UnloadLibrary( m_OpenCLLibraryHandle );
        m_OpenCLLibraryHandle = NULL;
    }

    {
        CContextCallbackInfoMap::iterator i = m_ContextCallbackInfoMap.begin();
        while( i != m_ContextCallbackInfoMap.end() )
        {
            SContextCallbackInfo* pContextCallbackInfo = (*i).second;

            if( pContextCallbackInfo )
            {
                delete pContextCallbackInfo;
            }

            (*i).second = NULL;
            ++i;
        }
    }

    {
        CPrecompiledKernelOverridesMap::iterator i = m_PrecompiledKernelOverridesMap.begin();
        while( i != m_PrecompiledKernelOverridesMap.end() )
        {
            SPrecompiledKernelOverrides* pOverrides = (*i).second;

            if( pOverrides )
            {
                // If we were able to release kernels or programs, we'd release
                // the override kernels and program here.

                delete pOverrides;
            }

            (*i).second = NULL;
            ++i;
        }
    }

    {
        CBuiltinKernelOverridesMap::iterator i = m_BuiltinKernelOverridesMap.begin();
        while( i != m_BuiltinKernelOverridesMap.end() )
        {
            SBuiltinKernelOverrides* pOverrides = (*i).second;

            if( pOverrides )
            {
                // If we were able to release kernels or programs, we'd release
                // the override kernels and program here.

                delete pOverrides;
            }

            (*i).second = NULL;
            ++i;
        }
    }

    m_ChromeTrace.flush();

    log( "... shutdown complete.\n" );
    m_InterceptLog.close();
}

///////////////////////////////////////////////////////////////////////////////
//
template <class T>
static bool GetControl(
    const OS::Services& OS,
    const char* name,
    T& value )
{
    unsigned int    readValue = 0;
    bool success = OS.GetControl( name, &readValue, sizeof(readValue) );
    if( success )
    {
        value = readValue;
    }

    return success;
}
template <>
bool GetControl<bool>(
    const OS::Services& OS,
    const char* name,
    bool& value )
{
    unsigned int    readValue = 0;
    bool success = OS.GetControl( name, &readValue, sizeof(readValue) );
    if( success )
    {
        value = ( readValue != 0 );
    }

    return success;
}
template <>
bool GetControl<std::string>(
    const OS::Services& OS,
    const char* name,
    std::string& value )
{
    char    readValue[256] = "";
    bool success = OS.GetControl( name, readValue, sizeof(readValue) );
    if( success )
    {
        value = readValue;
    }

    return success;
}

template<class T>
static std::string GetNonDefaultString(
    const char* name,
    const T& value )
{
    std::ostringstream ss;
    ss << std::boolalpha;
    ss << "Control " << name << " is set to non-default value: " << value << "\n";
    return ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::init()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if( m_OS.Init() == false )
    {
#ifdef __ANDROID__
         __android_log_print(ANDROID_LOG_INFO, "clIntercept", "OS.Init FAILED!\n" );
#endif
        return false;
    }

#if defined(_WIN32)
    OS::Services_Common::ENV_PREFIX = "CLI_";
    OS::Services_Common::REGISTRY_KEY = "SOFTWARE\\INTEL\\IGFX\\CLINTERCEPT";
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
    OS::Services_Common::ENV_PREFIX = "CLI_";
    OS::Services_Common::CONFIG_FILE = "clintercept.conf";
    OS::Services_Common::SYSTEM_DIR = "/etc/OpenCL";
#endif

    bool    breakOnLoad = false;
    GetControl( m_OS, "BreakOnLoad", breakOnLoad );

    if( breakOnLoad )
    {
        CLI_DEBUG_BREAK();
    }

    // A few control aliases, for backwards compatibility:
    GetControl( m_OS, "DevicePerformanceTimeHashTracking",m_Config.KernelNameHashTracking );
    GetControl( m_OS, "SimpleDumpProgram",                m_Config.SimpleDumpProgramSource );
    GetControl( m_OS, "DumpProgramsScript",               m_Config.DumpProgramSourceScript );
    GetControl( m_OS, "DumpProgramsInject",               m_Config.DumpProgramSource );
    GetControl( m_OS, "InjectPrograms",                   m_Config.InjectProgramSource );
    GetControl( m_OS, "LogDir",                           m_Config.DumpDir );

    std::string libName = "";
    GetControl( m_OS, "DllName", libName ); // alias
    GetControl( m_OS, "OpenCLFileName", libName );

#define CLI_CONTROL( _type, _name, _init, _desc ) GetControl( m_OS, #_name, m_Config . _name );
#include "controls.h"
#undef CLI_CONTROL

#if defined(_WIN32) || defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
    if( !m_Config.DumpDir.empty() )
    {
        std::replace( m_Config.DumpDir.begin(), m_Config.DumpDir.end(), '\\', '/' );
        OS::Services_Common::LOG_DIR = m_Config.DumpDir.c_str();
    }

    OS::Services_Common::APPEND_PID = m_Config.AppendPid;
#endif

    if( m_Config.LogToFile )
    {
        std::string fileName = "";

        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
        fileName += "/";
        fileName += sc_LogFileName;

        OS().MakeDumpDirectories( fileName );

        if( m_Config.AppendFiles )
        {
            m_InterceptLog.open(
                fileName.c_str(),
                std::ios::out | std::ios::binary | std::ios::app );
        }
        else
        {
            m_InterceptLog.open(
                fileName.c_str(),
                std::ios::out | std::ios::binary );
        }
    }

    if( m_Config.ChromeCallLogging ||
        m_Config.ChromePerformanceTiming )
    {
        std::string fileName = "";

        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
        fileName += "/";
        fileName += sc_TraceFileName;

        OS().MakeDumpDirectories( fileName );

        uint64_t    processId = OS().GetProcessID();
        uint32_t    bufferSize = m_Config.ChromeTraceBufferSize;
        bool        addFlowEvents = m_Config.ChromeFlowEvents;
        m_ChromeTrace.init( fileName, processId, bufferSize, addFlowEvents );

        uint64_t    threadId = OS().GetThreadID();
        std::string processName = OS().GetProcessName();
        m_ChromeTrace.addProcessMetadata( threadId, processName );
    }

    std::string name = "";
    OS().GetCLInterceptName( name );

    std::string bits =
        ( sizeof(void*) == 8 ) ? "64-bit" :
        ( sizeof(void*) == 4 ) ? "32-bit" :
        "XX-bit";

    log( "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n" );
    log( "CLIntercept (" + bits + ") is loading...\n" );
    log( "CLIntercept file location: " + name + "\n" );
    log( "CLIntercept URL: " + std::string(sc_URL) + "\n" );
#if defined(CLINTERCEPT_CMAKE)
    log( "CLIntercept git description: " + std::string(sc_GitDescribe) + "\n" );
    log( "CLIntercept git refspec: " + std::string(sc_GitRefSpec) + "\n" );
    log( "CLIntercept git hash: " + std::string(sc_GitHash) + "\n" );
#endif
    log( "CLIntercept optional features:\n"
// extra code only needed for Windows
#if defined(CLINTERCEPT_CLILOADER) || !defined(_WIN32)
        "    cliloader(supported)\n"
        "    cliprof(supported)\n"
#else
        "    cliloader(NOT supported)\n"
        "    cliprof(NOT supported)\n"
#endif
#if defined(USE_KERNEL_OVERRIDES)
        "    kernel overrides(supported)\n"
#else
        "    kernel overrides(NOT supported)\n"
#endif
#if defined(USE_ITT)
        "    ITT tracing(supported)\n"
#else
        "    ITT tracing(NOT supported)\n"
#endif
#if defined(USE_MDAPI)
        "    MDAPI(supported)\n"
#else
        "    MDAPI(NOT supported)\n"
#endif
#if defined(USE_DEMANGLE)
        "    Demangling(supported)\n"
#else
        "    Demangling(NOT supported)\n"
#endif
#if defined(CLINTERCEPT_HIGH_RESOLUTON_CLOCK)
        "    clock(high_resolution_clock)\n"
#else
        "    clock(steady_clock)\n"
#endif
    );
#if defined(_WIN32)
    log( "CLIntercept environment variable prefix: " + std::string( OS::Services_Common::ENV_PREFIX ) + "\n"  );
    log( "CLIntercept registry key: " + std::string( OS::Services_Common::REGISTRY_KEY ) + "\n" );
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
    log( "CLIntercept environment variable prefix: " + std::string( OS::Services_Common::ENV_PREFIX ) + "\n"  );
    log( "CLIntercept config file: " + std::string( OS::Services_Common::CONFIG_FILE ) + "\n" );
#endif

    // Windows and Linux load the real OpenCL library and retrieve
    // the OpenCL entry points from the real library dynamically.
#if defined(_WIN32) || defined(__linux__) || defined(__FreeBSD__)
    if( libName != "" )
    {
        log( "Read OpenCL file name from user parameters: " + libName + "\n" );
        log( "Trying to load dispatch from: " + libName + "\n" );

        if( initDispatch( libName ) )
        {
            log( "... success!\n" );
        }
    }
    else
    {
#if defined(_WIN32)

        char*   windir = NULL;
        size_t  length = 0;

        _dupenv_s( &windir, &length, "windir" );

        // Try some common DLL names.
        const std::string libNames[] =
        {
            "real_opencl.dll",
        #if defined(WIN32)
            std::string(windir) + "/syswow64/opencl.dll",
        #endif
            std::string(windir) + "/system32/opencl.dll",
        };

        free( windir );

#elif defined(__ANDROID__)

        const std::string libNames[] =
        {
            "/system/vendor/lib/real_libOpenCL.so",
            "real_libOpenCL.so",
        };

#elif defined(__linux__) || defined(__FreeBSD__)

        const std::string libNames[] =
        {
            "./real_libOpenCL.so",
#ifdef CLINTERCEPT_LIBRARY_ARCHITECTURE
            "/usr/lib/" CLINTERCEPT_LIBRARY_ARCHITECTURE "/libOpenCL.so.1",
            "/usr/lib/" CLINTERCEPT_LIBRARY_ARCHITECTURE "/libOpenCL.so",
#endif
            "/usr/lib/libOpenCL.so.1",
            "/usr/lib/libOpenCL.so",
            "/usr/local/lib/libOpenCL.so.1",
            "/usr/local/lib/libOpenCL.so",
            "/opt/intel/opencl/lib64/libOpenCL.so.1",
            "/opt/intel/opencl/lib64/libOpenCL.so",
            "/glob/development-tools/oneapi/inteloneapi/compiler/latest/linux/lib/libOpenCL.so.1",
            "/glob/development-tools/oneapi/inteloneapi/compiler/latest/linux/lib/libOpenCL.so",
        };

#else
#error Unknown OS!
#endif

        const int numNames = sizeof(libNames) / sizeof(libNames[0]);
        int i = 0;

        for( i = 0; i < numNames; i++ )
        {
            log( "Trying to load dispatch from: " + libNames[i] + "\n" );

            if( initDispatch( libNames[i] ) )
            {
                log( "... success!\n" );
                break;
            }
        }
    }
#elif defined(__APPLE__)
    if( initDispatch() )
    {
        log( "Dispatch table initialized.\n" );
    }
#else
#error Unknown OS!
#endif

#define CLI_CONTROL( _type, _name, _init, _desc )                   \
    if ( m_Config . _name != _init ) {                              \
        log( GetNonDefaultString( #_name, m_Config . _name ) );     \
    }
#include "controls.h"
#undef CLI_CONTROL

#if defined(USE_MDAPI)
    if( !m_Config.DevicePerfCounterCustom.empty() ||
        !m_Config.DevicePerfCounterFile.empty() )
    {
        if( !m_Config.DevicePerfCounterEventBasedSampling &&
            !m_Config.DevicePerfCounterTimeBasedSampling )
        {
            log("NOTE: Device Performance Counters are enabled without setting\n");
            log("    DevicePerfCounterEventBasedSampling or DevicePerfCounterTimeBasedSampling.\n");
            log("    Enabling DevicePerfCounterEventBasedSampling.  This behavior may be changed\n");
            log("    in a future version!\n");
            m_Config.DevicePerfCounterEventBasedSampling = true;
        }
        if( m_Config.DevicePerfCounterEventBasedSampling &&
            m_Config.DevicePerfCounterTimeBasedSampling )
        {
            log("NOTE: Both DevicePerfCounterEventBasedSampling and DevicePerfCounterTimeBasedSampling\n");
            log("    are enabled, but simultaneous collection of both types of counters is not\n");
            log("    currently supported.  Disabling DevicePerfCounterTimeBasedSampling.\n");
            m_Config.DevicePerfCounterTimeBasedSampling = false;
        }
        initCustomPerfCounters();
    }
#endif

    m_StartTime = clock::now();
    log( "Timer Started!\n" );

    if( m_Config.ChromeCallLogging ||
        m_Config.ChromePerformanceTiming )
    {
        uint64_t    threadId = OS().GetThreadID();

        using us = std::chrono::microseconds;
        uint64_t    usStartTime =
            std::chrono::duration_cast<us>(m_StartTime.time_since_epoch()).count();
        m_ChromeTrace.addStartTimeMetadata( threadId, usStartTime );
    }

    log( "... loading complete.\n" );

    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::report()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    char    filepath[MAX_PATH] = "";

#if defined(_WIN32)
    if( config().DumpProgramSourceScript )
    {
        char    dirname[MAX_PATH] = "";
        char    filename[MAX_PATH] = "";

        size_t  remaining = MAX_PATH;

        char    date[9] = "";
        char    time[9] = "";
        char*   curPos = NULL;
        char*   nextToken = NULL;
        char*   pch = NULL;

        // Directory:

        curPos = dirname;
        remaining = MAX_PATH;
        memset( curPos, 0, MAX_PATH );

        _strdate_s( date, 9 );
        _strtime_s( time, 9 );

        memcpy_s( curPos, remaining, "CLShaderDump_", 14 );
        curPos += 13;
        remaining -= 13;

        memcpy_s( curPos, remaining, strtok_s( date, "/", &nextToken ), 2 );
        curPos += 2;
        remaining -= 2;

        memcpy_s( curPos, remaining, strtok_s( NULL, "/", &nextToken ), 2 );
        curPos += 2;
        remaining -= 2;

        memcpy_s( curPos, remaining, strtok_s( NULL, "/", &nextToken ), 2 );
        curPos += 2;
        remaining -= 2;

        ::CreateDirectoryA( dirname, NULL );

        // File:

        curPos = filename;
        remaining = MAX_PATH;
        memset( curPos, 0, MAX_PATH );

        if( GetModuleFileNameA( NULL, filename, MAX_PATH-1 ) == 0 )
        {
            CLI_ASSERT( 0 );
            strcpy_s( curPos, remaining, "process.exe" );
        }

        pch = strrchr( filename, '\\' );
        pch++;
        memcpy_s( curPos, remaining, pch, strlen( pch ) );
        curPos += strlen( pch ) - 4;    // -4 to cut off ".exe"
        remaining -= strlen( pch ) - 4;

        memcpy_s( curPos, remaining, "_", 2 );
        curPos += 1;
        remaining -= 1;

        memcpy_s( curPos, remaining, strtok_s( time, ":", &nextToken ), 2 );
        curPos += 2;
        remaining -= 2;

        memcpy_s( curPos, remaining, strtok_s( NULL, ":", &nextToken ), 2 );
        curPos += 2;
        remaining -= 2;

        memcpy_s( curPos, remaining, strtok_s( NULL, ":", &nextToken ), 2 );
        curPos += 2;
        remaining -= 2;

        CLI_SPRINTF( curPos, remaining, "" );
        curPos += 1;
        remaining -= 1;

        CLI_SPRINTF( filepath, MAX_PATH, "%s/%s.%s", dirname, filename, "log" );
    }
    else
#endif
    {
        std::string fileName = "";

        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
        fileName += "/";
        fileName += sc_ReportFileName;

        OS().MakeDumpDirectories( fileName );

        CLI_SPRINTF( filepath, MAX_PATH, "%s", fileName.c_str() );
    }

    // Report

    if( m_Config.ReportToStderr )
    {
        writeReport( std::cerr );
    }

    if( m_Config.ReportToFile )
    {
        std::ofstream os;
        if( m_Config.AppendFiles )
        {
            os.open(
                filepath,
                std::ios::out | std::ios::binary | std::ios::app );
        }
        else
        {
            os.open(
                filepath,
                std::ios::out | std::ios::binary );
        }
        if( os.good() )
        {
            writeReport( os );
            os.close();
        }
        else
        {
            logf( "Failed to open report file for writing: %s\n", filepath );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::writeReport(
    std::ostream& os )
{
    if( config().FinishAfterEnqueue )
    {
        os << "*** WARNING *** FinishAfterEnqueue Enabled!" << std::endl << std::endl;
    }
    if( config().FlushAfterEnqueue )
    {
        os << "*** WARNING *** FlushAfterEnqueue Enabled!" << std::endl << std::endl;
    }
    if( config().NullEnqueue )
    {
        os << "*** WARNING *** NullEnqueue Enabled!" << std::endl << std::endl;
    }

    os << "Total Enqueues: " << m_EnqueueCounter.load(std::memory_order_relaxed) << std::endl << std::endl;

    if( config().LeakChecking )
    {
        os << std::endl << "Leak Checking:" << std::endl;
        m_ObjectTracker.writeReport( os );
    }

    if( !m_LongKernelNameMap.empty() )
    {
        os << std::endl << "Kernel name mapping:" << std::endl;

        os << std::endl
            << std::right << std::setw(10) << "Short Name" << ", "
            << std::right << std::setw(1) << "Long Name" << std::endl;

        CLongKernelNameMap::const_iterator i = m_LongKernelNameMap.begin();
        while( i != m_LongKernelNameMap.end() )
        {
            os << std::right << std::setw(10) << i->second << ", "
                << std::right << std::setw(1) << i->first << std::endl;

            ++i;
        }
    }

    if( config().HostPerformanceTiming &&
        !m_HostTimingStatsMap.empty() )
    {
        os << std::endl << "Host Performance Timing Results:" << std::endl;

        std::vector<std::string> keys;
        keys.reserve(m_HostTimingStatsMap.size());

        uint64_t    totalTotalNS = 0;
        size_t      longestName = 32;

        CHostTimingStatsMap::const_iterator i = m_HostTimingStatsMap.begin();
        while( i != m_HostTimingStatsMap.end() )
        {
            const std::string& name = (*i).first;
            const SHostTimingStats& hostTimingStats = (*i).second;

            if( !name.empty() )
            {
                keys.push_back(name);
                totalTotalNS += hostTimingStats.TotalNS;
                longestName = std::max< size_t >( name.length(), longestName );
            }

            ++i;
        }

        std::sort(keys.begin(), keys.end());

        os << std::endl << "Total Time (ns): " << totalTotalNS << std::endl;

        os << std::endl
            << std::right << std::setw(longestName) << "Function Name" << ", "
            << std::right << std::setw( 6) << "Calls" << ", "
            << std::right << std::setw(13) << "Time (ns)" << ", "
            << std::right << std::setw( 8) << "Time (%)" << ", "
            << std::right << std::setw(13) << "Average (ns)" << ", "
            << std::right << std::setw(13) << "Min (ns)" << ", "
            << std::right << std::setw(13) << "Max (ns)" << std::endl;

        for( const auto& name : keys )
        {
            const SHostTimingStats& hostTimingStats = m_HostTimingStatsMap.at(name);

            os << std::right << std::setw(longestName) << name << ", "
                << std::right << std::setw( 6) << hostTimingStats.NumberOfCalls << ", "
                << std::right << std::setw(13) << hostTimingStats.TotalNS << ", "
                << std::right << std::setw( 7) << std::fixed << std::setprecision(2) << hostTimingStats.TotalNS * 100.0f / totalTotalNS << "%, "
                << std::right << std::setw(13) << hostTimingStats.TotalNS / hostTimingStats.NumberOfCalls << ", "
                << std::right << std::setw(13) << hostTimingStats.MinNS << ", "
                << std::right << std::setw(13) << hostTimingStats.MaxNS << std::endl;
        }
    }

    if( config().DevicePerformanceTiming &&
        !m_DeviceTimingStatsMap.empty() )
    {
        CDeviceDeviceTimingStatsMap::const_iterator id = m_DeviceTimingStatsMap.begin();
        while( id != m_DeviceTimingStatsMap.end() )
        {
            const cl_device_id  device = (*id).first;
            const CDeviceTimingStatsMap& dtsm = (*id).second;

            const SDeviceInfo&  deviceInfo = m_DeviceInfoMap[device];

            os << std::endl << "Device Performance Timing Results for " << deviceInfo.NameForReport << ":" << std::endl;

            std::vector<std::string> keys;
            keys.reserve(dtsm.size());

            cl_ulong    totalTotalNS = 0;
            size_t      longestName = 32;

            CDeviceTimingStatsMap::const_iterator i = dtsm.begin();
            while( i != dtsm.end() )
            {
                const std::string& name = (*i).first;
                const SDeviceTimingStats& deviceTimingStats = (*i).second;

                if( !name.empty() )
                {
                    keys.push_back(name);
                    totalTotalNS += deviceTimingStats.TotalNS;
                    longestName = std::max< size_t >( name.length(), longestName );
                }

                ++i;
            }

            std::sort(keys.begin(), keys.end());

            os << std::endl << "Total Time (ns): " << totalTotalNS << std::endl;

            os << std::endl
                << std::right << std::setw(longestName) << "Function Name" << ", "
                << std::right << std::setw( 6) << "Calls" << ", "
                << std::right << std::setw(13) << "Time (ns)" << ", "
                << std::right << std::setw( 8) << "Time (%)" << ", "
                << std::right << std::setw(13) << "Average (ns)" << ", "
                << std::right << std::setw(13) << "Min (ns)" << ", "
                << std::right << std::setw(13) << "Max (ns)" << std::endl;

            for( const auto& name : keys )
            {
                const SDeviceTimingStats& deviceTimingStats = dtsm.at(name);

                os << std::right << std::setw(longestName) << name << ", "
                    << std::right << std::setw( 6) << deviceTimingStats.NumberOfCalls << ", "
                    << std::right << std::setw(13) << deviceTimingStats.TotalNS << ", "
                    << std::right << std::setw( 7) << std::fixed << std::setprecision(2) << deviceTimingStats.TotalNS * 100.0f / totalTotalNS << "%, "
                    << std::right << std::setw(13) << deviceTimingStats.TotalNS / deviceTimingStats.NumberOfCalls << ", "
                    << std::right << std::setw(13) << deviceTimingStats.MinNS << ", "
                    << std::right << std::setw(13) << deviceTimingStats.MaxNS << std::endl;
            }

            ++id;
        }
    }

#if defined(USE_MDAPI)
    if( config().DevicePerfCounterEventBasedSampling )
    {
        reportMDAPICounters( os );
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addShortKernelName(
    const std::string& kernelName )
{
    if( kernelName.length() > m_Config.LongKernelNameCutoff )
    {
        std::string shortKernelName("k_");
        shortKernelName += std::to_string(m_KernelID);
        m_LongKernelNameMap[ kernelName ] = shortKernelName;

        logf( "Added kernel name mapping: %s to %s\n",
            kernelName.c_str(),
            shortKernelName.c_str() );

        m_KernelID++;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getCallLoggingPrefix(
    std::string& str )
{
    if( m_Config.CallLoggingElapsedTime )
    {
        using us = std::chrono::microseconds;
        uint64_t usDelta =
            std::chrono::duration_cast<us>(clock::now() - m_StartTime).count();

        std::ostringstream  ss;

        ss << "Time: ";
        ss << usDelta;
        ss << " ";

        str += ss.str();
    }

    if( m_Config.CallLoggingThreadId ||
        m_Config.CallLoggingThreadNumber )
    {
        uint64_t    threadId = OS().GetThreadID();
        std::ostringstream  ss;

        if( m_Config.CallLoggingThreadId )
        {
            ss << "TID = ";
            ss << threadId;
            ss << " ";
        }
        if( m_Config.CallLoggingThreadNumber )
        {
            unsigned int    threadNumber = getThreadNumber( threadId );
            ss << "TNum = ";
            ss << threadNumber;
            ss << " ";
        }

        str += ss.str();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::callLoggingEnter(
    const char* functionName,
    const uint64_t enqueueCounter,
    const cl_kernel kernel )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    std::string str(">>>> ");
    getCallLoggingPrefix( str );

    str += functionName;

    if( kernel )
    {
        const std::string& kernelName = getShortKernelNameWithHash(kernel);
        str += "( ";
        str += kernelName;
        str += " )";
    }

    if( m_Config.CallLoggingEnqueueCounter )
    {
        std::ostringstream  ss;
        ss << "; EnqueueCounter: ";
        ss << enqueueCounter;
        str += ss.str();
    }

    str += "\n";

    log( str );
}
void CLIntercept::callLoggingEnter(
    const char* functionName,
    const uint64_t enqueueCounter,
    const cl_kernel kernel,
    const char* formatStr,
    ... )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    va_list args;
    va_start( args, formatStr );

    std::string str(">>>> ");
    getCallLoggingPrefix( str );

    str += functionName;

    if( kernel )
    {
        const std::string& kernelName = getShortKernelNameWithHash(kernel);
        str += "( ";
        str += kernelName;
        str += " )";
    }

    int size = CLI_VSPRINTF( m_StringBuffer, CLI_STRING_BUFFER_SIZE, formatStr, args );
    if( size >= 0 && size < CLI_STRING_BUFFER_SIZE )
    {
        str += ": ";
        str += m_StringBuffer;
    }
    else
    {
        str += ": too long";
    }

    if( m_Config.CallLoggingEnqueueCounter )
    {
        std::ostringstream  ss;
        ss << "; EnqueueCounter: ";
        ss << enqueueCounter;
        str += ss.str();
    }

    str += "\n";

    log( str );

    va_end( args );
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::callLoggingInfo(
    const std::string& str )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    log( "---- " + str + "\n" );
}

void CLIntercept::callLoggingInfo(
    const char* formatStr,
    ... )
{
    va_list args;
    va_start( args, formatStr );

    int size = CLI_VSPRINTF( m_StringBuffer, CLI_STRING_BUFFER_SIZE, formatStr, args );
    if( size >= 0 && size < CLI_STRING_BUFFER_SIZE )
    {
        callLoggingInfo( std::string( m_StringBuffer ) );
    }
    else
    {
        callLoggingInfo( std::string( "too long" ) );
    }

    va_end( args );
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::callLoggingExit(
    const char* functionName,
    const cl_int errorCode,
    const cl_event* event )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    std::string str("<<<< ");
    getCallLoggingPrefix( str );

    str += functionName;

    if( event )
    {
        CLI_SPRINTF( m_StringBuffer, CLI_STRING_BUFFER_SIZE, " created event = %p", *event );
        str += m_StringBuffer;
    }

    str += " -> ";
    str += m_EnumNameMap.name( errorCode );
    str += "\n";

    log( str );
}
void CLIntercept::callLoggingExit(
    const char* functionName,
    const cl_int errorCode,
    const cl_event* event,
    const char* formatStr,
    ... )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    va_list args;
    va_start( args, formatStr );

    std::string str;
    getCallLoggingPrefix( str );

    str += functionName;

    if( event )
    {
        CLI_SPRINTF( m_StringBuffer, CLI_STRING_BUFFER_SIZE, " created event = %p", *event );
        str += m_StringBuffer;
    }

    int size = CLI_VSPRINTF( m_StringBuffer, CLI_STRING_BUFFER_SIZE, formatStr, args );
    if( size >= 0 && size < CLI_STRING_BUFFER_SIZE )
    {
        str += ": ";
        str += m_StringBuffer;
    }
    else
    {
        str += ": too long";
    }

    str += " -> ";
    str += m_EnumNameMap.name( errorCode );

    log( "<<<< " + str + "\n" );

    va_end( args );
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::cacheDeviceInfo(
    cl_device_id device )
{
    if( device && m_DeviceInfoMap.find(device) == m_DeviceInfoMap.end() )
    {
        SDeviceInfo&    deviceInfo = m_DeviceInfoMap[device];

        CSubDeviceInfoMap::iterator iter = m_SubDeviceInfoMap.find( device );
        if( iter != m_SubDeviceInfoMap.end() )
        {
            deviceInfo.ParentDevice = iter->second.ParentDevice;
            deviceInfo.PlatformIndex = 0;
            deviceInfo.DeviceIndex = iter->second.SubDeviceIndex;
        }
        else
        {
            deviceInfo.ParentDevice = NULL;
            getDeviceIndex(
                device,
                deviceInfo.PlatformIndex,
                deviceInfo.DeviceIndex );
        }

        char*   deviceName = NULL;
        cl_uint deviceComputeUnits = 0;
        cl_uint deviceMaxClockFrequency = 0;

        allocateAndGetDeviceInfoString(
            device,
            CL_DEVICE_NAME,
            deviceName );
        dispatch().clGetDeviceInfo(
            device,
            CL_DEVICE_MAX_COMPUTE_UNITS,
            sizeof(deviceComputeUnits),
            &deviceComputeUnits,
            NULL );
        dispatch().clGetDeviceInfo(
            device,
            CL_DEVICE_MAX_CLOCK_FREQUENCY,
            sizeof(deviceMaxClockFrequency),
            &deviceMaxClockFrequency,
            NULL );
        dispatch().clGetDeviceInfo(
            device,
            CL_DEVICE_TYPE,
            sizeof(deviceInfo.Type),
            &deviceInfo.Type,
            NULL );
        if( deviceName )
        {
            std::ostringstream  ss;
            ss << deviceName << " ("
                << deviceComputeUnits << "CUs, "
                << deviceMaxClockFrequency << "MHz)";

            deviceInfo.Name = deviceName;
            deviceInfo.NameForReport = ss.str();
        }

        size_t majorVersion = 0;
        size_t minorVersion = 0;
        getDeviceMajorMinorVersion(
            device,
            majorVersion,
            minorVersion );
        deviceInfo.NumericVersion =
            CL_MAKE_VERSION_KHR( majorVersion, minorVersion, 0 );

        deviceInfo.NumComputeUnits = deviceComputeUnits;
        deviceInfo.MaxClockFrequency = deviceMaxClockFrequency;

        deviceInfo.HasDeviceAndHostTimer = false;
        deviceInfo.DeviceHostTimeDeltaNS = 0;

        // If the device numeric version is OpenCL 2.1 or newer and we have
        // the device and host timer APIs we might be able to use the device
        // and host timer.
        if( deviceInfo.NumericVersion >= CL_MAKE_VERSION_KHR(2, 1, 0) &&
            dispatch().clGetDeviceAndHostTimer &&
            dispatch().clGetHostTimer )
        {
            cl_ulong    deviceTimeNS = 0;
            cl_ulong    hostTimeNS = 0;
            cl_int  errorCode = dispatch().clGetDeviceAndHostTimer(
                device,
                &deviceTimeNS,
                &hostTimeNS);
            if( errorCode == CL_SUCCESS )
            {
                deviceInfo.HasDeviceAndHostTimer = true;
                deviceInfo.DeviceHostTimeDeltaNS = deviceTimeNS - hostTimeNS;
            }
        }

        deviceInfo.Supports_cl_khr_create_command_queue =
            checkDeviceForExtension( device, "cl_khr_create_command_queue" );
        deviceInfo.Supports_cl_khr_subgroups =
            checkDeviceForExtension( device, "cl_khr_subgroups" );

        delete [] deviceName;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getDeviceIndexString(
    cl_device_id device,
    std::string& str )
{
    cacheDeviceInfo( device );
    str = std::to_string(m_DeviceInfoMap[device].DeviceIndex);

    while( m_DeviceInfoMap[device].ParentDevice != NULL )
    {
        device = m_DeviceInfoMap[device].ParentDevice;
        cacheDeviceInfo( device );
        str = std::to_string(m_DeviceInfoMap[device].DeviceIndex) + '.' + str;
    }

    str = std::to_string(m_DeviceInfoMap[device].PlatformIndex) + '.' + str;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::getDeviceMajorMinorVersion(
    cl_device_id device,
    size_t& majorVersion,
    size_t& minorVersion ) const
{
    char*   deviceVersion = NULL;

    cl_int  errorCode = allocateAndGetDeviceInfoString(
        device,
        CL_DEVICE_VERSION,
        deviceVersion );
    if( errorCode == CL_SUCCESS && deviceVersion )
    {
        // According to the spec, the device version string should have the form:
        //   OpenCL <Major>.<Minor> <Vendor Specific Info>
        getMajorMinorVersionFromString(
            "OpenCL ",
            deviceVersion,
            majorVersion,
            minorVersion );
    }

    delete [] deviceVersion;
    deviceVersion = NULL;

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::getMajorMinorVersionFromString(
    const char* prefix,
    const char* str,
    size_t& major,
    size_t& minor ) const
{
    major = 0;
    minor = 0;

    if( str && prefix )
    {
        if( strncmp(str, prefix, strlen(prefix)) == 0 )
        {
            str += strlen(prefix);
            while( isdigit(str[0]) )
            {
                major *= 10;
                major += str[0] - '0';
                str++;
            }
            if( str[0] == '.' )
            {
                str++;
            }
            else
            {
                CLI_ASSERT( 0 );
            }
            while( isdigit(str[0]) )
            {
                minor *= 10;
                minor += str[0] - '0';
                str++;
            }
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::getDeviceIndex(
    cl_device_id device,
    cl_uint& platformIndex,
    cl_uint& deviceIndex ) const
{
    cl_platform_id  platform = getPlatform(device);

    cl_int  errorCode = CL_SUCCESS;
    bool    foundPlatform = false;
    bool    foundDevice = false;

    if( errorCode == CL_SUCCESS )
    {
        cl_uint numPlatforms = 0;
        errorCode = dispatch().clGetPlatformIDs(
            0,
            NULL,
            &numPlatforms );

        if( errorCode == CL_SUCCESS )
        {
            std::vector<cl_platform_id> platforms(numPlatforms);
            errorCode = dispatch().clGetPlatformIDs(
                numPlatforms,
                platforms.data(),
                NULL );
            if( errorCode == CL_SUCCESS )
            {
                auto it = std::find(platforms.begin(), platforms.end(), platform);
                if( it != platforms.end() )
                {
                    foundPlatform = true;
                    platformIndex = (cl_uint)std::distance(platforms.begin(), it);
                }
            }
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        cl_uint numDevices = 0;
        errorCode = dispatch().clGetDeviceIDs(
            platform,
            CL_DEVICE_TYPE_ALL,
            0,
            NULL,
            &numDevices );

        if( errorCode == CL_SUCCESS )
        {
            std::vector<cl_device_id>   devices(numDevices);
            errorCode = dispatch().clGetDeviceIDs(
                platform,
                CL_DEVICE_TYPE_ALL,
                numDevices,
                devices.data(),
                NULL );
            if( errorCode == CL_SUCCESS )
            {
                auto it = std::find(devices.begin(), devices.end(), device);
                if( it != devices.end() )
                {
                    foundDevice = true;
                    deviceIndex = (cl_uint)std::distance(devices.begin(), it);
                }
            }
        }
    }

    return foundPlatform && foundDevice;
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::checkDeviceForExtension(
    cl_device_id device,
    const char* extensionName ) const
{
    bool    supported = false;

    // Sanity check: Be sure the extension name is not NULL and doesn't
    // contain a space.
    if( !extensionName || strchr( extensionName, ' ' ) )
    {
        CLI_ASSERT( 0 );
    }
    else
    {
        char*   deviceExtensions = NULL;

        cl_int  errorCode = allocateAndGetDeviceInfoString(
            device,
            CL_DEVICE_EXTENSIONS,
            deviceExtensions );
        if( errorCode == CL_SUCCESS && deviceExtensions )
        {
            const char* start = deviceExtensions;
            while( true )
            {
                const char* where = strstr( start, extensionName );
                if( !where )
                {
                    break;
                }
                const char* terminator = where + strlen( extensionName );
                if( where == start || *(where - 1) == ' ' )
                {
                    if( *terminator == ' ' || *terminator == '\0' )
                    {
                        supported = true;
                        break;
                    }
                }
                start = terminator;
            }
        }

        delete [] deviceExtensions;
    }

    return supported;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::allocateAndGetPlatformInfoString(
    cl_platform_id platform,
    cl_platform_info param_name,
    char*& param_value ) const
{
    cl_int  errorCode = CL_SUCCESS;
    size_t  size = 0;

    if( errorCode == CL_SUCCESS )
    {
        if( param_value != NULL )
        {
            CLI_ASSERT( 0 );
            delete [] param_value;
            param_value = NULL;
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetPlatformInfo(
            platform,
            param_name,
            0,
            NULL,
            &size );
    }

    if( errorCode == CL_SUCCESS )
    {
        if( size != 0 )
        {
            param_value = new char[ size ];
            if( param_value == NULL )
            {
                errorCode = CL_OUT_OF_HOST_MEMORY;
            }
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetPlatformInfo(
            platform,
            param_name,
            size,
            param_value,
            NULL );
    }

    if( errorCode != CL_SUCCESS )
    {
        delete [] param_value;
        param_value = NULL;
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::allocateAndGetDeviceInfoString(
    cl_device_id device,
    cl_device_info param_name,
    char*& param_value ) const
{
    cl_int  errorCode = CL_SUCCESS;
    size_t  size = 0;

    if( errorCode == CL_SUCCESS )
    {
        if( param_value != NULL )
        {
            CLI_ASSERT( 0 );
            delete [] param_value;
            param_value = NULL;
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetDeviceInfo(
            device,
            param_name,
            0,
            NULL,
            &size );
    }

    if( errorCode == CL_SUCCESS )
    {
        if( size != 0 )
        {
            param_value = new char[ size ];
            if( param_value == NULL )
            {
                errorCode = CL_OUT_OF_HOST_MEMORY;
            }
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetDeviceInfo(
            device,
            param_name,
            size,
            param_value,
            NULL );
    }

    if( errorCode != CL_SUCCESS )
    {
        delete [] param_value;
        param_value = NULL;
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::allocateAndGetKernelInfoString(
    cl_kernel kernel,
    cl_kernel_info param_name,
    char*& param_value ) const
{
    cl_int  errorCode = CL_SUCCESS;
    size_t  size = 0;

    if( errorCode == CL_SUCCESS )
    {
        if( param_value != NULL )
        {
            CLI_ASSERT( 0 );
            delete [] param_value;
            param_value = NULL;
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetKernelInfo(
            kernel,
            param_name,
            0,
            NULL,
            &size );
    }

    if( errorCode == CL_SUCCESS )
    {
        if( size != 0 )
        {
            param_value = new char[ size ];
            if( param_value == NULL )
            {
                errorCode = CL_OUT_OF_HOST_MEMORY;
            }
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetKernelInfo(
            kernel,
            param_name,
            size,
            param_value,
            NULL );
    }

    if( errorCode != CL_SUCCESS )
    {
        delete [] param_value;
        param_value = NULL;
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::allocateAndGetProgramDeviceList(
    cl_program program,
    cl_uint& numDevices,
    cl_device_id*& deviceList ) const
{
    cl_int  errorCode = CL_SUCCESS;

    if( errorCode == CL_SUCCESS )
    {
        if( deviceList != NULL )
        {
            CLI_ASSERT( 0 );
            delete [] deviceList;
            deviceList = NULL;
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetProgramInfo(
            program,
            CL_PROGRAM_NUM_DEVICES,
            sizeof( numDevices ),
            &numDevices,
            NULL );
    }

    if( errorCode == CL_SUCCESS )
    {
        if( numDevices != 0 )
        {
            deviceList = new cl_device_id[ numDevices ];
            if( deviceList == NULL )
            {
                errorCode = CL_OUT_OF_HOST_MEMORY;
            }
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetProgramInfo(
            program,
            CL_PROGRAM_DEVICES,
            numDevices * sizeof( cl_device_id ),
            deviceList,
            NULL );
    }

    if( errorCode != CL_SUCCESS )
    {
        numDevices = 0;

        delete [] deviceList;
        deviceList = NULL;
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::allocateAndGetKernelISABinary(
    cl_kernel kernel,
    cl_device_id device,
    size_t& kernelISABinarySize,
    char*& kernelISABinary ) const
{
    cl_int  errorCode = CL_SUCCESS;

    if( errorCode == CL_SUCCESS )
    {
        if( kernelISABinary != NULL )
        {
            CLI_ASSERT( 0 );
            delete [] kernelISABinary;
            kernelISABinary = NULL;
        }
    }

    // Prefer to query for the kernel ISA binary using
    // clGetKernelWorkGroupInfo, which queries for a
    // specific device.
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetKernelWorkGroupInfo(
            kernel,
            device,
            CL_KERNEL_BINARY_PROGRAM_INTEL,
            0,
            NULL,
            &kernelISABinarySize );
        if( errorCode == CL_SUCCESS )
        {
            if( kernelISABinarySize != 0 )
            {
                kernelISABinary = new char[ kernelISABinarySize ];
            }
            if( kernelISABinary )
            {
                errorCode = dispatch().clGetKernelWorkGroupInfo(
                    kernel,
                    device,
                    CL_KERNEL_BINARY_PROGRAM_INTEL,
                    kernelISABinarySize,
                    kernelISABinary,
                    NULL );
                if( errorCode != CL_SUCCESS )
                {
                    delete [] kernelISABinary;
                    kernelISABinary = NULL;
                }
            }
        }
    }

    // If we weren't successful querying for the kernel ISA
    // binary using cletKernelWorkGroupInfo, try clGetKernelInfo,
    // which was supported by some earlier drivers but cannot query
    // for a specific device.
    if( errorCode != CL_SUCCESS )
    {
        errorCode = dispatch().clGetKernelInfo(
            kernel,
            CL_KERNEL_BINARY_PROGRAM_INTEL,
            0,
            NULL,
            &kernelISABinarySize );
        if( errorCode == CL_SUCCESS )
        {
            if( kernelISABinarySize != 0 )
            {
                kernelISABinary = new char[ kernelISABinarySize ];
            }
            if( kernelISABinary )
            {
                errorCode = dispatch().clGetKernelInfo(
                    kernel,
                    CL_KERNEL_BINARY_PROGRAM_INTEL,
                    kernelISABinarySize,
                    kernelISABinary,
                    NULL );
                if( errorCode != CL_SUCCESS )
                {
                    delete [] kernelISABinary;
                    kernelISABinary = NULL;
                }
            }
        }
    }

    if( errorCode != CL_SUCCESS )
    {
        kernelISABinarySize = 0;

        delete [] kernelISABinary;
        kernelISABinary = NULL;
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getPlatformInfoString(
    const cl_platform_id platform,
    std::string& str ) const
{
    str = "";

    cl_int  errorCode = CL_SUCCESS;

    char*   platformName = NULL;

    errorCode |= allocateAndGetPlatformInfoString(
        platform,
        CL_PLATFORM_NAME,
        platformName );

    if( errorCode != CL_SUCCESS )
    {
        CLI_ASSERT( 0 );
        str += "ERROR";
    }
    else
    {
        if( platformName )
        {
            str += platformName;
        }
        {
            char    s[256];
            CLI_SPRINTF( s, 256, " (%p)",
                platform );
            str += s;
        }
    }

    delete [] platformName;
    platformName = NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getDeviceInfoString(
    cl_uint numDevices,
    const cl_device_id* devices,
    std::string& str ) const
{
    str = "";

    for( cl_uint i = 0; i < numDevices; i++ )
    {
        cl_int  errorCode = CL_SUCCESS;

        cl_device_type  deviceType = CL_DEVICE_TYPE_DEFAULT;
        char*           deviceName = NULL;

        errorCode |= dispatch().clGetDeviceInfo(
            devices[i],
            CL_DEVICE_TYPE,
            sizeof( deviceType ),
            &deviceType,
            NULL );
        errorCode |= allocateAndGetDeviceInfoString(
            devices[i],
            CL_DEVICE_NAME,
            deviceName );

        if( errorCode != CL_SUCCESS )
        {
            CLI_ASSERT( 0 );
            str += "ERROR";
        }
        else
        {
            if( i != 0 )
            {
                str += ", ";
            }

            if( deviceName )
            {
                str += deviceName;
            }
            {
                char    s[256];
                CLI_SPRINTF( s, 256, " (%s) (%p)",
                    enumName().name_device_type( deviceType ).c_str(),
                    devices[i] );
                str += s;
            }
        }

        delete [] deviceName;
        deviceName = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getDevicePartitionPropertiesString(
    const cl_device_partition_property* properties,
    std::string& str ) const
{
    str = "";

    if( properties )
    {
        char    s[256];

        while( properties[0] != 0 )
        {
            cl_int  property = (cl_int)properties[0];
            str += enumName().name( property ) + " = ";

            switch( property )
            {
            case CL_DEVICE_PARTITION_EQUALLY:
            case CL_DEVICE_PARTITION_EQUALLY_EXT:
                {
                    auto pu = (const cl_uint*)( properties + 1 );
                    CLI_SPRINTF( s, 256, "%u", pu[0] );
                    str += s;

                    properties += 2;
                }
                break;
            case CL_DEVICE_PARTITION_BY_COUNTS:
            case CL_DEVICE_PARTITION_BY_COUNTS_EXT:
                {
                    ++properties;
                    str += "{ ";
                    do
                    {
                        if( *properties == CL_DEVICE_PARTITION_BY_COUNTS_LIST_END )
                        {
                            str += "CL_DEVICE_PARTITION_BY_COUNTS_LIST_END";
                        }
                        else
                        {
                            auto pu = (const cl_uint*)properties;
                            CLI_SPRINTF( s, 256, "%u, ", pu[0] );
                            str += s;
                        }
                    }
                    while( *properties++ != CL_DEVICE_PARTITION_BY_COUNTS_LIST_END );
                    str += " }";
                }
                break;
            case CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN:
                {
                    auto pd = (const cl_device_affinity_domain*)( properties + 1 );
                    str += enumName().name_device_affinity_domain(pd[0]);

                    properties += 2;
                }
                break;
            case CL_DEVICE_PARTITION_BY_NAMES_EXT:
                {
                    ++properties;
                    str += "{ ";
                    do
                    {
                        if( *properties == CL_PARTITION_BY_NAMES_LIST_END_EXT )
                        {
                            str += "CL_PARTITION_BY_NAMES_LIST_END_EXT";
                        }
                        else
                        {
                            auto pu = (const cl_uint*)properties;
                            CLI_SPRINTF( s, 256, "%u, ", pu[0] );
                            str += s;
                        }
                    }
                    while( *properties++ != CL_PARTITION_BY_NAMES_LIST_END_EXT );
                    str += " }";
                }
                break;
            case CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN_EXT:
                // The extension uses different enums than the OpenCL 1.2
                // feature, and we don't have an enum map for them (yet).
                {
                    CLI_SPRINTF( s, 256, "%04X", (cl_uint)properties[1] );
                    str += s;

                    properties += 2;
                }
                break;
            default:
                {
                    CLI_SPRINTF( s, 256, "<Unknown %08X!>", (cl_uint)property );
                    str += s;
                    // Advance by two properties.  This may not be correct,
                    // but it's the best we can do when the property is
                    // unknown.
                    properties += 2;
                }
                break;
            }

            if( properties[0] != 0 )
            {
                str += ", ";
            }
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getEventListString(
    cl_uint numEvents,
    const cl_event* eventList,
    std::string& str ) const
{
    {
        std::ostringstream  ss;
        ss << "( size = ";
        ss << numEvents;
        ss << " )[ ";
        str += ss.str();
    }
    if( eventList )
    {
        for( cl_uint i = 0; i < numEvents; i++ )
        {
            if( i > 0 )
            {
                str += ", ";
            }
            {
                char    s[256];
                CLI_SPRINTF( s, 256, "%p", eventList[i] );
                str += s;
            }
        }
    }
    str += " ]";
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getSemaphoreListString(
    cl_uint numSemaphores,
    const cl_semaphore_khr* semaphoreList,
    std::string& str ) const
{
    {
        std::ostringstream  ss;
        ss << "( size = ";
        ss << numSemaphores;
        ss << " )[ ";
        str += ss.str();
    }
    if( semaphoreList )
    {
        for( cl_uint i = 0; i < numSemaphores; i++ )
        {
            if( i > 0 )
            {
                str += ", ";
            }
            {
                char    s[256];
                CLI_SPRINTF( s, 256, "%p", semaphoreList[i] );
                str += s;
            }
        }
    }
    str += " ]";
}


///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getContextPropertiesString(
    const cl_context_properties* properties,
    std::string& str ) const
{
    str = "";

    if( properties )
    {
        char    s[256];

        while( properties[0] != 0 )
        {
            cl_int  property = (cl_int)properties[0];
            str += enumName().name( property ) + " = ";

            switch( property )
            {
            case CL_CONTEXT_PLATFORM:
                {
                    const cl_platform_id* pp = (const cl_platform_id*)( properties + 1 );
                    const cl_platform_id  platform = pp[0];
                    std::string platformInfo;
                    getPlatformInfoString( platform, platformInfo );
                    str += platformInfo;
                }
                break;
            case CL_GL_CONTEXT_KHR:
            case CL_EGL_DISPLAY_KHR:
            case CL_GLX_DISPLAY_KHR:
            case CL_WGL_HDC_KHR:
            case CL_CGL_SHAREGROUP_KHR:
#if defined(_WIN32)
            case CL_CONTEXT_D3D10_DEVICE_KHR:
            case CL_CONTEXT_D3D11_DEVICE_KHR:
            case CL_CONTEXT_ADAPTER_D3D9_KHR:
            case CL_CONTEXT_ADAPTER_D3D9EX_KHR:
            case CL_CONTEXT_ADAPTER_DXVA_KHR:
#endif
                {
                    const void** pp = (const void**)( properties + 1 );
                    const void*  value = pp[0];
                    CLI_SPRINTF( s, 256, "%p", value );
                    str += s;
                }
                break;
            case CL_CONTEXT_INTEROP_USER_SYNC:
            case CL_CONTEXT_TERMINATE_KHR:
                {
                    const cl_bool*  pb = (const cl_bool*)( properties + 1);
                    cl_bool value = pb[0];
                    str += enumName().name_bool( value );
                }
                break;
            case CL_CONTEXT_MEMORY_INITIALIZE_KHR:
                // TODO: this is a cl_context_memory_initialize_khr bitfield.
                // Fall through for now.
            default:
                {
                    CLI_SPRINTF( s, 256, "<Unknown %08X!>", (cl_uint)property );
                    str += s;
                }
                break;
            }

            properties += 2;
            if( properties[0] != 0 )
            {
                str += ", ";
            }
        }
    }
    else
    {
        str = "NULL";
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getSamplerPropertiesString(
    const cl_sampler_properties* properties,
    std::string& str ) const
{
    str = "";

    if( properties )
    {
        char    s[256];

        while( properties[0] != 0 )
        {
            cl_int  property = (cl_int)properties[0];
            str += enumName().name( property ) + " = ";

            switch( property )
            {
            case CL_SAMPLER_NORMALIZED_COORDS:
                {
                    const cl_bool*  pb = (const cl_bool*)( properties + 1);
                    cl_bool value = pb[0];
                    str += enumName().name_bool( value );
                }
                break;
            case CL_SAMPLER_ADDRESSING_MODE:
            case CL_SAMPLER_FILTER_MODE:
            case CL_SAMPLER_MIP_FILTER_MODE:
                {
                    const cl_int*   pi = (const cl_int*)( properties + 1);
                    cl_int  value = pi[0];
                    str += enumName().name( value );
                }
                break;
            case CL_SAMPLER_LOD_MIN:
            case CL_SAMPLER_LOD_MAX:
                {
                    const cl_float* pf = (const cl_float*)( properties + 1 );

                    cl_float    value = pf[0];

                    CLI_SPRINTF( s, 256, "%.2f", value );
                    str += s;
                }
                break;
            default:
                {
                    CLI_SPRINTF( s, 256, "<Unknown %08X!>", (cl_uint)property );
                    str += s;
                }
                break;
            }

            properties += 2;
            if( properties[0] != 0 )
            {
                str += ", ";
            }
        }
    }
    else
    {
        str = "NULL";
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getCommandQueuePropertiesString(
    const cl_queue_properties* properties,
    std::string& str ) const
{
    str = "";

    if( properties )
    {
        char    s[256];

        while( properties[0] != 0 )
        {
            cl_int  property = (cl_int)properties[0];
            str += enumName().name( property ) + " = ";

            switch( property )
            {
            case CL_QUEUE_PROPERTIES:
                {
                    const cl_command_queue_properties*  pp =
                        (const cl_command_queue_properties*)( properties + 1);
                    str += enumName().name_command_queue_properties( pp[0] );
                }
                break;
            case CL_QUEUE_SIZE:
            case CL_QUEUE_FAMILY_INTEL:
            case CL_QUEUE_INDEX_INTEL:
            case CL_QUEUE_MDAPI_PROPERTIES_INTEL:
            case CL_QUEUE_MDAPI_CONFIGURATION_INTEL:
                {
                    const cl_uint*  pu = (const cl_uint*)( properties + 1);
                    CLI_SPRINTF( s, 256, "%u", pu[0] );
                    str += s;
                }
                break;
            case CL_QUEUE_PRIORITY_KHR:
            case CL_QUEUE_THROTTLE_KHR:
                {
                    const cl_uint*  pu = (const cl_uint*)( properties + 1);
                    switch( pu[0] )
                    {
                    case CL_QUEUE_PRIORITY_HIGH_KHR: // and CL_QUEUE_THROTTLE_HIGH_KHR:
                        str += "HIGH";
                        break;
                    case CL_QUEUE_PRIORITY_MED_KHR: // and CL_QUEUE_THROTTLE_MED_KHR:
                        str += "MED";
                        break;
                    case CL_QUEUE_PRIORITY_LOW_KHR: // and CL_QUEUE_THROTTLE_LOW_KHR:
                        str += "LOW";
                        break;
                    default:
                        str += "<Unexpected!>";
                        break;
                    }
                }
                break;
            default:
                {
                    CLI_SPRINTF( s, 256, "<Unknown %08X!>", (cl_uint)property );
                    str += s;
                }
                break;
            }

            properties += 2;
            if( properties[0] != 0 )
            {
                str += ", ";
            }
        }
    }
    else
    {
        str = "NULL";
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getMemPropertiesString(
    const cl_mem_properties* properties,
    std::string& str ) const
{
    str = "";

    if( properties )
    {
        char    s[256];

        while( properties[0] != 0 )
        {
            cl_int  property = (cl_int)properties[0];
            str += enumName().name( property ) + " = ";

            switch( property )
            {
            case CL_DEVICE_HANDLE_LIST_KHR:
                {
                    ++properties;
                    str += "{ ";
                    while( true )
                    {
                        if( *properties == CL_DEVICE_HANDLE_LIST_END_KHR )
                        {
                            str += "CL_DEVICE_HANDLE_LIST_END_KHR";
                            properties++;
                            break;
                        }
                        else if( *properties == 0x2052 )
                        {
                            str += "CL_DEVICE_HANDLE_LIST_END_KHR_0x2502";
                            properties++;
                            break;
                        }
                        else
                        {
                            auto pDevice = (const cl_device_id*)properties++;
                            std::string deviceInfo;
                            getDeviceInfoString(
                                1,
                                pDevice,
                                deviceInfo );
                            str += deviceInfo;
                            str += ", ";
                        }
                    }
                    str += " }";
                }
                break;
            case CL_EXTERNAL_MEMORY_HANDLE_DMA_BUF_KHR:
            case CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_FD_KHR:
                {
                    auto pfd = (const int*)( properties + 1);
                    CLI_SPRINTF( s, 256, "%d", pfd[0] );
                    str += s;
                    properties += 2;
                }
                break;
            case CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KHR:
            case CL_EXTERNAL_MEMORY_HANDLE_OPAQUE_WIN32_KMT_KHR:
            case CL_EXTERNAL_MEMORY_HANDLE_D3D11_TEXTURE_KHR:
            case CL_EXTERNAL_MEMORY_HANDLE_D3D11_TEXTURE_KMT_KHR:
            case CL_EXTERNAL_MEMORY_HANDLE_D3D12_HEAP_KHR:
            case CL_EXTERNAL_MEMORY_HANDLE_D3D12_RESOURCE_KHR:
                {
                    auto pfd = (const void**)( properties + 1);
                    CLI_SPRINTF( s, 256, "%p", pfd[0] );
                    str += s;
                    properties += 2;
                }
                break;
            default:
                {
                    CLI_SPRINTF( s, 256, "<Unknown %08X!>", (cl_uint)property );
                    str += s;
                    // Advance by two properties.  This may not be correct,
                    // but it's the best we can do when the property is
                    // unknown.
                    properties += 2;
                }
                break;
            }

            if( properties[0] != 0 )
            {
                str += ", ";
            }
        }
    }
    else
    {
        str = "NULL";
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getSemaphorePropertiesString(
    const cl_semaphore_properties_khr* properties,
    std::string& str ) const
{
    str = "";

    if( properties )
    {
        char    s[256];

        while( properties[0] != 0 )
        {
            cl_int  property = (cl_int)properties[0];
            if( property == 0x2455 ) // workaround
            {
                str += "CL_SEMAPHORE_TYPE_KHR_0x2455 = ";
                property = CL_SEMAPHORE_TYPE_KHR;
            }
            else
            {
                str += enumName().name( property ) + " = ";
            }

            switch( property )
            {
            case CL_SEMAPHORE_TYPE_KHR:
                {
                    auto pt = (const cl_semaphore_type_khr*)( properties + 1 );
                    str += enumName().name_semaphore_type( pt[0] );
                    properties += 2;
                }
                break;
            case CL_DEVICE_HANDLE_LIST_KHR:
                {
                    ++properties;
                    str += "{ ";
                    while( true )
                    {
                        if( *properties == CL_DEVICE_HANDLE_LIST_END_KHR )
                        {
                            str += "CL_DEVICE_HANDLE_LIST_END_KHR";
                            properties++;
                            break;
                        }
                        else if( *properties == 0x2052 ) // workaround
                        {
                            str += "CL_DEVICE_HANDLE_LIST_END_KHR_0x2502";
                            properties++;
                            break;
                        }
                        else
                        {
                            auto pDevice = (const cl_device_id*)properties++;
                            std::string deviceInfo;
                            getDeviceInfoString(
                                1,
                                pDevice,
                                deviceInfo );
                            str += deviceInfo;
                            str += ", ";
                        }
                    }
                    str += " }";
                }
                break;
            case CL_SEMAPHORE_HANDLE_OPAQUE_FD_KHR:
            case CL_SEMAPHORE_HANDLE_SYNC_FD_KHR:
                {
                    auto pfd = (const int*)( properties + 1);
                    CLI_SPRINTF( s, 256, "%d", pfd[0] );
                    str += s;
                    properties += 2;
                }
                break;
            case CL_SEMAPHORE_HANDLE_D3D12_FENCE_KHR:
            case CL_SEMAPHORE_HANDLE_OPAQUE_WIN32_KHR:
            case CL_SEMAPHORE_HANDLE_OPAQUE_WIN32_KMT_KHR:
                {
                    auto pfd = (const void**)( properties + 1);
                    CLI_SPRINTF( s, 256, "%p", pfd[0] );
                    str += s;
                    properties += 2;
                }
                break;
            default:
                {
                    CLI_SPRINTF( s, 256, "<Unknown %08X!>", (cl_uint)property );
                    str += s;
                    // Advance by two properties.  This may not be correct,
                    // but it's the best we can do when the property is
                    // unknown.
                    properties += 2;
                }
                break;
            }

            if( properties[0] != 0 )
            {
                str += ", ";
            }
        }
    }
    else
    {
        str = "NULL";
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getCommandBufferPropertiesString(
    const cl_command_buffer_properties_khr* properties,
    std::string& str ) const
{
    str = "";

    if( properties )
    {
        char    s[256];

        while( properties[0] != 0 )
        {
            cl_int  property = (cl_int)properties[0];
            str += enumName().name( property ) + " = ";

            switch( property )
            {
            case CL_COMMAND_BUFFER_FLAGS_KHR:
                {
                    auto pt = (const cl_command_buffer_flags_khr*)( properties + 1 );
                    str += enumName().name_command_buffer_flags( pt[0] );
                    properties += 2;
                }
                break;
            default:
                {
                    CLI_SPRINTF( s, 256, "<Unknown %08X!>", (cl_uint)property );
                    str += s;
                    // Advance by two properties.  This may not be correct,
                    // but it's the best we can do when the property is
                    // unknown.
                    properties += 2;
                }
                break;
            }

            if( properties[0] != 0 )
            {
                str += ", ";
            }
        }
    }
    else
    {
        str = "NULL";
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getCommandBufferMutableConfigString(
    const cl_mutable_base_config_khr* mutable_config,
    std::string& str ) const
{
    str = "";

    if( mutable_config )
    {
        char s[256];
        CLI_SPRINTF(s, 256, "type = %s (%u), next = %p, num_mutable_dispatch = %u",
            enumName().name_command_buffer_structure_type(mutable_config->type).c_str(),
            mutable_config->type,
            mutable_config->next,
            mutable_config->num_mutable_dispatch);
        str += s;

        for( cl_uint i = 0; i < mutable_config->num_mutable_dispatch; i++ )
        {
            const cl_mutable_dispatch_config_khr* dispatchConfig =
                &mutable_config->mutable_dispatch_list[i];
            CLI_SPRINTF(s, 256, "\n  dispatch config %u: type = %s (%u), next = %p, command = %p:",
                i,
                enumName().name_command_buffer_structure_type(dispatchConfig->type).c_str(),
                dispatchConfig->type,
                dispatchConfig->next,
                dispatchConfig->command);
            str += s;
            if( dispatchConfig->type == CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR )
            {
                CLI_SPRINTF(s, 256, "\n    num_args = %u, num_svm_args = %u, num_exec_infos = %u, work_dim = %u",
                    dispatchConfig->num_args,
                    dispatchConfig->num_svm_args,
                    dispatchConfig->num_exec_infos,
                    dispatchConfig->work_dim);
                str += s;

                if( dispatchConfig->num_args != 0 &&
                    dispatchConfig->arg_list == nullptr )
                {
                        CLI_SPRINTF(s, 256, "\n      error: num_args is %u and arg_list is NULL!",
                            dispatchConfig->num_args);
                        str += s;
                }
                else
                {
                    for( cl_uint a = 0; a < dispatchConfig->num_args; a++ )
                    {
                        const cl_mutable_dispatch_arg_khr* arg =
                            &dispatchConfig->arg_list[a];
                        if( ( arg->arg_value != NULL ) &&
                            ( arg->arg_size == sizeof(cl_mem) ) )
                        {
                            cl_mem* pMem = (cl_mem*)arg->arg_value;
                            CLI_SPRINTF(s, 256, "\n      arg %u: arg_index = %u, arg_size = %zu, arg_value = %p",
                                a,
                                arg->arg_index,
                                arg->arg_size,
                                pMem[0] );
                        }
                        else if( ( arg->arg_value != NULL ) &&
                                 ( arg->arg_size == sizeof(cl_uint) ) )
                        {
                            cl_uint*    pData = (cl_uint*)arg->arg_value;
                            CLI_SPRINTF(s, 256, "\n      arg %u: arg_index = %u, arg_size = %zu, arg_value = 0x%x",
                                a,
                                arg->arg_index,
                                arg->arg_size,
                                pData[0]);
                        }
                        else if( ( arg->arg_value != NULL ) &&
                                 ( arg->arg_size == sizeof(cl_ulong) ) )
                        {
                            cl_ulong*   pData = (cl_ulong*)arg->arg_value;
                            CLI_SPRINTF(s, 256, "\n      arg %u: arg_index = %u, arg_size = %zu, arg_value = 0x%" PRIx64,
                                a,
                                arg->arg_index,
                                arg->arg_size,
                                pData[0]);
                        }
                        else
                        {
                            CLI_SPRINTF(s, 256, "\n      arg %u: arg_index = %u, arg_size = %zu",
                                a,
                                arg->arg_index,
                                arg->arg_size);
                        }

                        str += s;
                    }
                }

                if( dispatchConfig->num_svm_args != 0 &&
                    dispatchConfig->arg_svm_list == nullptr )
                {
                        CLI_SPRINTF(s, 256, "\n      error: num_svm_args is %u and arg_svm_list is NULL!",
                            dispatchConfig->num_svm_args);
                        str += s;
                }
                else
                {
                    for( cl_uint a = 0; a < dispatchConfig->num_svm_args; a++ )
                    {
                        const cl_mutable_dispatch_arg_khr* arg =
                            &dispatchConfig->arg_svm_list[a];
                        CLI_SPRINTF(s, 256, "\n      svm arg %u: arg_index = %u, arg_value = %p",
                            a,
                            arg->arg_index,
                            arg->arg_value);
                        str += s;
                    }
                }

                if( dispatchConfig->num_exec_infos != 0 &&
                    dispatchConfig->exec_info_list == nullptr )
                {
                        CLI_SPRINTF(s, 256, "\n      error: num_exec_infos is %u and exec_info_list is NULL!",
                            dispatchConfig->num_exec_infos);
                        str += s;
                }
                else
                {
                    for( cl_uint a = 0; a < dispatchConfig->num_exec_infos; a++ )
                    {
                        const cl_mutable_dispatch_exec_info_khr* info =
                            &dispatchConfig->exec_info_list[a];
                        CLI_SPRINTF(s, 256, "\n      exec info %u: param_name = %s (%04X), param_value_size = %zu, param_value = %p",
                            a,
                            enumName().name(info->param_name).c_str(),
                            info->param_name,
                            info->param_value_size,
                            info->param_value);
                        str += s;
                    }
                }

                if( dispatchConfig->global_work_offset != nullptr ||
                    dispatchConfig->global_work_size != nullptr ||
                    dispatchConfig->local_work_size != nullptr )
                {
                    cl_uint work_dim = dispatchConfig->work_dim;
                    if( work_dim == 0 )
                    {
                        // TODO: lock?
                        auto iter = m_MutableCommandInfoMap.find(
                            dispatchConfig->command);
                        if( iter != m_MutableCommandInfoMap.end() )
                        {
                            work_dim = iter->second.WorkDim;
                        }
                    }
                    if( work_dim != 0 )
                    {
                        std::string dispatchStr;
                        getEnqueueNDRangeKernelArgsString(
                            work_dim,
                            dispatchConfig->global_work_offset,
                            dispatchConfig->global_work_size,
                            dispatchConfig->local_work_size,
                            dispatchStr);
                        str += "\n      ";
                        str += dispatchStr;
                    }
                }
            }
        }
    }
    else
    {
        str = "NULL";
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getCreateKernelsInProgramRetString(
    cl_int retVal,
    cl_kernel* kernels,
    cl_uint* num_kernels_ret,
    std::string& str ) const
{
    if( kernels &&
        num_kernels_ret &&
        ( num_kernels_ret[0] != 0 ) )
    {
        char    s[256];

        cl_uint numKernels = num_kernels_ret[0];

        str += "kernels = [ ";
        for( cl_uint i = 0; i < numKernels; i++ )
        {
            CLI_SPRINTF( s, 256, "%p", kernels[i] );
            str += s;

            if( i < numKernels - 1 )
            {
                str += ", ";
            }
        }
        str += " ]";
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getKernelArgString(
    cl_uint arg_index,
    size_t arg_size,
    const void* arg_value,
    std::string& str ) const
{
    char    s[256];

    if( checkGetSamplerString(
            arg_size,
            arg_value,
            str ) )
    {
        CLI_SPRINTF( s, 256, "index = %u, size = %zu, value = %s\n",
            arg_index,
            arg_size,
            str.c_str() );
    }
    else if( ( arg_value != NULL ) &&
             ( arg_size == sizeof(cl_mem) ) )
    {
        cl_mem* pMem = (cl_mem*)arg_value;
        CLI_SPRINTF( s, 256, "index = %u, size = %zu, value = %p",
            arg_index,
            arg_size,
            pMem[0] );
    }
    else if( ( arg_value != NULL ) &&
             ( arg_size == sizeof(cl_uint) ) )
    {
        cl_uint*    pData = (cl_uint*)arg_value;
        CLI_SPRINTF( s, 256, "index = %u, size = %zu, value = 0x%x",
            arg_index,
            arg_size,
            pData[0] );
    }
    else if( ( arg_value != NULL ) &&
             ( arg_size == sizeof(cl_ulong) ) )
    {
        cl_ulong*   pData = (cl_ulong*)arg_value;
        CLI_SPRINTF( s, 256, "index = %u, size = %zu, value = 0x%" PRIx64,
            arg_index,
            arg_size,
            pData[0] );
    }
    else if( ( arg_value != NULL ) &&
             ( arg_size == sizeof(cl_int4) ) )
    {
        cl_int4*   pData = (cl_int4*)arg_value;
        CLI_SPRINTF( s, 256, "index = %u, size = %zu, valueX = 0x%0x, valueY = 0x%0x, valueZ = 0x%0x, valueW = 0x%0x",
            arg_index,
            arg_size,
            pData->s[0],
            pData->s[1],
            pData->s[2],
            pData->s[3]);
    }
    else
    {
        CLI_SPRINTF( s, 256, "index = %u, size = %zu",
            arg_index,
            arg_size );
    }

    str = s;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getEnqueueNDRangeKernelArgsString(
    cl_uint work_dim,
    const size_t* global_work_offset,
    const size_t* global_work_size,
    const size_t* local_work_size,
    std::string& str ) const
{
    std::ostringstream  ss;

    if( global_work_offset )
    {
        ss << "global_work_offset = < ";
        for( cl_uint i = 0; i < work_dim; i++ )
        {
            ss << global_work_offset[i];
            if( i < work_dim - 1 )
            {
                ss << ", ";
            }
        }
        ss << " >, ";
    }

    ss << "global_work_size = < ";
    if( global_work_size )
    {
        for( cl_uint i = 0; i < work_dim; i++ )
        {
            ss << global_work_size[i];
            if( i < work_dim - 1 )
            {
                ss << " x ";
            }
        }
    }
    else
    {
        ss << "NULL";
    }
    ss << " >, ";

    ss << "local_work_size = < ";
    if( local_work_size )
    {
        for( cl_uint i = 0; i < work_dim; i++ )
        {
            ss << local_work_size[i];
            if( i < work_dim - 1 )
            {
                ss << " x ";
            }
        }
    }
    else
    {
        ss << "NULL";
    }
    ss << " >";

    str = ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getCreateSubBufferArgsString(
    cl_buffer_create_type createType,
    const void *createInfo,
    std::string& str ) const
{
    std::ostringstream  ss;

    switch( createType )
    {
    case CL_BUFFER_CREATE_TYPE_REGION:
        ss << "region = ";
        if( createInfo != NULL )
        {
            cl_buffer_region*   pRegion = (cl_buffer_region*)createInfo;
            ss << "{ origin = "
                << pRegion->origin
                << ", size = "
                << pRegion->size
                << " }";
        }
        else
        {
            ss << "(NULL)";
        }
        break;
    default:
        ss << "<Unexpected!>";
        break;
    }

    str = ss.str();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::logCLInfo()
{
    if( m_LoggedCLInfo == false )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if( m_LoggedCLInfo == false )
        {
            m_LoggedCLInfo = true;

            cl_int  errorCode = CL_SUCCESS;
            cl_uint numPlatforms = 0;

            if( errorCode == CL_SUCCESS )
            {
                errorCode = dispatch().clGetPlatformIDs(
                    0,
                    NULL,
                    &numPlatforms );
            }

            if( errorCode == CL_SUCCESS && numPlatforms != 0 )
            {
                logf( "\nEnumerated %u platform%s.\n\n",
                    numPlatforms,
                    numPlatforms > 1 ? "s" : "" );

                cl_platform_id* platforms = new cl_platform_id[numPlatforms];
                if( platforms )
                {
                    errorCode = dispatch().clGetPlatformIDs(
                        numPlatforms,
                        platforms,
                        NULL );
                }
                else
                {
                    errorCode = CL_OUT_OF_HOST_MEMORY;
                }

                for( cl_uint p = 0; p < numPlatforms; p++ )
                {
                    if( errorCode == CL_SUCCESS )
                    {
                        logf( "Platform %u:\n", p );
                        logPlatformInfo( platforms[p] );
                    }

                    cl_uint numDevices = 0;

                    if( errorCode == CL_SUCCESS )
                    {
                        errorCode = dispatch().clGetDeviceIDs(
                            platforms[p],
                            CL_DEVICE_TYPE_ALL,
                            0,
                            NULL,
                            &numDevices );
                    }
                    if( errorCode == CL_SUCCESS && numDevices != 0 )
                    {
                        logf( "\tPlatform has %u device%s.\n\n",
                            numDevices,
                            numDevices > 1 ? "s" : "" );

                        cl_device_id*   devices = new cl_device_id[numDevices];
                        if( devices )
                        {
                            errorCode = dispatch().clGetDeviceIDs(
                                platforms[p],
                                CL_DEVICE_TYPE_ALL,
                                numDevices,
                                devices,
                                NULL );
                        }
                        else
                        {
                            errorCode = CL_OUT_OF_HOST_MEMORY;
                        }

                        for( cl_uint d = 0; d < numDevices; d++ )
                        {
                            if( errorCode == CL_SUCCESS )
                            {
                                logf( "Device %u:\n", d );
                                logDeviceInfo( devices[d] );
                                log( "\n" );
                            }
                        }

                        delete [] devices;
                    }
                }

                delete [] platforms;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::logBuild(
    clock::time_point buildTimeStart,
    const cl_program program,
    cl_uint numDevices,
    const cl_device_id* deviceList )
{
    std::chrono::duration<float, std::milli>    buildDuration =
        clock::now() - buildTimeStart;

    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_device_id*   localDeviceList = NULL;

    cl_int  errorCode = CL_SUCCESS;

    // There are two possibilities.  Either the device_list is NULL, in which
    // case we need to get the build log for all devices, or it's non-NULL,
    // in which case we only need to get the build log for all devices in
    // the device list.

    if( ( errorCode == CL_SUCCESS ) &&
        ( deviceList == NULL ) )
    {
        errorCode = allocateAndGetProgramDeviceList(
            program,
            numDevices,
            localDeviceList );
        if( errorCode == CL_SUCCESS )
        {
            deviceList = localDeviceList;
        }
    }

    if( m_Config.BuildLogging &&
        errorCode == CL_SUCCESS )
    {
        const SProgramInfo& programInfo = m_ProgramInfoMap[ program ];

        char    numberString[256] = "";
        if( config().OmitProgramNumber )
        {
            CLI_SPRINTF( numberString, 256, "%08X_%04u_%08X",
                (unsigned int)programInfo.ProgramHash,
                programInfo.CompileCount,
                (unsigned int)programInfo.OptionsHash );
        }
        else
        {
            CLI_SPRINTF( numberString, 256, "%04u_%08X_%04u_%08X",
                programInfo.ProgramNumber,
                (unsigned int)programInfo.ProgramHash,
                programInfo.CompileCount,
                (unsigned int)programInfo.OptionsHash );
        }

        logf( "Build Info for program %p (%s) for %u device(s):\n",
            program,
            numberString,
            numDevices );

        float   buildTimeMS = buildDuration.count();
        logf( "    Build finished in %.2f ms.\n", buildTimeMS );
    }

    if( errorCode == CL_SUCCESS )
    {
        for( cl_uint i = 0; i < numDevices; i++ )
        {
            if( m_Config.BuildLogging )
            {
                cl_build_status buildStatus = CL_BUILD_NONE;
                errorCode = dispatch().clGetProgramBuildInfo(
                    program,
                    deviceList[ i ],
                    CL_PROGRAM_BUILD_STATUS,
                    sizeof( buildStatus ),
                    &buildStatus,
                    NULL );

                if( errorCode == CL_SUCCESS )
                {
                    char*   deviceName = NULL;
                    char*   deviceOpenCLCVersion = NULL;
                    errorCode = allocateAndGetDeviceInfoString(
                        deviceList[i],
                        CL_DEVICE_NAME,
                        deviceName );
                    errorCode |= allocateAndGetDeviceInfoString(
                        deviceList[i],
                        CL_DEVICE_OPENCL_C_VERSION,
                        deviceOpenCLCVersion );

                    char    str[256] = "";

                    CLI_SPRINTF( str, 256, "Build Status for device %u = ", i );

                    std::string message = str;

                    if( errorCode == CL_SUCCESS )
                    {
                        message += deviceName;
                        message += " (";
                        message += deviceOpenCLCVersion;
                        message += "): ";
                    }

                    message += enumName().name_build_status( buildStatus );
                    message += "\n";

                    log( message );

                    delete [] deviceName;
                    deviceName = NULL;

                    delete [] deviceOpenCLCVersion;
                    deviceOpenCLCVersion = NULL;
                }
            }

            size_t  buildLogSize = 0;
            errorCode = dispatch().clGetProgramBuildInfo(
                program,
                deviceList[ i ],
                CL_PROGRAM_BUILD_LOG,
                0,
                NULL,
                &buildLogSize );

            if( errorCode == CL_SUCCESS )
            {
                char*   buildLog = new char[ buildLogSize + 1 ];
                if( buildLog )
                {
                    dispatch().clGetProgramBuildInfo(
                        program,
                        deviceList[ i ],
                        CL_PROGRAM_BUILD_LOG,
                        buildLogSize,
                        buildLog,
                        NULL );

                    // Check if the build log is already null-terminated.
                    // If it is, we're good, otherwise null terminate it.
                    if( buildLog[ buildLogSize - 1 ] == '\0' )
                    {
                        buildLogSize--;
                    }
                    else
                    {
                        buildLog[ buildLogSize ] = '\0';
                    }

                    if( m_Config.BuildLogging )
                    {
                        log( "-------> Start of Build Log:\n" );
                        log( std::string(buildLog) );
                        log( "<------- End of Build Log\n\n" );
                    }
                    if( m_Config.DumpProgramBuildLogs )
                    {
                        dumpProgramBuildLog(
                            program,
                            deviceList[ i ],
                            buildLog,
                            buildLogSize );
                    }

                    delete [] buildLog;
                }
            }
        }
    }

    delete [] localDeviceList;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::logError(
    const char* functionName,
    cl_int errorCode )
{
    std::ostringstream  ss;
    ss << "ERROR! " << functionName << " returned " << enumName().name(errorCode) << " (" << errorCode << ")\n";

    std::lock_guard<std::mutex> lock(m_Mutex);
    log( ss.str() );
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::logFlushOrFinishAfterEnqueueStart(
    const char* flushOrFinish,
    const char* functionName )
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    log( "Calling " + std::string(flushOrFinish) + " after " + std::string(functionName) + "...\n" );
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::logFlushOrFinishAfterEnqueueEnd(
    const char* flushOrFinish,
    const char* functionName,
    cl_int errorCode )
{
    std::ostringstream  ss;
    ss << "... " << flushOrFinish << " after " << functionName << " returned " << enumName().name( errorCode ) << " (" << errorCode << ")\n";

    std::lock_guard<std::mutex> lock(m_Mutex);
    log( ss.str() );
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::logKernelInfo(
    const cl_kernel* kernels,
    cl_uint numKernels )
{
    if( numKernels > 0 )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        cl_int  errorCode = CL_SUCCESS;

        // We can share the program and device list for all kernels.

        cl_kernel   queryKernel = kernels[0];

        // First, get the program for this kernel.
        cl_program  program = NULL;
        if( errorCode == CL_SUCCESS )
        {
            errorCode = dispatch().clGetKernelInfo(
                queryKernel,
                CL_KERNEL_PROGRAM,
                sizeof(program),
                &program,
                NULL );
        }

        // Next, get the list of devices for the program.
        cl_uint         numDevices = 0;
        cl_device_id*   deviceList = NULL;
        if( errorCode == CL_SUCCESS )
        {
            errorCode = allocateAndGetProgramDeviceList(
                program,
                numDevices,
                deviceList );
        }

        // Log the preferred work group size multiple for each kernel,
        // for each device.
        while( numDevices && numKernels-- )
        {
            cl_kernel   kernel = kernels[ numKernels ];

            const std::string& kernelName = getShortKernelNameWithHash(kernel);
            log( "Kernel Info for: " + kernelName + "\n" );

            for( cl_uint i = 0; i < numDevices; i++ )
            {
                char*   deviceName = NULL;
                errorCode = allocateAndGetDeviceInfoString(
                    deviceList[i],
                    CL_DEVICE_NAME,
                    deviceName );

                cl_uint args = 0;
                errorCode |= dispatch().clGetKernelInfo(
                    kernel,
                    CL_KERNEL_NUM_ARGS,
                    sizeof(args),
                    &args,
                    NULL );

                size_t  pwgsm = 0;
                errorCode |= dispatch().clGetKernelWorkGroupInfo(
                    kernel,
                    deviceList[i],
                    CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                    sizeof(pwgsm),
                    &pwgsm,
                    NULL );
                size_t  wgs = 0;
                errorCode |= dispatch().clGetKernelWorkGroupInfo(
                    kernel,
                    deviceList[i],
                    CL_KERNEL_WORK_GROUP_SIZE,
                    sizeof(wgs),
                    &wgs,
                    NULL );
                size_t  rwgs[3] = {0, 0, 0};
                errorCode |= dispatch().clGetKernelWorkGroupInfo(
                    kernel,
                    deviceList[i],
                    CL_KERNEL_COMPILE_WORK_GROUP_SIZE,
                    sizeof(rwgs),
                    rwgs,
                    NULL );
                cl_ulong pms = 0;
                errorCode |= dispatch().clGetKernelWorkGroupInfo(
                    kernel,
                    deviceList[i],
                    CL_KERNEL_PRIVATE_MEM_SIZE,
                    sizeof(pms),
                    &pms,
                    NULL );
                cl_ulong lms = 0;
                errorCode |= dispatch().clGetKernelWorkGroupInfo(
                    kernel,
                    deviceList[i],
                    CL_KERNEL_LOCAL_MEM_SIZE,
                    sizeof(lms),
                    &lms,
                    NULL );
                cl_ulong sms = 0;
                cl_int errorCode_sms = dispatch().clGetKernelWorkGroupInfo(
                    kernel,
                    deviceList[i],
                    CL_KERNEL_SPILL_MEM_SIZE_INTEL,
                    sizeof(sms),
                    &sms,
                    NULL );
                cl_uint regCount = 0;
                cl_int errorCode_regCount = dispatch().clGetKernelWorkGroupInfo(
                    kernel,
                    deviceList[i],
                    CL_KERNEL_REGISTER_COUNT_INTEL,
                    sizeof(regCount),
                    &regCount,
                    NULL );
                if( errorCode == CL_SUCCESS )
                {
                    logf( "    For device: %s\n",
                        deviceName );
                    if( config().KernelInfoLogging )
                    {
                        logf( "        Num Args: %u\n", args);
                    }
                    if( config().KernelInfoLogging ||
                        config().PreferredWorkGroupSizeMultipleLogging )
                    {
                        logf( "        Preferred Work Group Size Multiple: %zu\n", pwgsm);
                    }
                    if( config().KernelInfoLogging )
                    {
                        logf( "        Work Group Size: %zu\n", wgs);
                        if( rwgs[0] != 0 || rwgs[1] != 0 || rwgs[2] != 0 )
                        {
                            logf( "        Required Work Group Size: < %zu, %zu, %zu >\n",
                                rwgs[0], rwgs[1], rwgs[2]);
                        }
                        logf( "        Private Mem Size: %u\n", (cl_uint)pms);
                        logf( "        Local Mem Size: %u\n", (cl_uint)lms);
                        if( errorCode_sms == CL_SUCCESS )
                        {
                            logf( "        Spill Mem Size: %u\n", (cl_uint)sms);
                        }
                        if( errorCode_regCount == CL_SUCCESS )
                        {
                            logf( "        Register Count: %u\n", regCount);
                        }
                    }
                }
                else if( deviceName )
                {
                    logf( "Error querying kernel info for device %s!\n", deviceName );
                }
                else
                {
                    logf( "Error querying kernel info!\n" );
                }

                delete [] deviceName;
            }
        }

        delete [] deviceList;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::logQueueInfo(
    const cl_device_id device,
    const cl_command_queue queue )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_int  errorCode = CL_SUCCESS;

    logf( "Queue Info for %p:\n", queue );

    char*   deviceName = NULL;
    errorCode |= allocateAndGetDeviceInfoString(
        device,
        CL_DEVICE_NAME,
        deviceName );
    cl_command_queue_properties props = 0;
    errorCode |= dispatch().clGetCommandQueueInfo(
        queue,
        CL_QUEUE_PROPERTIES,
        sizeof(props),
        &props,
        NULL );
    if( errorCode == CL_SUCCESS )
    {
        logf( "    For device: %s\n", deviceName );
        logf( "    Queue properties: %s\n",
            props == 0 ?
            "(None)" :
            enumName().name_command_queue_properties(props).c_str() );
    }

    // Queue family information, may not be supported for all devices.
    cl_uint queueFamily = 0;
    cl_int errorCode_qf = dispatch().clGetCommandQueueInfo(
        queue,
        CL_QUEUE_FAMILY_INTEL,
        sizeof(queueFamily),
        &queueFamily,
        NULL );
    cl_uint queueIndex = 0;
    errorCode_qf |= dispatch().clGetCommandQueueInfo(
        queue,
        CL_QUEUE_INDEX_INTEL,
        sizeof(queueIndex),
        &queueIndex,
        NULL );
    if( errorCode_qf == CL_SUCCESS )
    {
        logf( "    Queue family: %u\n", queueFamily );
        logf( "    Queue index: %u\n", queueIndex );
    }

    delete [] deviceName;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::autoPartitionGetDeviceIDs(
    cl_platform_id platform,
    cl_device_type device_type,
    cl_uint num_entries,
    cl_device_id* devices,
    cl_uint* num_devices )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    CLI_ASSERT( config().AutoPartitionAllDevices ||
                config().AutoPartitionAllSubDevices ||
                config().AutoPartitionSingleSubDevice );

    cl_int  errorCode = CL_SUCCESS;
    std::vector<cl_device_id>   parentDevices;
    std::vector<cl_device_id>   returnedDevices;

    cl_uint numDevices = 0;
    errorCode = dispatch().clGetDeviceIDs(
        platform,
        device_type,
        0,
        NULL,
        &numDevices );
    if( errorCode == CL_SUCCESS && numDevices != 0 )
    {
        parentDevices.resize(numDevices);
        errorCode = dispatch().clGetDeviceIDs(
            platform,
            device_type,
            numDevices,
            parentDevices.data(),
            NULL );
    }

    for( auto parent : parentDevices )
    {
        std::vector<cl_device_id>&  subDevices = m_SubDeviceCacheMap[parent];

        if( subDevices.empty() )
        {
            std::string deviceInfo;
            getDeviceInfoString(
                1,
                &parent,
                deviceInfo );

            if( subDevices.size() == 0 &&
                config().AutoPartitionByAffinityDomain )
            {
                const cl_device_partition_property props[] = {
                    CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN,
                    CL_DEVICE_AFFINITY_DOMAIN_NEXT_PARTITIONABLE,
                    0
                };

                cl_uint numSubDevices = 0;
                dispatch().clCreateSubDevices(
                    parent,
                    props,
                    0,
                    NULL,
                    &numSubDevices );
                if( numSubDevices > 1 )
                {
                    logf("Partitioned device %s by affinity domain into %u sub-devices.\n",
                        deviceInfo.c_str(),
                        numSubDevices );

                    subDevices.resize(numSubDevices);
                    dispatch().clCreateSubDevices(
                        parent,
                        props,
                        numSubDevices,
                        subDevices.data(),
                        NULL );
                }
            }

            if( subDevices.size() == 0 &&
                config().AutoPartitionEqually )
            {
                const cl_device_partition_property props[] = {
                    CL_DEVICE_PARTITION_EQUALLY,
                    (cl_device_partition_property)config().AutoPartitionEqually,
                    0
                };

                cl_uint numSubDevices = 0;
                dispatch().clCreateSubDevices(
                    parent,
                    props,
                    0,
                    NULL,
                    &numSubDevices );
                if( numSubDevices > 1 )
                {
                    logf("Partitioned device %s equally into %u sub-devices with %u compute unit%s.\n",
                        deviceInfo.c_str(),
                        numSubDevices,
                        config().AutoPartitionEqually,
                        config().AutoPartitionEqually > 1 ? "s" : "" );

                    subDevices.resize(numSubDevices);
                    dispatch().clCreateSubDevices(
                        parent,
                        props,
                        numSubDevices,
                        subDevices.data(),
                        NULL );
                }
            }

            if( subDevices.size() == 0 )
            {
                logf("Couldn't partition device %s.\n",
                    deviceInfo.c_str() );
            }
        }

        if( subDevices.size() == 0 )
        {
            returnedDevices.push_back(parent);
        }
        else
        {
            if( config().AutoPartitionAllDevices )
            {
                returnedDevices.push_back(parent);
            }
            if( config().AutoPartitionAllDevices ||
                config().AutoPartitionAllSubDevices )
            {
                returnedDevices.insert(
                    returnedDevices.end(),
                    subDevices.begin(),
                    subDevices.end() );
            }
            else if( config().AutoPartitionSingleSubDevice )
            {
                returnedDevices.push_back( subDevices.front() );
            }
            else
            {
                CLI_ASSERT( 0 );
                returnedDevices.push_back( parent );
            }
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        for( cl_uint d = 0; d < returnedDevices.size(); d++ )
        {
            if( d < num_entries )
            {
                devices[d] = returnedDevices[d];
            }
        }

        if( num_devices )
        {
            num_devices[0] = (cl_uint)returnedDevices.size();
        }
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::contextCallbackCaller(
    const char* errinfo,
    const void* private_info,
    size_t cb,
    void* user_data )
{
    SContextCallbackInfo*   pContextCallbackInfo =
        (SContextCallbackInfo*)user_data;

    pContextCallbackInfo->pIntercept->contextCallback(
        errinfo,
        private_info,
        cb );
    if( pContextCallbackInfo->pApplicationCallback )
    {
        pContextCallbackInfo->pApplicationCallback(
            errinfo,
            private_info,
            cb,
            pContextCallbackInfo->pUserData );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::contextCallback(
    const std::string& errinfo,
    const void* private_info,
    size_t cb )
{
    char    str[256] = "";
    CLI_SPRINTF( str, 256, "=======> Context Callback (private_info = %p, cb = %zu):\n",
        private_info,
        cb );

    std::lock_guard<std::mutex> lock(m_Mutex);
    log( str + errinfo + "\n" + "<======= End of Context Callback\n" );
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::contextCallbackOverrideInit(
    const cl_context_properties* properties,
    void (CL_CALLBACK*& pCallback)( const char*, const void*, size_t, void* ),
    void*& pUserData,
    SContextCallbackInfo*& pContextCallbackInfo,
    cl_context_properties*& pLocalContextProperties )
{
    if( m_Config.ContextCallbackLogging )
    {
        pContextCallbackInfo = new SContextCallbackInfo;
        if( pContextCallbackInfo )
        {
            pContextCallbackInfo->pIntercept = this;
            pContextCallbackInfo->pApplicationCallback = pCallback;
            pContextCallbackInfo->pUserData = pUserData;

            pCallback = CLIntercept::contextCallbackCaller;
            pUserData = pContextCallbackInfo;
        }
    }

    if( m_Config.ContextHintLevel )
    {
        // We want to add a context hints to the context properties, unless
        // the context properties already requests performance hints
        // (requesting the same property twice is an error).  So, look through
        // the context properties for the performance hint enum.  We need to
        // do this anyways to count the number of property pairs.
        bool    foundPerformanceHintEnum = false;
        size_t  numProperties = 0;
        if( properties )
        {
            while( properties[ numProperties ] != 0 )
            {
                if( properties[ numProperties ] == CL_CONTEXT_SHOW_DIAGNOSTICS_INTEL )
                {
                    foundPerformanceHintEnum = true;
                }
                numProperties += 2;
            }
        }

        if( foundPerformanceHintEnum == false )
        {
            // The performance hint property isn't already set, so we'll
            // need to allocate an extra pair of properties for it.
            numProperties += 2;
        }

        // Allocate a new array of properties.  We need to allocate two
        // properties for each pair, plus one property for the terminating
        // zero.
        pLocalContextProperties = new cl_context_properties[ numProperties + 1 ];
        if( pLocalContextProperties )
        {
            // Copy the old properties array to the new properties array,
            // if the new properties array exists.
            numProperties = 0;
            if( properties )
            {
                while( properties[ numProperties ] != 0 )
                {
                    pLocalContextProperties[ numProperties ] = properties[ numProperties ];
                    if( properties[ numProperties ] == CL_CONTEXT_SHOW_DIAGNOSTICS_INTEL )
                    {
                        CLI_ASSERT( foundPerformanceHintEnum );
                        pLocalContextProperties[ numProperties + 1 ] = m_Config.ContextHintLevel;
                    }
                    else
                    {
                        pLocalContextProperties[ numProperties + 1 ] = properties[ numProperties + 1 ];
                    }
                    numProperties += 2;
                }
            }
            // Add the performance hint property if it wasn't already set.
            if( foundPerformanceHintEnum == false )
            {
                pLocalContextProperties[ numProperties ] = CL_CONTEXT_SHOW_DIAGNOSTICS_INTEL;
                pLocalContextProperties[ numProperties + 1 ] = m_Config.ContextHintLevel;
                numProperties += 2;
            }
            // Add the terminating zero.
            pLocalContextProperties[ numProperties ] = 0;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::contextCallbackOverrideCleanup(
    const cl_context context,
    SContextCallbackInfo*& pContextCallbackInfo,
    cl_context_properties*& pLocalContextProperties )
{
    if( context && pContextCallbackInfo )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        // Check if we already have a context callback info for this context.  If
        // we do, free it.
        SContextCallbackInfo* pOldContextCallbackInfo =
            m_ContextCallbackInfoMap[ context ];
        if( pOldContextCallbackInfo )
        {
            delete pOldContextCallbackInfo;
            pOldContextCallbackInfo = NULL;
        }

        m_ContextCallbackInfoMap[ context ]  = pContextCallbackInfo;
    }
    else
    {
        delete pContextCallbackInfo;
        pContextCallbackInfo = NULL;
    }

    if( pLocalContextProperties )
    {
        delete pLocalContextProperties;
        pLocalContextProperties = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::eventCallbackCaller(
    cl_event event,
    cl_int status,
    void* user_data )
{
    SEventCallbackInfo* pEventCallbackInfo =
        (SEventCallbackInfo*)user_data;

    CLIntercept*    pIntercept = pEventCallbackInfo->pIntercept;

    GET_ENQUEUE_COUNTER();
    CALL_LOGGING_ENTER( "event = %p, status = %s (%d)",
        event,
        pIntercept->enumName().name_command_exec_status( status ).c_str(),
        status );

    clock::time_point   cpuStart = clock::now();

    pIntercept->eventCallback(
        event,
        status );
    if( pEventCallbackInfo->pApplicationCallback )
    {
        pEventCallbackInfo->pApplicationCallback(
            event,
            status,
            pEventCallbackInfo->pUserData );
    }

    clock::time_point   cpuEnd = clock::now();

    CALL_LOGGING_EXIT( CL_SUCCESS );

    delete pEventCallbackInfo;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::eventCallback(
    cl_event event,
    cl_int status )
{
    // TODO: Since we call log the eventCallbackCaller, do we need to do
    //       anything here?
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::overrideNullLocalWorkSize(
    const cl_uint work_dim,
    const size_t* global_work_size,
    const size_t*& local_work_size )
{
    if( local_work_size == NULL )
    {
        switch( work_dim )
        {
        case 1:
            if( m_Config.NullLocalWorkSizeX != 0 )
            {
                if( global_work_size[0] % m_Config.NullLocalWorkSizeX == 0 )
                {
                    local_work_size = &m_Config.NullLocalWorkSizeX;
                }
                else
                {
                    std::lock_guard<std::mutex> lock(m_Mutex);
                    logf( "Couldn't override NULL local work size: < %zu > %% < %zu > != 0!\n",
                        global_work_size[0],
                        m_Config.NullLocalWorkSizeX );
                }
            }
            break;
        case 2:
            if( ( m_Config.NullLocalWorkSizeX != 0 ) &&
                ( m_Config.NullLocalWorkSizeY != 0 ) )
            {
                if( ( global_work_size[0] % m_Config.NullLocalWorkSizeX == 0 ) &&
                    ( global_work_size[1] % m_Config.NullLocalWorkSizeY == 0 ) )
                {
                    local_work_size = &m_Config.NullLocalWorkSizeX;
                }
                else
                {
                    std::lock_guard<std::mutex> lock(m_Mutex);
                    logf( "Couldn't override NULL local work size: < %zu x %zu > %% < %zu x %zu > != 0!\n",
                        global_work_size[0],
                        global_work_size[1],
                        m_Config.NullLocalWorkSizeX,
                        m_Config.NullLocalWorkSizeY );
                }
            }
            break;
        case 3:
            if( ( m_Config.NullLocalWorkSizeX != 0 ) &&
                ( m_Config.NullLocalWorkSizeY != 0 ) &&
                ( m_Config.NullLocalWorkSizeZ != 0 ) )
            {
                if( ( global_work_size[0] % m_Config.NullLocalWorkSizeX == 0 ) &&
                    ( global_work_size[1] % m_Config.NullLocalWorkSizeY == 0 ) &&
                    ( global_work_size[2] % m_Config.NullLocalWorkSizeZ == 0 ) )
                {
                    local_work_size = &m_Config.NullLocalWorkSizeX;
                }
                else
                {
                    std::lock_guard<std::mutex> lock(m_Mutex);
                    logf( "Couldn't override NULL local work size: < %zu x %zu x %zu > %% < %zu x %zu x %zu > != 0!\n",
                        global_work_size[0],
                        global_work_size[1],
                        global_work_size[2],
                        m_Config.NullLocalWorkSizeX,
                        m_Config.NullLocalWorkSizeY,
                        m_Config.NullLocalWorkSizeZ );
                }
            }
            break;
        default:
            // Nothing.
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::combineProgramStrings(
    cl_uint& count,
    const char**& strings,
    const size_t*& lengths,
    char*& singleString ) const
{
    size_t  allocSize = 0;
    cl_uint i = 0;

    for( i = 0; i < count; i++ )
    {
        size_t  length = 0;
        if( ( lengths == NULL ) ||
            ( lengths[i] == 0 ) )
        {
            if( strings != NULL && strings[i] != NULL )
            {
                length = strlen( strings[i] );
            }
        }
        else
        {
            length = lengths[i];
        }
        allocSize += length;
    }

    // Allocate some extra to make sure we're null terminated.
    allocSize++;

    singleString = new char[ allocSize ];
    if( singleString )
    {
        memset( singleString, 0, allocSize );

        char*   pDst = singleString;
        size_t  remaining = allocSize;
        for( i = 0; i < count; i++ )
        {
            size_t  length = 0;
            if( ( lengths == NULL ) ||
                ( lengths[i] == 0 ) )
            {
                if( strings != NULL && strings[i] != NULL )
                {
                    length = strlen( strings[i] );
                }
            }
            else
            {
                length = lengths[i];
            }
            if( length )
            {
                CLI_MEMCPY(
                    pDst,
                    remaining,
                    strings[i],
                    length );
                pDst += length;
                remaining -= length;
            }
        }

        // Replace any NULL chars between kernels with spaces.
        if( count > 1 )
        {
            for( char* pStr = singleString; pStr < pDst - 1; pStr++ )
            {
                if( *pStr == 0x0 )
                {
                    *pStr = 0x20;
                }
            }
        }

        count = 1;
        strings = ( const char** )&singleString;
        lengths = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::incrementProgramCompileCount(
    const cl_program program )
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ProgramInfoMap[ program ].CompileCount++;
}

///////////////////////////////////////////////////////////////////////////////
//
uint64_t CLIntercept::computeHash(
    const void* ptr,
    size_t length )
{
    uint64_t    hash = 0;

    if( ptr != NULL )
    {
        hash = Hash( ptr, length );
    }

    return hash;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::saveProgramHash(
    const cl_program program,
    uint64_t hash )
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if( program != NULL )
    {
        m_ProgramInfoMap[ program ].ProgramHash = hash;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::saveProgramOptionsHash(
    const cl_program program,
    const char* options )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if( program != NULL && options != NULL )
    {
        uint64_t hash = computeHash(
            options,
            strlen( options ) );

        m_ProgramInfoMap[ program ].OptionsHash = hash;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::injectProgramSource(
    const uint64_t hash,
    cl_uint& count,
    const char**& strings,
    const size_t*& lengths,
    char*& singleString )
{
    // We don't expect to get here unless we've combined the app's string(s)
    // into a single string and computed a hash from it.
    CLI_ASSERT( singleString );

    std::lock_guard<std::mutex> lock(m_Mutex);

    bool    injected = false;

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryNameWithoutPid( sc_DumpDirectoryName, fileName );
        fileName += "/Inject";
    }

    // Make two candidate filenames.  They will have the form:
    //   CLI_<program number>_<hash>_source.cl, or
    //   CLI_<hash>_source.cl
    {
        char    numberString1[256] = "";
        CLI_SPRINTF( numberString1, 256, "%04u_%08X",
            m_ProgramNumber,
            (unsigned int)hash );

        char    numberString2[256] = "";
        CLI_SPRINTF( numberString2, 256, "%08X",
            (unsigned int)hash );

        std::string fileName1;
        fileName1 = fileName;
        fileName1 += "/CLI_";
        fileName1 += numberString1;
        fileName1 += "_source.cl";

        std::string fileName2;
        fileName2 = fileName;
        fileName2 += "/CLI_";
        fileName2 += numberString2;
        fileName2 += "_source.cl";

        std::ifstream is;

        is.open(
            fileName1.c_str(),
            std::ios::in | std::ios::binary );
        if( is.good() )
        {
            log( "Injecting source file: " + fileName1 + "\n" );
        }
        else
        {
            log( "Injection source file doesn't exist: " + fileName1 + "\n" );

            is.clear();
            is.open(
                fileName2.c_str(),
                std::ios::in | std::ios::binary );
            if( is.good() )
            {
                log( "Injecting source file: " + fileName2 + "\n" );
            }
            else
            {
                log( "Injection source file doesn't exist: " + fileName2 + "\n" );
            }
        }

        if( is.good() )
        {
            // The file exists.  Figure out how big it is.
            size_t  filesize = 0;

            is.seekg(0, std::ios::end);
            filesize = (size_t)is.tellg();
            is.seekg(0, std::ios::beg);

            char*   newSingleString = new char[ filesize + 1 ];
            if( newSingleString )
            {
                memset( newSingleString, 0, filesize + 1 );

                is.read( newSingleString, filesize );

                delete [] singleString;

                singleString = newSingleString;
                count = 1;
                strings = ( const char** )&singleString;
                lengths = NULL;

                injected = true;
            }

            is.close();
        }
    }

    return injected;
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::prependProgramSource(
    const uint64_t hash,
    cl_uint& count,
    const char**& strings,
    const size_t*& lengths,
    char*& singleString )
{
    // We don't expect to get here unless we've combined the app's string(s)
    // into a single string and computed a hash from it.
    CLI_ASSERT( singleString );

    std::lock_guard<std::mutex> lock(m_Mutex);

    bool    injected = false;

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryNameWithoutPid( sc_DumpDirectoryName, fileName );
        fileName += "/Inject";
    }

    // Make three candidate filenames.  They will have the form:
    //   CLI_<program number>_<hash>_prepend.cl, or
    //   CLI_<hash>_prepend.cl, or
    //   CLI_prepend.cl
    {
        char    numberString1[256] = "";
        CLI_SPRINTF( numberString1, 256, "%04u_%08X",
            m_ProgramNumber,
            (unsigned int)hash );

        char    numberString2[256] = "";
        CLI_SPRINTF( numberString2, 256, "%08X",
            (unsigned int)hash );

        std::string fileName1;
        fileName1 = fileName;
        fileName1 += "/CLI_";
        fileName1 += numberString1;
        fileName1 += "_prepend.cl";

        std::string fileName2;
        fileName2 = fileName;
        fileName2 += "/CLI_";
        fileName2 += numberString2;
        fileName2 += "_prepend.cl";

        std::string fileName3;
        fileName3 = fileName;
        fileName3 += "/CLI_prepend.cl";

        std::ifstream is;

        is.open(
            fileName1.c_str(),
            std::ios::in | std::ios::binary );
        if( is.good() )
        {
            log( "Prepending source file: " + fileName1 + "\n" );
        }
        else
        {
            log( "Prepend source file doesn't exist: " + fileName1 + "\n" );

            is.clear();
            is.open(
                fileName2.c_str(),
                std::ios::in | std::ios::binary );
            if( is.good() )
            {
                log( "Prepending source file: " + fileName2 + "\n" );
            }
            else
            {
                log( "Prepend source file doesn't exist: " + fileName2 + "\n" );

                is.clear();
                is.open(
                    fileName3.c_str(),
                    std::ios::in | std::ios::binary );
                if( is.good() )
                {
                    log( "Prepending source file: " + fileName3 + "\n" );
                }
                else
                {
                    log( "Prepend source file doesn't exist: " + fileName3 + "\n" );
                }
            }
        }

        if( is.good() )
        {
            // The file exists.  Figure out how big it is.
            size_t  filesize = 0;

            is.seekg(0, std::ios::end);
            filesize = (size_t)is.tellg();
            is.seekg(0, std::ios::beg);

            size_t  newSize =
                filesize +
                strlen(singleString) +
                1;  // for the null terminator

            char*   newSingleString = new char[ newSize ];
            if( newSingleString )
            {
                memset( newSingleString, 0, newSize );

                is.read( newSingleString, filesize );

                CLI_STRCAT( newSingleString, newSize, singleString );

                delete [] singleString;

                singleString = newSingleString;
                count = 1;
                strings = ( const char** )&singleString;
                lengths = NULL;

                injected = true;
            }

            is.close();
        }
    }

    return injected;
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::injectProgramSPIRV(
    const uint64_t hash,
    size_t& length,
    const void*& il,
    char*& injectedIL )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    bool    injected = false;

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryNameWithoutPid( sc_DumpDirectoryName, fileName );
        fileName += "/Inject";
    }

    // Make two candidate filenames.  They will have the form:
    //   CLI_<program number>_<hash>_0000.spv, or
    //   CLI_<hash>_0000.spv
    {
        char    numberString1[256] = "";
        CLI_SPRINTF( numberString1, 256, "%04u_%08X_0000",
            m_ProgramNumber,
            (unsigned int)hash );

        char    numberString2[256] = "";
        CLI_SPRINTF( numberString2, 256, "%08X_0000",
            (unsigned int)hash );

        std::string fileName1;
        fileName1 = fileName;
        fileName1 += "/CLI_";
        fileName1 += numberString1;
        fileName1 += ".spv";

        std::string fileName2;
        fileName2 = fileName;
        fileName2 += "/CLI_";
        fileName2 += numberString2;
        fileName2 += ".spv";

        std::ifstream is;

        is.open(
            fileName1.c_str(),
            std::ios::in | std::ios::binary );
        if( is.good() )
        {
            log( "Injecting SPIR-V file: " + fileName1 + "\n" );
        }
        else
        {
            log( "Injection SPIR-V file doesn't exist: " + fileName1 + "\n" );

            is.clear();
            is.open(
                fileName2.c_str(),
                std::ios::in | std::ios::binary );
            if( is.good() )
            {
                log( "Injecting SPIR-V file: " + fileName2 + "\n" );
            }
            else
            {
                log( "Injection SPIR-V file doesn't exist: " + fileName2 + "\n" );
            }
        }

        if( is.good() )
        {
            // The file exists.  Figure out how big it is.
            size_t  filesize = 0;

            is.seekg(0, std::ios::end);
            filesize = (size_t)is.tellg();
            is.seekg(0, std::ios::beg);

            injectedIL = new char[ filesize ];
            if( injectedIL )
            {
                is.read( injectedIL, filesize );

                il = injectedIL;
                length = filesize;

                injected = true;
            }

            is.close();
        }
    }

    return injected;
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::injectProgramOptions(
    const cl_program program,
    cl_bool isCompile,
    cl_bool isLink,
    char*& newOptions )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    CLI_ASSERT( newOptions == NULL );

    bool    injected = false;

    const SProgramInfo& programInfo = m_ProgramInfoMap[ program ];

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryNameWithoutPid( sc_DumpDirectoryName, fileName );
        fileName += "/Inject";
    }
    // Make four candidate filenames.  They will have the form:
    //   CLI_<program number>_<program hash>_<compile count>_<options hash>_options.txt, or
    //   CLI_<program hash>_<compile count>_<options hash>_options.txt, or
    //   CLI_<program hash>_options.txt, or
    //   CLI_options.txt
    {
        char    numberString1[256] = "";
        CLI_SPRINTF( numberString1, 256, "%04u_%08X_%04u_%08X",
            programInfo.ProgramNumber,
            (unsigned int)programInfo.ProgramHash,
            programInfo.CompileCount,
            (unsigned int)programInfo.OptionsHash );

        char    numberString2[256] = "";
        CLI_SPRINTF( numberString2, 256, "%08X_%04u_%08X",
            (unsigned int)programInfo.ProgramHash,
            programInfo.CompileCount,
            (unsigned int)programInfo.OptionsHash );

        char    numberString3[256] = "";
        CLI_SPRINTF( numberString3, 256, "%08X",
            (unsigned int)programInfo.ProgramHash );

        const std::string suffix =
            isCompile ? "_compile_options.txt" :
            isLink ? "_link_options.txt" :
            "_options.txt";

        std::string fileName1;
        fileName1 = fileName;
        fileName1 += "/CLI_";
        fileName1 += numberString1;
        fileName1 += suffix;

        std::string fileName2;
        fileName2 = fileName;
        fileName2 += "/CLI_";
        fileName2 += numberString2;
        fileName2 += suffix;

        std::string fileName3;
        fileName3 = fileName;
        fileName3 += "/CLI_";
        fileName3 += numberString3;
        fileName3 += suffix;

        std::string fileName4;
        fileName4 = fileName;
        fileName4 += suffix;

        std::ifstream is;

        is.open(
            fileName1.c_str(),
            std::ios::in | std::ios::binary );
        if( is.good() )
        {
            log( "Injecting options file: " + fileName1 + "\n" );
        }
        else
        {
            log( "Injection options file doesn't exist: " + fileName1 + "\n" );

            is.clear();
            is.open(
                fileName2.c_str(),
                std::ios::in | std::ios::binary );
            if( is.good() )
            {
                log( "Injecting options file: " + fileName2 + "\n" );
            }
            else
            {
                log( "Injection options file doesn't exist: " + fileName2 + "\n" );

                is.clear();
                is.open(
                    fileName3.c_str(),
                    std::ios::in | std::ios::binary );
                if( is.good() )
                {
                    log( "Injecting options file: " + fileName3 + "\n" );
                }
                else
                {
                    log( "Injection options file doesn't exist: " + fileName3 + "\n" );

                    is.clear();
                    is.open(
                        fileName4.c_str(),
                        std::ios::in | std::ios::binary );
                    if( is.good() )
                    {
                        log( "Injecting options file: " + fileName4 + "\n" );
                    }
                    else
                    {
                        log( "Injection options file doesn't exist: " + fileName4 + "\n" );
                    }
                }
            }
        }

        if( is.good() )
        {
            // The file exists.  Figure out how big it is.
            size_t  filesize = 0;

            is.seekg(0, std::ios::end);
            filesize = (size_t)is.tellg();
            is.seekg(0, std::ios::beg);

            newOptions = new char[ filesize + 1 ];
            if( newOptions )
            {
                memset( newOptions, 0, filesize + 1 );

                is.read( newOptions, filesize );

                // Replace any newline characters with spaces.
                // Some editors insert newline characters, especially at the
                // end of the file, which can cause build options to become
                // invalid.
                for( size_t i = 0; i < filesize; i++ )
                {
                    if( newOptions[i] == '\n' )
                    {
                        newOptions[i] = ' ';
                    }
                }

                injected = true;
            }

            is.close();
        }
    }

    return injected;
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::appendBuildOptions(
    const char* append,
    const char* options,
    char*& newOptions ) const
{
    bool    modified = false;

    size_t  newSize = strlen(append) + 1;    // for the null terminator

    const char* oldOptions = newOptions ? newOptions : options;
    if( oldOptions )
    {
        newSize += strlen(oldOptions) + 1;  // for a space
    }

    char* newNewOptions = new char[ newSize ];
    if( newNewOptions )
    {
        memset( newNewOptions, 0, newSize );

        if( oldOptions )
        {
            CLI_STRCAT( newNewOptions, newSize, oldOptions );
            CLI_STRCAT( newNewOptions, newSize, " " );
        }
        CLI_STRCAT( newNewOptions, newSize, append );

        delete [] newOptions;
        newOptions = newNewOptions;

        modified = true;
    }

    return modified;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpProgramSourceScript(
    cl_program program,
    const char* singleString )
{
#if defined(_WIN32)

    std::lock_guard<std::mutex> lock(m_Mutex);

    CLI_ASSERT( config().DumpProgramSourceScript || config().SimpleDumpProgramSource );

    char    dirname[MAX_PATH] = "";
    char    filename[MAX_PATH] = "";
    char    filepath[MAX_PATH] = "";

    if( config().DumpProgramSourceScript )
    {
        size_t  remaining = MAX_PATH;

        char    date[9] = "";
        char    time[9] = "";
        char*   curPos = NULL;
        char*   nextToken = NULL;
        char*   pch = NULL;

        // Directory:

        curPos = dirname;
        remaining = MAX_PATH;
        memset( curPos, 0, MAX_PATH );

        _strdate_s( date, 9 );
        _strtime_s( time, 9 );

        memcpy_s( curPos, remaining, "CLShaderDump_", 14 );
        curPos += 13;
        remaining -= 13;

        memcpy_s( curPos, remaining, strtok_s( date, "/", &nextToken ), 2 );
        curPos += 2;
        remaining -= 2;

        memcpy_s( curPos, remaining, strtok_s( NULL, "/", &nextToken ), 2 );
        curPos += 2;
        remaining -= 2;

        memcpy_s( curPos, remaining, strtok_s( NULL, "/", &nextToken ), 2 );
        curPos += 2;
        remaining -= 2;

        ::CreateDirectoryA( dirname, NULL );

        // File:

        curPos = filename;
        remaining = MAX_PATH;
        memset( curPos, 0, MAX_PATH );

        if( GetModuleFileNameA( NULL, filename, MAX_PATH-1 ) == 0 )
        {
            CLI_ASSERT( 0 );
            strcpy_s( curPos, remaining, "process.exe" );
        }

        pch = strrchr( filename, '\\' );
        pch++;
        memcpy_s( curPos, remaining, pch, strlen( pch ) );
        curPos += strlen( pch ) - 4;    // -4 to cut off ".exe"
        remaining -= strlen( pch ) - 4;

        memcpy_s( curPos, remaining, "_", 2 );
        curPos += 1;
        remaining -= 1;

        memcpy_s( curPos, remaining, strtok_s( time, ":", &nextToken ), 2 );
        curPos += 2;
        remaining -= 2;

        memcpy_s( curPos, remaining, strtok_s( NULL, ":", &nextToken ), 2 );
        curPos += 2;
        remaining -= 2;

        memcpy_s( curPos, remaining, strtok_s( NULL, ":", &nextToken ), 2 );
        curPos += 2;
        remaining -= 2;

        CLI_SPRINTF( curPos, remaining, "_%8.8x", m_ProgramNumber );
        curPos += 9;
        remaining -= 9;
    }
    else
    {
        CLI_SPRINTF( dirname, MAX_PATH, "." );
        CLI_SPRINTF( filename, MAX_PATH, "kernel" );
    }

    CLI_SPRINTF( filepath, MAX_PATH, "%s/%s.%s", dirname, filename, "cl" );

    if( singleString )
    {
        std::ofstream os;
        os.open(
            filepath,
            std::ios::out | std::ios::binary );
        if( os.good() )
        {
            os.write( singleString, strlen( singleString ) );
            os.close();
        }
        else
        {
            logf( "Failed to open program source dump file for writing: %s\n",
                filepath );
        }
    }

    SProgramInfo&   programInfo = m_ProgramInfoMap[ program ];
    programInfo.ProgramNumber = m_ProgramNumber;
    programInfo.CompileCount = 0;

    m_ProgramNumber++;

#else
    CLI_ASSERT( 0 );
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpProgramSource(
    cl_program program,
    uint64_t hash,
    bool modified,
    const char* singleString )
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    CLI_ASSERT( config().DumpProgramSource || config().AutoCreateSPIRV );

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
    }
    if( modified )
    {
        fileName += "/Modified";
    }

    // Make the filename.  It will have the form:
    //   CLI_<program number>_<hash>_source.cl
    {
        char    numberString[256] = "";

        if( config().OmitProgramNumber )
        {
            CLI_SPRINTF( numberString, 256, "%08X",
                (unsigned int)hash );
        }
        else
        {
            CLI_SPRINTF( numberString, 256, "%04u_%08X",
                m_ProgramNumber,
                (unsigned int)hash );
        }

        fileName += "/CLI_";
        fileName += numberString;
        fileName += "_source.cl";
    }
    // Now make directories as appropriate.
    {
        OS().MakeDumpDirectories( fileName );
    }
    // Dump the program source to a .cl file.
    if( singleString )
    {


        std::ofstream os;
        os.open(
            fileName.c_str(),
            std::ios::out | std::ios::binary );
        if( os.good() )
        {
            log( "Dumping program to file (inject): " + fileName + "\n" );

            // don't write the null terminator to the file
            os.write( singleString, strlen( singleString ) );
            os.close();
        }
        else
        {
            logf( "Failed to open program source dump file for writing: %s\n",
                fileName.c_str() );
        }
    }

    SProgramInfo&   programInfo = m_ProgramInfoMap[ program ];
    programInfo.ProgramNumber = m_ProgramNumber;
    programInfo.CompileCount = 0;

    m_ProgramNumber++;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpInputProgramBinaries(
    const cl_program program,
    uint64_t hash,
    bool modified,
    cl_uint num_devices,
    const cl_device_id* device_list,
    const size_t* lengths,
    const unsigned char** binaries )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    CLI_ASSERT( config().DumpInputProgramBinaries );

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
    }
    if( modified )
    {
        fileName += "/Modified";
    }

    // Make the filename.  It will have the form:
    //   CLI_<program number>_<hash>
    // Leave off the extension for now.
    {
        char    numberString[256] = "";

        if( config().OmitProgramNumber )
        {
            CLI_SPRINTF( numberString, 256, "%08X",
                (unsigned int)hash );
        }
        else
        {
            CLI_SPRINTF( numberString, 256, "%04u_%08X",
                m_ProgramNumber,
                (unsigned int)hash );
        }

        fileName += "/CLI_";
        fileName += numberString;
    }
    // Now make directories as appropriate.
    {
        OS().MakeDumpDirectories( fileName );
    }

    for( size_t i = 0; i < num_devices; i++ )
    {
        cl_device_type  deviceType = CL_DEVICE_TYPE_DEFAULT;

        // It's OK if this fails.  If it does, it just
        // means that our output file won't have a device
        // type.
        dispatch().clGetDeviceInfo(
            device_list[ i ],
            CL_DEVICE_TYPE,
            sizeof( deviceType ),
            &deviceType,
            NULL );

        std::string outputFileName = fileName;

        if( deviceType & CL_DEVICE_TYPE_CPU )
        {
            outputFileName += "_CPU";
        }
        if( deviceType & CL_DEVICE_TYPE_GPU )
        {
            outputFileName += "_GPU";
        }
        if( deviceType & CL_DEVICE_TYPE_ACCELERATOR )
        {
            outputFileName += "_ACC";
        }
        if( deviceType & CL_DEVICE_TYPE_CUSTOM )
        {
            outputFileName += "_CUSTOM";
        }

        outputFileName += ".bin";

        std::ofstream os;
        os.open(
            outputFileName.c_str(),
            std::ios::out | std::ios::binary );
        if( os.good() )
        {
            log( "Dumping input program binary to file: " + outputFileName + "\n" );

            os.write(
                (const char*)binaries[ i ],
                lengths[ i ] );
            os.close();
        }
        else
        {
            logf( "Failed to open program binary dump file for writing: %s\n",
                outputFileName.c_str());
        }
    }

    SProgramInfo&   programInfo = m_ProgramInfoMap[ program ];
    programInfo.ProgramNumber = m_ProgramNumber;
    programInfo.CompileCount = 0;

    m_ProgramNumber++;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpProgramSPIRV(
    cl_program program,
    uint64_t hash,
    bool modified,
    const size_t length,
    const void* il )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    CLI_ASSERT( config().DumpProgramSPIRV );

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
    }
    if( modified )
    {
        fileName += "/Modified";
    }

    // Make the filename.  It will have the form:
    //   CLI_<program number>_<hash>_0000.spv
    {
        char    numberString[256] = "";

        if( config().OmitProgramNumber )
        {
            CLI_SPRINTF( numberString, 256, "%08X_0000",
                (unsigned int)hash );
        }
        else
        {
            CLI_SPRINTF( numberString, 256, "%04u_%08X_0000",
                m_ProgramNumber,
                (unsigned int)hash );
        }

        fileName += "/CLI_";
        fileName += numberString;
        fileName += ".spv";
    }

    // Now make directories as appropriate.
    {
        OS().MakeDumpDirectories( fileName );
    }

    // Dump the program source to a .spv file.
    {
        std::ofstream os;
        os.open(
            fileName.c_str(),
            std::ios::out | std::ios::binary );
        if( os.good() )
        {
            log( "Dumping program to file (inject): " + fileName + "\n" );

            os.write( (const char*)il, length );
            os.close();

            // Optionally, run spirv-dis to disassemble the generated module.
            if( !config().SPIRVDis.empty() )
            {
                std::string command =
                    config().SPIRVDis +
                    " -o " + fileName + "t" +
                    " " + fileName;

                logf( "Running: %s\n", command.c_str() );
                OS().ExecuteCommand( command );
            }
        }
        else
        {
            logf( "Failed to open il program dump file for writing: %s\n",
                fileName.c_str() );
        }
    }

    SProgramInfo&   programInfo = m_ProgramInfoMap[ program ];
    programInfo.ProgramNumber = m_ProgramNumber;
    programInfo.CompileCount = 0;

    m_ProgramNumber++;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpProgramOptionsScript(
    const cl_program program,
    const char* options )
{
#if defined(_WIN32)

    std::lock_guard<std::mutex> lock(m_Mutex);

    CLI_ASSERT( config().DumpProgramSource || config().SimpleDumpProgramSource );

    const SProgramInfo& programInfo = m_ProgramInfoMap[ program ];

    if( options )
    {
        char    dirname[MAX_PATH] = "";
        char    filename[MAX_PATH] = "";
        char    filepath[MAX_PATH] = "";

        if( config().DumpProgramSourceScript )
        {
            size_t  remaining = MAX_PATH;

            char    date[9] = "";
            char    time[9] = "";
            char*   curPos = NULL;
            char*   nextToken = NULL;
            char*   pch = NULL;

            // Directory:

            curPos = dirname;
            remaining = MAX_PATH;
            memset( curPos, 0, MAX_PATH );

            _strdate_s( date, 9 );
            _strtime_s( time, 9 );

            memcpy_s( curPos, remaining, "CLShaderDump_", 14 );
            curPos += 13;
            remaining -= 13;

            memcpy_s( curPos, remaining, strtok_s( date, "/", &nextToken ), 2 );
            curPos += 2;
            remaining -= 2;

            memcpy_s( curPos, remaining, strtok_s( NULL, "/", &nextToken ), 2 );
            curPos += 2;
            remaining -= 2;

            memcpy_s( curPos, remaining, strtok_s( NULL, "/", &nextToken ), 2 );
            curPos += 2;
            remaining -= 2;

            ::CreateDirectoryA( dirname, NULL );

            // File:

            curPos = filename;
            remaining = MAX_PATH;
            memset( curPos, 0, MAX_PATH );

            if( GetModuleFileNameA( NULL, filename, MAX_PATH-1 ) == 0 )
            {
                CLI_ASSERT( 0 );
                strcpy_s( curPos, remaining, "process.exe" );
            }

            pch = strrchr( filename, '\\' );
            pch++;
            memcpy_s( curPos, remaining, pch, strlen( pch ) );
            curPos += strlen( pch ) - 4;    // -4 to cut off ".exe"
            remaining -= strlen( pch ) - 4;

            memcpy_s( curPos, remaining, "_", 2 );
            curPos += 1;
            remaining -= 1;

            memcpy_s( curPos, remaining, strtok_s( time, ":", &nextToken ), 2 );
            curPos += 2;
            remaining -= 2;

            memcpy_s( curPos, remaining, strtok_s( NULL, ":", &nextToken ), 2 );
            curPos += 2;
            remaining -= 2;

            memcpy_s( curPos, remaining, strtok_s( NULL, ":", &nextToken ), 2 );
            curPos += 2;
            remaining -= 2;

            CLI_SPRINTF( curPos, remaining, "_%8.8x", programInfo.ProgramNumber );
            curPos += 9;
            remaining -= 9;
        }
        else
        {
            CLI_SPRINTF( dirname, MAX_PATH, "." );
            CLI_SPRINTF( filename, MAX_PATH, "kernel" );
        }

        CLI_SPRINTF( filepath, MAX_PATH, "%s/%s.%s", dirname, filename, "txt" );

        std::ofstream os;
        os.open(
            filepath,
            std::ios::out | std::ios::binary );
        if( os.good() )
        {
            os.write( options, strlen( options ) );
            os.close();
        }
        else
        {
            logf( "Failed to open program options dump file for writing: %s\n",
                filepath );
        }
    }

#else
    CLI_ASSERT( 0 );
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpProgramOptions(
    const cl_program program,
    bool modified,
    cl_bool isCompile,
    cl_bool isLink,
    const char* options )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    CLI_ASSERT(
        config().DumpProgramSource ||
        config().DumpInputProgramBinaries ||
        config().DumpProgramBinaries ||
        config().DumpProgramSPIRV );

    const SProgramInfo& programInfo = m_ProgramInfoMap[ program ];

    if( options )
    {
        std::string fileName;

        // Get the dump directory name.
        {
            OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
        }
        if( modified )
        {
            fileName += "/Modified";
        }

        // Make the filename.  It will have the form:
        //   CLI_<program number>_<program hash>_<compile count>_<options hash>
        // Leave off the extension for now.
        {
            char    numberString[256] = "";

            if( config().OmitProgramNumber )
            {
                CLI_SPRINTF( numberString, 256, "%08X_%04u_%08X",
                    (unsigned int)programInfo.ProgramHash,
                    programInfo.CompileCount,
                    (unsigned int)programInfo.OptionsHash );
            }
            else
            {
                CLI_SPRINTF( numberString, 256, "%04u_%08X_%04u_%08X",
                    programInfo.ProgramNumber,
                    (unsigned int)programInfo.ProgramHash,
                    programInfo.CompileCount,
                    (unsigned int)programInfo.OptionsHash );
            }

            fileName += "/CLI_";
            fileName += numberString;
        }

        // Now make directories as appropriate.
        {
            OS().MakeDumpDirectories( fileName );
        }

        // Dump the program options to a .txt file.
        {
            fileName +=
                isCompile ? "_compile_options.txt" :
                isLink ? "_link_options.txt" :
                "_options.txt";
            std::ofstream os;
            os.open(
                fileName.c_str(),
                std::ios::out | std::ios::binary );
            if( os.good() )
            {
                log( "Dumping program options to file (inject): " + fileName + "\n" );

                // don't write the null terminator to the file
                os.write( options, strlen( options) );
                os.close();
            }
            else
            {
                logf( "Failed to open program options dump file for writing: %s\n",
                    fileName.c_str() );
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpProgramBuildLog(
    const cl_program program,
    const cl_device_id device,
    const char* buildLog,
    const size_t buildLogSize )
{
    // We're already in a critical section when we get here, so we don't need to
    // grab the critical section again.

    CLI_ASSERT( config().DumpProgramBuildLogs );
    CLI_ASSERT( buildLog );

    const SProgramInfo& programInfo = m_ProgramInfoMap[ program ];

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
    }
    // Make the filename.  It will have the form:
    //   CLI_<program number>_<program hash>_<compile count>_<options hash>
    // Leave off the extension for now.
    {
        char    numberString[256] = "";

        if( config().OmitProgramNumber )
        {
            CLI_SPRINTF( numberString, 256, "%08X_%04u_%08X",
                (unsigned int)programInfo.ProgramHash,
                programInfo.CompileCount,
                (unsigned int)programInfo.OptionsHash );
        }
        else
        {
            CLI_SPRINTF( numberString, 256, "%04u_%08X_%04u_%08X",
                programInfo.ProgramNumber,
                (unsigned int)programInfo.ProgramHash,
                programInfo.CompileCount,
                (unsigned int)programInfo.OptionsHash );
        }

        fileName += "/CLI_";
        fileName += numberString;
    }
    // Now make directories as appropriate.
    {
        OS().MakeDumpDirectories( fileName );
    }

    cl_device_type  deviceType = CL_DEVICE_TYPE_DEFAULT;

    // It's OK if this fails.  If it does, it just
    // means that our output file won't have a device
    // type.
    dispatch().clGetDeviceInfo(
        device,
        CL_DEVICE_TYPE,
        sizeof( deviceType ),
        &deviceType,
        NULL );

    if( deviceType & CL_DEVICE_TYPE_CPU )
    {
        fileName += "_CPU";
    }
    if( deviceType & CL_DEVICE_TYPE_GPU )
    {
        fileName += "_GPU";
    }
    if( deviceType & CL_DEVICE_TYPE_ACCELERATOR )
    {
        fileName += "_ACC";
    }
    if( deviceType & CL_DEVICE_TYPE_CUSTOM )
    {
        fileName += "_CUSTOM";
    }

    fileName += "_build_log.txt";

    std::ofstream os;
    os.open(
        fileName.c_str(),
        std::ios::out | std::ios::binary );
    if( os.good() )
    {
        log( "Dumping build log to file: " + fileName + "\n" );

        os.write(
            buildLog,
            buildLogSize );
        os.close();
    }
    else
    {
        logf( "Failed to open program build log dump file for writing: %s\n",
            fileName.c_str() );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getTimingTagBlocking(
    const char* functionName,
    const cl_bool blocking,
    const size_t size,
    std::string& hostTag,
    std::string& deviceTag )
{
    deviceTag.reserve(128);
    deviceTag = functionName;

    if( size && config().DevicePerformanceTimeTransferTracking )
    {
        char    s[256];
        CLI_SPRINTF( s, 256, "( %zu bytes )", size );
        deviceTag += s;
    }

    if( blocking == CL_TRUE )
    {
        hostTag += "blocking";
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getTimingTagsMap(
    const char* functionName,
    const cl_map_flags flags,
    const cl_bool blocking,
    const size_t size,
    std::string& hostTag,
    std::string& deviceTag )
{
    // Note: we do not currently need a lock for this function.

    if( flags & CL_MAP_WRITE_INVALIDATE_REGION )
    {
        hostTag += "WI";
    }
    else if( flags & CL_MAP_WRITE )
    {
        hostTag += "RW";
    }
    else if( flags & CL_MAP_READ )
    {
        hostTag += "R";
    }

    if( flags & ~(CL_MAP_READ | CL_MAP_WRITE | CL_MAP_WRITE_INVALIDATE_REGION) )
    {
        hostTag += "?";
    }

    deviceTag.reserve(128);
    deviceTag = functionName;
    deviceTag += "( ";
    deviceTag += hostTag;
    if( size && config().DevicePerformanceTimeTransferTracking )
    {
        char    s[256];
        CLI_SPRINTF( s, 256, "; %zu bytes", size );
        deviceTag += s;
    }
    deviceTag += " )";

    if( blocking == CL_TRUE )
    {
        hostTag += "; blocking";
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getTimingTagsUnmap(
    const char* functionName,
    const void* ptr,
    std::string& hostTag,
    std::string& deviceTag )
{
    if( ptr )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        CMapPointerInfoMap::iterator iter = m_MapPointerInfoMap.find( ptr );
        if( iter != m_MapPointerInfoMap.end() )
        {
            const cl_map_flags flags = iter->second.Flags;
            const size_t size = iter->second.Size;

            if( flags & CL_MAP_WRITE_INVALIDATE_REGION )
            {
                hostTag += "WI";
            }
            else if( flags & CL_MAP_WRITE )
            {
                hostTag += "RW";
            }
            else if( flags & CL_MAP_READ )
            {
                hostTag += "R";
            }

            if( flags & ~(CL_MAP_READ | CL_MAP_WRITE | CL_MAP_WRITE_INVALIDATE_REGION) )
            {
                hostTag += "?";
            }

            deviceTag.reserve(128);
            deviceTag = functionName;
            deviceTag += "( ";
            deviceTag += hostTag;
            if( size && config().DevicePerformanceTimeTransferTracking )
            {
                char    s[256];
                CLI_SPRINTF( s, 256, "; %zu bytes", size );
                deviceTag += s;
            }
            deviceTag += " )";
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getTimingTagsMemfill(
    const char* functionName,
    const cl_command_queue queue,
    const void* dst,
    const size_t size,
    std::string& hostTag,
    std::string& deviceTag )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_platform_id  platform = getPlatform(queue);

    // If we don't have a function pointer for clGetMemAllocINFO, try to
    // get one.  It's possible that the function pointer exists but
    // the application hasn't queried for it yet, in which case it won't
    // be installed into the dispatch table.
    if( dispatchX(platform).clGetMemAllocInfoINTEL == NULL )
    {
        getExtensionFunctionAddress(
            platform,
            "clGetMemAllocInfoINTEL" );
    }

    const auto& dispatchX = this->dispatchX(platform);
    if( dispatchX.clGetMemAllocInfoINTEL != NULL )
    {
        cl_context  context = NULL;
        dispatch().clGetCommandQueueInfo(
            queue,
            CL_QUEUE_CONTEXT,
            sizeof( context ),
            &context,
            NULL );

        if( context )
        {
            cl_unified_shared_memory_type_intel dstType = CL_MEM_TYPE_UNKNOWN_INTEL;
            dispatchX.clGetMemAllocInfoINTEL(
                context,
                dst,
                CL_MEM_ALLOC_TYPE_INTEL,
                sizeof(dstType),
                &dstType,
                NULL );
            switch( dstType )
            {
            case CL_MEM_TYPE_DEVICE_INTEL:  hostTag += "D"; break;
            case CL_MEM_TYPE_HOST_INTEL:    hostTag += "H"; break;
            case CL_MEM_TYPE_SHARED_INTEL:  hostTag += "S"; break;
            default:                        hostTag += "M"; break;
            }

            deviceTag.reserve(128);
            deviceTag = functionName;
            deviceTag += "( ";
            deviceTag += hostTag;
            if( size && config().DevicePerformanceTimeTransferTracking )
            {
                char    s[256];
                CLI_SPRINTF( s, 256, "; %zu bytes", size );
                deviceTag += s;
            }
            deviceTag += " )";
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getTimingTagsMemcpy(
    const char* functionName,
    const cl_command_queue queue,
    const cl_bool blocking,
    const void* dst,
    const void* src,
    const size_t size,
    std::string& hostTag,
    std::string& deviceTag )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_platform_id  platform = getPlatform(queue);

    // If we don't have a function pointer for clGetMemAllocINFO, try to
    // get one.  It's possible that the function pointer exists but
    // the application hasn't queried for it yet, in which case it won't
    // be installed into the dispatch table.
    if( dispatchX(platform).clGetMemAllocInfoINTEL == NULL )
    {
        getExtensionFunctionAddress(
            platform,
            "clGetMemAllocInfoINTEL" );
    }

    const auto& dispatchX = this->dispatchX(platform);
    if( dispatchX.clGetMemAllocInfoINTEL != NULL )
    {
        cl_context  context = NULL;
        dispatch().clGetCommandQueueInfo(
            queue,
            CL_QUEUE_CONTEXT,
            sizeof( context ),
            &context,
            NULL );

        if( context )
        {
            cl_unified_shared_memory_type_intel dstType = CL_MEM_TYPE_UNKNOWN_INTEL;
            dispatchX.clGetMemAllocInfoINTEL(
                context,
                dst,
                CL_MEM_ALLOC_TYPE_INTEL,
                sizeof(dstType),
                &dstType,
                NULL );
            cl_unified_shared_memory_type_intel srcType = CL_MEM_TYPE_UNKNOWN_INTEL;
            dispatchX.clGetMemAllocInfoINTEL(
                context,
                src,
                CL_MEM_ALLOC_TYPE_INTEL,
                sizeof(srcType),
                &srcType,
                NULL );
            switch( srcType )
            {
            case CL_MEM_TYPE_DEVICE_INTEL:  hostTag += "Dto"; break;
            case CL_MEM_TYPE_HOST_INTEL:    hostTag += "Hto"; break;
            case CL_MEM_TYPE_SHARED_INTEL:  hostTag += "Sto"; break;
            default:                        hostTag += "Mto"; break;
            }
            switch( dstType )
            {
            case CL_MEM_TYPE_DEVICE_INTEL:  hostTag += "D"; break;
            case CL_MEM_TYPE_HOST_INTEL:    hostTag += "H"; break;
            case CL_MEM_TYPE_SHARED_INTEL:  hostTag += "S"; break;
            default:                        hostTag += "M"; break;
            }

            deviceTag.reserve(128);
            deviceTag = functionName;
            deviceTag += "( ";
            deviceTag += hostTag;
            if( size && config().DevicePerformanceTimeTransferTracking )
            {
                char    s[256];
                CLI_SPRINTF( s, 256, "; %zu bytes", size );
                deviceTag += s;
            }
            deviceTag += " )";
        }
    }

    if( blocking == CL_TRUE )
    {
        hostTag += "; blocking";
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getTimingTagsKernel(
    const cl_command_queue queue,
    const cl_kernel kernel,
    const cl_uint workDim,
    const size_t* gwo,
    const size_t* gws,
    const size_t* lws,
    std::string& hostTag,
    std::string& deviceTag )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_device_id device = NULL;
    dispatch().clGetCommandQueueInfo(
        queue,
        CL_QUEUE_DEVICE,
        sizeof(device),
        &device,
        NULL );

    // Cache the device info if it's not cached already, since we'll print
    // the device name and other device properties as part of the report.
    cacheDeviceInfo( device );

    if( kernel )
    {
        hostTag += getShortKernelNameWithHash(kernel);

        deviceTag += hostTag;

        if( config().DevicePerformanceTimeKernelInfoTracking && device )
        {
            const SDeviceInfo& deviceInfo = m_DeviceInfoMap[device];

            std::ostringstream  ss;
            {
                size_t  maxsgs = 0;
                size_t  pwgsm = 0;
                size_t  simd = 0;

                // Use the kernel "max subgroup size" and "preferred work
                // group size multiple" to get a reasonable estimate of
                // the kernel "SIMD size".  We'll try to query both values,
                // and if both queries are successful, we'll pick the
                // smallest one.  Empirically, this is reasonably accurate.

                // First, query the "max subgroup size":

                // The query for the max subgroup size requires passing in
                // a local work size.  We'll use the local work size provided
                // by the app if there is one, otherwise we will query the
                // max local work size for this kernel and use it.
                const size_t*   queryLWS = lws;
                cl_uint queryWorkDim = workDim;

                size_t  kwgs = 0;
                if( queryLWS == NULL )
                {
                    dispatch().clGetKernelWorkGroupInfo(
                        kernel,
                        device,
                        CL_KERNEL_WORK_GROUP_SIZE,
                        sizeof(kwgs),
                        &kwgs,
                        NULL );
                    queryLWS = &kwgs;
                    queryWorkDim = 1;
                }

                if( maxsgs == 0 &&
                    deviceInfo.Supports_cl_khr_subgroups )
                {
                    cl_platform_id  platform = getPlatform(device);
                    if( dispatchX(platform).clGetKernelSubGroupInfoKHR == NULL )
                    {
                        getExtensionFunctionAddress(
                            platform,
                            "clGetKernelSubGroupInfoKHR" );
                    }

                    const auto& dispatchX = this->dispatchX(platform);
                    if( dispatchX.clGetKernelSubGroupInfoKHR )
                    {
                        dispatchX.clGetKernelSubGroupInfoKHR(
                            kernel,
                            device,
                            CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE_KHR,
                            queryWorkDim * sizeof(queryLWS[0]),
                            queryLWS,
                            sizeof(maxsgs),
                            &maxsgs,
                            NULL );
                    }
                }
                if( maxsgs == 0 &&
                    deviceInfo.NumericVersion >= CL_MAKE_VERSION_KHR(2, 1, 0) &&
                    dispatch().clGetKernelSubGroupInfo )
                {
                    dispatch().clGetKernelSubGroupInfo(
                        kernel,
                        device,
                        CL_KERNEL_MAX_SUB_GROUP_SIZE_FOR_NDRANGE,
                        queryWorkDim * sizeof(queryLWS[0]),
                        queryLWS,
                        sizeof(maxsgs),
                        &maxsgs,
                        NULL );
                }

                // Next, query the "preferred work group size multiple":

                dispatch().clGetKernelWorkGroupInfo(
                    kernel,
                    device,
                    CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                    sizeof(pwgsm),
                    &pwgsm,
                    NULL );

                // If either query is zero, use the other query.
                // Otherwise, use the smallest query.

                maxsgs = ( maxsgs == 0 ) ? pwgsm : maxsgs;
                pwgsm = ( pwgsm == 0 ) ? maxsgs : pwgsm;
                simd = ( maxsgs < pwgsm ) ? maxsgs : pwgsm;

                if( simd )
                {
                    ss << " SIMD" << simd;
                }
            }
            {
                cl_uint regCount = 0;
                dispatch().clGetKernelWorkGroupInfo(
                    kernel,
                    device,
                    CL_KERNEL_REGISTER_COUNT_INTEL,
                    sizeof(regCount),
                    &regCount,
                    NULL );
                if( regCount )
                {
                    ss << " REG" << regCount;
                }
            }
            {
                cl_ulong slm = 0;
                dispatch().clGetKernelWorkGroupInfo(
                    kernel,
                    device,
                    CL_KERNEL_LOCAL_MEM_SIZE,
                    sizeof(slm),
                    &slm,
                    NULL );
                if( slm )
                {
                    ss << " SLM=" << slm;
                }
            }
            {
                cl_ulong tpm = 0;
                dispatch().clGetKernelWorkGroupInfo(
                    kernel,
                    device,
                    CL_KERNEL_PRIVATE_MEM_SIZE,
                    sizeof(tpm),
                    &tpm,
                    NULL );
                if( tpm )
                {
                    ss << " TPM=" << tpm;
                }
            }
            {
                cl_ulong spill = 0;
                dispatch().clGetKernelWorkGroupInfo(
                    kernel,
                    device,
                    CL_KERNEL_SPILL_MEM_SIZE_INTEL,
                    sizeof(spill),
                    &spill,
                    NULL );
                if( spill )
                {
                    ss << " SPILL=" << spill;
                }
            }
            deviceTag += ss.str();
        }

        if( config().DevicePerformanceTimeGWOTracking )
        {
            std::ostringstream  ss;
            ss << " GWO[ ";
            if( gwo )
            {
                if( workDim >= 1 )
                {
                    ss << gwo[0];
                }
                if( workDim >= 2 )
                {
                    ss << ", " << gwo[1];
                }
                if( workDim >= 3 )
                {
                    ss << ", " << gwo[2];
                }
            }
            else
            {
                ss << "NULL";
            }
            ss << " ]";
            deviceTag += ss.str();
        }

        if( config().DevicePerformanceTimeGWSTracking && gws )
        {
            std::ostringstream  ss;
            ss << " GWS[ ";
            if( workDim >= 1 )
            {
                ss << gws[0];
            }
            if( workDim >= 2 )
            {
                ss << " x " << gws[1];
            }
            if( workDim >= 3 )
            {
                ss << " x " << gws[2];
            }
            ss << " ]";
            deviceTag += ss.str();
        }

        if( config().DevicePerformanceTimeLWSTracking )
        {
            bool    useSuggestedLWS = false;
            size_t  suggestedLWS[3] = { 0, 0, 0 };
            size_t  emptyGWO[3] = { 0, 0, 0 };

            if( lws == NULL &&
                workDim <= 3 &&
                config().DevicePerformanceTimeSuggestedLWSTracking )
            {
                cl_platform_id  platform = getPlatform(device);

                // Try the cl_khr_suggested_local_work_size version first.
                if( useSuggestedLWS == false )
                {
                    if( dispatchX(platform).clGetKernelSuggestedLocalWorkSizeKHR == NULL )
                    {
                        getExtensionFunctionAddress(
                            platform,
                            "clGetKernelSuggestedLocalWorkSizeKHR" );
                    }

                    const auto& dispatchX = this->dispatchX(platform);
                    if( dispatchX.clGetKernelSuggestedLocalWorkSizeKHR )
                    {
                        cl_int testErrorCode = dispatchX.clGetKernelSuggestedLocalWorkSizeKHR(
                            queue,
                            kernel,
                            workDim,
                            gwo,
                            gws,
                            suggestedLWS );
                        useSuggestedLWS = ( testErrorCode == CL_SUCCESS );
                    }
                }

                // If this fails, try the unofficial Intel version next.
                if( useSuggestedLWS == false )
                {
                    if( dispatchX(platform).clGetKernelSuggestedLocalWorkSizeINTEL == NULL )
                    {
                        getExtensionFunctionAddress(
                            platform,
                            "clGetKernelSuggestedLocalWorkSizeINTEL" );
                    }

                    const auto& dispatchX = this->dispatchX(platform);
                    if( dispatchX.clGetKernelSuggestedLocalWorkSizeINTEL )
                    {
                        cl_int testErrorCode = dispatchX.clGetKernelSuggestedLocalWorkSizeINTEL(
                            queue,
                            kernel,
                            workDim,
                            gwo == NULL ? emptyGWO : gwo,
                            gws,
                            suggestedLWS );
                        useSuggestedLWS = ( testErrorCode == CL_SUCCESS );
                    }
                }
            }

            std::ostringstream  ss;
            if( useSuggestedLWS )
            {
                ss << " SLWS[ ";
                lws = suggestedLWS;
            }
            else
            {
                ss << " LWS[ ";
            }

            if( lws )
            {
                if( workDim >= 1 )
                {
                    ss << lws[0];
                }
                if( workDim >= 2 )
                {
                    ss << " x " << lws[1];
                }
                if( workDim >= 3 )
                {
                    ss << " x " << lws[2];
                }
            }
            else
            {
                ss << "NULL";
            }

            ss << " ]";
            deviceTag += ss.str();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::updateHostTimingStats(
    const char* functionName,
    const std::string& tag,
    clock::time_point start,
    clock::time_point end )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    std::string key( functionName );
    if( !tag.empty() )
    {
        key += "( ";
        key += tag;
        key += " )";
    }

    SHostTimingStats& hostTimingStats = m_HostTimingStatsMap[ key ];

    using ns = std::chrono::nanoseconds;
    uint64_t    nsDelta = std::chrono::duration_cast<ns>(end - start).count();

    hostTimingStats.NumberOfCalls++;
    hostTimingStats.TotalNS += nsDelta;
    hostTimingStats.MinNS = std::min<uint64_t>( hostTimingStats.MinNS, nsDelta );
    hostTimingStats.MaxNS = std::max<uint64_t>( hostTimingStats.MaxNS, nsDelta );

    if( config().HostPerformanceTimeLogging )
    {
        uint64_t    numberOfCalls = hostTimingStats.NumberOfCalls;
        logf( "Host Time for call %" PRIu64 ": %s = %" PRIu64 " ns\n",
            numberOfCalls,
            key.c_str(),
            nsDelta );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::modifyCommandQueueProperties(
    cl_command_queue_properties& props ) const
{
    if( config().InOrderQueue )
    {
        props &= ~(cl_command_queue_properties)CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
    }
    if( config().NoProfilingQueue )
    {
        props &= ~(cl_command_queue_properties)CL_QUEUE_PROFILING_ENABLE;
    }
    if( config().DevicePerformanceTiming ||
        config().ITTPerformanceTiming ||
        config().ChromePerformanceTiming ||
        config().DevicePerfCounterEventBasedSampling )
    {
        props |= (cl_command_queue_properties)CL_QUEUE_PROFILING_ENABLE;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::createCommandQueueProperties(
    cl_device_id device,
    cl_command_queue_properties props,
    cl_queue_properties*& pLocalQueueProperties ) const
{
    bool    addCommandQueuePropertiesEnum = true;
    bool    addPriorityHintEnum =
                config().DefaultQueuePriorityHint != 0 &&
                checkDeviceForExtension( device, "cl_khr_priority_hints" );
    bool    addThrottleHintEnum =
                config().DefaultQueueThrottleHint != 0 &&
                checkDeviceForExtension( device, "cl_khr_throttle_hints" );

    size_t  numProperties = 0;
    if( addCommandQueuePropertiesEnum )
    {
        numProperties += 2;
    }
    if( addThrottleHintEnum )
    {
        numProperties += 2;
    }
    if( addThrottleHintEnum )
    {
        numProperties += 2;
    }

    // Allocate a new array of properties.  We need to allocate two
    // properties for each pair, plus one property for the terminating
    // zero.
    pLocalQueueProperties = new cl_queue_properties[ numProperties + 1 ];
    if( pLocalQueueProperties )
    {
        numProperties = 0;

        if( addPriorityHintEnum )
        {
            CLI_ASSERT( config().DefaultQueuePriorityHint != 0 );
            pLocalQueueProperties[ numProperties] = CL_QUEUE_PRIORITY_KHR;
            pLocalQueueProperties[ numProperties + 1 ] = config().DefaultQueuePriorityHint;
            numProperties += 2;
        }
        if( config().DefaultQueueThrottleHint != 0 )
        {
            CLI_ASSERT( config().DefaultQueueThrottleHint != 0 );
            pLocalQueueProperties[ numProperties] = CL_QUEUE_THROTTLE_KHR;
            pLocalQueueProperties[ numProperties + 1 ] = config().DefaultQueueThrottleHint;
            numProperties += 2;
        }

        // This setting is added last in the list, in order to better behave with runtimes
        // truncating it when a property has a value of 0, which may be the case below.
        if( addCommandQueuePropertiesEnum )
        {
            modifyCommandQueueProperties(props);

            pLocalQueueProperties[numProperties] = CL_QUEUE_PROPERTIES;
            pLocalQueueProperties[numProperties + 1] = props;
            numProperties += 2;
        }

        // Add the terminating zero.
        pLocalQueueProperties[ numProperties ] = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::createCommandQueuePropertiesOverride(
    cl_device_id device,
    const cl_queue_properties* properties,
    cl_queue_properties*& pLocalQueueProperties ) const
{
    // We're always going to add command queue properties, unless command queue
    // properties already exist (requesting the same property twice is an
    // error).  We also may add priority hints or throttle hints, in some cases.
    // So, look through the queue properties for these enums.  We need to do
    // this anyways to count the number of property pairs.
    bool    addCommandQueuePropertiesEnum = true;
    bool    addPriorityHintEnum =
                config().DefaultQueuePriorityHint != 0 &&
                checkDeviceForExtension( device, "cl_khr_priority_hints" );
    bool    addThrottleHintEnum =
                config().DefaultQueueThrottleHint != 0 &&
                checkDeviceForExtension( device, "cl_khr_throttle_hints" );

    size_t  numProperties = 0;
    if( properties )
    {
        while( properties[ numProperties ] != 0 )
        {
            switch( properties[ numProperties ] )
            {
            case CL_QUEUE_PROPERTIES:
                addCommandQueuePropertiesEnum = false;
                break;
            case CL_QUEUE_PRIORITY_KHR:
                addPriorityHintEnum = false;
                break;
            case CL_QUEUE_THROTTLE_KHR:
                addThrottleHintEnum = false;
                break;
            default:
                break;
            }
            numProperties += 2;
        }
    }

    if( addCommandQueuePropertiesEnum )
    {
        numProperties += 2;
    }
    if( addThrottleHintEnum )
    {
        numProperties += 2;
    }
    if( addThrottleHintEnum )
    {
        numProperties += 2;
    }

    // Allocate a new array of properties.  We need to allocate two
    // properties for each pair, plus one property for the terminating
    // zero.
    pLocalQueueProperties = new cl_queue_properties[ numProperties + 1 ];
    if( pLocalQueueProperties )
    {
        // Copy the old properties array to the new properties array,
        // if the new properties array exists.
        numProperties = 0;
        if( properties )
        {
            while( properties[ numProperties ] != 0 )
            {
                pLocalQueueProperties[ numProperties ] = properties[ numProperties ];
                if( properties[ numProperties ] == CL_QUEUE_PROPERTIES )
                {
                    CLI_ASSERT( addCommandQueuePropertiesEnum == false );

                    cl_command_queue_properties props = properties[ numProperties + 1 ];

                    modifyCommandQueueProperties( props );

                    pLocalQueueProperties[ numProperties + 1 ] = props;
                }
                else
                {
                    pLocalQueueProperties[ numProperties + 1 ] =
                        properties[ numProperties + 1 ];
                }
                numProperties += 2;
            }
        }
        if( addPriorityHintEnum )
        {
            CLI_ASSERT( config().DefaultQueuePriorityHint != 0 );
            pLocalQueueProperties[ numProperties] = CL_QUEUE_PRIORITY_KHR;
            pLocalQueueProperties[ numProperties + 1 ] = config().DefaultQueuePriorityHint;
            numProperties += 2;
        }
        if( addThrottleHintEnum )
        {
            CLI_ASSERT( config().DefaultQueueThrottleHint != 0 );
            pLocalQueueProperties[ numProperties] = CL_QUEUE_THROTTLE_KHR;
            pLocalQueueProperties[ numProperties + 1 ] = config().DefaultQueueThrottleHint;
            numProperties += 2;
        }

        // This setting is added last in the list, in order to better behave with runtimes
        // truncating it when a property has a value of 0, which may be the case below.
        if( addCommandQueuePropertiesEnum )
        {
            cl_command_queue_properties props = 0;

            modifyCommandQueueProperties(props);

            pLocalQueueProperties[numProperties] = CL_QUEUE_PROPERTIES;
            pLocalQueueProperties[numProperties + 1] = props;
            numProperties += 2;
        }

        // Add the terminating zero.
        pLocalQueueProperties[ numProperties ] = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dummyCommandQueue(
    cl_context context,
    cl_device_id device )
{
    if( config().DummyOutOfOrderQueue )
    {
        cl_command_queue_properties props;
        dispatch().clGetDeviceInfo(
            device,
            CL_DEVICE_QUEUE_ON_HOST_PROPERTIES,
            sizeof(props),
            &props,
            NULL );
        if( props & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE )
        {
            std::lock_guard<std::mutex> lock(m_Mutex);

            log( "Creating and destroying a dummy out-of-order queue.\n" );

            cl_int  errorCode = CL_SUCCESS;
            cl_command_queue dummy = dispatch().clCreateCommandQueue(
                context,
                device,
                CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE,
                &errorCode );
            if( errorCode == CL_SUCCESS )
            {
                dispatch().clReleaseCommandQueue( dummy );
            }
            else
            {
                logf( "Error creating dummy command queue!  %s (%i)\n",
                    enumName().name( errorCode ).c_str(),
                    errorCode );
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addTimingEvent(
    const char* functionName,
    const uint64_t enqueueCounter,
    const clock::time_point queuedTime,
    const std::string& tag,
    const cl_command_queue queue,
    cl_event event )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if( event == NULL )
    {
        logf( "Unexpectedly got a NULL timing event for %s, check for OpenCL errors!\n",
            functionName );
        return;
    }

    m_EventList.emplace_back();

    SEventListNode& node = m_EventList.back();

    cl_device_id device = NULL;
    dispatch().clGetCommandQueueInfo(
        queue,
        CL_QUEUE_DEVICE,
        sizeof(device),
        &device,
        NULL );

    // Cache the device info if it's not cached already, since we'll print
    // the device name and other device properties as part of the report.
    cacheDeviceInfo( device );

    dispatch().clRetainEvent( event );

    node.Device = device;
    node.QueueNumber = m_QueueNumberMap[ queue ];
    node.Name = !tag.empty() ? tag : functionName;
    node.EnqueueCounter = enqueueCounter;
    node.QueuedTime = queuedTime;
    node.UseProfilingDelta = false;
    node.ProfilingDeltaNS = 0;
    node.Event = event;

    if( device )
    {
        const SDeviceInfo& deviceInfo = m_DeviceInfoMap[device];

        // Note: Even though ideally the intercept timer and the host timer should advance
        // at a consistent rate and hence the delta between the two timers should remain
        // constant, empirically this does not appear to be the case.  Synchronizing the
        // two timers is relatively inexpensive, and reduces the timer drift, so compute
        // the current delta for each event.

        if( deviceInfo.HasDeviceAndHostTimer )
        {
            // These conditions should have been checked for HasDeviceAndHostTimer to be true:
            CLI_ASSERT( deviceInfo.NumericVersion >= CL_MAKE_VERSION_KHR(2, 1, 0) );
            CLI_ASSERT( dispatch().clGetHostTimer );

            using ns = std::chrono::nanoseconds;
            const uint64_t  interceptTimeStartNS =
                std::chrono::duration_cast<ns>(clock::now().time_since_epoch()).count();

            cl_ulong    hostTimeNS = 0;
            dispatch().clGetHostTimer(
                device,
                &hostTimeNS);

            const uint64_t  interceptTimeEndNS =
                std::chrono::duration_cast<ns>(clock::now().time_since_epoch()).count();

            const int64_t   interceptHostTimeDeltaNS =
                ( interceptTimeEndNS - interceptTimeStartNS ) / 2 +
                ( interceptTimeStartNS - hostTimeNS );

            node.UseProfilingDelta = true;
            node.ProfilingDeltaNS =
                interceptHostTimeDeltaNS -
                deviceInfo.DeviceHostTimeDeltaNS;

            //logf( "Current Profiling Delta is %lld ns (%.2f us, %.2f ms)\n"
            //    "\tIntercept to Host Timer delta: %lld ns (%.2f us, %.2f ms)\n"
            //    "\tIntercept Start %llu ns, Intercept End %llu ns (delta %lld ns)\n"
            //    "\tHost %llu ns\n",
            //    node.ProfilingDeltaNS, node.ProfilingDeltaNS / 1000.0, node.ProfilingDeltaNS / 1000000.0,
            //    interceptHostTimeDeltaNS, interceptHostTimeDeltaNS / 1000.0, interceptHostTimeDeltaNS / 1000000.0,
            //    interceptTimeStartNS, interceptTimeEndNS, interceptTimeEndNS - interceptTimeStartNS,
            //    hostTimeNS );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::checkTimingEvents()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    CEventList::iterator    current = m_EventList.begin();
    CEventList::iterator    next;

    while( current != m_EventList.end() )
    {
        cl_int  errorCode = CL_SUCCESS;
        cl_int  eventStatus = 0;

        next = current;
        ++next;

        const SEventListNode& node = *current;

        errorCode = dispatch().clGetEventInfo(
            node.Event,
            CL_EVENT_COMMAND_EXECUTION_STATUS,
            sizeof( eventStatus ),
            &eventStatus,
            NULL );

        switch( errorCode )
        {
        case CL_SUCCESS:
            if( eventStatus == CL_COMPLETE )
            {
                if( config().DevicePerformanceTiming ||
                    config().ITTPerformanceTiming ||
                    config().ChromePerformanceTiming )
                {
                    cl_ulong    commandQueued = 0;
                    cl_ulong    commandSubmit = 0;
                    cl_ulong    commandStart = 0;
                    cl_ulong    commandEnd = 0;

                    errorCode |= dispatch().clGetEventProfilingInfo(
                        node.Event,
                        CL_PROFILING_COMMAND_QUEUED,
                        sizeof( commandQueued ),
                        &commandQueued,
                        NULL );
                    errorCode |= dispatch().clGetEventProfilingInfo(
                        node.Event,
                        CL_PROFILING_COMMAND_SUBMIT,
                        sizeof( commandSubmit ),
                        &commandSubmit,
                        NULL );
                    errorCode |= dispatch().clGetEventProfilingInfo(
                        node.Event,
                        CL_PROFILING_COMMAND_START,
                        sizeof( commandStart ),
                        &commandStart,
                        NULL );
                    errorCode |= dispatch().clGetEventProfilingInfo(
                        node.Event,
                        CL_PROFILING_COMMAND_END,
                        sizeof( commandEnd ),
                        &commandEnd,
                        NULL );
                    if( errorCode == CL_SUCCESS )
                    {
                        cl_ulong delta = commandEnd - commandStart;

                        SDeviceTimingStats& deviceTimingStats = m_DeviceTimingStatsMap[node.Device][node.Name];

                        deviceTimingStats.NumberOfCalls++;
                        deviceTimingStats.TotalNS += delta;
                        deviceTimingStats.MinNS = std::min< cl_ulong >( deviceTimingStats.MinNS, delta );
                        deviceTimingStats.MaxNS = std::max< cl_ulong >( deviceTimingStats.MaxNS, delta );

                        //uint64_t    numberOfCalls = deviceTimingStats.NumberOfCalls;

                        if( config().DevicePerformanceTimeLogging )
                        {
                            cl_ulong    queuedDelta = commandSubmit - commandQueued;
                            cl_ulong    submitDelta = commandStart - commandSubmit;

                            std::ostringstream  ss;

                            ss << "Device Time for "
                                //<< "call " << numberOfCalls << " to "
                                << node.Name << " (enqueue " << node.EnqueueCounter << ") = "
                                << queuedDelta << " ns (queued -> submit), "
                                << submitDelta << " ns (submit -> start), "
                                << delta << " ns (start -> end)\n";

                            log( ss.str() );
                        }

                        if( config().DevicePerformanceTimelineLogging )
                        {
                            std::ostringstream  ss;

                            ss << "Device Timeline for "
                                //<< "call " << numberOfCalls << " to "
                                << node.Name << " (enqueue " << node.EnqueueCounter << ") = "
                                << commandQueued << " ns (queued), "
                                << commandSubmit << " ns (submit), "
                                << commandStart << " ns (start), "
                                << commandEnd << " ns (end)\n";

                            log( ss.str() );
                        }

#if defined(USE_ITT)
                        if( config().ITTPerformanceTiming )
                        {
                            ittTraceEvent(
                                node.Name,
                                node.Event,
                                node.QueuedTime,
                                commandQueued,
                                commandSubmit,
                                commandStart,
                                commandEnd );
                        }
#endif

                        if( config().ChromePerformanceTiming )
                        {
                            bool useProfilingDelta =
                                node.UseProfilingDelta &&
                                !config().ChromePerformanceTimingEstimateQueuedTime;

                            chromeTraceEvent(
                                node.Name,
                                useProfilingDelta,
                                node.ProfilingDeltaNS,
                                node.EnqueueCounter,
                                node.QueueNumber,
                                node.QueuedTime,
                                commandQueued,
                                commandSubmit,
                                commandStart,
                                commandEnd );
                        }
                    }
                }

#if defined(USE_MDAPI)
                if( config().DevicePerfCounterEventBasedSampling )
                {
                    getMDAPICountersFromEvent(
                        node.Name,
                        node.Event );
                }
#endif

                dispatch().clReleaseEvent( node.Event );

                m_EventList.erase( current );
            }
            break;
        case CL_INVALID_EVENT:
            {
                // This is unexpected.  We retained the event when we
                // added it to the list.  Remove the event from the
                // list.
                logf( "Unexpectedly got CL_INVALID_EVENT for an event from %s!\n",
                    node.Name.c_str() );

                m_EventList.erase( current );
            }
            break;
        default:
            // nothing
            break;
        }

        current = next;
    }

#if defined(USE_MDAPI)
    if( config().DevicePerfCounterTimeBasedSampling )
    {
        getMDAPICountersFromStream();
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
cl_command_queue CLIntercept::getCommandBufferCommandQueue(
    cl_uint numQueues,
    cl_command_queue* queues,
    cl_command_buffer_khr cmdbuf )
{
    if( numQueues != 0 && queues != NULL )
    {
        return queues[0];
    }

    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_command_queue    queue = NULL;

    CCommandBufferInfoMap::iterator iter = m_CommandBufferInfoMap.find( cmdbuf );
    if( iter != m_CommandBufferInfoMap.end() )
    {
        cl_platform_id  platform = iter->second;
        if( dispatchX(platform).clGetCommandBufferInfoKHR == NULL )
        {
            getExtensionFunctionAddress(
                platform,
                "clGetCommandBufferInfoKHR" );
        }

        const auto& dispatchX = this->dispatchX(platform);
        if( dispatchX.clGetCommandBufferInfoKHR )
        {
            dispatchX.clGetCommandBufferInfoKHR(
                cmdbuf,
                CL_COMMAND_BUFFER_NUM_QUEUES_KHR,
                sizeof( numQueues ),
                &numQueues,
                NULL );
            if( numQueues == 1)
            {
                dispatchX.clGetCommandBufferInfoKHR(
                    cmdbuf,
                    CL_COMMAND_BUFFER_QUEUES_KHR,
                    sizeof( queue ),
                    &queue,
                    NULL );
            }
            else if( numQueues > 1 )
            {
                std::vector<cl_command_queue> queues(numQueues);
                dispatchX.clGetCommandBufferInfoKHR(
                    cmdbuf,
                    CL_COMMAND_BUFFER_QUEUES_KHR,
                    queues.size() * sizeof(queues[0]),
                    queues.data(),
                    NULL );
                queue = queues[0];
            }
        }
    }

    return queue;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_command_queue CLIntercept::createCommandQueueWithProperties(
    cl_context context,
    cl_device_id device,
    const cl_queue_properties* properties,
    cl_int* errcode_ret )
{
    cl_command_queue    retVal = NULL;

    // Cache the device info if it's not cached already.
    cacheDeviceInfo( device );

    const SDeviceInfo& deviceInfo = m_DeviceInfoMap[device];

    // First, check if this is an OpenCL 2.0 or newer device.  If it is, we can
    // simply call the clCreateCommandQueueWithProperties function.
    if( retVal == NULL &&
        deviceInfo.NumericVersion >= CL_MAKE_VERSION_KHR(2, 0, 0) )
    {
        retVal = dispatch().clCreateCommandQueueWithProperties(
            context,
            device,
            properties,
            errcode_ret );
    }

    // If this didn't work, try to use the create command queue with properties
    // extension.
    if( retVal == NULL &&
        deviceInfo.Supports_cl_khr_create_command_queue )
    {
        cl_platform_id  platform = getPlatform(device);
        if( dispatchX(platform).clCreateCommandQueueWithPropertiesKHR == NULL )
        {
            getExtensionFunctionAddress(
                platform,
                "clCreateCommandQueueWithPropertiesKHR" );
        }

        const auto& dispatchX = this->dispatchX(platform);
        if( dispatchX.clCreateCommandQueueWithPropertiesKHR )
        {
            retVal = dispatchX.clCreateCommandQueueWithPropertiesKHR(
                context,
                device,
                properties,
                errcode_ret );
        }
    }

    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addSubDeviceInfo(
    const cl_device_id parentDevice,
    const cl_device_id* devices,
    cl_uint numSubDevices )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    while( numSubDevices-- )
    {
        cl_device_id    device = devices[ numSubDevices ];
        if( device )
        {
            SSubDeviceInfo& subDeviceInfo = m_SubDeviceInfoMap[ device ];

            subDeviceInfo.ParentDevice = parentDevice;
            subDeviceInfo.SubDeviceIndex = numSubDevices;

            // Currently, information about a device is assumed to be
            // invariant, though for sub-device handles the information
            // about a device can change if sub-device handles are recycled.
            // Since this is expected to occur rarely, for now simply log a
            // warning if this occurs.
            if( m_DeviceInfoMap.find(device) != m_DeviceInfoMap.end() )
            {
                logf( "Warning: found a recycled sub-device handle %p!\n",
                    device );
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::checkRemoveDeviceInfo(
    cl_device_id device )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_uint refCount = getRefCount( device );
    if( refCount == 1 )
    {
        m_SubDeviceInfoMap.erase( device );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addKernelInfo(
    const cl_kernel kernel,
    const cl_program program,
    const std::string& kernelName )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    const SProgramInfo& programInfo = m_ProgramInfoMap[ program ];

    SKernelInfo& kernelInfo = m_KernelInfoMap[ kernel ];
    std::string demangledName = config().DemangleKernelNames ?
        demangle(kernelName) :
        kernelName;

    kernelInfo.KernelName = demangledName;

    kernelInfo.ProgramHash = programInfo.ProgramHash;
    kernelInfo.OptionsHash = programInfo.OptionsHash;

    kernelInfo.ProgramNumber = programInfo.ProgramNumber;
    kernelInfo.CompileCount = programInfo.CompileCount - 1;

    addShortKernelName( demangledName );
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addKernelInfo(
    const cl_kernel* kernels,
    const cl_program program,
    cl_uint numKernels )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    const SProgramInfo& programInfo = m_ProgramInfoMap[ program ];

    while( numKernels-- )
    {
        cl_kernel   kernel = kernels[ numKernels ];
        char*       kernelName = NULL;
        size_t      kernelNameSize = 0;
        cl_int      errorCode = CL_SUCCESS;

        errorCode = dispatch().clGetKernelInfo(
            kernel,
            CL_KERNEL_FUNCTION_NAME,
            0,
            NULL,
            &kernelNameSize );
        if( errorCode == CL_SUCCESS )
        {
            kernelName = new char[ kernelNameSize + 1 ];
            if( kernelName )
            {
                errorCode = dispatch().clGetKernelInfo(
                    kernel,
                    CL_KERNEL_FUNCTION_NAME,
                    kernelNameSize,
                    kernelName,
                    NULL );
                if( errorCode == CL_SUCCESS )
                {
                    kernelName[ kernelNameSize ] = 0;

                    SKernelInfo& kernelInfo = m_KernelInfoMap[ kernel ];
                    std::string demangledName = config().DemangleKernelNames ?
                        demangle(kernelName) :
                        kernelName;

                    kernelInfo.KernelName = demangledName;

                    kernelInfo.ProgramHash = programInfo.ProgramHash;
                    kernelInfo.OptionsHash = programInfo.OptionsHash;

                    kernelInfo.ProgramNumber = programInfo.ProgramNumber;
                    kernelInfo.CompileCount = programInfo.CompileCount - 1;

                    addShortKernelName( demangledName );
                }

                delete [] kernelName;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addKernelInfo(
    const cl_kernel kernel,
    const cl_kernel source_kernel )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    m_KernelInfoMap[ kernel ] = m_KernelInfoMap[ source_kernel ];
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::checkRemoveKernelInfo( cl_kernel kernel )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_uint refCount = getRefCount( kernel );
    if( refCount == 1 )
    {
#if 0
        // We shouldn't remove the kernel name from the local kernel name map
        // here since the mapping may be included in the device performance
        // time report.
        m_LongKernelNameMap.erase( m_KernelInfoMap[ kernel ].KernelName );
#endif

        m_KernelInfoMap.erase( kernel );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addAcceleratorInfo(
    cl_accelerator_intel accelerator,
    cl_context context )
{
    if( accelerator )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        m_AcceleratorInfoMap[accelerator] = getPlatform(context);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::checkRemoveAcceleratorInfo(
    cl_accelerator_intel accelerator )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    CAcceleratorInfoMap::iterator iter = m_AcceleratorInfoMap.find( accelerator );
    if( iter != m_AcceleratorInfoMap.end() )
    {
        cl_platform_id  platform = iter->second;
        if( dispatchX(platform).clGetAcceleratorInfoINTEL == NULL )
        {
            getExtensionFunctionAddress(
                platform,
                "clGetAcceleratorInfoINTEL" );
        }

        const auto& dispatchX = this->dispatchX(platform);
        if( dispatchX.clGetAcceleratorInfoINTEL )
        {
            cl_uint refCount = 0;
            cl_int  errorCode = CL_SUCCESS;

            errorCode = dispatchX.clGetAcceleratorInfoINTEL(
                accelerator,
                CL_ACCELERATOR_REFERENCE_COUNT_INTEL,
                sizeof( refCount ),
                &refCount,
                NULL );
            if( errorCode == CL_SUCCESS && refCount == 1 )
            {
                m_AcceleratorInfoMap.erase( iter );
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addSemaphoreInfo(
    cl_semaphore_khr semaphore,
    cl_context context )
{
    if( semaphore )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        m_SemaphoreInfoMap[semaphore] = getPlatform(context);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::checkRemoveSemaphoreInfo(
    cl_semaphore_khr semaphore )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    CSemaphoreInfoMap::iterator iter = m_SemaphoreInfoMap.find( semaphore );
    if( iter != m_SemaphoreInfoMap.end() )
    {
        cl_platform_id  platform = iter->second;
        if( dispatchX(platform).clGetSemaphoreInfoKHR == NULL )
        {
            getExtensionFunctionAddress(
                platform,
                "clGetSemaphoreInfoKHR" );
        }

        const auto& dispatchX = this->dispatchX(platform);
        if( dispatchX.clGetSemaphoreInfoKHR )
        {
            cl_uint refCount = 0;
            cl_int  errorCode = CL_SUCCESS;
            errorCode = dispatchX.clGetSemaphoreInfoKHR(
                semaphore,
                CL_SEMAPHORE_REFERENCE_COUNT_KHR,
                sizeof( refCount ),
                &refCount,
                NULL );
            if( errorCode == CL_SUCCESS && refCount == 1 )
            {
                m_SemaphoreInfoMap.erase( iter );
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addCommandBufferInfo(
    cl_command_buffer_khr cmdbuf,
    cl_command_queue queue )
{
    if( cmdbuf )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        m_CommandBufferInfoMap[cmdbuf] = getPlatform(queue);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::checkRemoveCommandBufferInfo(
    cl_command_buffer_khr cmdbuf)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    CCommandBufferInfoMap::iterator iter = m_CommandBufferInfoMap.find( cmdbuf );
    if( iter != m_CommandBufferInfoMap.end() )
    {
        cl_platform_id  platform = iter->second;
        if( dispatchX(platform).clGetCommandBufferInfoKHR == NULL )
        {
            getExtensionFunctionAddress(
                platform,
                "clGetCommandBufferInfoKHR" );
        }

        const auto& dispatchX = this->dispatchX(platform);
        if( dispatchX.clGetCommandBufferInfoKHR )
        {
            cl_uint refCount = 0;
            cl_int  errorCode = CL_SUCCESS;
            errorCode = dispatchX.clGetCommandBufferInfoKHR(
                cmdbuf,
                CL_COMMAND_BUFFER_REFERENCE_COUNT_KHR,
                sizeof( refCount ),
                &refCount,
                NULL );
            if( errorCode == CL_SUCCESS && refCount == 1 )
            {
                m_CommandBufferInfoMap.erase( iter );

                CCommandBufferMutableCommandsMap::iterator cmditer =
                    m_CommandBufferMutableCommandsMap.find( cmdbuf );
                if( cmditer != m_CommandBufferMutableCommandsMap.end() )
                {
                    CMutableCommandList&    cmdList = cmditer->second;
                    for( auto cmd : cmdList )
                    {
                        m_MutableCommandInfoMap.erase(cmd);
                    }

                    m_CommandBufferMutableCommandsMap.erase(cmditer);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addMutableCommandInfo(
    cl_mutable_command_khr cmd,
    cl_command_buffer_khr cmdbuf,
    cl_uint dim )
{
    if( cmd && cmdbuf )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        SMutableCommandInfo& info = m_MutableCommandInfoMap[cmd];

        info.Platform = getPlatform(cmdbuf);
        info.WorkDim = dim;

        m_CommandBufferMutableCommandsMap[cmdbuf].push_back(cmd);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addSamplerString(
    cl_sampler sampler,
    const std::string& str )
{
    if( sampler )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_SamplerDataMap[sampler] = str;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::checkRemoveSamplerString(
    cl_sampler sampler )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_uint refCount = getRefCount( sampler );
    if( refCount == 1 )
    {
        m_SamplerDataMap.erase( sampler );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::checkGetSamplerString(
    size_t size,
    const void *arg_value,
    std::string& str ) const
{
    bool found = false;

    if( ( arg_value != NULL ) && ( size == sizeof( cl_sampler ) ) )
    {
        const cl_sampler sampler = *(const cl_sampler *)arg_value;

        CSamplerDataMap::const_iterator iter = m_SamplerDataMap.find( sampler );
        if( iter != m_SamplerDataMap.end() )
        {
            str = iter->second;
            found = true;
        }
    }

    return found;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addQueue(
    cl_context context,
    cl_command_queue queue )
{
    if( queue )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        m_QueueNumberMap[ queue ] = m_QueueNumber + 1;  // should be nonzero
        m_QueueNumber++;

        m_ContextQueuesMap[context].push_back(queue);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::checkRemoveQueue(
    cl_command_queue queue )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_uint refCount = getRefCount( queue );
    if( refCount == 1 )
    {
        m_QueueNumberMap.erase( queue );

        cl_context  context = NULL;

        cl_int errorCode = dispatch().clGetCommandQueueInfo(
            queue,
            CL_QUEUE_CONTEXT,
            sizeof(context),
            &context,
            NULL );
        if( errorCode == CL_SUCCESS && context )
        {
            CQueueList& queues = m_ContextQueuesMap[context];

            queues.erase(
                std::find(
                    queues.begin(),
                    queues.end(),
                    queue ) );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addEvent(
    cl_event event,
    uint64_t enqueueCounter )
{
    if( event )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        m_EventIdMap[ event ] = enqueueCounter;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::checkRemoveEvent(
    cl_event event )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_uint refCount = getRefCount( event );
    if( refCount == 1 )
    {
        m_EventIdMap.erase( event );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addBuffer(
    cl_mem buffer )
{
    if( buffer )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        cl_int  errorCode = CL_SUCCESS;
        size_t  size = 0;

        errorCode |= dispatch().clGetMemObjectInfo(
            buffer,
            CL_MEM_SIZE,
            sizeof( size_t ),
            &size,
            NULL );

        if( errorCode == CL_SUCCESS )
        {
            m_MemAllocNumberMap[ buffer ] = m_MemAllocNumber;
            m_BufferInfoMap[ buffer ] = size;
            m_MemAllocNumber++;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addImage(
    cl_mem image )
{
    if( image )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        cl_int  errorCode = CL_SUCCESS;

        size_t  width = 0;
        size_t  height = 0;
        size_t  depth = 0;
        size_t  arraySize = 0;
        size_t  elementSize = 0;
        size_t  rowPitch = 0;
        size_t  slicePitch = 0;
        cl_image_format format;

        errorCode |= dispatch().clGetImageInfo(
            image,
            CL_IMAGE_WIDTH,
            sizeof(width),
            &width,
            NULL );
        errorCode |= dispatch().clGetImageInfo(
            image,
            CL_IMAGE_HEIGHT,
            sizeof(height),
            &height,
            NULL );
        errorCode |= dispatch().clGetImageInfo(
            image,
            CL_IMAGE_DEPTH,
            sizeof(depth),
            &depth,
            NULL );
        errorCode |= dispatch().clGetImageInfo(
            image,
            CL_IMAGE_ARRAY_SIZE,
            sizeof(arraySize),
            &arraySize,
            NULL );
        errorCode |= dispatch().clGetImageInfo(
            image,
            CL_IMAGE_ELEMENT_SIZE,
            sizeof(elementSize),
            &elementSize,
            NULL );
        errorCode |= dispatch().clGetImageInfo(
            image,
            CL_IMAGE_ROW_PITCH,
            sizeof(rowPitch),
            &rowPitch,
            nullptr );
        errorCode |= dispatch().clGetImageInfo(
            image,
            CL_IMAGE_SLICE_PITCH,
            sizeof(slicePitch),
            &slicePitch,
            nullptr );
        errorCode |= dispatch().clGetImageInfo(
            image,
            CL_IMAGE_FORMAT,
            sizeof(cl_image_format),
            &format,
            nullptr );

        if( errorCode == CL_SUCCESS )
        {
            SImageInfo  imageInfo;

            imageInfo.Region[0] = width;
            if( height == 0 )
            {
                if( arraySize == 0 )
                {
                    imageInfo.Region[1] = 1;            // 1D image
                    imageInfo.ImageType = CL_MEM_OBJECT_IMAGE1D;
                }
                else
                {
                    imageInfo.Region[1] = arraySize;    // 1D image array
                    imageInfo.ImageType = CL_MEM_OBJECT_IMAGE1D_ARRAY;
                }
            }
            else
            {
                imageInfo.Region[1] = height;           // 2D image, 3D image, or 3D image array
            }

            if( depth == 0 )
            {
                if( arraySize == 0 )
                {
                    imageInfo.Region[2] = 1;            // 2D image
                    imageInfo.ImageType = CL_MEM_OBJECT_IMAGE2D;
                }
                else
                {
                    imageInfo.Region[2] = arraySize;    // 2D image array
                    imageInfo.ImageType = CL_MEM_OBJECT_IMAGE2D_ARRAY;
                }
            }
            else
            {
                // What about an array of 3D images?
                imageInfo.Region[2] = depth;            // 3D image
                imageInfo.ImageType = CL_MEM_OBJECT_IMAGE3D;
            }

            imageInfo.ElementSize = elementSize;
            imageInfo.Format = format;
            imageInfo.RowPitch = rowPitch;
            imageInfo.SlicePitch = slicePitch;

            m_MemAllocNumberMap[ image ] = m_MemAllocNumber;
            m_ImageInfoMap[ image ] = imageInfo;
            m_MemAllocNumber++;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::checkRemoveMemObj(
    cl_mem memobj )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_uint refCount = getRefCount( memobj );
    if( refCount == 1 )
    {
        m_MemAllocNumberMap.erase( memobj );
        m_BufferInfoMap.erase( memobj );
        m_ImageInfoMap.erase( memobj );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addSVMAllocation(
    void* svmPtr,
    size_t size )
{
    if( svmPtr )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        m_MemAllocNumberMap[ svmPtr ] = m_MemAllocNumber;
        m_SVMAllocInfoMap[ svmPtr ] = size;
        m_MemAllocNumber++;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::removeSVMAllocation(
    void* svmPtr )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    m_MemAllocNumberMap.erase( svmPtr );
    m_SVMAllocInfoMap.erase( svmPtr );
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addUSMAllocation(
    void* usmPtr,
    size_t size )
{
    if( usmPtr )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        m_MemAllocNumberMap[ usmPtr ] = m_MemAllocNumber;
        m_USMAllocInfoMap[ usmPtr ] = size;
        m_MemAllocNumber++;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::removeUSMAllocation(
    void* usmPtr )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    m_MemAllocNumberMap.erase( usmPtr );
    m_USMAllocInfoMap.erase( usmPtr );
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::setKernelArg(
    cl_kernel kernel,
    cl_uint arg_index,
    cl_mem memobj )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if( m_MemAllocNumberMap.find( memobj ) != m_MemAllocNumberMap.end() )
    {
        CKernelArgMemMap&   kernelArgMap = m_KernelArgMap[ kernel ];
        kernelArgMap[ arg_index ] = memobj;
    }
}

void CLIntercept::setKernelArg(
    cl_kernel kernel,
    cl_uint arg_index,
    const void* arg_value,
    size_t arg_size )
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (arg_value != nullptr)
    {
        (m_KernelArgVectorMap[kernel])[arg_index] =
            std::vector<unsigned char>(reinterpret_cast<const unsigned char*>(arg_value),
                                       reinterpret_cast<const unsigned char*>(arg_value) + arg_size);
        return;
    }
    // Run time __local buffers
    (m_KernelArgLocalMap[kernel])[arg_index] = arg_size;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::setKernelArgSVMPointer(
    cl_kernel kernel,
    cl_uint arg_index,
    const void* arg )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    // Unlike clSetKernelArg(), which must pass a cl_mem, clSetKernelArgSVMPointer
    // can pass a pointer to the base of a SVM allocation or anywhere inside of
    // an SVM allocation.  As a result, we need to search the SVM map to find the
    // base address and size of the SVM allocation.

    CKernelArgMemMap&   kernelArgMap = m_KernelArgMap[ kernel ];

    CSVMAllocInfoMap::iterator iter = m_SVMAllocInfoMap.lower_bound( arg );
    if( iter->first != arg && iter != m_SVMAllocInfoMap.begin() )
    {
        // Go to the previous iterator.
        --iter;
    }

    const void* startPtr = iter->first;
    const void* endPtr = (const char*)startPtr + iter->second;
    if( arg >= startPtr && arg < endPtr )
    {
        kernelArgMap[ arg_index ] = startPtr;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::setKernelArgUSMPointer(
    cl_kernel kernel,
    cl_uint arg_index,
    const void* arg )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    CKernelArgMemMap&   kernelArgMap = m_KernelArgMap[ kernel ];

    CUSMAllocInfoMap::iterator iter = m_USMAllocInfoMap.lower_bound( arg );
    if( iter->first != arg && iter != m_USMAllocInfoMap.begin() )
    {
        // Go to the previous iterator.
        --iter;
    }

    const void* startPtr = iter->first;
    const void* endPtr = (const char*)startPtr + iter->second;
    if( arg >= startPtr && arg < endPtr )
    {
        kernelArgMap[ arg_index ] = startPtr;
    }
}

void CLIntercept::dumpKernelSourceOrDeviceBinary( cl_kernel kernel,
                                                  uint64_t enqueueCounter,
                                                  bool byKernelName )
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::string fileNamePrefix = "";
    OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileNamePrefix );
    fileNamePrefix += "/Replay/Enqueue_";
    if (byKernelName)
        fileNamePrefix += getShortKernelName(kernel);
    else
        fileNamePrefix += std::to_string(enqueueCounter);
    fileNamePrefix += "/";
    OS().MakeDumpDirectories( fileNamePrefix );

    cl_program tmp_program;
    dispatch().clGetKernelInfo(kernel, CL_KERNEL_PROGRAM, sizeof(cl_program), &tmp_program, nullptr);

    size_t size = 0;
    dispatch().clGetProgramInfo(tmp_program, CL_PROGRAM_SOURCE, sizeof(char*), nullptr, &size);

    std::string sourceCode(size, ' ');
    int error = dispatch().clGetProgramInfo(tmp_program, CL_PROGRAM_SOURCE, size, &sourceCode[0], nullptr);

    if (error == CL_SUCCESS && size > 1)
    {
        std::ofstream output(fileNamePrefix + "kernel.cl", std::ios::out | std::ios::binary);
        output.write(sourceCode.c_str(), size);
        return;
    }

    log("[[Warning]]: Kernel source is not available! Make sure that the kernel is compiled from source (and is not cached)\n");
    log("Now will try to output binaries, these probably won't work on other platforms!\n");

    cl_uint num_devices;
    dispatch().clGetProgramInfo(tmp_program, CL_PROGRAM_NUM_DEVICES, sizeof(cl_uint), &num_devices, nullptr);

    // Grab the device ids
    std::vector<cl_device_id> devices(num_devices);

    dispatch().clGetProgramInfo(tmp_program, CL_PROGRAM_DEVICES, num_devices * sizeof(cl_device_id), devices.data(), 0);

    std::vector<size_t> sizes(num_devices);
    dispatch().clGetProgramInfo(tmp_program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t) * sizes.size(), sizes.data(), nullptr);

    std::vector<std::vector<unsigned char>> binaries;
    std::vector<unsigned char *> binariesDatas;
    for (size_t device = 0; device != num_devices; ++device)
    {
        std::vector<unsigned char> binary(sizes[device]);
        binaries.emplace_back(binary);
        binariesDatas.emplace_back(binaries[device].data());
    }

    error = dispatch().clGetProgramInfo(tmp_program, CL_PROGRAM_BINARIES, binariesDatas.size() * sizeof(unsigned char*), binariesDatas.data(), nullptr);

    for (size_t device = 0; device != num_devices; ++device)
    {
        std::ofstream output(fileNamePrefix + "DeviceBinary" + std::to_string(device) + ".bin", std::ios::out | std::ios::binary);
        output.write(reinterpret_cast<char const*>(binaries[device].data()), binaries[device].size());
    }
}

void CLIntercept::dumpKernelInfo(
    cl_kernel kernel,
    uint64_t enqueueCounter,
    size_t work_dim,
    const size_t* gws_offset,
    const size_t* gws,
    const size_t* lws,
    bool byKernelName)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::string fileNamePrefix = "";
    OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileNamePrefix );
    fileNamePrefix += "/Replay/Enqueue_";
    if (byKernelName)
        fileNamePrefix += getShortKernelName(kernel);
    else
        fileNamePrefix += std::to_string(enqueueCounter);
    fileNamePrefix += "/";
    OS().MakeDumpDirectories( fileNamePrefix );
    std::ofstream output{fileNamePrefix + "worksizes.txt"};

    // Print the values of the worksizes and offsets on a line in the order:
    // gws
    // lws
    // gws_offset
    for (unsigned idx = 0; idx != work_dim; ++idx)
    {
        output << (!gws        ? 0 : gws[idx]) << ' ';
    }
    output << '\n';

    for (unsigned idx = 0; idx != work_dim; ++idx)
    {
        output << (!lws        ? 0 : lws[idx]) << ' ';
    }
    output << '\n';

    for (unsigned idx = 0; idx != work_dim; ++idx)
    {
        output << (!gws_offset ? 0 : gws_offset[idx]) << ' ';
    }
    output << '\n';

    cl_program tmp_program;
    dispatch().clGetKernelInfo(kernel, CL_KERNEL_PROGRAM, sizeof(cl_program), &tmp_program, nullptr);

    cl_context context;
    dispatch().clGetProgramInfo(tmp_program, CL_PROGRAM_CONTEXT, sizeof(cl_context), &context, nullptr);

    cl_device_id device_ids;
    dispatch().clGetContextInfo(context, CL_CONTEXT_DEVICES, sizeof(cl_context_info*), &device_ids, nullptr);

    size_t sizeOfOptions = 0;
    dispatch().clGetProgramBuildInfo(tmp_program, device_ids,
                                                 CL_PROGRAM_BUILD_OPTIONS, sizeof(char*), nullptr, &sizeOfOptions);

    std::string optionsString(sizeOfOptions, ' ');
    dispatch().clGetProgramBuildInfo(tmp_program, device_ids,
                                                 CL_PROGRAM_BUILD_OPTIONS, sizeOfOptions, &optionsString[0], &sizeOfOptions);

    std::ofstream outputBuildOptions{fileNamePrefix + "buildOptions.txt", std::ios::out | std::ios::binary};
    outputBuildOptions.write(optionsString.c_str(), optionsString.length() - 1);

    std::string knlName = getShortKernelName(kernel);
    std::ofstream outputKnlName{fileNamePrefix + "knlName.txt"};
    outputKnlName << knlName;

    const char* pPythonScript = NULL;
    size_t pythonScriptLength = 0;
    if( m_OS.GetReplayScriptString(
            pPythonScript,
            pythonScriptLength ) )
    {
        std::ofstream outputPythonScript{fileNamePrefix + "run.py", std::ios::out | std::ios::binary};
        outputPythonScript.write(pPythonScript, pythonScriptLength);
    }

    std::ofstream outputKernelNumber{fileNamePrefix + "enqueueNumber.txt"};
    outputKernelNumber << std::to_string(enqueueCounter) << '\n';

    cl_uint numArgs = 0;
    dispatch().clGetKernelInfo(kernel, CL_KERNEL_NUM_ARGS, sizeof(cl_uint), &numArgs, nullptr);

    std::ofstream outputArgTypes{fileNamePrefix + "ArgumentDataTypes.txt"};
    for ( cl_uint idx = 0; idx != numArgs; ++idx )
    {
        size_t argNameSize = 0;
        dispatch().clGetKernelArgInfo(kernel, idx, CL_KERNEL_ARG_TYPE_NAME, 0, nullptr, &argNameSize);

        std::string argName(argNameSize, ' ');
        int error = dispatch().clGetKernelArgInfo(kernel, idx, CL_KERNEL_ARG_TYPE_NAME, argNameSize, &argName[0], nullptr);
        if ( error == CL_KERNEL_ARG_INFO_NOT_AVAILABLE )
        {
            log("Note: Kernel Argument info not available for replaying.\n");
            return;
        }
        outputArgTypes << argName << '\n';
    }
}

void CLIntercept::dumpArgumentsForKernel(
        cl_kernel kernel,
        uint64_t enqueueCounter,
        bool byKernelName )
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::string fileNamePrefix;
    OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileNamePrefix );
    fileNamePrefix += "/Replay/Enqueue_";
    if( byKernelName )
    {
        fileNamePrefix += getShortKernelName(kernel);
    }
    else
    {
        fileNamePrefix += std::to_string(enqueueCounter);
    }
    fileNamePrefix += "/";
    OS().MakeDumpDirectories( fileNamePrefix );

    const auto& argumentVectorMap = m_KernelArgVectorMap[kernel];
    for( const auto& arg: argumentVectorMap )
    {
        const auto pos = arg.first;
        const auto& value = arg.second;
        std::string fileName = fileNamePrefix + "Argument" + std::to_string(pos) + ".bin";
        std::ofstream out{fileName, std::ios::out | std::ios::binary};
        out.write(reinterpret_cast<char const*>(value.data()), value.size());
    }

    const auto& localMemSizes = m_KernelArgLocalMap[kernel];
    for( const auto& arg: localMemSizes )
    {
        const auto pos = arg.first;
        const auto value = arg.second;
        std::string fileName = fileNamePrefix + "Local" + std::to_string(pos) + ".txt";
        std::ofstream out{fileName};
        out << std::to_string(value);
    }

    const auto& samplerValues = m_samplerKernelArgMap[kernel];
    for( const auto& arg: samplerValues)
    {
        const auto pos = arg.first;
        const auto& value = arg.second;
        std::string fileName = fileNamePrefix + "Sampler" + std::to_string(pos) + ".txt";
        std::ofstream out{fileName};
        out << value;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpBuffersForKernel(
    const std::string& name,
    const uint64_t enqueueCounter,
    cl_kernel kernel,
    cl_command_queue command_queue,
    bool replay,
    bool byKernelName )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_platform_id  platform = getPlatform(kernel);

    std::vector<char>   transferBuf;
    std::string fileNamePrefix = "";

    if (replay)
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileNamePrefix );
        fileNamePrefix += "/Replay/Enqueue_";
        if (byKernelName)
        {
            fileNamePrefix += getShortKernelName(kernel);
        }
        else
        {
            fileNamePrefix += std::to_string(enqueueCounter);
        }
        fileNamePrefix += "/";
        OS().MakeDumpDirectories( fileNamePrefix );
    }
    else
    {
        // Get the dump directory name.
        {
            OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileNamePrefix );
            fileNamePrefix += "/memDump";
            fileNamePrefix += name;
            fileNamePrefix += "Enqueue/";
        }

        // Now make directories as appropriate.
        {
            OS().MakeDumpDirectories( fileNamePrefix );
        }
    }

    CKernelArgMemMap&   kernelArgMemMap = m_KernelArgMap[ kernel ];
    CKernelArgMemMap::iterator  i = kernelArgMemMap.begin();
    while( i != kernelArgMemMap.end() )
    {
        CLI_C_ASSERT( sizeof(void*) == sizeof(cl_mem) );
        cl_uint arg_index = (*i).first;
        void*   allocation = (void*)(*i).second;
        cl_mem  memobj = (cl_mem)allocation;
        ++i;
        if( ( m_USMAllocInfoMap.find( allocation ) != m_USMAllocInfoMap.end() ) ||
            ( m_SVMAllocInfoMap.find( allocation ) != m_SVMAllocInfoMap.end() ) ||
            ( m_BufferInfoMap.find( memobj ) != m_BufferInfoMap.end() ) )
        {
            unsigned int        number = m_MemAllocNumberMap[ memobj ];

            std::string fileName = fileNamePrefix;
            if (replay)
            {
                fileName += "Buffer" + std::to_string(arg_index) + ".bin";
            }
            else
            {
                char    tmpStr[ MAX_PATH ];

                // Add the enqueue count to file name
                {
                    CLI_SPRINTF( tmpStr, MAX_PATH, "%04u",
                        (unsigned int)enqueueCounter );

                    fileName += "Enqueue_";
                    fileName += tmpStr;
                }

                // Add the kernel name to the filename
                {
                    fileName += "_Kernel_";
                    fileName += getShortKernelName(kernel);
                }

                // Add the arg number to the file name
                {
                    CLI_SPRINTF( tmpStr, MAX_PATH, "%u", arg_index );

                    fileName += "_Arg_";
                    fileName += tmpStr;
                }

                // Add the buffer number to the file name
                {
                    CLI_SPRINTF( tmpStr, MAX_PATH, "%04u", number );

                    fileName += "_Buffer_";
                    fileName += tmpStr;
                }

                // Add extension to file name
                {
                    fileName += ".bin";
                }
            }
            // Dump the buffer contents to the file.
            if( m_USMAllocInfoMap.find( allocation ) != m_USMAllocInfoMap.end() )
            {
                size_t  size = m_USMAllocInfoMap[ allocation ];

                if( dispatchX(platform).clEnqueueMemcpyINTEL == NULL )
                {
                    getExtensionFunctionAddress(
                        platform,
                        "clEnqueueMemcpyINTEL" );
                }
                if( transferBuf.size() < size )
                {
                    transferBuf.resize(size);
                }

                const auto& dispatchX = this->dispatchX(platform);
                if( dispatchX.clEnqueueMemcpyINTEL &&
                    transferBuf.size() >= size )
                {
                    cl_int  error = dispatchX.clEnqueueMemcpyINTEL(
                        command_queue,
                        CL_TRUE,
                        transferBuf.data(),
                        allocation,
                        size,
                        0,
                        NULL,
                        NULL );
                    if( error == CL_SUCCESS )
                    {
                        std::ofstream os;
                        os.open(
                            fileName.c_str(),
                            std::ios::out | std::ios::binary );

                        if( os.good() )
                        {
                            os.write( transferBuf.data(), size );
                            os.close();
                        }
                        else
                        {
                            logf( "Failed to open buffer dump file for writing: %s\n",
                                fileName.c_str() );
                        }
                    }
                }
            }
            else if( m_SVMAllocInfoMap.find( allocation ) != m_SVMAllocInfoMap.end() )
            {
                size_t  size = m_SVMAllocInfoMap[ allocation ];

                cl_int  error = dispatch().clEnqueueSVMMap(
                    command_queue,
                    CL_TRUE,
                    CL_MAP_READ,
                    allocation,
                    size,
                    0,
                    NULL,
                    NULL );
                if( error == CL_SUCCESS )
                {
                    std::ofstream os;
                    os.open(
                        fileName.c_str(),
                        std::ios::out | std::ios::binary );

                    if( os.good() )
                    {
                        os.write( (const char*)allocation, size );
                        os.close();
                    }
                    else
                    {
                        logf( "Failed to open buffer dump file for writing: %s\n",
                            fileName.c_str() );
                    }

                    dispatch().clEnqueueSVMUnmap(
                        command_queue,
                        allocation,
                        0,
                        NULL,
                        NULL );
                }
            }
            else if( m_BufferInfoMap.find( memobj ) != m_BufferInfoMap.end() )
            {
                size_t  size = m_BufferInfoMap[ memobj ];

                cl_int  error = CL_SUCCESS;
                void*   ptr = dispatch().clEnqueueMapBuffer(
                    command_queue,
                    memobj,
                    CL_TRUE,
                    CL_MAP_READ,
                    0,
                    size,
                    0,
                    NULL,
                    NULL,
                    &error );
                if( error == CL_SUCCESS )
                {
                    std::ofstream os;
                    os.open(
                        fileName.c_str(),
                        std::ios::out | std::ios::binary );

                    if( os.good() )
                    {
                        os.write( (const char*)ptr, size );
                        os.close();
                    }
                    else
                    {
                        logf( "Failed to open buffer dump file for writing: %s\n",
                            fileName.c_str() );
                    }

                    dispatch().clEnqueueUnmapMemObject(
                        command_queue,
                        memobj,
                        ptr,
                        0,
                        NULL,
                        NULL );
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpImagesForKernel(
    const std::string& name,
    const uint64_t enqueueCounter,
    cl_kernel kernel,
    cl_command_queue command_queue,
    bool replay,
    bool byKernelName )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    std::string fileNamePrefix = "";

    // Get the dump directory name.
    if (replay)
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileNamePrefix );
        fileNamePrefix += "/Replay/Enqueue_";
        if (byKernelName)
            fileNamePrefix += getShortKernelName(kernel);
        else
            fileNamePrefix += std::to_string(enqueueCounter);
        fileNamePrefix += "/";
        OS().MakeDumpDirectories( fileNamePrefix );
    }
    else
    {
        // Get the dump directory name.
        {
            OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileNamePrefix );
            fileNamePrefix += "/memDump";
            fileNamePrefix += name;
            fileNamePrefix += "Enqueue/";
        }

        // Now make directories as appropriate.
        {
            OS().MakeDumpDirectories( fileNamePrefix );
        }
    }

    // Now make directories as appropriate.
    {
        OS().MakeDumpDirectories( fileNamePrefix );
    }

    CKernelArgMemMap&   kernelArgMemMap = m_KernelArgMap[ kernel ];
    CKernelArgMemMap::iterator  i = kernelArgMemMap.begin();

    while( i != kernelArgMemMap.end() )
    {
        CLI_C_ASSERT( sizeof(void*) == sizeof(cl_mem) );

        cl_uint arg_index = (*i).first;
        cl_mem  memobj = (cl_mem)(*i).second;

        ++i;

        if( m_ImageInfoMap.find( memobj ) != m_ImageInfoMap.end() )
        {
            const SImageInfo&   info = m_ImageInfoMap[ memobj ];
            unsigned int        number = m_MemAllocNumberMap[ memobj ];

            std::string fileName = fileNamePrefix;
            if (replay)
            {
                fileName += "Image" + std::to_string(arg_index) + ".raw";

                // write image meta data to file
                std::ofstream metaData{fileNamePrefix + "Image_MetaData_" + std::to_string(arg_index) + ".txt"};
                metaData << info.Region[0] << '\n'
                         << info.Region[1] << '\n'
                         << info.Region[2] << '\n'
                         << info.ElementSize << '\n'
                         << info.RowPitch << '\n'
                         << info.SlicePitch << '\n'
                         << info.Format.image_channel_data_type << '\n'
                         << info.Format.image_channel_order << '\n'
                         << static_cast<int>(info.ImageType);
            }
            else
            {
                char    tmpStr[ MAX_PATH ];

                // Add the enqueue count to file name
                {
                    CLI_SPRINTF( tmpStr, MAX_PATH, "%04u",
                        (unsigned int)enqueueCounter );

                    fileName += "Enqueue_";
                    fileName += tmpStr;
                }

                // Add the kernel name to the filename
                {
                    fileName += "_Kernel_";
                    fileName += getShortKernelName(kernel);
                }

                // Add the arg number to the file name
                {
                    CLI_SPRINTF( tmpStr, MAX_PATH, "%u", arg_index );

                    fileName += "_Arg_";
                    fileName += tmpStr;
                }

                // Add the image number to the file name
                {
                    CLI_SPRINTF( tmpStr, MAX_PATH, "%04u", number );

                    fileName += "_Image_";
                    fileName += tmpStr;
                }

                // Add the image dimensions to the file name
                {
                    CLI_SPRINTF( tmpStr, MAX_PATH, "_%zux%zux%zu_%zubpp",
                        info.Region[0],
                        info.Region[1],
                        info.Region[2],
                        info.ElementSize * 8 );

                    fileName += tmpStr;
                }

                // Add extension to file name
                {
                    fileName += ".raw";
                }
            }

            // Dump the image contents to the file.
            {
                size_t  size =
                    info.Region[0] *
                    info.Region[1] *
                    info.Region[2] *
                    info.ElementSize;
                char*   readImageData = new char[ size ];

                if( readImageData )
                {
                    size_t  origin[3] = { 0, 0, 0 };
                    cl_int  error = dispatch().clEnqueueReadImage(
                        command_queue,
                        memobj,
                        CL_TRUE,
                        origin,
                        info.Region,
                        0,
                        0,
                        readImageData,
                        0,
                        NULL,
                        NULL );

                    if( error == CL_SUCCESS )
                    {
                        std::ofstream os;
                        os.open(
                            fileName.c_str(),
                            std::ios::out | std::ios::binary );

                        if( os.good() )
                        {
                            os.write( readImageData, size );
                            os.close();
                        }
                        else
                        {
                            logf( "Failed to open image dump file for writing: %s\n",
                                fileName.c_str() );
                        }
                    }

                    delete [] readImageData;
                }
            }
        }
    }
}

void CLIntercept::saveSampler(cl_kernel kernel, cl_uint arg_index, std::string const& sampler)
{
    std::unique_lock<std::mutex> lock(m_Mutex);
    auto& samplerArgMap = m_samplerKernelArgMap[kernel];
    samplerArgMap[arg_index] = sampler;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpArgument(
    const uint64_t enqueueCounter,
    cl_kernel kernel,
    cl_int arg_index,
    size_t size,
    const void *pBuffer)
{
    if( kernel )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        std::string fileName = "";

        // Get the dump directory name.
        {
            OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
            fileName += "/SetKernelArg/";
        }

        // Now make directories as appropriate.
        {
            OS().MakeDumpDirectories( fileName );
        }

        // Add the enqueue count to file name
        {
            char    enqueueCount[ MAX_PATH ];

            CLI_SPRINTF( enqueueCount, MAX_PATH, "%04u",
                (unsigned int)enqueueCounter );
            fileName += "SetKernelArg_";
            fileName += enqueueCount;
        }

        // Add the kernel name to the filename
        {
            fileName += "_Kernel_";
            fileName += getShortKernelName(kernel);
        }

        // Add the arg number to the file name
        {
            char    argName[ MAX_PATH ];

            CLI_SPRINTF( argName, MAX_PATH, "%d", arg_index );

            fileName += "_Arg_";
            fileName += argName;
        }

        // Add extension to file name
        {
            fileName += ".bin";
        }

        // Dump the buffer contents to the file.
        {
            if( pBuffer != NULL)
            {
                std::ofstream os;
                os.open(
                    fileName.c_str(),
                    std::ios_base::out | std::ios_base::binary );

                if( os.good() )
                {
                    os.write( (const char *)pBuffer, size );
                    os.close();
                }
                else
                {
                    logf( "Failed to open program arg dump file for writing: %s\n",
                        fileName.c_str() );
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpBuffer(
    const std::string& name,
    const uint64_t enqueueCounter,
    cl_mem memobj,
    cl_command_queue command_queue,
    void* ptr,
    size_t offset,
    size_t size )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if( m_BufferInfoMap.find( memobj ) != m_BufferInfoMap.end() )
    {
        std::string fileName = "";

        // Get the dump directory name.
        {
            OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
            fileName += "/memDumpCreateMapUnmap/";
        }

        // Now make directories as appropriate.
        {
            OS().MakeDumpDirectories( fileName );
        }

        fileName += name;

        // Add the buffer number to the file name
        {
            unsigned int        number = m_MemAllocNumberMap[ memobj ];

            char    bufferName[ MAX_PATH ];

            CLI_SPRINTF( bufferName, MAX_PATH, "%04u", number );

            fileName += "_Buffer_";
            fileName += bufferName;
        }

        // Add the offset to the file name
        {
            char    offsetName[ MAX_PATH ];

            CLI_SPRINTF( offsetName, MAX_PATH, "%04zu",
                offset );

            fileName += "_Offset_";
            fileName += offsetName;
        }

        // Add the enqueue count to file name
        {
            char    enqueueCount[ MAX_PATH ];

            CLI_SPRINTF( enqueueCount, MAX_PATH, "%04u",
                (unsigned int)enqueueCounter );

            fileName += "_Enqueue_";
            fileName += enqueueCount;
        }

        // Add extension to file name
        {
            fileName += ".bin";
        }

        // Dump the buffer contents to the file.
        // There are two possibilities:
        // 1) We have a pointer and size already.  This might happen
        //    when the buffer is being created or was just mapped.
        //    In this case, we can just write this to the file.
        // 2) We have no pointer or size.  This usually happens when
        //    the buffer is being unmapped.  In this case, we'll
        //    map and dump the entire buffer.
        if( ptr != NULL && size != 0 )
        {
            std::ofstream os;
            os.open(
                fileName.c_str(),
                std::ios::out | std::ios::binary );

            if( os.good() )
            {
                os.write( (const char*)ptr, size );
                os.close();
            }
            else
            {
                logf( "Failed to open buffer dump file for writing: %s\n",
                    fileName.c_str() );
            }
        }
        else
        {
            // We should have checked this already...
            CLI_ASSERT( m_BufferInfoMap.find( memobj ) != m_BufferInfoMap.end() );

            size_t  size = m_BufferInfoMap[ memobj ];

            cl_int  error = CL_SUCCESS;
            ptr = dispatch().clEnqueueMapBuffer(
                command_queue,
                memobj,
                CL_TRUE,
                CL_MAP_READ,
                0,
                size,
                0,
                NULL,
                NULL,
                &error );
            if( error == CL_SUCCESS )
            {
                std::ofstream os;
                os.open(
                    fileName.c_str(),
                    std::ios::out | std::ios::binary );

                if( os.good() )
                {
                    os.write( (const char*)ptr, size );
                    os.close();
                }
                else
                {
                    logf( "Failed to open buffer dump file for writing: %s\n",
                        fileName.c_str() );
                }

                dispatch().clEnqueueUnmapMemObject(
                    command_queue,
                    memobj,
                    ptr,
                    0,
                    NULL,
                    NULL );
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addMapPointer(
    const void* ptr,
    const cl_map_flags flags,
    const size_t size )
{
    if( ptr )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if( m_MapPointerInfoMap.find(ptr) != m_MapPointerInfoMap.end() )
        {
            log( "Ignoring duplicate mapped pointer.\n" );
        }
        else
        {
            SMapPointerInfo&    mapPointerInfo = m_MapPointerInfoMap[ptr];

            mapPointerInfo.Flags = flags;
            mapPointerInfo.Size = size;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::removeMapPointer(
    const void* ptr )
{
    if( ptr )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_MapPointerInfoMap.erase(ptr);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::checkEventList(
    const char* functionName,
    cl_uint numEvents,
    const cl_event* eventList,
    cl_event* event )
{
    if( numEvents != 0 && eventList == NULL )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        logf( "Check Events for %s: Num Events is %u, but Event List is NULL!\n",
            functionName,
            numEvents );
    }
    else
    {
        for( cl_uint i = 0; i < numEvents; i++ )
        {
            if( event != NULL && *event == eventList[i] )
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                logf( "Check Events for %s: outgoing event %p is also in the event wait list!\n",
                    functionName,
                    eventList[i] );
                continue;
            }

            cl_int  eventCommandExecutionStatus = 0;
            cl_int  errorCode = dispatch().clGetEventInfo(
                eventList[i],
                CL_EVENT_COMMAND_EXECUTION_STATUS,
                sizeof(eventCommandExecutionStatus),
                &eventCommandExecutionStatus,
                NULL );
            if( errorCode != CL_SUCCESS )
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                logf( "Check Events for %s: clGetEventInfo for wait event %p returned %s (%d)!\n",
                    functionName,
                    eventList[i],
                    enumName().name(errorCode).c_str(),
                    errorCode );
            }
            else if( eventCommandExecutionStatus < 0 )
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                logf( "Check Events for %s: wait event %p is in an error state (%d)!\n",
                    functionName,
                    eventList[i],
                    eventCommandExecutionStatus );
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::checkKernelArgUSMPointer(
    cl_kernel kernel,
    const void* arg )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_int errorCode = CL_SUCCESS;

    cl_platform_id  platform = getPlatform(kernel);

    // If we don't have a function pointer for clGetMemAllocINFO, try to
    // get one.  It's possible that the function pointer exists but
    // the application hasn't queried for it yet, in which case it won't
    // be installed into the dispatch table.
    if( dispatchX(platform).clGetMemAllocInfoINTEL == NULL )
    {
        getExtensionFunctionAddress(
            platform,
            "clGetMemAllocInfoINTEL" );
    }

    const auto& dispatchX = this->dispatchX(platform);
    if( dispatchX.clGetMemAllocInfoINTEL == NULL )
    {
        logf( "function pointer for clGetMemAllocInfoINTEL is NULL\n" );
    }
    else if( arg == NULL )
    {
        logf( "mem pointer %p is NULL\n", arg );
    }
    else
    {
        cl_context  context = NULL;
        if( errorCode == CL_SUCCESS )
        {
            errorCode = dispatch().clGetKernelInfo(
                kernel,
                CL_KERNEL_CONTEXT,
                sizeof(context),
                &context,
                NULL );
        }

        if( errorCode == CL_SUCCESS )
        {
            cl_unified_shared_memory_type_intel memType = CL_MEM_TYPE_UNKNOWN_INTEL;
            cl_device_id associatedDevice = NULL;

            dispatchX.clGetMemAllocInfoINTEL(
                context,
                arg,
                CL_MEM_ALLOC_TYPE_INTEL,
                sizeof(memType),
                &memType,
                NULL );
            dispatchX.clGetMemAllocInfoINTEL(
                context,
                arg,
                CL_MEM_ALLOC_DEVICE_INTEL,
                sizeof(associatedDevice),
                &associatedDevice,
                NULL );

            char* deviceName = NULL;
            if( associatedDevice )
            {
                allocateAndGetDeviceInfoString(
                    associatedDevice,
                    CL_DEVICE_NAME,
                    deviceName );
            }

            if( memType == CL_MEM_TYPE_DEVICE_INTEL )
            {
                if( deviceName )
                {
                    logf( "mem pointer %p is a DEVICE pointer associated with device %s\n",
                        arg,
                        deviceName );
                }
                else if( associatedDevice )
                {
                    logf( "mem pointer %p is a DEVICE pointer associated with device %p\n",
                        arg,
                        associatedDevice );
                }
                else
                {
                    CLI_ASSERT( 0 );
                    logf( "mem pointer %p is a DEVICE pointer without an associated device???\n",
                        arg );
                }
            }
            else if( memType == CL_MEM_TYPE_HOST_INTEL )
            {
                logf( "mem pointer %p is a HOST pointer\n",
                    arg );
            }
            else if( memType == CL_MEM_TYPE_SHARED_INTEL )
            {
                if( deviceName )
                {
                    logf( "mem pointer %p is a SHARED pointer associated with device %s\n",
                        arg,
                        deviceName );
                }
                else if( associatedDevice )
                {
                    logf( "mem pointer %p is a SHARED pointer associated with device %p\n",
                        arg,
                        associatedDevice );
                }
                else
                {
                    logf( "mem pointer %p is a SHARED pointer without an associated device\n",
                        arg );
                }
            }
            else if( memType == CL_MEM_TYPE_UNKNOWN_INTEL )
            {
                // This could be a system shared USM pointer, or this could be an error.
                // Check the devices associated with this kernel to see if any support
                // system shared USM allocations.
                cl_program  program = NULL;
                if( errorCode == CL_SUCCESS )
                {
                    errorCode = dispatch().clGetKernelInfo(
                        kernel,
                        CL_KERNEL_PROGRAM,
                        sizeof(program),
                        &program,
                        NULL );
                }

                cl_uint         numDevices = 0;
                cl_device_id*   deviceList = NULL;
                if( errorCode == CL_SUCCESS )
                {
                    errorCode = allocateAndGetProgramDeviceList(
                        program,
                        numDevices,
                        deviceList );
                }

                if( errorCode == CL_SUCCESS )
                {
                    bool supportsSystemSharedUSM = false;
                    for( cl_uint d = 0; d < numDevices; d++ )
                    {
                        cl_device_unified_shared_memory_capabilities_intel usmCaps = 0;
                        dispatch().clGetDeviceInfo(
                            deviceList[d],
                            CL_DEVICE_SHARED_SYSTEM_MEM_CAPABILITIES_INTEL,
                            sizeof(usmCaps),
                            &usmCaps,
                            NULL);
                        if( usmCaps != 0 )
                        {
                            supportsSystemSharedUSM = true;
                            break;
                        }
                    }

                    if( supportsSystemSharedUSM )
                    {
                        logf( "mem pointer %p is an UNKNOWN pointer and could be a shared system pointer\n",
                            arg );
                    }
                    else
                    {
                        logf( "mem pointer %p is an UNKNOWN pointer and no device support shared system pointers!\n",
                            arg );
                    }
                }
                else
                {
                    logf( "mem pointer %p is an UNKNOWN pointer and additional queries returned an error!\n",
                        arg );
                }

                delete [] deviceList;
            }
            else
            {
                CLI_ASSERT( 0 );
                logf( "query for mem pointer %p returned an unknown memory type %08X!\n",
                    arg,
                    memType );
            }

            delete [] deviceName;
        }
        else
        {
            logf( "couldn't query context for kernel %p for mem pointer %p!\n",
                kernel,
                arg );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::checkRelaxAllocationLimitsSupport(
    cl_program program ) const
{
    cl_int  errorCode = CL_SUCCESS;
    bool    supported = true;

    cl_uint         numDevices = 0;
    cl_device_id*   deviceList = NULL;
    if( errorCode == CL_SUCCESS )
    {
        errorCode = allocateAndGetProgramDeviceList(
            program,
            numDevices,
            deviceList );
    }

    if( errorCode == CL_SUCCESS )
    {
        supported = checkRelaxAllocationLimitsSupport(
            numDevices,
            deviceList );
    }

    delete [] deviceList;

    return ( errorCode == CL_SUCCESS ) && supported;
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::checkRelaxAllocationLimitsSupport(
    cl_uint numDevices,
    const cl_device_id* deviceList ) const
{
    cl_int  errorCode = CL_SUCCESS;
    bool    supported = true;

    // For now, check for Intel GPU devices to determine whether relaxed
    // allocations are supported.  Eventually this can be checked using
    // formal mechanisms.

    for( cl_uint i = 0; i < numDevices; i++ )
    {
        cl_device_type  deviceType = 0;
        cl_uint deviceVendorId;

        errorCode |= dispatch().clGetDeviceInfo(
            deviceList[ i ],
            CL_DEVICE_TYPE,
            sizeof( deviceType ),
            &deviceType,
            NULL );
        errorCode |= dispatch().clGetDeviceInfo(
            deviceList[ i ],
            CL_DEVICE_VENDOR_ID,
            sizeof( deviceVendorId ),
            &deviceVendorId,
            NULL );
        if( ( deviceType & CL_DEVICE_TYPE_GPU ) == 0 ||
            deviceVendorId != 0x8086 )
        {
            supported = false;
            break;
        }
    }

    return ( errorCode == CL_SUCCESS ) && supported;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::usmAllocPropertiesOverride(
    const cl_mem_properties_intel* properties,
    cl_mem_properties_intel*& pLocalAllocProperties ) const
{
    const cl_mem_flags CL_MEM_ALLOW_UNRESTRICTED_SIZE_INTEL = (1 << 23);

    bool    addMemFlagsEnum = config().RelaxAllocationLimits != 0;

    size_t  numProperties = 0;
    if( properties )
    {
        while( properties[ numProperties ] != 0 )
        {
            switch( properties[ numProperties ] )
            {
            case CL_MEM_FLAGS:
                addMemFlagsEnum = false;
                break;
            default:
                break;
            }
            numProperties += 2;
        }
    }

    if( addMemFlagsEnum )
    {
        numProperties += 2;
    }

    // Allocate a new array of properties.  We need to allocate two
    // properties for each pair, plus one property for the terminating
    // zero.
    pLocalAllocProperties = new cl_queue_properties[ numProperties + 1 ];
    if( pLocalAllocProperties )
    {
        // Copy the old properties array to the new properties array,
        // if the new properties array exists.
        numProperties = 0;
        if( properties )
        {
            while( properties[ numProperties ] != 0 )
            {
                pLocalAllocProperties[ numProperties ] = properties[ numProperties ];
                if( properties[ numProperties ] == CL_MEM_FLAGS )
                {
                    CLI_ASSERT( addMemFlagsEnum == false );

                    cl_mem_properties_intel flags = properties[ numProperties + 1 ];
                    if( config().RelaxAllocationLimits )
                    {
                        flags |= CL_MEM_ALLOW_UNRESTRICTED_SIZE_INTEL;
                    }

                    pLocalAllocProperties[ numProperties + 1 ] = flags;
                }
                else
                {
                    pLocalAllocProperties[ numProperties + 1 ] =
                        properties[ numProperties + 1 ];
                }
                numProperties += 2;
            }
        }
        if( addMemFlagsEnum )
        {
            pLocalAllocProperties[ numProperties] = CL_MEM_FLAGS;

            cl_mem_properties_intel flags = 0;
            if( config().RelaxAllocationLimits )
            {
                flags |= CL_MEM_ALLOW_UNRESTRICTED_SIZE_INTEL;
            }

            pLocalAllocProperties[ numProperties + 1 ] = flags;
            numProperties += 2;
        }

        // Add the terminating zero.
        pLocalAllocProperties[ numProperties ] = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::startAubCapture(
    const char* functionName,
    const uint64_t enqueueCounter,
    const cl_kernel kernel,
    const cl_uint workDim,
    const size_t* gws,
    const size_t* lws,
    cl_command_queue command_queue )
{
    if( m_AubCaptureStarted == false )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        // For kernels, perform aub capture skip checks.  We'll skip aubcapture if:
        // - the current skip counter is less than the specified skip counter, or
        // - the current capture counter is greater than or equal to the specified capture counter.

        bool    skip = false;
        if( kernel != NULL )
        {
            if( m_AubCaptureKernelEnqueueSkipCounter < m_Config.AubCaptureNumKernelEnqueuesSkip )
            {
                logf( "Skipping kernel aub capture: current skip counter is %u, requested skip counter is %u.\n",
                    m_AubCaptureKernelEnqueueSkipCounter,
                    m_Config.AubCaptureNumKernelEnqueuesSkip );

                skip = true;
                ++m_AubCaptureKernelEnqueueSkipCounter;
            }
            else
            {
                if( m_AubCaptureKernelEnqueueCaptureCounter >= m_Config.AubCaptureNumKernelEnqueuesCapture )
                {
                    logf( "Skipping kernel aub capture: current capture counter is %u, requested capture counter is %u.\n",
                        m_AubCaptureKernelEnqueueCaptureCounter,
                        m_Config.AubCaptureNumKernelEnqueuesCapture );
                    skip = true;
                }

                ++m_AubCaptureKernelEnqueueCaptureCounter;
            }
        }

        if( skip == false &&
            m_AubCaptureStarted == false )
        {
            // Try to call clFinish() on the passed-in command queue.
            // This isn't perfect, since we'd really rather call
            // clFinish on all command queues to start with a fresh
            // capture, but it's better than nothing.
            // TODO: Is Flush() sufficient?
            dispatch().clFinish( command_queue );

            char    charBuf[ MAX_PATH ];

            std::string fileName = "";

            // Get the dump directory name.
            {
                OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
                fileName += "/";
                fileName += "AubCapture";

                if( m_Config.AubCaptureIndividualEnqueues )
                {
                    fileName += "_Enqueue_";

                    CLI_SPRINTF( charBuf, MAX_PATH, "%08u", (cl_uint)enqueueCounter );

                    fileName += charBuf;
                    fileName += "_";

                    if( kernel )
                    {
                        const std::string& kernelName = getShortKernelName(kernel);
                        fileName += "kernel_";
                        fileName += kernelName;

                        std::ostringstream  ss;
                        ss << "_G_";
                        if( gws )
                        {
                            if( workDim >= 1 )
                            {
                                ss << gws[0];
                            }
                            if( workDim >= 2 )
                            {
                                ss << "x" << gws[1];
                            }
                            if( workDim >= 3 )
                            {
                                ss << "x" << gws[2];
                            }
                        }
                        else
                        {
                            ss << "NULL";
                        }
                        ss << "_L_";
                        if( lws )
                        {
                            if( workDim >= 1 )
                            {
                                ss << lws[0];
                            }
                            if( workDim >= 2 )
                            {
                                ss << "x" << lws[1];
                            }
                            if( workDim >= 3 )
                            {
                                ss << "x" << lws[2];
                            }
                        }
                        else
                        {
                            ss << "NULL";
                        }
                        fileName += ss.str();
                    }
                    else
                    {
                        fileName += functionName;
                    }
                }
                else if( m_Config.AubCaptureMinEnqueue != 0 ||
                         m_Config.AubCaptureMaxEnqueue != UINT_MAX )
                {
                    fileName += "_Enqueue_";

                    CLI_SPRINTF( charBuf, MAX_PATH, "%08u", m_Config.AubCaptureMinEnqueue );

                    fileName += charBuf;
                    fileName += "_to_";

                    CLI_SPRINTF( charBuf, MAX_PATH, "%08u", m_Config.AubCaptureMaxEnqueue );

                    fileName += charBuf;
                }
            }

            // Now make directories as appropriate.
            {
                OS().MakeDumpDirectories( fileName );
            }

#if defined(_WIN32)
            if( m_Config.AubCaptureKDC )
            {
                fileName += ".daf";
                OS().StartAubCaptureKDC(
                    fileName,
                    config().AubCaptureStartWait );
            }
            else
#endif
            {
                fileName += ".aub";
                OS().StartAubCapture(
                    fileName,
                    config().AubCaptureStartWait );
            }
            log( "AubCapture started... maybe.  Filename is: " + fileName + "\n" );

            // No matter what, set the flag that aubcapture is started, so we
            // don't try again.
            m_AubCaptureStarted = true;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::stopAubCapture(
    cl_command_queue command_queue )
{
    if( m_AubCaptureStarted == true )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if( m_AubCaptureStarted == true )
        {
            if( command_queue )
            {
                dispatch().clFinish( command_queue );
            }

#if defined(_WIN32)
            if( m_Config.AubCaptureKDC )
            {
                OS().StopAubCaptureKDC(
                    config().AubCaptureEndWait );
            }
            else
#endif
            {
                OS().StopAubCapture(
                    config().AubCaptureEndWait );
            }
            log( "AubCapture stopped.\n" );

            // No matter what, clar the flag that aubcapture is started, so we
            // don't try again.
            m_AubCaptureStarted = false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::initPrecompiledKernelOverrides(
    const cl_context context )
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    log( "Initializing precompiled kernel overrides...\n" );

    cl_int  errorCode = CL_SUCCESS;

    // Check to see if overrides already exist.  If they do, release them.
    SPrecompiledKernelOverrides*    pOverrides =
        m_PrecompiledKernelOverridesMap[ context ];
    if( pOverrides )
    {
        errorCode = dispatch().clReleaseKernel( pOverrides->Kernel_CopyBufferBytes );
        errorCode = dispatch().clReleaseKernel( pOverrides->Kernel_CopyBufferUInts );
        errorCode = dispatch().clReleaseKernel( pOverrides->Kernel_CopyBufferUInt4s );
        errorCode = dispatch().clReleaseKernel( pOverrides->Kernel_CopyBufferUInt16s );

        errorCode = dispatch().clReleaseKernel( pOverrides->Kernel_CopyImage2Dto2DFloat );
        errorCode = dispatch().clReleaseKernel( pOverrides->Kernel_CopyImage2Dto2DInt );
        errorCode = dispatch().clReleaseKernel( pOverrides->Kernel_CopyImage2Dto2DUInt );

        errorCode = dispatch().clReleaseProgram( pOverrides->Program );

        delete pOverrides;
        pOverrides = NULL;

        m_PrecompiledKernelOverridesMap[ context ] = NULL;
    }

    // Allocate new overrides.
    pOverrides = new SPrecompiledKernelOverrides;
    if( pOverrides )
    {
        pOverrides->Program = NULL;

        pOverrides->Kernel_CopyBufferBytes = NULL;
        pOverrides->Kernel_CopyBufferUInts = NULL;
        pOverrides->Kernel_CopyBufferUInt4s = NULL;
        pOverrides->Kernel_CopyBufferUInt16s = NULL;

        pOverrides->Kernel_CopyImage2Dto2DFloat = NULL;
        pOverrides->Kernel_CopyImage2Dto2DInt = NULL;
        pOverrides->Kernel_CopyImage2Dto2DUInt = NULL;

        const char* pProgramString = NULL;
        size_t  programStringLength = 0;

        // Get the precompiled kernel string.
        if( errorCode == CL_SUCCESS )
        {
            if( m_OS.GetPrecompiledKernelString(
                    pProgramString,
                    programStringLength ) == false )
            {
                errorCode = CL_INVALID_VALUE;
            }
        }

        // Create the program:
        if( errorCode == CL_SUCCESS )
        {
            pOverrides->Program = dispatch().clCreateProgramWithSource(
                context,
                1,
                &pProgramString,
                &programStringLength,
                &errorCode );
        }

        // Build the program:
        if( errorCode == CL_SUCCESS )
        {
            errorCode = dispatch().clBuildProgram(
                pOverrides->Program,
                0,
                NULL,
                NULL,
                NULL,
                NULL );

            if( errorCode != CL_SUCCESS )
            {
                cl_int  tempErrorCode = CL_SUCCESS;

                // Get the number of devices for this context.
                cl_uint numDevices = 0;
                tempErrorCode = dispatch().clGetContextInfo(
                    context,
                    CL_CONTEXT_NUM_DEVICES,
                    sizeof( numDevices ),
                    &numDevices,
                    NULL );

                if( numDevices != 0 )
                {
                    cl_device_id*   devices = new cl_device_id[ numDevices ];
                    if( devices )
                    {
                        tempErrorCode = dispatch().clGetContextInfo(
                            context,
                            CL_CONTEXT_DEVICES,
                            numDevices * sizeof( cl_device_id ),
                            devices,
                            NULL );

                        if( tempErrorCode == CL_SUCCESS )
                        {
                            for( cl_uint i = 0; i < numDevices; i++ )
                            {
                                size_t  buildLogSize = 0;
                                dispatch().clGetProgramBuildInfo(
                                    pOverrides->Program,
                                    devices[ i ],
                                    CL_PROGRAM_BUILD_LOG,
                                    0,
                                    NULL,
                                    &buildLogSize );

                                char*   buildLog = new char[ buildLogSize + 1 ];
                                if( buildLog )
                                {
                                    dispatch().clGetProgramBuildInfo(
                                        pOverrides->Program,
                                        devices[ i ],
                                        CL_PROGRAM_BUILD_LOG,
                                        buildLogSize * sizeof( char ),
                                        buildLog,
                                        NULL );

                                    buildLog[ buildLogSize ] = '\0';

                                    log( "-------> Start of Build Log:\n" );
                                    log( buildLog );
                                    log( "<------- End of Build Log!\n" );

                                    delete [] buildLog;
                                }
                            }
                        }
                    }
                }
            }
        }

        // Create all of the kernels in the program:

        if( config().OverrideReadBuffer ||
            config().OverrideWriteBuffer ||
            config().OverrideCopyBuffer )
        {
            if( errorCode == CL_SUCCESS )
            {
                pOverrides->Kernel_CopyBufferBytes = dispatch().clCreateKernel(
                    pOverrides->Program,
                    "CopyBufferBytes",
                    &errorCode );
            }
            if( errorCode == CL_SUCCESS )
            {
                pOverrides->Kernel_CopyBufferUInts = dispatch().clCreateKernel(
                    pOverrides->Program,
                    "CopyBufferUInts",
                    &errorCode );
            }
            if( errorCode == CL_SUCCESS )
            {
                pOverrides->Kernel_CopyBufferUInt4s = dispatch().clCreateKernel(
                    pOverrides->Program,
                    "CopyBufferUInt4s",
                    &errorCode );
            }
            if( errorCode == CL_SUCCESS )
            {
                pOverrides->Kernel_CopyBufferUInt16s = dispatch().clCreateKernel(
                    pOverrides->Program,
                    "CopyBufferUInt16s",
                    &errorCode );
            }
        }

        if( config().OverrideReadImage ||
            config().OverrideWriteImage ||
            config().OverrideCopyImage )
        {
            // TODO: Check to see if images are supported?
            //       What should happen if this is a multiple-device context,
            //       and one device supports images, but another doesn't?

            if( errorCode == CL_SUCCESS )
            {
                pOverrides->Kernel_CopyImage2Dto2DFloat = dispatch().clCreateKernel(
                    pOverrides->Program,
                    "CopyImage2Dto2DFloat",
                    &errorCode );
            }
            if( errorCode == CL_SUCCESS )
            {
                pOverrides->Kernel_CopyImage2Dto2DInt = dispatch().clCreateKernel(
                    pOverrides->Program,
                    "CopyImage2Dto2DInt",
                    &errorCode );
            }
            if( errorCode == CL_SUCCESS )
            {
                pOverrides->Kernel_CopyImage2Dto2DUInt = dispatch().clCreateKernel(
                    pOverrides->Program,
                    "CopyImage2Dto2DUInt",
                    &errorCode );
            }
        }

        if( errorCode == CL_SUCCESS )
        {
            m_PrecompiledKernelOverridesMap[ context ] = pOverrides;
        }
        else
        {
            delete pOverrides;
            pOverrides = NULL;
        }
    }

    log( "... precompiled kernel override initialization complete.\n" );
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::initBuiltinKernelOverrides(
    const cl_context context )
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    log( "Initializing builtin kernel overrides...\n" );

    cl_int  errorCode = CL_SUCCESS;

    // Check to see if overrides already exist.  If they do, release them.
    SBuiltinKernelOverrides*    pOverrides =
        m_BuiltinKernelOverridesMap[ context ];
    if( pOverrides )
    {
        errorCode = dispatch().clReleaseKernel( pOverrides->Kernel_block_motion_estimate_intel );

        errorCode = dispatch().clReleaseProgram( pOverrides->Program );

        delete pOverrides;
        pOverrides = NULL;

        m_BuiltinKernelOverridesMap[ context ] = NULL;
    }

    // Allocate new overrides.
    pOverrides = new SBuiltinKernelOverrides;
    if( pOverrides )
    {
        pOverrides->Program = NULL;

        pOverrides->Kernel_block_motion_estimate_intel = NULL;

        const char* pProgramString = NULL;
        size_t  programStringLength = 0;

        // Get the builtin kernel program string.
        if( errorCode == CL_SUCCESS )
        {
            if( m_OS.GetBuiltinKernelString(
                    pProgramString,
                    programStringLength ) == false )
            {
                errorCode = CL_INVALID_VALUE;
            }
        }

        // Create the program:
        if( errorCode == CL_SUCCESS )
        {
            pOverrides->Program = dispatch().clCreateProgramWithSource(
                context,
                1,
                &pProgramString,
                &programStringLength,
                &errorCode );
        }

        // Build the program:
        if( errorCode == CL_SUCCESS )
        {
            errorCode = dispatch().clBuildProgram(
                pOverrides->Program,
                0,
                NULL,
                "-Dcl_intel_device_side_vme_enable -DHW_NULL_CHECK",
                NULL,
                NULL );

            if( errorCode != CL_SUCCESS )
            {
                cl_int  tempErrorCode = CL_SUCCESS;

                // Get the number of devices for this context.
                cl_uint numDevices = 0;
                tempErrorCode = dispatch().clGetContextInfo(
                    context,
                    CL_CONTEXT_NUM_DEVICES,
                    sizeof( numDevices ),
                    &numDevices,
                    NULL );

                if( numDevices != 0 )
                {
                    cl_device_id*   devices = new cl_device_id[ numDevices ];
                    if( devices )
                    {
                        tempErrorCode = dispatch().clGetContextInfo(
                            context,
                            CL_CONTEXT_DEVICES,
                            numDevices * sizeof( cl_device_id ),
                            devices,
                            NULL );

                        if( tempErrorCode == CL_SUCCESS )
                        {
                            for( cl_uint i = 0; i < numDevices; i++ )
                            {
                                size_t  buildLogSize = 0;
                                dispatch().clGetProgramBuildInfo(
                                    pOverrides->Program,
                                    devices[ i ],
                                    CL_PROGRAM_BUILD_LOG,
                                    0,
                                    NULL,
                                    &buildLogSize );

                                char*   buildLog = new char[ buildLogSize + 1 ];
                                if( buildLog )
                                {
                                    dispatch().clGetProgramBuildInfo(
                                        pOverrides->Program,
                                        devices[ i ],
                                        CL_PROGRAM_BUILD_LOG,
                                        buildLogSize * sizeof( char ),
                                        buildLog,
                                        NULL );

                                    buildLog[ buildLogSize ] = '\0';

                                    log( "-------> Start of Build Log:\n" );
                                    log( buildLog );
                                    log( "<------- End of Build Log!\n" );

                                    delete [] buildLog;
                                }
                            }
                        }
                    }
                }
            }
        }

        // Create all of the kernels in the program:

        if( errorCode == CL_SUCCESS )
        {
            pOverrides->Kernel_block_motion_estimate_intel = dispatch().clCreateKernel(
                pOverrides->Program,
                "block_motion_estimate_intel",
                &errorCode );
        }

        if( errorCode == CL_SUCCESS )
        {
            m_BuiltinKernelOverridesMap[ context ] = pOverrides;
        }
        else
        {
            delete pOverrides;
            pOverrides = NULL;
        }
    }

    log( "... builtin kernel override initialization complete.\n" );
}

///////////////////////////////////////////////////////////////////////////////
//
cl_program CLIntercept::createProgramWithInjectionBinaries(
    uint64_t hash,
    cl_context context,
    cl_int* errcode_ret )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_int      errorCode = CL_SUCCESS;
    cl_program  program = NULL;

    std::string fileName1;
    std::string fileName2;
    size_t      numDevices = 0;

    if( errorCode == CL_SUCCESS )
    {
        std::string fileName;

        // Get the dump directory name.
        {
            OS().GetDumpDirectoryNameWithoutPid( sc_DumpDirectoryName, fileName );
            fileName += "/Inject";
        }
        // Make two candidate filenames.  They will have the form:
        //   CLI_<program number>_<hash>_0000, or
        //   CLI_<hash>_0000
        // Leave off the extension for now.
        {
            char    numberString1[256] = "";
            CLI_SPRINTF( numberString1, 256, "%04u_%08X_0000",
                m_ProgramNumber,
                (unsigned int)hash );

            char    numberString2[256] = "";
            CLI_SPRINTF( numberString2, 256, "%08X_0000",
                (unsigned int)hash );

            fileName1 = fileName;
            fileName1 += "/CLI_";
            fileName1 += numberString1;

            fileName2 = fileName;
            fileName2 += "/CLI_";
            fileName2 += numberString2;
        }

        errorCode = dispatch().clGetContextInfo(
            context,
            CL_CONTEXT_DEVICES,
            0,
            NULL,
            &numDevices );
    }

    cl_device_id*   devices = NULL;

    char**          programBinaries = NULL;
    size_t*         programBinarySizes = NULL;

    if( errorCode == CL_SUCCESS )
    {
        numDevices = numDevices / sizeof( cl_device_id );

        devices = new cl_device_id [ numDevices ];

        programBinaries = new char*[ numDevices ];
        programBinarySizes = new size_t[ numDevices ];

        if( ( devices == NULL ) ||
            ( programBinaries == NULL ) ||
            ( programBinarySizes == NULL ) )
        {
            CLI_ASSERT( 0 );
            errorCode = CL_OUT_OF_HOST_MEMORY;
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        for( size_t i = 0; i < numDevices; i++ )
        {
            programBinaries[i] = NULL;
        }

        errorCode = dispatch().clGetContextInfo(
            context,
            CL_CONTEXT_DEVICES,
            numDevices * sizeof( cl_device_id ),
            devices,
            NULL );

        if( errorCode == CL_SUCCESS )
        {
            // Assume all binaries exist, until this is proven otherwise.
            bool    allBinariesExist = true;

            for( size_t i = 0; i < numDevices; i++ )
            {
                cl_device_type  deviceType = CL_DEVICE_TYPE_DEFAULT;

                // It's OK if this fails.  If it does, it just
                // means that our output file won't have a device
                // type.
                dispatch().clGetDeviceInfo(
                    devices[ i ],
                    CL_DEVICE_TYPE,
                    sizeof( deviceType ),
                    &deviceType,
                    NULL );

                std::string suffix;

                if( deviceType & CL_DEVICE_TYPE_CPU )
                {
                    suffix += "_CPU";
                }
                if( deviceType & CL_DEVICE_TYPE_GPU )
                {
                    suffix += "_GPU";
                }
                if( deviceType & CL_DEVICE_TYPE_ACCELERATOR )
                {
                    suffix += "_ACC";
                }
                if( deviceType & CL_DEVICE_TYPE_CUSTOM )
                {
                    suffix += "_CUSTOM";
                }

                suffix += ".bin";

                std::string inputFileName = fileName1 + suffix;

                std::ifstream is;
                is.open(
                    inputFileName.c_str(),
                    std::ios::in | std::ios::binary );
                if( is.good() )
                {
                    log( "Injection binary file exists: " + inputFileName + "\n" );
                }
                else
                {
                    log( "Injection binary file doesn't exist: " + inputFileName + "\n" );

                    inputFileName = fileName2 + suffix;
                    is.clear();
                    is.open(
                        inputFileName.c_str(),
                        std::ios::in | std::ios::binary );
                    if( is.good() )
                    {
                        log( "Injection binary file exists: " + inputFileName + "\n" );
                    }
                    else
                    {
                        log( "Injection binary file doesn't exist: " + inputFileName + "\n" );
                    }
                }

                if( is.good() )
                {
                    // The file exists.  Figure out how big it is.
                    is.seekg( 0, std::ios::end );
                    programBinarySizes[i] = (size_t)is.tellg();
                    is.seekg( 0, std::ios::beg );

                    programBinaries[i] = new char[ programBinarySizes[i] ];
                    if( programBinaries[i] == NULL )
                    {
                        CLI_ASSERT( 0 );
                        errorCode = CL_OUT_OF_HOST_MEMORY;
                    }
                    else
                    {
                        is.read( programBinaries[i],  programBinarySizes[i] );
                    }

                    is.close();
                }
                else
                {
                    log( "Injection binary is missing!\n" );
                    allBinariesExist = false;
                }
            }

            if( allBinariesExist &&
                ( errorCode == CL_SUCCESS ) )
            {
                log( "All injection binaries exist.\n" );

                program = dispatch().clCreateProgramWithBinary(
                    context,
                    (cl_uint)numDevices,
                    devices,
                    programBinarySizes,
                    (const unsigned char**)programBinaries,
                    NULL,   // binary_status
                    &errorCode );
                if( program )
                {
                    logf("Injection successful: clCreateProgramWithBinary() returned %p\n",
                        program );
                }
                if( errorCode != CL_SUCCESS )
                {
                    logf("Injecting binaries failed: clCreateProgramWithBinary() returned %s\n",
                        enumName().name( errorCode ).c_str() );
                }
            }

            for( size_t i = 0; i < numDevices; i++ )
            {
                programBinarySizes[i] = 0;

                delete [] programBinaries[i];
                programBinaries[i] = NULL;
            }
        }
    }

    delete [] devices;

    delete [] programBinaries;
    delete [] programBinarySizes;

    if( errcode_ret )
    {
        errcode_ret[0] = errorCode;
    }

    return program;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpProgramBinary(
    const cl_program program )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    const SProgramInfo& programInfo = m_ProgramInfoMap[ program ];

    std::string     fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
    }
    // Make the filename.  It will have the form:
    //   CLI_<program number>_<program hash>_<compile count>_<options hash>
    // Leave off the extension for now.
    {
        char    numberString[256] = "";

        if( config().OmitProgramNumber )
        {
            CLI_SPRINTF( numberString, 256, "%08X_%04u_%08X",
                (unsigned int)programInfo.ProgramHash,
                programInfo.CompileCount,
                (unsigned int)programInfo.OptionsHash );
        }
        else
        {
            CLI_SPRINTF( numberString, 256, "%04u_%08X_%04u_%08X",
                programInfo.ProgramNumber,
                (unsigned int)programInfo.ProgramHash,
                programInfo.CompileCount,
                (unsigned int)programInfo.OptionsHash );
        }

        fileName += "/CLI_";
        fileName += numberString;
    }
    // Now make directories as appropriate.
    {
        OS().MakeDumpDirectories( fileName );
    }

    cl_int  errorCode = CL_SUCCESS;

    size_t  numDevices = 0;

    if( errorCode == CL_SUCCESS )
    {
        // Get all of the devices associated with this program.
        errorCode = dispatch().clGetProgramInfo(
            program,
            CL_PROGRAM_DEVICES,
            0,
            NULL,
            &numDevices );
    }

    cl_device_id*   devices = NULL;
    char**          programBinaries = NULL;
    size_t*         programBinarySizes = NULL;

    if( errorCode == CL_SUCCESS )
    {
        numDevices /= sizeof( cl_device_id );

        devices = new cl_device_id[ numDevices ];
        programBinaries = new char*[ numDevices ];
        programBinarySizes = new size_t[ numDevices ];

        if( ( devices == NULL ) ||
            ( programBinaries == NULL ) ||
            ( programBinarySizes == NULL ) )
        {
            CLI_ASSERT( 0 );
            errorCode = CL_OUT_OF_HOST_MEMORY;
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        for( size_t i = 0; i < numDevices; i++ )
        {
            programBinaries[i] = NULL;
        }

        errorCode = dispatch().clGetProgramInfo(
            program,
            CL_PROGRAM_DEVICES,
            numDevices * sizeof( cl_device_id ),
            devices,
            NULL );
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetProgramInfo(
            program,
            CL_PROGRAM_BINARY_SIZES,
            numDevices * sizeof( size_t ),
            programBinarySizes,
            NULL );
    }

    if( errorCode == CL_SUCCESS )
    {
        for( size_t i = 0; i < numDevices; i++ )
        {
            programBinaries[ i ] = new char[ programBinarySizes[ i ] ];
            if( programBinaries[ i ] == NULL )
            {
                errorCode = CL_OUT_OF_HOST_MEMORY;
                break;
            }
        }

        if( errorCode == CL_SUCCESS )
        {
            errorCode = dispatch().clGetProgramInfo(
                program,
                CL_PROGRAM_BINARIES,
                numDevices * sizeof( char* ),
                programBinaries,
                NULL );
        }

        if( errorCode == CL_SUCCESS )
        {
            for( size_t i = 0; i < numDevices; i++ )
            {
                cl_device_type  deviceType = CL_DEVICE_TYPE_DEFAULT;

                // It's OK if this fails.  If it does, it just
                // means that our output file won't have a device
                // type.
                dispatch().clGetDeviceInfo(
                    devices[ i ],
                    CL_DEVICE_TYPE,
                    sizeof( deviceType ),
                    &deviceType,
                    NULL );

                std::string outputFileName = fileName;

                if( deviceType & CL_DEVICE_TYPE_CPU )
                {
                    outputFileName += "_CPU";
                }
                if( deviceType & CL_DEVICE_TYPE_GPU )
                {
                    outputFileName += "_GPU";
                }
                if( deviceType & CL_DEVICE_TYPE_ACCELERATOR )
                {
                    outputFileName += "_ACC";
                }
                if( deviceType & CL_DEVICE_TYPE_CUSTOM )
                {
                    outputFileName += "_CUSTOM";
                }

                outputFileName += ".bin";

                std::ofstream os;
                os.open(
                    outputFileName.c_str(),
                    std::ios::out | std::ios::binary );
                if( os.good() )
                {
                    log( "Dumping program binary to file: " + outputFileName + "\n" );

                    os.write(
                        programBinaries[ i ],
                        programBinarySizes[ i ] );
                    os.close();
                }
                else
                {
                    logf( "Failed to open program binary dump file for writing: %s\n",
                        outputFileName.c_str() );
                }
            }
        }

        for( size_t i = 0; i < numDevices; i++ )
        {
            delete [] programBinaries[ i ];
            programBinaries[ i ] = NULL;
        }
    }

    delete [] devices;
    devices = NULL;

    delete [] programBinaries;
    programBinaries = NULL;

    delete [] programBinarySizes;
    programBinarySizes = NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpKernelISABinaries(
    const cl_program program )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_int  errorCode = CL_SUCCESS;

    // Since the kernel ISA binaries are retrieved via kernel queries, the first
    // thing we need to do is to create the kernels for this program.

    cl_uint numKernels = 0;
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clCreateKernelsInProgram(
            program,
            0,
            NULL,
            &numKernels );
    }
    cl_kernel*  kernels = NULL;
    if( errorCode == CL_SUCCESS && numKernels != 0 )
    {
        kernels = new cl_kernel[ numKernels ];
        if( kernels )
        {
            for( cl_uint k = 0; k < numKernels; k++ )
            {
                kernels[k] = NULL;
            }
            errorCode = dispatch().clCreateKernelsInProgram(
                program,
                numKernels,
                kernels,
                NULL );
        }
        else
        {
            errorCode = CL_OUT_OF_HOST_MEMORY;
        }
    }

    // Also, get the list of devices for the program.
    cl_uint         numDevices = 0;
    cl_device_id*   deviceList = NULL;
    if( errorCode == CL_SUCCESS )
    {
        errorCode = allocateAndGetProgramDeviceList(
            program,
            numDevices,
            deviceList );
    }

    if( errorCode == CL_SUCCESS && program != NULL && kernels != NULL )
    {
        const SProgramInfo& programInfo = m_ProgramInfoMap[ program ];

        std::string fileNamePrefix;

        // Get the dump directory name.
        {
            OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileNamePrefix );
        }
        // Make the filename prefix.  It will have the form:
        //   CLI_<program number>_<program hash>_<compile count>_<options hash>_<device type>_<kernel name>.isabin
        // We'll fill in the device type and kernel name later.
        {
            char    numberString[256] = "";

            if( config().OmitProgramNumber )
            {
                CLI_SPRINTF( numberString, 256, "%08X_%04u_%08X_",
                    (unsigned int)programInfo.ProgramHash,
                    programInfo.CompileCount,
                    (unsigned int)programInfo.OptionsHash );
            }
            else
            {
                CLI_SPRINTF( numberString, 256, "%04u_%08X_%04u_%08X_",
                    programInfo.ProgramNumber,
                    (unsigned int)programInfo.ProgramHash,
                    programInfo.CompileCount,
                    (unsigned int)programInfo.OptionsHash );
            }

            fileNamePrefix += "/CLI_";
            fileNamePrefix += numberString;
        }
        // Now make directories as appropriate.
        {
            OS().MakeDumpDirectories( fileNamePrefix );
        }

        for( cl_uint k = 0; k < numKernels; k++ )
        {
            cl_kernel   kernel = kernels[ k ];

            // Get the kernel name.  We can't use the kernel name map yet, so
            // use a kernel query instead.
            char* kernelName = NULL;
            if( errorCode == CL_SUCCESS )
            {
                errorCode = allocateAndGetKernelInfoString(
                    kernel,
                    CL_KERNEL_FUNCTION_NAME,
                    kernelName );
            }

            for( cl_uint d = 0; d < numDevices; d++ )
            {
                size_t      kernelISABinarySize = 0;
                char*       kernelISABinary = NULL;

                if( errorCode == CL_SUCCESS )
                {
                    errorCode = allocateAndGetKernelISABinary(
                        kernel,
                        deviceList[d],
                        kernelISABinarySize,
                        kernelISABinary );
                }

                if( errorCode == CL_SUCCESS )
                {
                    std::string fileName( fileNamePrefix );

                    cl_device_type  deviceType = CL_DEVICE_TYPE_DEFAULT;

                    // It's OK if this fails.  If it does, it just
                    // means that our output file won't have a device
                    // type.
                    dispatch().clGetDeviceInfo(
                        deviceList[d],
                        CL_DEVICE_TYPE,
                        sizeof( deviceType ),
                        &deviceType,
                        NULL );

                    if( deviceType & CL_DEVICE_TYPE_CPU )
                    {
                        fileName += "CPU_";
                    }
                    if( deviceType & CL_DEVICE_TYPE_GPU )
                    {
                        fileName += "GPU_";
                    }
                    if( deviceType & CL_DEVICE_TYPE_ACCELERATOR )
                    {
                        fileName += "ACC_";
                    }
                    if( deviceType & CL_DEVICE_TYPE_CUSTOM )
                    {
                        fileName+= "CUSTOM_";
                    }

                    fileName += kernelName;
                    fileName += ".isabin";

                    std::ofstream os;
                    os.open(
                        fileName.c_str(),
                        std::ios::out | std::ios::binary );
                    if( os.good() )
                    {
                        log( "Dumping kernel ISA binary to file: " + fileName + "\n" );

                        os.write(
                            kernelISABinary,
                            kernelISABinarySize );
                        os.close();
                    }
                    else
                    {
                        logf( "Failed to open kernel ISA dump file for writing: %s\n",
                            fileName.c_str() );
                    }
                }

                delete [] kernelISABinary;
                kernelISABinary = NULL;
            }

            delete [] kernelName;
            kernelName = NULL;
        }
    }

    // If we have kernels, release them.
    if( kernels )
    {
        for( cl_uint k = 0; k < numKernels; k++ )
        {
            if( kernels[k] )
            {
                dispatch().clReleaseKernel( kernels[k] );
                kernels[k] = NULL;
            }
        }
    }

    delete [] kernels;
    kernels = NULL;

    delete [] deviceList;
    deviceList = NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_program CLIntercept::createProgramWithInjectionSPIRV(
    uint64_t hash,
    cl_context context,
    cl_int* errcode_ret )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_program  program = NULL;

    // Don't bother with any of this if we weren't able to get a pointer to
    // the entry point to create a program with IL.
    if( dispatch().clCreateProgramWithIL == NULL )
    {
        log( "Aborting InjectProgramSPIRV because clCreateProgramWithIL is NULL!\n" );
    }
    else
    {
        std::string fileName;

        // Get the dump directory name.
        {
            OS().GetDumpDirectoryNameWithoutPid(sc_DumpDirectoryName, fileName);
            fileName += "/Inject";
        }

        // Make three candidate filenames.  They will have the form:
        //   CLI_<program number>_<hash>_0000.spv, or
        //   CLI_<hash>_0000.spv
        {
            char    numberString1[256] = "";
            CLI_SPRINTF(numberString1, 256, "%04u_%08X_0000",
                m_ProgramNumber,
                (unsigned int)hash);

            char    numberString2[256] = "";
            CLI_SPRINTF(numberString2, 256, "%08X_0000",
                (unsigned int)hash);

            std::string fileName1;
            fileName1 = fileName;
            fileName1 += "/CLI_";
            fileName1 += numberString1;
            fileName1 += ".spv";

            std::string fileName2;
            fileName2 = fileName;
            fileName2 += "/CLI_";
            fileName2 += numberString2;
            fileName2 += ".spv";

            std::ifstream is;

            is.open(
                fileName1.c_str(),
                std::ios::in | std::ios::binary);
            if( is.good() )
            {
                log("Injecting SPIR-V file: " + fileName1 + "\n");
            }
            else
            {
                log("Injection SPIR-V file doesn't exist: " + fileName1 + "\n");

                is.clear();
                is.open(
                    fileName2.c_str(),
                    std::ios::in | std::ios::binary);
                if( is.good() )
                {
                    log("Injecting SPIR-V file: " + fileName2 + "\n");
                }
                else
                {
                    log("Injection SPIR-V file doesn't exist: " + fileName2 + "\n");
                }
            }

            if( is.good() )
            {
                // The file exists.  Figure out how big it is.
                size_t  filesize = 0;

                is.seekg( 0, std::ios::end );
                filesize = (size_t)is.tellg();
                is.seekg( 0, std::ios::beg );

                char*   newILBinary = new char[ filesize ];
                if( newILBinary == NULL )
                {
                    CLI_ASSERT( 0 );
                }
                else
                {
                    is.read(newILBinary, filesize);

                    // Right now, this can still die in the ICD loader if the ICD loader
                    // exports this entry point but the vendor didn't implement it.  It
                    // would be nice to enhance the ICD loader so it called into a safe
                    // stub function if the vendor didn't implement an entry point...
                    program = dispatch().clCreateProgramWithIL(
                        context,
                        newILBinary,
                        filesize,
                        errcode_ret );
                    if( program )
                    {
                        logf("Injection successful: clCreateProgramWithIL() returned %p\n",
                            program );
                    }

                    delete[] newILBinary;
                }

                is.close();
            }
        }
    }

    return program;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::autoCreateSPIRV(
    const cl_program program,
    const char* raw_options )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    const SProgramInfo& programInfo = m_ProgramInfoMap[ program ];

    std::string     dumpDirectoryName;
    std::string     inputFileName;
    std::string     outputFileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, dumpDirectoryName );
    }

    // Re-create the input file name.  This will be a program source file we dumped
    // earlier.  It will have the form:
    //   CLI_<program number>_<program hash>_source.cl
    {
        char    numberString[256] = "";

        if( config().OmitProgramNumber )
        {
            CLI_SPRINTF( numberString, 256, "%08X",
                (unsigned int)programInfo.ProgramHash );
        }
        else
        {
            CLI_SPRINTF( numberString, 256, "%04u_%08X",
                programInfo.ProgramNumber,
                (unsigned int)programInfo.ProgramHash );
        }

        inputFileName = dumpDirectoryName;
        inputFileName += "/CLI_";
        inputFileName += numberString;
        inputFileName += "_source.cl";
    }

    // Make the output file name.  It will have the form:
    //   CLI_<program number>_<program hash>_<compile count>_<options hash>.spv
    {
        char    numberString[256] = "";

        if( config().OmitProgramNumber )
        {
            CLI_SPRINTF( numberString, 256, "%08X_%04u_%08X",
                (unsigned int)programInfo.ProgramHash,
                programInfo.CompileCount,
                (unsigned int)programInfo.OptionsHash );
        }
        else
        {
            CLI_SPRINTF( numberString, 256, "%04u_%08X_%04u_%08X",
                programInfo.ProgramNumber,
                (unsigned int)programInfo.ProgramHash,
                programInfo.CompileCount,
                (unsigned int)programInfo.OptionsHash );
        }

        outputFileName = dumpDirectoryName;
        outputFileName += "/CLI_";
        outputFileName += numberString;
        outputFileName += ".spv";
    }

    // Now make directories as appropriate.  We can use either the input
    // or output file name to do this.
    {
        OS().MakeDumpDirectories( inputFileName );
    }

    std::string options(raw_options ? raw_options : "");
    std::string command;

    // Create the command we will use to invoke CLANG with the right options.
    // How we do this will depend on whether this is an OpenCL 1.x or 2.0
    // compilation.  We don't distinguish between different versions of
    // OpenCL 1.x right now, but we can add this in the future, if desired.
    if( options.find( "-cl-std=CL2.0" ) != std::string::npos )
    {
        // This is an OpenCL 2.0 compilation.
        command =
            config().SPIRVClang +
            " " + config().OpenCL2Options +
            " -include " + config().SPIRVCLHeader +
            " " + options +
            " -o " + outputFileName +
            " " + inputFileName;
    }
    else
    {
        // This is an OpenCL 1.x compilation.
        command =
            config().SPIRVClang +
            " " + config().DefaultOptions +
            " -include " + config().SPIRVCLHeader +
            " " + options +
            " -o " + outputFileName +
            " " + inputFileName;
    }

    logf( "Running: %s\n", command.c_str() );
    OS().ExecuteCommand( command );

    // Optionally, run spirv-dis to disassemble the generated module.
    if( !config().SPIRVDis.empty() )
    {
        command =
            config().SPIRVDis +
            " -o " + outputFileName + "t" +
            " " + outputFileName;

        logf( "Running: %s\n", command.c_str() );
        OS().ExecuteCommand( command );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::writeStringToMemory(
    size_t param_value_size,
    const std::string& param,
    size_t* param_value_size_ret,
    char* pointer ) const
{
    cl_int  errorCode = CL_SUCCESS;

    size_t  length = param.length() + 1;

    if( pointer != NULL )
    {
        if( param_value_size < length )
        {
            errorCode = CL_INVALID_VALUE;
        }
        else
        {
            strcpy_s(
                pointer,
                length,
                param.c_str() );
        }
    }

    if( param_value_size_ret != NULL )
    {
        *param_value_size_ret = length;
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
static cl_int parseExtensionString(
    const char* originalStr,
    cl_name_version_khr* ptr,
    size_t param_value_size,
    size_t* param_value_size_ret )
{
    cl_int  errorCode = CL_SUCCESS;

    // Go through the string once to count the number of tokens:
    const char* str = originalStr;
    int     numTokens = 0;
    while( str != NULL && str[0] != '\0' )
    {
        // Skip any preceding spaces
        while( isspace(str[0]) )
        {
            str++;
        }
        if( str[0] != '\0' )
        {
            // Find the next space, or end of string
            size_t  nameSize = 0;
            while( str[0] != '\0' && !isspace(str[0]) )
            {
                str++;
                nameSize++;
            }
            CLI_ASSERT( nameSize < CL_NAME_VERSION_MAX_NAME_SIZE_KHR );
            numTokens++;
        }
    }

    if( ptr != NULL )
    {
        if( param_value_size < numTokens * sizeof(cl_name_version_khr) )
        {
            errorCode = CL_INVALID_VALUE;
        }
        else
        {
            str = originalStr;
            while( str != NULL && str[0] != '\0' )
            {
                // skip any preceding spaces
                while( isspace(str[0]) )
                {
                    str++;
                }
                if( str[0] != '\0' )
                {
                    memset(ptr->name, 0, sizeof(ptr->name));
                    ptr->version = 0;

                    // find the next space, or end of string
                    size_t  nameSize = 0;
                    while( str[0] != '\0' && !isspace(str[0]) )
                    {
                        if( nameSize < sizeof(ptr->name) )
                        {
                            ptr->name[nameSize] = str[0];
                        }
                        str++;
                        nameSize++;
                    }
                    ptr->name[sizeof(ptr->name) - 1] = '\0';
                    ptr++;
                }
            }
        }
    }
    if( param_value_size_ret != NULL )
    {
        *param_value_size_ret = numTokens * sizeof(cl_name_version_khr);
    }

    return errorCode;
}

static cl_int parseILString(
    const char* originalStr,
    cl_name_version_khr* ptr,
    size_t param_value_size,
    size_t* param_value_size_ret )
{
    cl_int  errorCode = CL_SUCCESS;

    // Go through the string once to count the number of tokens:
    const char* str = originalStr;
    int     numTokens = 0;
    while( str != NULL && str[0] != '\0' )
    {
        // Skip any preceding spaces
        while( isspace(str[0]) )
        {
            str++;
        }
        if( str[0] != '\0' )
        {
            // Find the next space, or end of string
            size_t  nameSize = 0;
            while( str[0] != '\0' && !isspace(str[0]) )
            {
                str++;
                nameSize++;
            }
            CLI_ASSERT( nameSize < CL_NAME_VERSION_MAX_NAME_SIZE_KHR );
            numTokens++;
        }
    }

    if( ptr != NULL )
    {
        if( param_value_size < numTokens * sizeof(cl_name_version_khr) )
        {
            errorCode = CL_INVALID_VALUE;
        }
        else
        {
            str = originalStr;
            while( str != NULL && str[0] != '\0' )
            {
                // skip any preceding spaces
                while( isspace(str[0]) )
                {
                    str++;
                }
                if( str[0] != '\0' )
                {
                    memset(ptr->name, 0, sizeof(ptr->name));

                    // find the next space, underscore, or end of string
                    size_t  nameSize = 0;
                    while( str[0] != '\0' && str[0] != '_' && !isspace(str[0]) )
                    {
                        if( nameSize < sizeof(ptr->name) )
                        {
                            ptr->name[nameSize] = str[0];
                        }
                        str++;
                        nameSize++;
                    }
                    ptr->name[sizeof(ptr->name) - 1] = '\0';

                    // version
                    cl_uint major = 0;
                    cl_uint minor = 0;
                    cl_uint patch = 0;
                    if( str[0] == '_' )
                    {
                        str++;
                        while( isdigit(str[0]) )
                        {
                            major *= 10;
                            major += str[0] - '0';
                            str++;
                        }
                        if( str[0] == '.' )
                        {
                            str++;
                        }
                        while( isdigit(str[0]) )
                        {
                            minor *= 10;
                            minor += str[0] - '0';
                            str++;
                        }
                        if( str[0] == '.' )
                        {
                            str++;
                        }
                        while( isdigit(str[0]) )
                        {
                            patch *= 10;
                            patch += str[0] - '0';
                            str++;
                        }
                    }
                    ptr->version = CL_MAKE_VERSION_KHR( major, minor, patch );

                    // find the next space or end of string
                    while( str[0] != '\0' && !isspace(str[0]) )
                    {
                        str++;
                    }

                    ptr++;
                }
            }
        }
    }
    if( param_value_size_ret != NULL )
    {
        *param_value_size_ret = numTokens * sizeof(cl_name_version_khr);
    }

    return errorCode;
}

static cl_int parseBuiltInKernelsString(
    const char* originalStr,
    cl_name_version_khr* ptr,
    size_t param_value_size,
    size_t* param_value_size_ret )
{
    cl_int  errorCode = CL_SUCCESS;

    // Go through the string once to count the number of tokens:
    const char* str = originalStr;
    int     numTokens = 0;
    while( str != NULL && str[0] != '\0' )
    {
        // Skip any preceding spaces or semicolons
        while( isspace(str[0]) || str[0] == ';' )
        {
            str++;
        }
        if( str[0] != '\0' )
        {
            // Find the next semicolon, or end of string
            size_t  nameSize = 0;
            while( str[0] != '\0' && str[0] != ';' )
            {
                str++;
                nameSize++;
            }
            CLI_ASSERT( nameSize < CL_NAME_VERSION_MAX_NAME_SIZE_KHR );
            numTokens++;
        }
    }

    if( ptr != NULL )
    {
        if( param_value_size < numTokens * sizeof(cl_name_version_khr) )
        {
            errorCode = CL_INVALID_VALUE;
        }
        else
        {
            str = originalStr;
            while( str != NULL && str[0] != '\0' )
            {
                // Skip any preceding spaces or semicolons
                while( isspace(str[0]) || str[0] == ';' )
                {
                    str++;
                }
                if( str[0] != '\0' )
                {
                    memset(ptr->name, 0, sizeof(ptr->name));
                    ptr->version = 0;

                    // find the next semicolon, or end of string
                    size_t  nameSize = 0;
                    while( str[0] != '\0' && str[0] != ';' )
                    {
                        if( nameSize < sizeof(ptr->name) )
                        {
                            ptr->name[nameSize] = str[0];
                        }
                        str++;
                        nameSize++;
                    }
                    ptr->name[sizeof(ptr->name) - 1] = '\0';
                    ptr++;
                }
            }
        }
    }
    if( param_value_size_ret != NULL )
    {
        *param_value_size_ret = numTokens * sizeof(cl_name_version_khr);
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::overrideGetPlatformInfo(
    cl_platform_id platform,
    cl_platform_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret,
    cl_int& errorCode ) const
{
    bool    override = false;

    switch( param_name )
    {
    case CL_PLATFORM_NAME:
        if( m_Config.PlatformName != "" )
        {
            char*   ptr = (char*)param_value;
            errorCode = writeStringToMemory(
                param_value_size,
                m_Config.PlatformName,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_PLATFORM_VENDOR:
        if( m_Config.PlatformVendor != "" )
        {
            char*   ptr = (char*)param_value;
            errorCode = writeStringToMemory(
                param_value_size,
                m_Config.PlatformVendor,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_PLATFORM_PROFILE:
        if( m_Config.PlatformProfile != "" )
        {
            char*   ptr = (char*)param_value;
            errorCode = writeStringToMemory(
                param_value_size,
                m_Config.PlatformProfile,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_PLATFORM_VERSION:
        if( m_Config.PlatformVersion != "" )
        {
            char*   ptr = (char*)param_value;
            errorCode = writeStringToMemory(
                param_value_size,
                m_Config.PlatformVersion,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_PLATFORM_NUMERIC_VERSION_KHR:
        if( m_Config.Emulate_cl_khr_extended_versioning )
        {
            char*   platformVersion = NULL;

            errorCode = allocateAndGetPlatformInfoString(
                platform,
                CL_PLATFORM_VERSION,
                platformVersion );
            if( errorCode == CL_SUCCESS && platformVersion )
            {
                // According to the spec, the device version string should have the form:
                //   OpenCL <Major>.<Minor> <Vendor Specific Info>
                size_t  major = 0;
                size_t  minor = 0;
                if( getMajorMinorVersionFromString(
                        "OpenCL ",
                        platformVersion,
                        major,
                        minor ) )
                {
                    cl_version_khr* ptr = (cl_version_khr*)param_value;
                    cl_version_khr  version = CL_MAKE_VERSION_KHR( major, minor, 0 );
                    errorCode = writeParamToMemory(
                        param_value_size,
                        version,
                        param_value_size_ret,
                        ptr );
                    override = true;
                }
            }

            delete [] platformVersion;
            platformVersion = NULL;
        }
        break;
    case CL_PLATFORM_EXTENSIONS_WITH_VERSION_KHR:
        if( m_Config.Emulate_cl_khr_extended_versioning )
        {
            char*   platformExtensions = NULL;

            allocateAndGetPlatformInfoString(
                platform,
                CL_PLATFORM_EXTENSIONS,
                platformExtensions );

            // Parse the extension string even if the query returned an error.
            // In this case we will simply return that zero extensions are supported.
            cl_name_version_khr*    ptr = (cl_name_version_khr*)param_value;
            errorCode = parseExtensionString(
                platformExtensions,
                ptr,
                param_value_size,
                param_value_size_ret );
            override = true;

            delete [] platformExtensions;
            platformExtensions = NULL;
        }
        break;
    case CL_PLATFORM_SEMAPHORE_TYPES_KHR:
        if( m_Config.Emulate_cl_khr_semaphore )
        {
            // If we decide to emulate multiple semaphore types we will need
            // to return an array, but for now we can return just the binary
            // semaphore type.
            auto    ptr = (cl_semaphore_type_khr*)param_value;
            cl_semaphore_type_khr type = CL_SEMAPHORE_TYPE_BINARY_KHR;
            errorCode = writeParamToMemory(
                param_value_size,
                type,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    default:
        break;
    }

    return override;
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::overrideGetDeviceInfo(
    cl_device_id device,
    cl_device_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret,
    cl_int& errorCode ) const
{
    bool    override = false;

    switch( param_name )
    {
    case CL_DEVICE_TYPE:
        if( m_Config.DeviceType != 0 )
        {
            cl_device_type* ptr = (cl_device_type*)param_value;
            cl_device_type  d = m_Config.DeviceType;
            errorCode = writeParamToMemory(
                param_value_size,
                d,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_NAME:
        if( m_Config.DeviceName != "" )
        {
            char*   ptr = (char*)param_value;
            errorCode = writeStringToMemory(
                param_value_size,
                m_Config.DeviceName,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_EXTENSIONS:
        if( m_Config.DeviceExtensions != "" )
        {
            char*   ptr = (char*)param_value;
            errorCode = writeStringToMemory(
                param_value_size,
                m_Config.DeviceExtensions,
                param_value_size_ret,
                ptr );
            override = true;
        }
        else if( m_Config.Emulate_cl_khr_extended_versioning ||
                 m_Config.Emulate_cl_khr_semaphore ||
                 m_Config.Emulate_cl_intel_unified_shared_memory )
        {
            std::string newExtensions;
            if( m_Config.Emulate_cl_khr_extended_versioning &&
                !checkDeviceForExtension( device, "cl_khr_extended_versioning") )
            {
                newExtensions += "cl_khr_extended_versioning ";
            }
            if( m_Config.Emulate_cl_khr_semaphore &&
                !checkDeviceForExtension( device, "cl_khr_semaphore") )
            {
                newExtensions += "cl_khr_semaphore ";
            }
            if( m_Config.Emulate_cl_intel_unified_shared_memory &&
                !checkDeviceForExtension( device, "cl_intel_unified_shared_memory") )
            {
                newExtensions += "cl_intel_unified_shared_memory ";
            }

            if( !newExtensions.empty() )
            {
                char*   deviceExtensions = NULL;
                cl_int  errorCode = allocateAndGetDeviceInfoString(
                    device,
                    CL_DEVICE_EXTENSIONS,
                    deviceExtensions );
                if( errorCode == CL_SUCCESS && deviceExtensions )
                {
                    newExtensions += deviceExtensions;

                    char*   ptr = (char*)param_value;
                    errorCode = writeStringToMemory(
                        param_value_size,
                        newExtensions,
                        param_value_size_ret,
                        ptr );
                    override = true;
                }
                delete [] deviceExtensions;
            }
        }
        break;
    case CL_DEVICE_VENDOR:
        if( m_Config.DeviceVendor != "" )
        {
            char*   ptr = (char*)param_value;
            errorCode = writeStringToMemory(
                param_value_size,
                m_Config.DeviceVendor,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_PROFILE:
        if( m_Config.DeviceProfile != "" )
        {
            char*   ptr = (char*)param_value;
            errorCode = writeStringToMemory(
                param_value_size,
                m_Config.DeviceProfile,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_VERSION:
        if( m_Config.DeviceVersion != "" )
        {
            char*   ptr = (char*)param_value;
            errorCode = writeStringToMemory(
                param_value_size,
                m_Config.DeviceVersion,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_OPENCL_C_VERSION:
        if( m_Config.DeviceCVersion != "" )
        {
            char*   ptr = (char*)param_value;
            errorCode = writeStringToMemory(
                param_value_size,
                m_Config.DeviceCVersion,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_IL_VERSION:
        if( m_Config.DeviceILVersion != "" )
        {
            char*   ptr = (char*)param_value;
            errorCode = writeStringToMemory(
                param_value_size,
                m_Config.DeviceILVersion,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_VENDOR_ID:
        if( m_Config.DeviceVendorID != 0 )
        {
            cl_uint*    ptr = (cl_uint*)param_value;
            errorCode = writeParamToMemory(
                param_value_size,
                m_Config.DeviceVendorID,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_MAX_COMPUTE_UNITS:
        if( m_Config.DeviceMaxComputeUnits != 0 )
        {
            cl_uint*    ptr = (cl_uint*)param_value;
            errorCode = writeParamToMemory(
                param_value_size,
                m_Config.DeviceMaxComputeUnits,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
        if( m_Config.DevicePreferredVectorWidthChar != UINT_MAX )
        {
            cl_uint*    ptr = (cl_uint*)param_value;
            errorCode = writeParamToMemory(
                param_value_size,
                m_Config.DevicePreferredVectorWidthChar,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
        if( m_Config.DevicePreferredVectorWidthShort != UINT_MAX )
        {
            cl_uint*    ptr = (cl_uint*)param_value;
            errorCode = writeParamToMemory(
                param_value_size,
                m_Config.DevicePreferredVectorWidthShort,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
        if( m_Config.DevicePreferredVectorWidthInt != UINT_MAX )
        {
            cl_uint*    ptr = (cl_uint*)param_value;
            errorCode = writeParamToMemory(
                param_value_size,
                m_Config.DevicePreferredVectorWidthInt,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
        if( m_Config.DevicePreferredVectorWidthLong != UINT_MAX )
        {
            cl_uint*    ptr = (cl_uint*)param_value;
            errorCode = writeParamToMemory(
                param_value_size,
                m_Config.DevicePreferredVectorWidthLong,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
        if( m_Config.DevicePreferredVectorWidthHalf != UINT_MAX )
        {
            cl_uint*    ptr = (cl_uint*)param_value;
            errorCode = writeParamToMemory(
                param_value_size,
                m_Config.DevicePreferredVectorWidthHalf,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
        if( m_Config.DevicePreferredVectorWidthFloat != UINT_MAX )
        {
            cl_uint*    ptr = (cl_uint*)param_value;
            errorCode = writeParamToMemory(
                param_value_size,
                m_Config.DevicePreferredVectorWidthFloat,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
        if( m_Config.DevicePreferredVectorWidthDouble != UINT_MAX )
        {
            cl_uint*    ptr = (cl_uint*)param_value;
            errorCode = writeParamToMemory(
                param_value_size,
                m_Config.DevicePreferredVectorWidthDouble,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_NUMERIC_VERSION_KHR:
        if( m_Config.Emulate_cl_khr_extended_versioning )
        {
            char*   deviceVersion = NULL;

            errorCode = allocateAndGetDeviceInfoString(
                device,
                CL_DEVICE_VERSION,
                deviceVersion );
            if( errorCode == CL_SUCCESS && deviceVersion )
            {
                // According to the spec, the device version string should have the form:
                //   OpenCL <Major>.<Minor> <Vendor Specific Info>
                size_t  major = 0;
                size_t  minor = 0;
                if( getMajorMinorVersionFromString(
                        "OpenCL ",
                        deviceVersion,
                        major,
                        minor ) )
                {
                    cl_version_khr* ptr = (cl_version_khr*)param_value;
                    cl_version_khr  version = CL_MAKE_VERSION_KHR( major, minor, 0 );
                    errorCode = writeParamToMemory(
                        param_value_size,
                        version,
                        param_value_size_ret,
                        ptr );
                    override = true;
                }
            }

            delete [] deviceVersion;
            deviceVersion = NULL;
        }
        break;
    case CL_DEVICE_OPENCL_C_NUMERIC_VERSION_KHR:
        if( m_Config.Emulate_cl_khr_extended_versioning )
        {
            char*   deviceOpenCLCVersion = NULL;

            errorCode = allocateAndGetDeviceInfoString(
                device,
                CL_DEVICE_OPENCL_C_VERSION,
                deviceOpenCLCVersion );
            if( errorCode == CL_SUCCESS && deviceOpenCLCVersion )
            {
                // According to the spec, the OpenCL C version string should have the form:
                //   OpenCL C <Major>.<Minor> <Vendor Specific Info>
                size_t  major = 0;
                size_t  minor = 0;
                if( getMajorMinorVersionFromString(
                        "OpenCL C ",
                        deviceOpenCLCVersion,
                        major,
                        minor ) )
                {
                    cl_version_khr* ptr = (cl_version_khr*)param_value;
                    cl_version_khr  version = CL_MAKE_VERSION_KHR( major, minor, 0 );
                    errorCode = writeParamToMemory(
                        param_value_size,
                        version,
                        param_value_size_ret,
                        ptr );
                    override = true;
                }
            }

            delete [] deviceOpenCLCVersion;
            deviceOpenCLCVersion = NULL;
        }
        break;
    case CL_DEVICE_EXTENSIONS_WITH_VERSION_KHR:
        if( m_Config.Emulate_cl_khr_extended_versioning )
        {
            std::string deviceExtensions;
            if( m_Config.Emulate_cl_khr_extended_versioning &&
                !checkDeviceForExtension( device, "cl_khr_extended_versioning") )
            {
                deviceExtensions += "cl_khr_extended_versioning ";
            }
            if( m_Config.Emulate_cl_intel_unified_shared_memory &&
                !checkDeviceForExtension( device, "cl_intel_unified_shared_memory") )
            {
                deviceExtensions += "cl_intel_unified_shared_memory ";
            }

            char*   tempDeviceExtensions = NULL;

            allocateAndGetDeviceInfoString(
                device,
                CL_DEVICE_EXTENSIONS,
                tempDeviceExtensions );
            deviceExtensions += tempDeviceExtensions;

            // Parse the extension string even if the query returned an error.
            // In this case we will simply return that zero extensions are supported.
            cl_name_version_khr*    ptr = (cl_name_version_khr*)param_value;
            errorCode = parseExtensionString(
                deviceExtensions.c_str(),
                ptr,
                param_value_size,
                param_value_size_ret );
            override = true;

            delete [] tempDeviceExtensions;
            tempDeviceExtensions = NULL;
        }
        break;
    case CL_DEVICE_ILS_WITH_VERSION_KHR:
        if( m_Config.Emulate_cl_khr_extended_versioning )
        {
            char*   deviceILs = NULL;

            allocateAndGetDeviceInfoString(
                device,
                CL_DEVICE_IL_VERSION,
                deviceILs );

            // Parse the extension string even if the query returned an error.
            // In this case we will simply return that zero ILs are supported.
            cl_name_version_khr*    ptr = (cl_name_version_khr*)param_value;
            errorCode = parseILString(
                deviceILs,
                ptr,
                param_value_size,
                param_value_size_ret );
            override = true;

            delete [] deviceILs;
            deviceILs = NULL;
        }
        break;
    case CL_DEVICE_BUILT_IN_KERNELS_WITH_VERSION_KHR:
        if( m_Config.Emulate_cl_khr_extended_versioning )
        {
            char*   deviceBuiltInKernels = NULL;

            allocateAndGetDeviceInfoString(
                device,
                CL_DEVICE_BUILT_IN_KERNELS,
                deviceBuiltInKernels );

            // Parse the built-in kernels string even if the query returned
            // an error.  In this case we will simply return that zero
            // built-in kernels are supported.
            cl_name_version_khr*    ptr = (cl_name_version_khr*)param_value;
            errorCode = parseBuiltInKernelsString(
                deviceBuiltInKernels,
                ptr,
                param_value_size,
                param_value_size_ret );
            override = true;

            delete [] deviceBuiltInKernels;
            deviceBuiltInKernels = NULL;
        }
        break;
    case CL_DRIVER_VERSION:
        if( m_Config.DriverVersion != "" )
        {
            char*   ptr = (char*)param_value;
            errorCode = writeStringToMemory(
                param_value_size,
                m_Config.DriverVersion,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL:
    case CL_DEVICE_DEVICE_MEM_CAPABILITIES_INTEL:
    case CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL:
    case CL_DEVICE_CROSS_DEVICE_SHARED_MEM_CAPABILITIES_INTEL:
    case CL_DEVICE_SHARED_SYSTEM_MEM_CAPABILITIES_INTEL:
        if( m_Config.Emulate_cl_intel_unified_shared_memory )
        {
            // Shorthand:
            const cl_bitfield acc = CL_UNIFIED_SHARED_MEMORY_ACCESS_INTEL;
            const cl_bitfield ato = CL_UNIFIED_SHARED_MEMORY_ATOMIC_ACCESS_INTEL;
            const cl_bitfield cacc = CL_UNIFIED_SHARED_MEMORY_CONCURRENT_ACCESS_INTEL;
            const cl_bitfield cato = CL_UNIFIED_SHARED_MEMORY_CONCURRENT_ATOMIC_ACCESS_INTEL;

            // Caps:
            cl_device_unified_shared_memory_capabilities_intel hostCaps = 0;
            cl_device_unified_shared_memory_capabilities_intel deviceCaps = 0;
            cl_device_unified_shared_memory_capabilities_intel sdSharedCaps = 0;
            cl_device_unified_shared_memory_capabilities_intel cdSharedCaps = 0;
            cl_device_unified_shared_memory_capabilities_intel sysSharedCaps = 0;

            cl_device_svm_capabilities svmCaps = 0;
            dispatch().clGetDeviceInfo(
                device,
                CL_DEVICE_SVM_CAPABILITIES,
                sizeof(svmCaps),
                &svmCaps,
                NULL );
            if( svmCaps & CL_DEVICE_SVM_FINE_GRAIN_SYSTEM )
            {
                //log( "Returning USM caps for: System SVM\n" );
                hostCaps =      acc | ato | cacc;
                deviceCaps =    acc | ato;
                sdSharedCaps =  acc | ato | cacc;
                cdSharedCaps =  acc | ato | cacc;
                sysSharedCaps = acc | ato | cacc;
                if( svmCaps & CL_DEVICE_SVM_ATOMICS )
                {
                    hostCaps |=      cato;
                    deviceCaps |=    cato;
                    sdSharedCaps |=  cato;
                    cdSharedCaps |=  cato;
                    sysSharedCaps |= cato;
                }
            }
            else if( svmCaps & CL_DEVICE_SVM_FINE_GRAIN_BUFFER )
            {
                //log( "Returning USM caps for: Fine Grain Buffer SVM\n" );
                hostCaps =      acc | ato | cacc;
                deviceCaps =    acc | ato;
                sdSharedCaps =  acc | ato | cacc;
                cdSharedCaps =  acc | ato | cacc;
                if( svmCaps & CL_DEVICE_SVM_ATOMICS )
                {
                    hostCaps |=     cato;
                    deviceCaps |=   cato;
                    sdSharedCaps |= cato;
                    cdSharedCaps |=   cato;
                }
            }
            else if( svmCaps & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER )
            {
                //log( "Returning USM caps for: Coarse Grain Buffer SVM\n" );
                deviceCaps = acc | ato;
            }

            cl_device_unified_shared_memory_capabilities_intel*    ptr =
                (cl_device_unified_shared_memory_capabilities_intel*)param_value;

            switch( param_name )
            {
            case CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL:
                errorCode = writeParamToMemory(
                    param_value_size,
                    hostCaps,
                    param_value_size_ret,
                    ptr );
                override = true;
                break;
            case CL_DEVICE_DEVICE_MEM_CAPABILITIES_INTEL:
                errorCode = writeParamToMemory(
                    param_value_size,
                    deviceCaps,
                    param_value_size_ret,
                    ptr );
                override = true;
                break;
            case CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL:
                errorCode = writeParamToMemory(
                    param_value_size,
                    sdSharedCaps,
                    param_value_size_ret,
                    ptr );
                override = true;
                break;
            case CL_DEVICE_CROSS_DEVICE_SHARED_MEM_CAPABILITIES_INTEL:
                errorCode = writeParamToMemory(
                    param_value_size,
                    cdSharedCaps,
                    param_value_size_ret,
                    ptr );
                override = true;
                break;
            case CL_DEVICE_SHARED_SYSTEM_MEM_CAPABILITIES_INTEL:
                errorCode = writeParamToMemory(
                    param_value_size,
                    sysSharedCaps,
                    param_value_size_ret,
                    ptr );
                override = true;
                break;
            default:
                break;
            }
        }
        break;
    case CL_DEVICE_SEMAPHORE_TYPES_KHR:
        if( m_Config.Emulate_cl_khr_semaphore )
        {
            // If we decide to emulate multiple semaphore types we will need
            // to return an array, but for now we can return just the binary
            // semaphore type.
            auto    ptr = (cl_semaphore_type_khr*)param_value;
            cl_semaphore_type_khr type = CL_SEMAPHORE_TYPE_BINARY_KHR;
            errorCode = writeParamToMemory(
                param_value_size,
                type,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    default:
        break;
    }

    return override;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::ReadBuffer(
    cl_command_queue commandQueue,
    cl_mem srcBuffer,
    cl_bool blockingRead,
    size_t srcOffset,
    size_t bytesToRead,
    void* dstPtr,
    cl_uint numEventsInWaitList,
    const cl_event* eventWaitList,
    cl_event* event )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_int  errorCode = CL_SUCCESS;

    cl_context  context = NULL;

    // Get the context for this command queue.
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetCommandQueueInfo(
            commandQueue,
            CL_QUEUE_CONTEXT,
            sizeof( context ),
            &context,
            NULL );
    }

    size_t  dstOffset = 0;

    // Align the passed-in pointer to a page boundary.
    if( errorCode == CL_SUCCESS )
    {
        const size_t    alignSize = 4096;

        unsigned char*  bptr = (unsigned char*)dstPtr;
        uintptr_t   uiptr = (uintptr_t)bptr;

        dstOffset = uiptr % alignSize;
        bptr -= dstOffset;

        dstPtr = bptr;
    }

    cl_mem  dstBuffer = NULL;

    // Create a USE_HOST_PTR buffer for the passed-in pointer.
    // The size of the buffer will be at least dstOffset + bytesToRead.
    if( errorCode == CL_SUCCESS )
    {
        size_t  dstBufferSize = dstOffset + bytesToRead;

        dstBuffer = dispatch().clCreateBuffer(
            context,
            CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
            dstBufferSize,
            dstPtr,
            &errorCode );
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = CopyBufferHelper(
            context,
            commandQueue,
            srcBuffer,
            dstBuffer,
            srcOffset,
            dstOffset,
            bytesToRead,
            numEventsInWaitList,
            eventWaitList,
            event );
    }

    // Technically, we need to map and unmap the destination buffer
    // to transfer data to our pointer.  This will also handle
    // blockingRead.
    if( errorCode == CL_SUCCESS )
    {
        void* mappedPointer = dispatch().clEnqueueMapBuffer(
            commandQueue,
            dstBuffer,
            blockingRead,
            CL_MAP_READ,
            dstOffset,
            bytesToRead,
            0,
            NULL,
            NULL,
            &errorCode );

        if( errorCode == CL_SUCCESS )
        {
            errorCode = dispatch().clEnqueueUnmapMemObject(
                commandQueue,
                dstBuffer,
                mappedPointer,
                0,
                NULL,
                NULL );
        }
    }

    dispatch().clReleaseMemObject( dstBuffer );

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::WriteBuffer(
    cl_command_queue commandQueue,
    cl_mem dstBuffer,
    cl_bool blockingWrite,
    size_t dstOffset,
    size_t bytesToWrite,
    const void* srcPtr,
    cl_uint numEventsInWaitList,
    const cl_event* eventWaitList,
    cl_event* event )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_int  errorCode = CL_SUCCESS;

    cl_context  context = NULL;

    // Get the context for this command queue.
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetCommandQueueInfo(
            commandQueue,
            CL_QUEUE_CONTEXT,
            sizeof( context ),
            &context,
            NULL );
    }

    size_t  srcOffset = 0;

    // Align the passed-in pointer to a page boundary.
    if( errorCode == CL_SUCCESS )
    {
        const size_t    alignSize = 4096;

        unsigned char*  bptr = (unsigned char*)srcPtr;
        uintptr_t   uiptr = (uintptr_t)bptr;

        srcOffset = uiptr % alignSize;
        bptr -= srcOffset;

        srcPtr = bptr;
    }

    cl_mem  srcBuffer = NULL;

    // Create a USE_HOST_PTR buffer for the passed-in pointer.
    // The size of the buffer will be at least srcOffset + bytesToWrite.
    if( errorCode == CL_SUCCESS )
    {
        size_t  srcBufferSize = srcOffset + bytesToWrite;

        srcBuffer = dispatch().clCreateBuffer(
            context,
            CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
            srcBufferSize,
            (void*)srcPtr,
            &errorCode );
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = CopyBufferHelper(
            context,
            commandQueue,
            srcBuffer,
            dstBuffer,
            srcOffset,
            dstOffset,
            bytesToWrite,
            numEventsInWaitList,
            eventWaitList,
            event );
    }

    if( errorCode == CL_SUCCESS )
    {
        if( blockingWrite )
        {
            errorCode = dispatch().clFinish(
                commandQueue );
        }
    }

    dispatch().clReleaseMemObject( srcBuffer );

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::CopyBuffer(
    cl_command_queue commandQueue,
    cl_mem srcBuffer,
    cl_mem dstBuffer,
    size_t srcOffset,
    size_t dstOffset,
    size_t bytesToCopy,
    cl_uint numEventsInWaitList,
    const cl_event* eventWaitList,
    cl_event* event )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_int  errorCode = CL_SUCCESS;

    cl_context  context = NULL;

    // Get the context for this command queue.
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetCommandQueueInfo(
            commandQueue,
            CL_QUEUE_CONTEXT,
            sizeof( context ),
            &context,
            NULL );
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = CopyBufferHelper(
            context,
            commandQueue,
            srcBuffer,
            dstBuffer,
            srcOffset,
            dstOffset,
            bytesToCopy,
            numEventsInWaitList,
            eventWaitList,
            event );
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::CopyBufferHelper(
    cl_context context,
    cl_command_queue commandQueue,
    cl_mem srcBuffer,
    cl_mem dstBuffer,
    size_t srcOffset,
    size_t dstOffset,
    size_t bytesToCopy,
    cl_uint numEventsInWaitList,
    const cl_event* eventWaitList,
    cl_event* event )
{
    // This function assumes that it is being called from within a critical
    // section, so it does not enter the critical section again.

    cl_int  errorCode = CL_SUCCESS;

    SPrecompiledKernelOverrides*    pOverrides = NULL;

    // Get the overrides for this context.
    if( errorCode == CL_SUCCESS )
    {
        pOverrides = m_PrecompiledKernelOverridesMap[ context ];
        if( pOverrides == NULL )
        {
            errorCode = CL_INVALID_VALUE;
        }
    }

    if( false &&    // disabled - this kernel is slower than the UInt4 kernel
        ( m_Config.ForceByteBufferOverrides == false ) &&
        ( ( srcOffset % 64 ) == 0 ) &&
        ( ( dstOffset % 64 ) == 0 ) )
    {
        if( errorCode == CL_SUCCESS )
        {
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInt16s,
                0,
                sizeof( srcBuffer ),
                &srcBuffer );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInt16s,
                1,
                sizeof( dstBuffer ),
                &dstBuffer );

            cl_uint uiSrcOffsetInUint16s = (cl_uint)( srcOffset / 64 );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInt16s,
                2,
                sizeof( uiSrcOffsetInUint16s ),
                &uiSrcOffsetInUint16s );

            cl_uint uiDstOffsetInUint16s = (cl_uint)( dstOffset / 64 );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInt16s,
                3,
                sizeof( uiDstOffsetInUint16s ),
                &uiDstOffsetInUint16s );

            cl_uint uiBytesToCopy = (cl_uint)( bytesToCopy );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInt16s,
                4,
                sizeof( uiBytesToCopy ),
                &uiBytesToCopy );

            if( errorCode == CL_SUCCESS )
            {
                size_t  global_work_size = bytesToCopy / 64;
                size_t  local_work_size = 32;

                // Round up if we don't have an even multiple of UInt16s
                if( ( bytesToCopy % 64 ) != 0 )
                {
                    global_work_size++;
                }

                // Make sure global_work_size is an even multiple of local_work_size
                if( ( global_work_size % local_work_size ) != 0 )
                {
                    global_work_size +=
                        local_work_size -
                        ( global_work_size % local_work_size );
                }

                // Execute kernel
                errorCode = dispatch().clEnqueueNDRangeKernel(
                    commandQueue,
                    pOverrides->Kernel_CopyBufferUInt16s,
                    1,
                    NULL,
                    &global_work_size,
                    &local_work_size,
                    numEventsInWaitList,
                    eventWaitList,
                    event );
            }
        }
    }
    else if( ( m_Config.ForceByteBufferOverrides == false ) &&
             ( ( srcOffset % 16 ) == 0 ) &&
             ( ( dstOffset % 16 ) == 0 ) )
    {
        if( errorCode == CL_SUCCESS )
        {
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInt4s,
                0,
                sizeof( srcBuffer ),
                &srcBuffer );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInt4s,
                1,
                sizeof( dstBuffer ),
                &dstBuffer );

            cl_uint uiSrcOffsetInUint4s = (cl_uint)( srcOffset / 16 );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInt4s,
                2,
                sizeof( uiSrcOffsetInUint4s ),
                &uiSrcOffsetInUint4s );

            cl_uint uiDstOffsetInUint4s = (cl_uint)( dstOffset / 16 );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInt4s,
                3,
                sizeof( uiDstOffsetInUint4s ),
                &uiDstOffsetInUint4s );

            cl_uint uiBytesToCopy = (cl_uint)( bytesToCopy );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInt4s,
                4,
                sizeof( uiBytesToCopy ),
                &uiBytesToCopy );

            if( errorCode == CL_SUCCESS )
            {
                size_t  global_work_size = bytesToCopy / 16;
                size_t  local_work_size = 32;

                // Round up if we don't have an even multiple of UInt4s
                if( ( bytesToCopy % 16 ) != 0 )
                {
                    global_work_size++;
                }

                // Make sure global_work_size is an even multiple of local_work_size
                if( ( global_work_size % local_work_size ) != 0 )
                {
                    global_work_size +=
                        local_work_size -
                        ( global_work_size % local_work_size );
                }

                // Execute kernel
                errorCode = dispatch().clEnqueueNDRangeKernel(
                    commandQueue,
                    pOverrides->Kernel_CopyBufferUInt4s,
                    1,
                    NULL,
                    &global_work_size,
                    &local_work_size,
                    numEventsInWaitList,
                    eventWaitList,
                    event );
            }
        }
    }
    else if( ( m_Config.ForceByteBufferOverrides == false ) &&
             ( ( srcOffset % 4 ) == 0 ) &&
             ( ( dstOffset % 4 ) == 0 ) )
    {
        if( errorCode == CL_SUCCESS )
        {
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInts,
                0,
                sizeof( srcBuffer ),
                &srcBuffer );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInts,
                1,
                sizeof( dstBuffer ),
                &dstBuffer );

            cl_uint uiSrcOffsetInUints = (cl_uint)( srcOffset / 4 );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInts,
                2,
                sizeof( uiSrcOffsetInUints ),
                &uiSrcOffsetInUints );

            cl_uint uiDstOffsetInUints = (cl_uint)( dstOffset / 4 );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInts,
                3,
                sizeof( uiDstOffsetInUints ),
                &uiDstOffsetInUints );

            cl_uint uiBytesToCopy = (cl_uint)( bytesToCopy );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferUInts,
                4,
                sizeof( uiBytesToCopy ),
                &uiBytesToCopy );

            if( errorCode == CL_SUCCESS )
            {
                size_t  global_work_size = bytesToCopy / 4;
                size_t  local_work_size = 32;

                // Round up if we don't have an even multiple of UInts
                if( ( bytesToCopy % 4 ) != 0 )
                {
                    global_work_size++;
                }

                // Make sure global_work_size is an even multiple of local_work_size
                if( ( global_work_size % local_work_size ) != 0 )
                {
                    global_work_size +=
                        local_work_size -
                        ( global_work_size % local_work_size );
                }

                // Execute kernel
                errorCode = dispatch().clEnqueueNDRangeKernel(
                    commandQueue,
                    pOverrides->Kernel_CopyBufferUInts,
                    1,
                    NULL,
                    &global_work_size,
                    &local_work_size,
                    numEventsInWaitList,
                    eventWaitList,
                    event );
            }
        }
    }
    else
    {
        if( errorCode == CL_SUCCESS )
        {
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferBytes,
                0,
                sizeof( srcBuffer ),
                &srcBuffer );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferBytes,
                1,
                sizeof( dstBuffer ),
                &dstBuffer );

            cl_uint uiSrcOffset = (cl_uint)( srcOffset );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferBytes,
                2,
                sizeof( uiSrcOffset ),
                &uiSrcOffset );

            cl_uint uiDstOffset = (cl_uint)( dstOffset );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferBytes,
                3,
                sizeof( uiDstOffset ),
                &uiDstOffset );

            cl_uint uiBytesToCopy = (cl_uint)( bytesToCopy );
            errorCode |= dispatch().clSetKernelArg(
                pOverrides->Kernel_CopyBufferBytes,
                4,
                sizeof( uiBytesToCopy ),
                &uiBytesToCopy );

            if( errorCode == CL_SUCCESS )
            {
                size_t  global_work_size = bytesToCopy;
                size_t  local_work_size = 32;

                // Make sure global_work_size is an even multiple of local_work_size
                if( ( global_work_size % local_work_size ) != 0 )
                {
                    global_work_size +=
                        local_work_size -
                        ( global_work_size % local_work_size );
                }

                // Execute kernel
                errorCode = dispatch().clEnqueueNDRangeKernel(
                    commandQueue,
                    pOverrides->Kernel_CopyBufferBytes,
                    1,
                    NULL,
                    &global_work_size,
                    &local_work_size,
                    numEventsInWaitList,
                    eventWaitList,
                    event );
            }
        }
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::ReadImage(
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
    cl_event* event )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_int  errorCode = CL_SUCCESS;

    // Basic error checking, to avoid possible null pointer dereferences.
    if( errorCode == CL_SUCCESS )
    {
        if( srcOrigin == NULL || region == NULL )
        {
            errorCode = CL_INVALID_VALUE;
        }
    }

    cl_context  context = NULL;

    // Get the context for this command queue.
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetCommandQueueInfo(
            commandQueue,
            CL_QUEUE_CONTEXT,
            sizeof( context ),
            &context,
            NULL );
    }

    // Create a USE_HOST_PTR image for the passed-in pointer.
    // The size of the buffer will be at least as big as the region to read.

    // We need to know what type of image to create.
    // If region[2] is 1, then a 2D image will suffice,
    // otherwise we'll need to create a 3D image.

    // The image will have the same image format as srcImage.

    cl_image_format srcFormat = { 0 };

    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetImageInfo(
            srcImage,
            CL_IMAGE_FORMAT,
            sizeof( srcFormat ),
            &srcFormat,
            NULL );
    }

    cl_mem  dstImage = NULL;

    if( errorCode == CL_SUCCESS )
    {
        if( region[2] == 1 )
        {
            // 2D image
            dstImage = dispatch().clCreateImage2D(
                context,
                CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                &srcFormat,
                region[0],
                region[1],
                dstRowPitch,
                dstPtr,
                &errorCode );
        }
        else
        {
            // 3D image
            dstImage = dispatch().clCreateImage3D(
                context,
                CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
                &srcFormat,
                region[0],
                region[1],
                region[2],
                dstRowPitch,
                dstSlicePitch,
                dstPtr,
                &errorCode );
        }
    }

    size_t  dstOrigin[3] = { 0, 0, 0 };

    if( errorCode == CL_SUCCESS )
    {
        errorCode = CopyImageHelper(
            context,
            commandQueue,
            srcImage,
            dstImage,
            srcOrigin,
            dstOrigin,
            region,
            numEventsInWaitList,
            eventWaitList,
            event );
    }

    // Technically, we need to map and unmap the destination image
    // to transfer data to our pointer.  This will also handle
    // blockingRead.
    if( errorCode == CL_SUCCESS )
    {
        void* mappedPointer = dispatch().clEnqueueMapImage(
            commandQueue,
            dstImage,
            blockingRead,
            CL_MAP_READ,
            dstOrigin,
            region,
            &dstRowPitch,
            &dstSlicePitch,
            0,
            NULL,
            NULL,
            &errorCode );

        if( errorCode == CL_SUCCESS )
        {
            errorCode = dispatch().clEnqueueUnmapMemObject(
                commandQueue,
                dstImage,
                mappedPointer,
                0,
                NULL,
                NULL );
        }
    }

    dispatch().clReleaseMemObject( dstImage );

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::WriteImage(
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
    cl_event* event )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_int  errorCode = CL_SUCCESS;

    // Basic error checking, to avoid possible null pointer dereferences.
    if( errorCode == CL_SUCCESS )
    {
        if( dstOrigin == NULL || region == NULL )
        {
            errorCode = CL_INVALID_VALUE;
        }
    }

    cl_context  context = NULL;

    // Get the context for this command queue.
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetCommandQueueInfo(
            commandQueue,
            CL_QUEUE_CONTEXT,
            sizeof( context ),
            &context,
            NULL );
    }

    // Create a USE_HOST_PTR image for the passed-in pointer.
    // The size of the buffer will be at least as big as the region to read.

    // We need to know what type of image to create.
    // If region[2] is 1, then a 2D image will suffice,
    // otherwise we'll need to create a 3D image.

    // The image will have the same image format as srcImage.

    cl_image_format dstFormat = { 0 };

    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetImageInfo(
            dstImage,
            CL_IMAGE_FORMAT,
            sizeof( dstFormat ),
            &dstFormat,
            NULL );
    }

    cl_mem  srcImage = NULL;

    if( errorCode == CL_SUCCESS )
    {
        if( region[2] == 1 )
        {
            // 2D image
            srcImage = dispatch().clCreateImage2D(
                context,
                CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                &dstFormat,
                region[0],
                region[1],
                srcRowPitch,
                (void*)srcPtr,
                &errorCode );
        }
        else
        {
            // 3D image
            srcImage = dispatch().clCreateImage3D(
                context,
                CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                &dstFormat,
                region[0],
                region[1],
                region[2],
                srcRowPitch,
                srcSlicePitch,
                (void*)srcPtr,
                &errorCode );
        }
    }

    size_t  srcOrigin[3] = { 0, 0, 0 };

    if( errorCode == CL_SUCCESS )
    {
        errorCode = CopyImageHelper(
            context,
            commandQueue,
            srcImage,
            dstImage,
            srcOrigin,
            dstOrigin,
            region,
            numEventsInWaitList,
            eventWaitList,
            event );
    }

    if( errorCode == CL_SUCCESS )
    {
        if( blockingWrite )
        {
            errorCode = dispatch().clFinish(
                commandQueue );
        }
    }

    dispatch().clReleaseMemObject( srcImage );

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::CopyImage(
    cl_command_queue commandQueue,
    cl_mem srcImage,
    cl_mem dstImage,
    const size_t* srcOrigin,
    const size_t* dstOrigin,
    const size_t* region,
    cl_uint numEventsInWaitList,
    const cl_event* eventWaitList,
    cl_event* event )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_int  errorCode = CL_SUCCESS;

    cl_context  context = NULL;

    // Get the context for this command queue.
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetCommandQueueInfo(
            commandQueue,
            CL_QUEUE_CONTEXT,
            sizeof( context ),
            &context,
            NULL );
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode = CopyImageHelper(
            context,
            commandQueue,
            srcImage,
            dstImage,
            srcOrigin,
            dstOrigin,
            region,
            numEventsInWaitList,
            eventWaitList,
            event );
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::CopyImageHelper(
    cl_context context,
    cl_command_queue commandQueue,
    cl_mem srcImage,
    cl_mem dstImage,
    const size_t* srcOrigin,
    const size_t* dstOrigin,
    const size_t* region,
    cl_uint numEventsInWaitList,
    const cl_event* eventWaitList,
    cl_event* event )
{
    // This function assumes that it is being called from within a critical
    // section, so it does not enter the critical section again.

    cl_int  errorCode = CL_SUCCESS;

    SPrecompiledKernelOverrides*    pOverrides = NULL;

    // Get the overrides for this context.
    if( errorCode == CL_SUCCESS )
    {
        pOverrides = m_PrecompiledKernelOverridesMap[ context ];
        if( pOverrides == NULL )
        {
            errorCode = CL_INVALID_VALUE;
        }
    }

    // Figure out the type of the source image.
    cl_mem_object_type  srcType = CL_MEM_OBJECT_BUFFER;
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetMemObjectInfo(
            srcImage,
            CL_MEM_TYPE,
            sizeof( srcType ),
            &srcType,
            NULL );
    }

    // Figure out the type of the destination image.
    cl_mem_object_type  dstType = CL_MEM_OBJECT_BUFFER;
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetMemObjectInfo(
            srcImage,
            CL_MEM_TYPE,
            sizeof( dstType ),
            &dstType,
            NULL );
    }

    // Figure out the format of the source image.
    cl_image_format srcFormat = { 0 };
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetImageInfo(
            srcImage,
            CL_IMAGE_FORMAT,
            sizeof( srcFormat ),
            &srcFormat,
            NULL );
    }

    // Figure out the format of the destination image.
    cl_image_format dstFormat = { 0 };
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetImageInfo(
            dstImage,
            CL_IMAGE_FORMAT,
            sizeof( dstFormat ),
            &dstFormat,
            NULL );
    }

    // Image formats must match.
    if( errorCode == CL_SUCCESS )
    {
        if( ( srcFormat.image_channel_data_type != dstFormat.image_channel_data_type ) ||
            ( srcFormat.image_channel_order != dstFormat.image_channel_order ) )
        {
            errorCode = CL_IMAGE_FORMAT_MISMATCH;
        }
        switch( srcType )
        {
        case CL_MEM_OBJECT_IMAGE2D:
            if( ( srcOrigin[2] != 0 ) ||
                ( region[2] != 1 ) )
            {
                errorCode = CL_INVALID_VALUE;
            }
            break;
        case CL_MEM_OBJECT_IMAGE3D:
            break;
        default:
            errorCode = CL_INVALID_OPERATION;
            break;
        }
        switch( dstType )
        {
        case CL_MEM_OBJECT_IMAGE2D:
            if( ( dstOrigin[2] != 0 ) ||
                ( region[2] != 1 ) )
            {
                errorCode = CL_INVALID_VALUE;
            }
            break;
        case CL_MEM_OBJECT_IMAGE3D:
            break;
        default:
            errorCode = CL_INVALID_OPERATION;
            break;
        }
    }

    cl_kernel   kernel = NULL;
    if( errorCode == CL_SUCCESS )
    {
        switch( srcFormat.image_channel_data_type )
        {
        case CL_UNORM_INT8:
        case CL_UNORM_INT16:
        case CL_SNORM_INT8:
        case CL_SNORM_INT16:
        case CL_HALF_FLOAT:
        case CL_FLOAT:
            // "Float" Images
            switch( srcType )
            {
            case CL_MEM_OBJECT_IMAGE2D:
                switch( dstType )
                {
                case CL_MEM_OBJECT_IMAGE2D:
                    // 2D to 2D
                    kernel = pOverrides->Kernel_CopyImage2Dto2DFloat;
                    break;
                default:
                    CLI_ASSERT( 0 );
                    errorCode = CL_INVALID_OPERATION;
                    break;
                }
                break;
            default:
                CLI_ASSERT( 0 );
                errorCode = CL_INVALID_OPERATION;
                break;
            }
            break;

        case CL_SIGNED_INT8:
        case CL_SIGNED_INT16:
        case CL_SIGNED_INT32:
            // "Int" Images
            switch( srcType )
            {
            case CL_MEM_OBJECT_IMAGE2D:
                switch( dstType )
                {
                case CL_MEM_OBJECT_IMAGE2D:
                    // 2D to 2D
                    kernel = pOverrides->Kernel_CopyImage2Dto2DInt;
                    break;
                default:
                    CLI_ASSERT( 0 );
                    errorCode = CL_INVALID_OPERATION;
                    break;
                }
                break;
            default:
                CLI_ASSERT( 0 );
                errorCode = CL_INVALID_OPERATION;
                break;
            }
            break;

        case CL_UNSIGNED_INT8:
        case CL_UNSIGNED_INT16:
        case CL_UNSIGNED_INT32:
            // "UInt" Images
            switch( srcType )
            {
            case CL_MEM_OBJECT_IMAGE2D:
                switch( dstType )
                {
                case CL_MEM_OBJECT_IMAGE2D:
                    // 2D to 2D
                    kernel = pOverrides->Kernel_CopyImage2Dto2DUInt;
                    break;
                default:
                    CLI_ASSERT( 0 );
                    errorCode = CL_INVALID_OPERATION;
                    break;
                }
                break;
            default:
                CLI_ASSERT( 0 );
                errorCode = CL_INVALID_OPERATION;
                break;
            }
            break;

        default:
            CLI_ASSERT( 0 );
            errorCode = CL_INVALID_IMAGE_FORMAT_DESCRIPTOR;
            break;
        }
    }

    if( errorCode == CL_SUCCESS )
    {
        errorCode |= dispatch().clSetKernelArg(
            kernel,
            0,
            sizeof( srcImage ),
            &srcImage );
        errorCode |= dispatch().clSetKernelArg(
            kernel,
            1,
            sizeof( dstImage ),
            &dstImage );

        cl_uint uiArg = (cl_uint)( srcOrigin[0] );
        errorCode |= dispatch().clSetKernelArg(
            kernel,
            2,
            sizeof( uiArg ),
            &uiArg );
        uiArg = (cl_uint)( srcOrigin[1] );
        errorCode |= dispatch().clSetKernelArg(
            kernel,
            3,
            sizeof( uiArg ),
            &uiArg );
        uiArg = (cl_uint)( srcOrigin[2] );
        errorCode |= dispatch().clSetKernelArg(
            kernel,
            4,
            sizeof( uiArg ),
            &uiArg );

        uiArg = (cl_uint)( dstOrigin[0] );
        errorCode |= dispatch().clSetKernelArg(
            kernel,
            5,
            sizeof( uiArg ),
            &uiArg );
        uiArg = (cl_uint)( dstOrigin[1] );
        errorCode |= dispatch().clSetKernelArg(
            kernel,
            6,
            sizeof( uiArg ),
            &uiArg );
        uiArg = (cl_uint)( dstOrigin[2] );
        errorCode |= dispatch().clSetKernelArg(
            kernel,
            7,
            sizeof( uiArg ),
            &uiArg );

        uiArg = (cl_uint)( region[0] );
        errorCode |= dispatch().clSetKernelArg(
            kernel,
            8,
            sizeof( uiArg ),
            &uiArg );
        uiArg = (cl_uint)( region[1] );
        errorCode |= dispatch().clSetKernelArg(
            kernel,
            9,
            sizeof( uiArg ),
            &uiArg );
        uiArg = (cl_uint)( region[2] );
        errorCode |= dispatch().clSetKernelArg(
            kernel,
            10,
            sizeof( uiArg ),
            &uiArg );

        if( errorCode == CL_SUCCESS )
        {
            size_t  global_work_size[3] =
            {
                region[0],
                region[1],
                region[2]
            };
            size_t  local_work_size[3] =
            {
                32,
                1,
                1
            };

            // Make sure global_work_size is an even multiple of local_work_size
            if( ( global_work_size[0] % local_work_size[0] ) != 0 )
            {
                global_work_size[0] +=
                    local_work_size[0] -
                    ( global_work_size[0] % local_work_size[0] );
            }
            CLI_ASSERT( local_work_size[1] == 1 );
            CLI_ASSERT( local_work_size[2] == 1 );

            // Execute kernel
            errorCode = dispatch().clEnqueueNDRangeKernel(
                commandQueue,
                kernel,
                3,
                NULL,
                global_work_size,
                local_work_size,
                numEventsInWaitList,
                eventWaitList,
                event );
        }
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_program CLIntercept::createProgramWithBuiltinKernels(
    cl_context context )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_program  program = NULL;

    SBuiltinKernelOverrides*    pOverrides = m_BuiltinKernelOverridesMap[ context ];
    if( pOverrides )
    {
        program = pOverrides->Program;
        dispatch().clRetainProgram( program );
    }

    return program;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_kernel CLIntercept::createBuiltinKernel(
    cl_program program,
    const std::string& kernel_name,
    cl_int* errcode_ret )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_int  errorCode = CL_SUCCESS;

    cl_context  context = NULL;
    cl_kernel   kernel = NULL;

    // Get the context for this program.
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetProgramInfo(
            program,
            CL_PROGRAM_CONTEXT,
            sizeof( context ),
            &context,
            NULL );
    }

    SBuiltinKernelOverrides*    pOverrides = NULL;

    // Get the overrides for this context.
    if( errorCode == CL_SUCCESS )
    {
        pOverrides = m_BuiltinKernelOverridesMap[ context ];
        if( pOverrides != NULL )
        {
            if( kernel_name == "block_motion_estimate_intel" )
            {
                kernel = pOverrides->Kernel_block_motion_estimate_intel;
                dispatch().clRetainKernel( kernel );
                if( errcode_ret )
                {
                    errcode_ret[0] = CL_SUCCESS;
                }
            }
        }
    }

    return kernel;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::NDRangeBuiltinKernel(
    cl_command_queue commandQueue,
    cl_kernel kernel,
    cl_uint work_dim,
    const size_t* global_work_offset,
    const size_t* global_work_size,
    const size_t* local_work_size,
    cl_uint num_events_in_wait_list,
    const cl_event* event_wait_list,
    cl_event* event )
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    cl_int  errorCode = CL_SUCCESS;

    cl_context  context = NULL;

    // Get the context for this command queue.
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetCommandQueueInfo(
            commandQueue,
            CL_QUEUE_CONTEXT,
            sizeof( context ),
            &context,
            NULL );
    }

    SBuiltinKernelOverrides*    pOverrides = NULL;

    // Get the overrides for this context.
    if( errorCode == CL_SUCCESS )
    {
        pOverrides = m_BuiltinKernelOverridesMap[ context ];
        if( pOverrides == NULL )
        {
            errorCode = CL_INVALID_VALUE;
        }
    }

    // See if this kernel is one of our overridden builtin kernels.
    if( errorCode == CL_SUCCESS )
    {
        if( kernel == pOverrides->Kernel_block_motion_estimate_intel )
        {
            if( ( work_dim == 2 ) &&
                ( global_work_size != NULL ) &&
                ( local_work_size == NULL ) )
            {
                const size_t    BLOCK_SIZE = 16;
                const size_t    w = ( global_work_size[0] + BLOCK_SIZE - 1 ) / BLOCK_SIZE;
                const size_t    h = ( global_work_size[1] + BLOCK_SIZE - 1 ) / BLOCK_SIZE;
#if 0
                const size_t    new_global_work_size[] = { w * BLOCK_SIZE, h };
                const size_t    new_local_work_size[] = { BLOCK_SIZE, 1 };

                int iterations = 1;
#else
                const size_t    new_global_work_size[] = { w * BLOCK_SIZE, 1 };
                const size_t    new_local_work_size[] = { BLOCK_SIZE, 1 };

                int iterations = (int)h;
#endif
                errorCode = dispatch().clSetKernelArg(
                    kernel,
                    6,
                    sizeof( iterations ),
                    &iterations );

                if( errorCode == CL_SUCCESS )
                {
                    errorCode = dispatch().clEnqueueNDRangeKernel(
                        commandQueue,
                        kernel,
                        2,
                        global_work_offset,
                        new_global_work_size,
                        new_local_work_size,
                        num_events_in_wait_list,
                        event_wait_list,
                        event );
                }
            }
        }
        else
        {
            errorCode = CL_INVALID_VALUE;
        }
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
#define CHECK_RETURN_ICD_LOADER_EXTENSION_FUNCTION(funcname)                \
{                                                                           \
    if( func_name == #funcname )                                            \
    {                                                                       \
        if( dispatch() . funcname == NULL )                                 \
        {                                                                   \
            void *func = NULL;                                              \
            if( platform &&                                                 \
                dispatch().clGetExtensionFunctionAddressForPlatform )       \
            {                                                               \
                func = dispatch().clGetExtensionFunctionAddressForPlatform( \
                    platform,                                               \
                    #funcname );                                            \
            } else if( dispatch().clGetExtensionFunctionAddress )           \
            {                                                               \
                func = dispatch().clGetExtensionFunctionAddress(#funcname); \
            }                                                               \
            void** pfunc = (void**)( &m_Dispatch . funcname );              \
            *pfunc = func;                                                  \
        }                                                                   \
        if( dispatch() . funcname )                                         \
        {                                                                   \
            return (void*)( funcname );                                     \
        }                                                                   \
    }                                                                       \
}

#define CHECK_RETURN_EXTENSION_FUNCTION(funcname)                           \
{                                                                           \
    if( func_name == #funcname )                                            \
    {                                                                       \
        if( dispatchX(platform) . funcname == NULL )                        \
        {                                                                   \
            void *func = NULL;                                              \
            if( platform &&                                                 \
                dispatch().clGetExtensionFunctionAddressForPlatform )       \
            {                                                               \
                func = dispatch().clGetExtensionFunctionAddressForPlatform( \
                    platform,                                               \
                    #funcname );                                            \
            } else if( dispatch().clGetExtensionFunctionAddress )           \
            {                                                               \
                func = dispatch().clGetExtensionFunctionAddress(#funcname); \
            }                                                               \
            void** pfunc = (void**)( &m_DispatchX[platform] . funcname );   \
            *pfunc = func;                                                  \
        }                                                                   \
        if( dispatchX(platform) . funcname )                                \
        {                                                                   \
            return (void*)( funcname );                                     \
        }                                                                   \
    }                                                                       \
}

#define CHECK_RETURN_EXTENSION_FUNCTION_EMU(funcname)                       \
{                                                                           \
    if( func_name == #funcname )                                            \
    {                                                                       \
        if( dispatchX(platform) . funcname == NULL )                        \
        {                                                                   \
            void *func = (void*)funcname##_EMU;                             \
            void** pfunc = (void**)( &m_DispatchX[platform] . funcname );   \
            *pfunc = func;                                                  \
        }                                                                   \
        if( dispatchX(platform) . funcname )                                \
        {                                                                   \
            return (void*)( funcname );                                     \
        }                                                                   \
    }                                                                       \
}


///////////////////////////////////////////////////////////////////////////////
//
void* CLIntercept::getExtensionFunctionAddress(
    cl_platform_id platform,
    const std::string& func_name )
{
    // KHR Extensions

    // cl_khr_gl_sharing

    // The cl_khr_gl_sharing APIs and especially clGetGLContextInfoKHR are a
    // special-case: they are extension functions but do not necessarily pass
    // a dispatchable object as their first argument and are implemented in
    // the ICD loader and called into via the ICD dispatch table.  This means
    // that we can install it into our core API dispatch table as well and
    // don't need to look it up per-platform.
    CHECK_RETURN_ICD_LOADER_EXTENSION_FUNCTION( clGetGLContextInfoKHR );

#if defined(_WIN32) || defined(__linux__) || defined(__FreeBSD__)
    CHECK_RETURN_ICD_LOADER_EXTENSION_FUNCTION( clCreateFromGLBuffer );
    CHECK_RETURN_ICD_LOADER_EXTENSION_FUNCTION( clCreateFromGLTexture );
    CHECK_RETURN_ICD_LOADER_EXTENSION_FUNCTION( clCreateFromGLTexture2D );
    CHECK_RETURN_ICD_LOADER_EXTENSION_FUNCTION( clCreateFromGLTexture3D );
    CHECK_RETURN_ICD_LOADER_EXTENSION_FUNCTION( clCreateFromGLRenderbuffer );
    CHECK_RETURN_ICD_LOADER_EXTENSION_FUNCTION( clGetGLObjectInfo );
    CHECK_RETURN_ICD_LOADER_EXTENSION_FUNCTION( clGetGLTextureInfo );
    CHECK_RETURN_ICD_LOADER_EXTENSION_FUNCTION( clEnqueueAcquireGLObjects );
    CHECK_RETURN_ICD_LOADER_EXTENSION_FUNCTION( clEnqueueReleaseGLObjects );
#endif

#if defined(_WIN32)
    // cl_khr_d3d10_sharing
    CHECK_RETURN_EXTENSION_FUNCTION( clGetDeviceIDsFromD3D10KHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromD3D10BufferKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromD3D10Texture2DKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromD3D10Texture3DKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueAcquireD3D10ObjectsKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueReleaseD3D10ObjectsKHR );

    // cl_khr_d3d11_sharing
    CHECK_RETURN_EXTENSION_FUNCTION( clGetDeviceIDsFromD3D11KHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromD3D11BufferKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromD3D11Texture2DKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromD3D11Texture3DKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueAcquireD3D11ObjectsKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueReleaseD3D11ObjectsKHR );

    // cl_khr_dx9_media_sharing
    CHECK_RETURN_EXTENSION_FUNCTION( clGetDeviceIDsFromDX9MediaAdapterKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromDX9MediaSurfaceKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueAcquireDX9MediaSurfacesKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueReleaseDX9MediaSurfacesKHR );
#endif

    // cl_khr_command_buffer
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateCommandBufferKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clFinalizeCommandBufferKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clRetainCommandBufferKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clReleaseCommandBufferKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueCommandBufferKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCommandBarrierWithWaitListKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCommandCopyBufferKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCommandCopyBufferRectKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCommandCopyBufferToImageKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCommandCopyImageKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCommandCopyImageToBufferKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCommandFillBufferKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCommandFillImageKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCommandSVMMemcpyKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCommandSVMMemFillKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clCommandNDRangeKernelKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clGetCommandBufferInfoKHR );

    // cl_khr_command_buffer_multi_device
    CHECK_RETURN_EXTENSION_FUNCTION( clRemapCommandBufferKHR );

    // cl_khr_command_buffer_mutable_dispatch
    CHECK_RETURN_EXTENSION_FUNCTION( clUpdateMutableCommandsKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clGetMutableCommandInfoKHR );

    // cl_khr_create_command_queue
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateCommandQueueWithPropertiesKHR );

    // cl_khr_external_memory
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueAcquireExternalMemObjectsKHR );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueReleaseExternalMemObjectsKHR );

    // cl_khr_external_semaphore
    CHECK_RETURN_EXTENSION_FUNCTION( clGetSemaphoreHandleForTypeKHR );

    // cl_khr_gl_event
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateEventFromGLsyncKHR );

    // cl_khr_il_program
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateProgramWithILKHR );

    // cl_khr_semaphore
    if( m_Config.Emulate_cl_khr_semaphore )
    {
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clCreateSemaphoreWithPropertiesKHR );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clEnqueueWaitSemaphoresKHR );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clEnqueueSignalSemaphoresKHR );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clGetSemaphoreInfoKHR );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clRetainSemaphoreKHR );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clReleaseSemaphoreKHR );
    }
    else
    {
        CHECK_RETURN_EXTENSION_FUNCTION( clCreateSemaphoreWithPropertiesKHR );
        CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueWaitSemaphoresKHR );
        CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueSignalSemaphoresKHR );
        CHECK_RETURN_EXTENSION_FUNCTION( clGetSemaphoreInfoKHR );
        CHECK_RETURN_EXTENSION_FUNCTION( clRetainSemaphoreKHR );
        CHECK_RETURN_EXTENSION_FUNCTION( clReleaseSemaphoreKHR );
    }

    // cl_khr_subgroups
    CHECK_RETURN_EXTENSION_FUNCTION( clGetKernelSubGroupInfoKHR );

    // cl_khr_suggested_local_work_size
    CHECK_RETURN_EXTENSION_FUNCTION( clGetKernelSuggestedLocalWorkSizeKHR );

    // cl_ext_image_requirements_info
    CHECK_RETURN_EXTENSION_FUNCTION( clGetImageRequirementsInfoEXT );

    // Unofficial MDAPI extension:
    CHECK_RETURN_EXTENSION_FUNCTION( clCreatePerfCountersCommandQueueINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clSetPerformanceConfigurationINTEL );

    // Unofficial suggested local work size extension:
    CHECK_RETURN_EXTENSION_FUNCTION( clGetKernelSuggestedLocalWorkSizeINTEL );

    // cl_intel_accelerator
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateAcceleratorINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clGetAcceleratorInfoINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clRetainAcceleratorINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clReleaseAcceleratorINTEL );

#if defined(_WIN32)
    // cl_intel_dx9_media_sharing
    CHECK_RETURN_EXTENSION_FUNCTION( clGetDeviceIDsFromDX9INTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromDX9MediaSurfaceINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueAcquireDX9ObjectsINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueReleaseDX9ObjectsINTEL );
#endif

    // cl_intel_sharing_format_query
    CHECK_RETURN_EXTENSION_FUNCTION( clGetSupportedGLTextureFormatsINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clGetSupportedDX9MediaSurfaceFormatsINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clGetSupportedD3D10TextureFormatsINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clGetSupportedD3D11TextureFormatsINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clGetSupportedVA_APIMediaSurfaceFormatsINTEL );

    // cl_intel_unified_shared_memory
    if( m_Config.Emulate_cl_intel_unified_shared_memory )
    {
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clHostMemAllocINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clDeviceMemAllocINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clSharedMemAllocINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clMemFreeINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clMemBlockingFreeINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clGetMemAllocInfoINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clSetKernelArgMemPointerINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clEnqueueMemsetINTEL );    // Deprecated
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clEnqueueMemFillINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clEnqueueMemcpyINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clEnqueueMigrateMemINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION_EMU( clEnqueueMemAdviseINTEL );
    }
    else
    {
        CHECK_RETURN_EXTENSION_FUNCTION( clHostMemAllocINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION( clDeviceMemAllocINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION( clSharedMemAllocINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION( clMemFreeINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION( clMemBlockingFreeINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION( clGetMemAllocInfoINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION( clSetKernelArgMemPointerINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueMemsetINTEL );    // Deprecated
        CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueMemFillINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueMemcpyINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueMigrateMemINTEL );
        CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueMemAdviseINTEL );
    }

    // cl_intel_va_api_media_sharing
    CHECK_RETURN_EXTENSION_FUNCTION( clGetDeviceIDsFromVA_APIMediaAdapterINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromVA_APIMediaSurfaceINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueAcquireVA_APIMediaSurfacesINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueReleaseVA_APIMediaSurfacesINTEL );

    // cl_nv_create_buffer
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateBufferNV );

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// This function assumes that CLIntercept already has entered its
// critical section.  If it hasn't, bad things could happen.
void CLIntercept::log( const std::string& s )
{
    if( m_Config.SuppressLogging == false )
    {
        std::string logString( m_Config.LogIndent, ' ' );
        logString += s;
        if( m_Config.LogToFile )
        {
            m_InterceptLog << logString;
            if( m_Config.FlushFiles )
            {
                m_InterceptLog.flush();
            }
        }
        if( m_Config.LogToDebugger )
        {
            OS().OutputDebugString( logString );
        }

        if( ( m_Config.LogToFile == false ) &&
            ( m_Config.LogToDebugger == false ) )
        {
            std::cerr << logString;
        }
    }
}
void CLIntercept::logf( const char* formatStr, ... )
{
    va_list args;
    va_start( args, formatStr );

    int size = CLI_VSPRINTF( m_StringBuffer, CLI_STRING_BUFFER_SIZE, formatStr, args );
    if( size >= 0 && size < CLI_STRING_BUFFER_SIZE )
    {
        log( std::string( m_StringBuffer ) );
    }
    else
    {
        log( std::string( "too long" ) );
    }

    va_end( args );
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::logPlatformInfo( cl_platform_id platform )
{
    cl_int  errorCode = CL_SUCCESS;

    char*   platformName = NULL;
    char*   platformVendor = NULL;
    char*   platformVersion = NULL;
    char*   platformProfile = NULL;
    char*   platformExtensions = NULL;

    errorCode |= allocateAndGetPlatformInfoString(
        platform,
        CL_PLATFORM_NAME,
        platformName );
    errorCode |= allocateAndGetPlatformInfoString(
        platform,
        CL_PLATFORM_VENDOR,
        platformVendor );
    errorCode |= allocateAndGetPlatformInfoString(
        platform,
        CL_PLATFORM_VERSION,
        platformVersion );
    errorCode |= allocateAndGetPlatformInfoString(
        platform,
        CL_PLATFORM_PROFILE,
        platformProfile );
    errorCode |= allocateAndGetPlatformInfoString(
        platform,
        CL_PLATFORM_EXTENSIONS,
        platformExtensions );

    if( errorCode == CL_SUCCESS )
    {
        logf( "\tName:           %s\n", platformName );
        logf( "\tVendor:         %s\n", platformVendor );
        logf( "\tDriver Version: %s\n", platformVersion );
        logf( "\tProfile:        %s\n", platformProfile );

        int     numberOfExtensions = 0;
        logf( "\tExtensions:\n" );
        if( platformExtensions )
        {
            char*   extension = NULL;
            char*   nextExtension = NULL;
            extension = CLI_STRTOK( platformExtensions, " ", &nextExtension );
            while( extension != NULL )
            {
                numberOfExtensions++;
                logf( "\t\t%s\n", extension );
                extension = CLI_STRTOK( NULL, " ", &nextExtension );
            }
        }
        logf( "\t\t%d Platform Extensions Found\n", numberOfExtensions );
    }
    else
    {
        log( "\tError getting platform info!\n" );
    }

    delete [] platformName;
    delete [] platformVendor;
    delete [] platformVersion;
    delete [] platformProfile;
    delete [] platformExtensions;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::logDeviceInfo( cl_device_id device )
{
    cl_int  errorCode = CL_SUCCESS;

    cl_device_type  deviceType;
    char*   deviceName = NULL;
    char*   deviceVendor = NULL;
    char*   deviceVersion = NULL;
    char*   driverVersion = NULL;
    char*   deviceExtensions = NULL;

    errorCode |= dispatch().clGetDeviceInfo(
        device,
        CL_DEVICE_TYPE,
        sizeof( deviceType ),
        &deviceType,
        NULL );
    errorCode |= allocateAndGetDeviceInfoString(
        device,
        CL_DEVICE_NAME,
        deviceName );
    errorCode |= allocateAndGetDeviceInfoString(
        device,
        CL_DEVICE_VENDOR,
        deviceVendor );
    errorCode |= allocateAndGetDeviceInfoString(
        device,
        CL_DEVICE_VERSION,
        deviceVersion );
    errorCode |= allocateAndGetDeviceInfoString(
        device,
        CL_DRIVER_VERSION,
        driverVersion );
    errorCode |= allocateAndGetDeviceInfoString(
        device,
        CL_DEVICE_EXTENSIONS,
        deviceExtensions );

    if( errorCode == CL_SUCCESS )
    {
        logf( "\tName:           %s\n", deviceName );
        logf( "\tVendor:         %s\n", deviceVendor );
        logf( "\tVersion:        %s\n", deviceVersion );
        logf( "\tDriver Version: %s\n", driverVersion );
        logf( "\tType:           %s\n", enumName().name_device_type( deviceType ).c_str() );

        int     numberOfExtensions = 0;
        logf( "\tExtensions:\n" );
        if( deviceExtensions )
        {
            char*   extension = NULL;
            char*   nextExtension = NULL;
            extension = CLI_STRTOK( deviceExtensions, " ", &nextExtension );
            while( extension != NULL )
            {
                numberOfExtensions++;
                logf( "\t\t%s\n", extension );
                extension = CLI_STRTOK( NULL, " ", &nextExtension );
            }
        }
        logf( "\t\t%d Device Extensions Found\n", numberOfExtensions );
    }
    else
    {
        log( "Error getting device info!\n" );
    }

    delete [] deviceName;
    delete [] deviceVendor;
    delete [] deviceVersion;
    delete [] driverVersion;
    delete [] deviceExtensions;
}

///////////////////////////////////////////////////////////////////////////////
//
#if defined(_WIN32) || defined(__linux__) || defined(__FreeBSD__)
#define INIT_EXPORTED_FUNC(funcname)                                        \
{                                                                           \
    void* func = OS().GetFunctionPointer(m_OpenCLLibraryHandle, #funcname); \
    if( func == NULL )                                                      \
    {                                                                       \
        log( std::string("Couldn't get exported function pointer to: ") + #funcname + "\n" );\
        success = false;                                                    \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        void** pfunc = (void**)( &m_Dispatch . funcname );                  \
        *pfunc = func;                                                      \
    }                                                                       \
}
bool CLIntercept::initDispatch( const std::string& libName )
{
    bool success = true;

    if( success )
    {
        m_OpenCLLibraryHandle = OS().LoadLibrary( libName.c_str() );
        if( m_OpenCLLibraryHandle == NULL )
        {
            log( std::string("Couldn't load library: ") + libName + "\n");
            success = false;
        }
    }

    if( success )
    {
        INIT_EXPORTED_FUNC(clGetPlatformIDs);
        INIT_EXPORTED_FUNC(clGetPlatformInfo);
        INIT_EXPORTED_FUNC(clGetDeviceIDs);
        INIT_EXPORTED_FUNC(clGetDeviceInfo);
        INIT_EXPORTED_FUNC(clCreateContext);
        INIT_EXPORTED_FUNC(clCreateContextFromType);
        INIT_EXPORTED_FUNC(clRetainContext);
        INIT_EXPORTED_FUNC(clReleaseContext);
        INIT_EXPORTED_FUNC(clGetContextInfo);
        INIT_EXPORTED_FUNC(clCreateCommandQueue);
        INIT_EXPORTED_FUNC(clRetainCommandQueue);
        INIT_EXPORTED_FUNC(clReleaseCommandQueue);
        INIT_EXPORTED_FUNC(clGetCommandQueueInfo);
        INIT_EXPORTED_FUNC(clSetCommandQueueProperty);
        INIT_EXPORTED_FUNC(clCreateBuffer);
        INIT_EXPORTED_FUNC(clCreateImage2D);
        INIT_EXPORTED_FUNC(clCreateImage3D);
        INIT_EXPORTED_FUNC(clRetainMemObject);
        INIT_EXPORTED_FUNC(clReleaseMemObject);
        INIT_EXPORTED_FUNC(clGetSupportedImageFormats);
        INIT_EXPORTED_FUNC(clGetMemObjectInfo);
        INIT_EXPORTED_FUNC(clGetImageInfo);
        INIT_EXPORTED_FUNC(clCreateSampler);
        INIT_EXPORTED_FUNC(clRetainSampler);
        INIT_EXPORTED_FUNC(clReleaseSampler);
        INIT_EXPORTED_FUNC(clGetSamplerInfo);
        INIT_EXPORTED_FUNC(clCreateProgramWithSource);
        INIT_EXPORTED_FUNC(clCreateProgramWithBinary);
        INIT_EXPORTED_FUNC(clRetainProgram);
        INIT_EXPORTED_FUNC(clReleaseProgram);
        INIT_EXPORTED_FUNC(clBuildProgram);
        INIT_EXPORTED_FUNC(clUnloadCompiler);
        INIT_EXPORTED_FUNC(clGetProgramInfo);
        INIT_EXPORTED_FUNC(clGetProgramBuildInfo);
        INIT_EXPORTED_FUNC(clCreateKernel);
        INIT_EXPORTED_FUNC(clCreateKernelsInProgram);
        INIT_EXPORTED_FUNC(clRetainKernel);
        INIT_EXPORTED_FUNC(clReleaseKernel);
        INIT_EXPORTED_FUNC(clSetKernelArg);
        INIT_EXPORTED_FUNC(clGetKernelInfo);
        INIT_EXPORTED_FUNC(clGetKernelWorkGroupInfo);
        INIT_EXPORTED_FUNC(clWaitForEvents);
        INIT_EXPORTED_FUNC(clGetEventInfo);
        INIT_EXPORTED_FUNC(clRetainEvent);
        INIT_EXPORTED_FUNC(clReleaseEvent);
        INIT_EXPORTED_FUNC(clGetEventProfilingInfo);
        INIT_EXPORTED_FUNC(clFlush);
        INIT_EXPORTED_FUNC(clFinish);
        INIT_EXPORTED_FUNC(clEnqueueReadBuffer);
        INIT_EXPORTED_FUNC(clEnqueueWriteBuffer);
        INIT_EXPORTED_FUNC(clEnqueueCopyBuffer);
        INIT_EXPORTED_FUNC(clEnqueueReadImage);
        INIT_EXPORTED_FUNC(clEnqueueWriteImage);
        INIT_EXPORTED_FUNC(clEnqueueCopyImage);
        INIT_EXPORTED_FUNC(clEnqueueCopyImageToBuffer);
        INIT_EXPORTED_FUNC(clEnqueueCopyBufferToImage);
        INIT_EXPORTED_FUNC(clEnqueueMapBuffer);
        INIT_EXPORTED_FUNC(clEnqueueMapImage);
        INIT_EXPORTED_FUNC(clEnqueueUnmapMemObject);
        INIT_EXPORTED_FUNC(clEnqueueNDRangeKernel);
        INIT_EXPORTED_FUNC(clEnqueueTask);
        INIT_EXPORTED_FUNC(clEnqueueNativeKernel);
        INIT_EXPORTED_FUNC(clEnqueueMarker);
        INIT_EXPORTED_FUNC(clEnqueueWaitForEvents);
        INIT_EXPORTED_FUNC(clEnqueueBarrier);

        bool    savedSuccess = success;

        INIT_EXPORTED_FUNC(clGetExtensionFunctionAddress);

        // cl_khr_gl_sharing (optional)
        // The entry points for this extension are exported from the ICD
        // loader even though they are extension APIs.
        INIT_EXPORTED_FUNC( clGetGLContextInfoKHR );
#if defined(_WIN32) || defined(__linux__) || defined(__FreeBSD__)
        INIT_EXPORTED_FUNC( clCreateFromGLBuffer );
        INIT_EXPORTED_FUNC( clCreateFromGLTexture );
        INIT_EXPORTED_FUNC( clCreateFromGLTexture2D );
        INIT_EXPORTED_FUNC( clCreateFromGLTexture3D );
        INIT_EXPORTED_FUNC( clCreateFromGLRenderbuffer );
        INIT_EXPORTED_FUNC( clGetGLObjectInfo );
        INIT_EXPORTED_FUNC( clGetGLTextureInfo );   // OpenCL 1.2
        INIT_EXPORTED_FUNC( clEnqueueAcquireGLObjects );
        INIT_EXPORTED_FUNC( clEnqueueReleaseGLObjects );
#endif

        // OpenCL 1.1 Entry Points (optional)
        INIT_EXPORTED_FUNC(clSetEventCallback);
        INIT_EXPORTED_FUNC(clCreateSubBuffer);
        INIT_EXPORTED_FUNC(clSetMemObjectDestructorCallback);
        INIT_EXPORTED_FUNC(clCreateUserEvent);
        INIT_EXPORTED_FUNC(clSetUserEventStatus);
        INIT_EXPORTED_FUNC(clEnqueueReadBufferRect);
        INIT_EXPORTED_FUNC(clEnqueueWriteBufferRect);
        INIT_EXPORTED_FUNC(clEnqueueCopyBufferRect);

        // OpenCL 1.2 Entry Points (optional)
        INIT_EXPORTED_FUNC(clCreateSubDevices);
        INIT_EXPORTED_FUNC(clRetainDevice);
        INIT_EXPORTED_FUNC(clReleaseDevice);
        INIT_EXPORTED_FUNC(clCreateImage);
        INIT_EXPORTED_FUNC(clCreateProgramWithBuiltInKernels);
        INIT_EXPORTED_FUNC(clCompileProgram);
        INIT_EXPORTED_FUNC(clLinkProgram);
        INIT_EXPORTED_FUNC(clUnloadPlatformCompiler);
        INIT_EXPORTED_FUNC(clGetKernelArgInfo);
        INIT_EXPORTED_FUNC(clEnqueueFillBuffer);
        INIT_EXPORTED_FUNC(clEnqueueFillImage);
        INIT_EXPORTED_FUNC(clEnqueueMigrateMemObjects);
        INIT_EXPORTED_FUNC(clEnqueueMarkerWithWaitList);
        INIT_EXPORTED_FUNC(clEnqueueBarrierWithWaitList);
        INIT_EXPORTED_FUNC(clGetExtensionFunctionAddressForPlatform);

        // OpenCL 2.0 Entry Points (optional)
        INIT_EXPORTED_FUNC(clCreateCommandQueueWithProperties);
        INIT_EXPORTED_FUNC(clCreatePipe);
        INIT_EXPORTED_FUNC(clGetPipeInfo);
        INIT_EXPORTED_FUNC(clSVMAlloc);
        INIT_EXPORTED_FUNC(clSVMFree);
        INIT_EXPORTED_FUNC(clEnqueueSVMFree);
        INIT_EXPORTED_FUNC(clEnqueueSVMMemcpy);
        INIT_EXPORTED_FUNC(clEnqueueSVMMemFill);
        INIT_EXPORTED_FUNC(clEnqueueSVMMap);
        INIT_EXPORTED_FUNC(clEnqueueSVMUnmap);
        INIT_EXPORTED_FUNC(clCreateSamplerWithProperties);
        INIT_EXPORTED_FUNC(clSetKernelArgSVMPointer);
        INIT_EXPORTED_FUNC(clSetKernelExecInfo);

        // OpenCL 2.1 Entry Points (optional)
        INIT_EXPORTED_FUNC(clCloneKernel);
        INIT_EXPORTED_FUNC(clCreateProgramWithIL);
        INIT_EXPORTED_FUNC(clEnqueueSVMMigrateMem);
        INIT_EXPORTED_FUNC(clGetDeviceAndHostTimer);
        INIT_EXPORTED_FUNC(clGetHostTimer);
        INIT_EXPORTED_FUNC(clGetKernelSubGroupInfo);
        INIT_EXPORTED_FUNC(clSetDefaultDeviceCommandQueue);

        // OpenCL 2.2 Entry Points (optional)
        INIT_EXPORTED_FUNC(clSetProgramReleaseCallback);
        INIT_EXPORTED_FUNC(clSetProgramSpecializationConstant);

        // OpenCL 3.0 Entry Points (optional)
        INIT_EXPORTED_FUNC(clCreateBufferWithProperties);
        INIT_EXPORTED_FUNC(clCreateImageWithProperties);
        INIT_EXPORTED_FUNC(clSetContextDestructorCallback);

        success = savedSuccess;
    }

    if( !success )
    {
        if( m_OpenCLLibraryHandle != NULL )
        {
            OS().UnloadLibrary( m_OpenCLLibraryHandle );
            m_OpenCLLibraryHandle = NULL;
        }
    }

    return success;
}
///////////////////////////////////////////////////////////////////////////////
//
#elif defined(__APPLE__)
#define INIT_CL_FUNC(funcname)                                              \
{                                                                           \
    m_Dispatch . funcname = funcname;                                       \
}
bool CLIntercept::initDispatch( void )
{
    INIT_CL_FUNC(clGetPlatformIDs);
    INIT_CL_FUNC(clGetPlatformInfo);
    INIT_CL_FUNC(clGetDeviceIDs);
    INIT_CL_FUNC(clGetDeviceInfo);
    INIT_CL_FUNC(clCreateContext);
    INIT_CL_FUNC(clCreateContextFromType);
    INIT_CL_FUNC(clRetainContext);
    INIT_CL_FUNC(clReleaseContext);
    INIT_CL_FUNC(clGetContextInfo);
    INIT_CL_FUNC(clCreateCommandQueue);
    INIT_CL_FUNC(clRetainCommandQueue);
    INIT_CL_FUNC(clReleaseCommandQueue);
    INIT_CL_FUNC(clGetCommandQueueInfo);
    INIT_CL_FUNC(clSetCommandQueueProperty);
    INIT_CL_FUNC(clCreateBuffer);
    INIT_CL_FUNC(clCreateImage2D);
    INIT_CL_FUNC(clCreateImage3D);
    INIT_CL_FUNC(clRetainMemObject);
    INIT_CL_FUNC(clReleaseMemObject);
    INIT_CL_FUNC(clGetSupportedImageFormats);
    INIT_CL_FUNC(clGetMemObjectInfo);
    INIT_CL_FUNC(clGetImageInfo);
    INIT_CL_FUNC(clCreateSampler);
    INIT_CL_FUNC(clRetainSampler);
    INIT_CL_FUNC(clReleaseSampler);
    INIT_CL_FUNC(clGetSamplerInfo);
    INIT_CL_FUNC(clCreateProgramWithSource);
    INIT_CL_FUNC(clCreateProgramWithBinary);
    INIT_CL_FUNC(clRetainProgram);
    INIT_CL_FUNC(clReleaseProgram);
    INIT_CL_FUNC(clBuildProgram);
    INIT_CL_FUNC(clUnloadCompiler);
    INIT_CL_FUNC(clGetProgramInfo);
    INIT_CL_FUNC(clGetProgramBuildInfo);
    INIT_CL_FUNC(clCreateKernel);
    INIT_CL_FUNC(clCreateKernelsInProgram);
    INIT_CL_FUNC(clRetainKernel);
    INIT_CL_FUNC(clReleaseKernel);
    INIT_CL_FUNC(clSetKernelArg);
    INIT_CL_FUNC(clGetKernelInfo);
    INIT_CL_FUNC(clGetKernelWorkGroupInfo);
    INIT_CL_FUNC(clWaitForEvents);
    INIT_CL_FUNC(clGetEventInfo);
    INIT_CL_FUNC(clRetainEvent);
    INIT_CL_FUNC(clReleaseEvent);
    INIT_CL_FUNC(clGetEventProfilingInfo);
    INIT_CL_FUNC(clFlush);
    INIT_CL_FUNC(clFinish);
    INIT_CL_FUNC(clEnqueueReadBuffer);
    INIT_CL_FUNC(clEnqueueWriteBuffer);
    INIT_CL_FUNC(clEnqueueCopyBuffer);
    INIT_CL_FUNC(clEnqueueReadImage);
    INIT_CL_FUNC(clEnqueueWriteImage);
    INIT_CL_FUNC(clEnqueueCopyImage);
    INIT_CL_FUNC(clEnqueueCopyImageToBuffer);
    INIT_CL_FUNC(clEnqueueCopyBufferToImage);
    INIT_CL_FUNC(clEnqueueMapBuffer);
    INIT_CL_FUNC(clEnqueueMapImage);
    INIT_CL_FUNC(clEnqueueUnmapMemObject);
    INIT_CL_FUNC(clEnqueueNDRangeKernel);
    INIT_CL_FUNC(clEnqueueTask);
    INIT_CL_FUNC(clEnqueueNativeKernel);
    INIT_CL_FUNC(clEnqueueMarker);
    INIT_CL_FUNC(clEnqueueWaitForEvents);
    INIT_CL_FUNC(clEnqueueBarrier);
    INIT_CL_FUNC(clGetExtensionFunctionAddress);

    // OpenCL 1.1 Entry Points
    INIT_CL_FUNC(clSetEventCallback);
    INIT_CL_FUNC(clCreateSubBuffer);
    INIT_CL_FUNC(clSetMemObjectDestructorCallback);
    INIT_CL_FUNC(clCreateUserEvent);
    INIT_CL_FUNC(clSetUserEventStatus);
    INIT_CL_FUNC(clEnqueueReadBufferRect);
    INIT_CL_FUNC(clEnqueueWriteBufferRect);
    INIT_CL_FUNC(clEnqueueCopyBufferRect);

    // OpenCL 1.2 Entry Points
    INIT_CL_FUNC(clCreateSubDevices);
    INIT_CL_FUNC(clRetainDevice);
    INIT_CL_FUNC(clReleaseDevice);
    INIT_CL_FUNC(clCreateImage);
    INIT_CL_FUNC(clCreateProgramWithBuiltInKernels);
    INIT_CL_FUNC(clCompileProgram);
    INIT_CL_FUNC(clLinkProgram);
    INIT_CL_FUNC(clUnloadPlatformCompiler);
    INIT_CL_FUNC(clGetKernelArgInfo);
    INIT_CL_FUNC(clEnqueueFillBuffer);
    INIT_CL_FUNC(clEnqueueFillImage);
    INIT_CL_FUNC(clEnqueueMigrateMemObjects);
    INIT_CL_FUNC(clEnqueueMarkerWithWaitList);
    INIT_CL_FUNC(clEnqueueBarrierWithWaitList);
    INIT_CL_FUNC(clGetExtensionFunctionAddressForPlatform);

    return true;
}
#else
#error Unknown OS!
#endif

///////////////////////////////////////////////////////////////////////////////
//
#if defined(USE_ITT)
void CLIntercept::ittInit()
{
    if( m_ITTInitialized == false )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if( m_ITTInitialized == false )
        {
            log( "Initializing ITT...\n" );

            m_ITTInitialized = true;

            m_ITTDomain = __itt_domain_create( "com.intel.clintercept" );
            if( m_ITTDomain == NULL )
            {
                log( "__itt_domain_create() returned NULL!\n" );
            }

            //m_ITTQueuedState    = __ittx_task_state_create( m_ITTDomain, "QUEUED" );
            //m_ITTSubmittedState = __ittx_task_state_create( m_ITTDomain, "SUBMITTED" );
            //m_ITTExecutingState = __ittx_task_state_create( m_ITTDomain, "EXECUTING" );

            //m_ITTQueueTrackGroup = __itt_track_group_create(
            //    __itt_string_handle_create("Queue tracks"),
            //    __itt_track_group_type_normal );
            //if( m_ITTQueueTrackGroup == NULL )
            //{
            //    log( "__itt_track_group_create() returned NULL!\n" );
            //}

            log( "... done!\n" );
        }
    }
}

void CLIntercept::ittCallLoggingEnter(
    const char* functionName,
    const cl_kernel kernel )
{
    std::string str( functionName );
    if( kernel )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        const std::string& kernelName = getShortKernelNameWithHash(kernel);
        str += "( ";
        str += kernelName;
        str += " )";
    }

    __itt_string_handle* itt_string_handle = __itt_string_handle_create( str.c_str() );
    __itt_task_begin(m_ITTDomain, __itt_null, __itt_null, itt_string_handle);
}

void CLIntercept::ittCallLoggingExit()
{
    __itt_task_end(m_ITTDomain);
}

void CLIntercept::ittRegisterCommandQueue(
    cl_command_queue queue,
    bool supportsPerfCounters )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_int  errorCode = CL_SUCCESS;

    cl_device_id                device = NULL;
    cl_device_type              deviceType = 0;
    cl_command_queue_properties properties = 0;

    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetCommandQueueInfo(
            queue,
            CL_QUEUE_DEVICE,
            sizeof(device),
            &device,
            NULL);
    }
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetDeviceInfo(
            device,
            CL_DEVICE_TYPE,
            sizeof(deviceType),
            &deviceType,
            NULL );
    }
    if( errorCode == CL_SUCCESS )
    {
        errorCode = dispatch().clGetCommandQueueInfo(
            queue,
            CL_QUEUE_PROPERTIES,
            sizeof(properties),
            &properties,
            NULL );
    }

    if( errorCode == CL_SUCCESS )
    {
        SITTQueueInfo& queueInfo = m_ITTQueueInfoMap[ queue ];

        queueInfo.pIntercept = this;
        queueInfo.SupportsPerfCounters = supportsPerfCounters;

        queueInfo.itt_track = NULL;
        queueInfo.itt_clock_domain = NULL;
        queueInfo.CLReferenceTime = 0;

#if 0
        std::string trackName = "OpenCL";

        if( properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE )
        {
            trackName += " Out-Of-Order";
        }
        else
        {
            trackName += " In-Order";
        }

        if( deviceType & CL_DEVICE_TYPE_CPU )
        {
            trackName += " CPU";
        }
        if( deviceType & CL_DEVICE_TYPE_GPU )
        {
            trackName += " GPU";
        }
        if( deviceType & CL_DEVICE_TYPE_ACCELERATOR )
        {
            trackName += " ACC";
        }
        if( deviceType & CL_DEVICE_TYPE_CUSTOM )
        {
            trackName += " CUSTOM";
        }

        trackName += " Queue, ";

        {
            CLI_SPRINTF( m_StringBuffer, CLI_STRING_BUFFER_SIZE, "Handle = %p", queue );
            trackName = trackName + m_StringBuffer;
        }

        // Don't fail if the track cannot be created, it just means we
        // won't be as detailed in our tracking.
        __itt_track* track = __itt_track_create(
            m_ITTQueueTrackGroup,
            __itt_string_handle_create(trackName.c_str()),
            __itt_track_type_queue );
        if( track != NULL )
        {
            queueInfo.itt_track = track;

            __itt_set_track(track);

            __ittx_set_default_state(
                m_ITTDomain,
                m_ITTQueuedState );

            __itt_set_track(NULL);
        }
#endif

        dispatch().clRetainCommandQueue( queue );
    }
}

void CLIntercept::ittReleaseCommandQueue(
    cl_command_queue queue )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if( m_ITTQueueInfoMap.find(queue) != m_ITTQueueInfoMap.end() )
    {
        cl_uint refCount = getRefCount( queue );
        if( refCount == 1 )
        {
            dispatch().clReleaseCommandQueue( queue );
            m_ITTQueueInfoMap.erase( queue );
        }
    }
}

void ITTAPI CLIntercept::ittClockInfoCallback(
    __itt_clock_info* pClockInfo,
    void* pData )
{
    const SITTQueueInfo* pQueueInfo = (const SITTQueueInfo*)pData;

    using ns = std::chrono::nanoseconds;
    uint64_t    nsDelta =
        std::chrono::duration_cast<ns>(
            clock::now() - pQueueInfo->CPUReferenceTime).count();

    pClockInfo->clock_base = pQueueInfo->CLReferenceTime + nsDelta;
    pClockInfo->clock_freq = 1000000000;    // NS
}

void CLIntercept::ittTraceEvent(
    const std::string& name,
    cl_event event,
    clock::time_point queuedTime,
    cl_ulong commandQueued,
    cl_ulong commandSubmit,
    cl_ulong commandStart,
    cl_ulong commandEnd )
{
    cl_int  errorCode = CL_SUCCESS;

    cl_command_queue    queue = NULL;
    cl_command_type     type = 0;

    errorCode |= dispatch().clGetEventInfo(
        event,
        CL_EVENT_COMMAND_QUEUE,
        sizeof( queue ),
        &queue,
        NULL );

    errorCode |= dispatch().clGetEventInfo(
        event,
        CL_EVENT_COMMAND_TYPE,
        sizeof(type),
        &type,
        NULL );

    if( errorCode == CL_SUCCESS )
    {
        // It's possible we don't have any ITT info for this queue.
        if( m_ITTQueueInfoMap.find(queue) != m_ITTQueueInfoMap.end() )
        {
            SITTQueueInfo&  queueInfo = m_ITTQueueInfoMap[ queue ];

            __itt_clock_domain* clockDomain = queueInfo.itt_clock_domain;
            if( clockDomain == NULL )
            {
                queueInfo.CPUReferenceTime = queuedTime;
                queueInfo.CLReferenceTime = commandQueued;

                clockDomain = __itt_clock_domain_create(
                    ittClockInfoCallback,
                    &queueInfo );
                if( clockDomain == NULL )
                {
                    log( "__itt_clock_domain_create() returned NULL!\n");
                }

                queueInfo.itt_clock_domain = clockDomain;
            }

            __itt_track*    track = queueInfo.itt_track;
            uint64_t        clockOffset = 0;

            if( commandQueued == 0 )
            {
                using ns = std::chrono::nanoseconds;
                clockOffset = std::chrono::duration_cast<ns>(
                    queuedTime - queueInfo.CPUReferenceTime).count();
            }

            commandQueued += clockOffset;
            commandSubmit += clockOffset;
            commandStart += clockOffset;
            commandEnd += clockOffset;

            __itt_set_track( track );

            __itt_string_handle*    nameHandle = __itt_string_handle_create( name.c_str() );
            __itt_id    eventId = __itt_id_make( NULL, (uint64_t)event );

            __itt_id_create_ex( m_ITTDomain, clockDomain, commandQueued, eventId );

            if( config().ITTShowOnlyExecutingEvents )
            {
                __itt_task_begin_overlapped_ex( m_ITTDomain, clockDomain, commandStart, eventId, __itt_null, nameHandle );
                //__ittx_task_set_state( m_ITTDomain, clockDomain, commandStart, eventId, m_ITTExecutingState );
                __itt_task_end_overlapped_ex( m_ITTDomain, clockDomain, commandEnd, eventId );
            }
            else
            {
                __itt_task_begin_overlapped_ex( m_ITTDomain, clockDomain, commandQueued, eventId, __itt_null, nameHandle );
                //__ittx_task_set_state( m_ITTDomain, clockDomain, commandSubmit, eventId, m_ITTSubmittedState);
                //__ittx_task_set_state( m_ITTDomain, clockDomain, commandStart, eventId, m_ITTExecutingState );
                __itt_task_end_overlapped_ex( m_ITTDomain, clockDomain, commandEnd, eventId );
            }

            if( queueInfo.SupportsPerfCounters )
            {
                // TODO: This needs to be updated to use MDAPI.
                CLI_ASSERT( 0 );
            }

            __itt_id_destroy_ex( m_ITTDomain, clockDomain, commandEnd, eventId );

            __itt_set_track(NULL);
        }
        else
        {
            log( "ittTraceEvent(): no queue info\n" );
        }
    }
    else
    {
        log( "ittTraceEvent(): OpenCL error\n" );
    }
}

#endif

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::chromeCallLoggingExit(
    const char* functionName,
    const std::string& tag,
    bool includeId,
    const uint64_t enqueueCounter,
    clock::time_point tickStart,
    clock::time_point tickEnd )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    uint64_t    threadId = OS().GetThreadID();

    // This will name the thread if it is not named already.
    getThreadNumber( threadId );

    using ns = std::chrono::nanoseconds;
    uint64_t    nsStart =
        std::chrono::duration_cast<ns>(tickStart - m_StartTime).count();
    uint64_t    nsDelta =
        std::chrono::duration_cast<ns>(tickEnd - tickStart).count();

    if( !tag.empty() && includeId )
    {
        m_ChromeTrace.addCallLogging( functionName, tag, threadId, nsStart, nsDelta, enqueueCounter );
    }
    else if( !tag.empty() )
    {
        m_ChromeTrace.addCallLogging( functionName, tag, threadId, nsStart, nsDelta );
    }
    else if( includeId )
    {
        m_ChromeTrace.addCallLogging( functionName, threadId, nsStart, nsDelta, enqueueCounter );
    }
    else
    {
        m_ChromeTrace.addCallLogging( functionName, threadId, nsStart, nsDelta );
    }

    if( m_Config.FlushFiles )
    {
        m_ChromeTrace.flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::chromeRegisterCommandQueue(
    cl_command_queue queue )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_int  errorCode = CL_SUCCESS;

    cl_device_id                device = NULL;
    cl_device_type              deviceType = 0;
    cl_command_queue_properties properties = 0;

    errorCode |= dispatch().clGetCommandQueueInfo(
        queue,
        CL_QUEUE_DEVICE,
        sizeof(device),
        &device,
        NULL);
    errorCode |= dispatch().clGetDeviceInfo(
        device,
        CL_DEVICE_TYPE,
        sizeof(deviceType),
        &deviceType,
        NULL );
    errorCode |= dispatch().clGetCommandQueueInfo(
        queue,
        CL_QUEUE_PROPERTIES,
        sizeof(properties),
        &properties,
        NULL );

    if( errorCode == CL_SUCCESS )
    {
        unsigned int    queueNumber = m_QueueNumberMap[ queue ];

        std::string trackName;

        if( properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE )
        {
            trackName += "OOQ";
        }
        else
        {
            trackName += "IOQ";
        }

        cacheDeviceInfo( device );

        const SDeviceInfo& deviceInfo = m_DeviceInfoMap[device];

        {
            std::string deviceIndexString;
            getDeviceIndexString(
                device,
                deviceIndexString );
            CLI_SPRINTF( m_StringBuffer, CLI_STRING_BUFFER_SIZE, " %p.%s %s (%s)",
                queue,
                deviceIndexString.c_str(),
                deviceInfo.Name.c_str(),
                enumName().name_device_type( deviceInfo.Type ).c_str() );
            trackName = trackName + m_StringBuffer;
        }

        {
            cl_int  testError = CL_SUCCESS;

            cl_uint queueFamily = 0;
            cl_uint queueIndex = 0;

            testError |= dispatch().clGetCommandQueueInfo(
                queue,
                CL_QUEUE_FAMILY_INTEL,
                sizeof(queueFamily),
                &queueFamily,
                NULL );
            testError |= dispatch().clGetCommandQueueInfo(
                queue,
                CL_QUEUE_INDEX_INTEL,
                sizeof(queueIndex),
                &queueIndex,
                NULL );
            if( testError == CL_SUCCESS )
            {
                CLI_SPRINTF( m_StringBuffer, CLI_STRING_BUFFER_SIZE, " (F:%u I:%u)",
                    queueFamily,
                    queueIndex );
                trackName = trackName + m_StringBuffer;
            }
        }

        m_ChromeTrace.addQueueMetadata( queueNumber, trackName );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::chromeTraceEvent(
    const std::string& name,
    bool useProfilingDelta,
    int64_t profilingDeltaNS,
    uint64_t enqueueCounter,
    unsigned int queueNumber,
    clock::time_point queuedTime,
    cl_ulong commandQueued,
    cl_ulong commandSubmit,
    cl_ulong commandStart,
    cl_ulong commandEnd )
{
    using ns = std::chrono::nanoseconds;
    const uint64_t  startTimeNS =
        std::chrono::duration_cast<ns>(m_StartTime.time_since_epoch()).count();
    const uint64_t  estimatedQueuedTimeNS =
        std::chrono::duration_cast<ns>(queuedTime.time_since_epoch()).count();
    const uint64_t  profilingQueuedTimeNS =
        commandQueued + profilingDeltaNS;

    // Use the profiling queued time directly if the profiling delta is
    // valid and if it is within a threshold of the measured queued time.
    // The threshold is to work around buggy device and host timers.
    const uint64_t  threshold = 1000000000;   // 1s
    const uint64_t  normalizedQueuedTimeNS =
        useProfilingDelta &&
        profilingQueuedTimeNS >= estimatedQueuedTimeNS &&
        profilingQueuedTimeNS - estimatedQueuedTimeNS < threshold ?
        profilingQueuedTimeNS - startTimeNS :
        estimatedQueuedTimeNS - startTimeNS;

    //if( useProfilingDelta )
    //{
    //    int64_t deltaNS =
    //        profilingQueuedTimeNS - estimatedQueuedTimeNS;
    //    logf( "For command %s:\n"
    //        "\tcommandQueued is %llu ns (%.2f us)\n"
    //        "\testimatedQueuedTimeNS is %llu ns (%.2f us)\n"
    //        "\tprofilingQueuedTimeNS is %llu ns (%.2f us)\n"
    //        "\testimated time is %s than profiling time\n"
    //        "\tdeltaNS is %llu ns (%.2f us)\n",
    //        name.c_str(),
    //        commandQueued, commandQueued / 1000.0,
    //        estimatedQueuedTimeNS, estimatedQueuedTimeNS / 1000.0,
    //        profilingQueuedTimeNS, profilingQueuedTimeNS / 1000.0,
    //        estimatedQueuedTimeNS > profilingQueuedTimeNS ? "GREATER" : "LESS",
    //        deltaNS, deltaNS / 1000.0 );
    //}

    const uint64_t  nsQueued = normalizedQueuedTimeNS;
    const uint64_t  nsSubmit =
        commandSubmit - commandQueued + normalizedQueuedTimeNS;
    const uint64_t  nsStart =
        commandStart - commandQueued + normalizedQueuedTimeNS;
    const uint64_t  nsEnd =
        commandEnd - commandQueued + normalizedQueuedTimeNS;

    if( m_Config.ChromePerformanceTimingInStages )
    {
        if( m_Config.ChromePerformanceTimingPerKernel )
        {
            m_ChromeTrace.addDeviceTiming(
                name,
                nsQueued,
                nsSubmit,
                nsStart,
                nsEnd,
                enqueueCounter );
        }
        else
        {
            m_ChromeTrace.addDeviceTiming(
                name,
                m_EventsChromeTraced,
                queueNumber,
                nsQueued,
                nsSubmit,
                nsStart,
                nsEnd,
                enqueueCounter );
        }
        m_EventsChromeTraced++;
    }
    else
    {
        if( m_Config.ChromePerformanceTimingPerKernel )
        {
            m_ChromeTrace.addDeviceTiming( name, nsStart, nsEnd, enqueueCounter );
        }
        else
        {
            m_ChromeTrace.addDeviceTiming( name, queueNumber, nsStart, nsEnd, enqueueCounter );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::checkAubCaptureKernelSignature(
    const cl_kernel kernel,
    cl_uint workDim,
    const size_t* gws,
    const size_t* lws )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    bool    match = true;

    // If the aubcapture kernel name is set, make sure it matches the name
    // of the passed-in kernel:

    if( match &&
        m_Config.AubCaptureKernelName != "" &&
        // Note: This currently checks the long kernel name.
        // Should it be the short kernel name instead?
        m_KernelInfoMap[ kernel ].KernelName != m_Config.AubCaptureKernelName )
    {
        //logf( "Skipping aub capture: kernel name '%s' doesn't match the requested kernel name '%s'.\n",
        //    m_KernelInfoMap[ kernel ].KernelName.c_str(),
        //    m_Config.AubCaptureKernelName.c_str() );
        match = false;
    }

    // If the aubcapture global work size is set, and it is not set to the
    // wildcard ("*"), make sure it matches the passed-in global work size:

    if( match &&
        m_Config.AubCaptureKernelGWS != "" &&
        m_Config.AubCaptureKernelGWS != "*" )
    {
        std::ostringstream  ss;
        if( gws )
        {
            if( workDim >= 1 )
            {
                ss << gws[0];
            }
            if( workDim >= 2 )
            {
                ss << "x" << gws[1];
            }
            if( workDim >= 3 )
            {
                ss << "x" << gws[2];
            }
        }
        else
        {
            ss << "NULL";
        }
        if( m_Config.AubCaptureKernelGWS != ss.str() )
        {
            //logf( "Skipping aub capture: global work size %s doesn't match the requested global work size %s.\n",
            //    ss.str(),
            //    m_Config.AubCaptureKernelGWS.c_str() );
            match = false;
        }
    }

    // If the aubcapture local work size is set, and it is not set to the
    // wildcard ("*"), make sure it matches the passed-in local work size:

    if( match &&
        m_Config.AubCaptureKernelLWS != "" &&
        m_Config.AubCaptureKernelLWS != "*" )
    {
        std::ostringstream  ss;
        if( lws )
        {
            if( workDim >= 1 )
            {
                ss << lws[0];
            }
            if( workDim >= 2 )
            {
                ss << "x" << lws[1];
            }
            if( workDim >= 3 )
            {
                ss << "x" << lws[2];
            }
        }
        else
        {
            ss << "NULL";
        }
        if( m_Config.AubCaptureKernelLWS != ss.str() )
        {
            //logf( "Skipping aub capture: local work size %s doesn't match the requested local work size %s.\n",
            //    ss.str(),
            //    m_Config.AubCaptureKernelLWS.c_str() );
            match = false;
        }
    }

    if( match &&
        m_Config.AubCaptureUniqueKernels )
    {
        const SKernelInfo& kernelInfo = m_KernelInfoMap[ kernel ];

        // Note: This currently uses the long kernel name.
        // Should it be the short kernel name instead?
        std::string key = kernelInfo.KernelName;

        {
            char    hashString[256] = "";
            if( config().OmitProgramNumber )
            {
                CLI_SPRINTF( hashString, 256, "(%08X_%04u_%08X)",
                    (unsigned int)kernelInfo.ProgramHash,
                    kernelInfo.CompileCount,
                    (unsigned int)kernelInfo.OptionsHash );
            }
            else
            {
                CLI_SPRINTF( hashString, 256, "(%04u_%08X_%04u_%08X)",
                    kernelInfo.ProgramNumber,
                    (unsigned int)kernelInfo.ProgramHash,
                    kernelInfo.CompileCount,
                    (unsigned int)kernelInfo.OptionsHash );
            }
            key += hashString;
        }

        if( gws )
        {
            std::ostringstream  ss;
            ss << " GWS[ ";
            if( gws )
            {
                if( workDim >= 1 )
                {
                    ss << gws[0];
                }
                if( workDim >= 2 )
                {
                    ss << "x" << gws[1];
                }
                if( workDim >= 3 )
                {
                    ss << "x" << gws[2];
                }
            }
            else
            {
                ss << "NULL";
            }
            ss << " ]";
            key += ss.str();
        }

        {
            std::ostringstream  ss;
            ss << " LWS[ ";
            if( lws )
            {
                if( workDim >= 1 )
                {
                    ss << lws[0];
                }
                if( workDim >= 2 )
                {
                    ss << "x" << lws[1];
                }
                if( workDim >= 3 )
                {
                    ss << "x" << lws[2];
                }
            }
            else
            {
                ss << "NULL";
            }
            ss << " ]";
            key += ss.str();
        }

        if( m_AubCaptureSet.find( key ) == m_AubCaptureSet.end() )
        {
            m_AubCaptureSet.insert( key );
        }
        else
        {
            //logf( "Skipping aub capture: key %s was already captured.\n",
            //    key.c_str() );
            match = false;
        }
    }

    return match;
}

#define USE_DRIVER_SVM

///////////////////////////////////////////////////////////////////////////////
//
static bool validateUSMMemProperties(
    const cl_mem_properties_intel* properties )
{
    if( properties )
    {
        while( properties[0] != 0 )
        {
            cl_int  property = (cl_int)properties[0];
            switch( property )
            {
            case CL_MEM_ALLOC_FLAGS_INTEL:
                {
                    const cl_mem_alloc_flags_intel* pf =
                        (const cl_mem_alloc_flags_intel*)( properties + 1 );
                    cl_mem_alloc_flags_intel    flags = pf[0];
                    cl_mem_alloc_flags_intel    valid =
                        CL_MEM_ALLOC_WRITE_COMBINED_INTEL;
                    if( flags & ~valid )
                    {
                        return false;
                    }
                }
                break;
            default:
                return false;
            }

            properties += 2;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
void* CLIntercept::emulatedHostMemAlloc(
    cl_context context,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if( !validateUSMMemProperties(properties) )
    {
        if( errcode_ret )
        {
            errcode_ret[0] = CL_INVALID_PROPERTY;
        }
        return NULL;
    }

#ifdef USE_DRIVER_SVM
    void*   ptr = dispatch().clSVMAlloc ?
        dispatch().clSVMAlloc(
            context,
            CL_MEM_READ_WRITE | CL_MEM_SVM_FINE_GRAIN_BUFFER,
            size,
            alignment ) : NULL;
#else
    // For now, the only valid alignments are "0":
    if( alignment != 0 )
    {
        if( errcode_ret )
        {
            errcode_ret[0] = CL_INVALID_VALUE;
        }
        return NULL;
    }

    void*   ptr = new char[size];
#endif
    if( ptr == NULL )
    {
        if( errcode_ret )
        {
            errcode_ret[0] = CL_OUT_OF_RESOURCES;// TODO: Which error?
        }
        return NULL;
    }

    // Record this allocation in the alloc map:
    SUSMContextInfo&    usmContextInfo = m_USMContextInfoMap[context];
    SUSMAllocInfo&      allocInfo = usmContextInfo.AllocMap[ptr];
    allocInfo.Type = CL_MEM_TYPE_HOST_INTEL;
    allocInfo.BaseAddress = ptr;
    allocInfo.Size = size;
    allocInfo.Alignment = alignment;

    usmContextInfo.HostAllocVector.push_back( ptr );

    if( errcode_ret )
    {
        errcode_ret[0] = CL_SUCCESS;
    }
    return ptr;
}

///////////////////////////////////////////////////////////////////////////////
//
void* CLIntercept::emulatedDeviceMemAlloc(
    cl_context context,
    cl_device_id device,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if( !validateUSMMemProperties(properties) )
    {
        if( errcode_ret )
        {
            errcode_ret[0] = CL_INVALID_PROPERTY;
        }
        return NULL;
    }

    // Unconditionally use coarse grain SVM for device allocations:

    void*   ptr = dispatch().clSVMAlloc ?
        dispatch().clSVMAlloc(
            context,
            CL_MEM_READ_WRITE,
            size,
            alignment ) : NULL;
    if( ptr == NULL )
    {
        if( errcode_ret )
        {
            errcode_ret[0] = CL_OUT_OF_RESOURCES;// TODO: Which error?
        }
        return NULL;
    }

    // Record this allocation in the alloc map:
    SUSMContextInfo&    usmContextInfo = m_USMContextInfoMap[context];
    SUSMAllocInfo&      allocInfo = usmContextInfo.AllocMap[ptr];
    allocInfo.Type = CL_MEM_TYPE_DEVICE_INTEL;
    allocInfo.Device = device;
    allocInfo.BaseAddress = ptr;
    allocInfo.Size = size;
    allocInfo.Alignment = alignment;

    usmContextInfo.DeviceAllocVector.push_back( ptr );

    if( errcode_ret )
    {
        errcode_ret[0] = CL_SUCCESS;
    }
    return ptr;
}

///////////////////////////////////////////////////////////////////////////////
//
void* CLIntercept::emulatedSharedMemAlloc(
    cl_context context,
    cl_device_id device,
    const cl_mem_properties_intel* properties,
    size_t size,
    cl_uint alignment,
    cl_int* errcode_ret)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    if( !validateUSMMemProperties(properties) )
    {
        if( errcode_ret )
        {
            errcode_ret[0] = CL_INVALID_PROPERTY;
        }
        return NULL;
    }

#ifdef USE_DRIVER_SVM
    void*   ptr = dispatch().clSVMAlloc ?
        dispatch().clSVMAlloc(
            context,
            CL_MEM_READ_WRITE | CL_MEM_SVM_FINE_GRAIN_BUFFER,
            size,
            alignment ) : NULL;
#else
    // For now, the only valid alignments are "0":
    if( alignment != 0 )
    {
        if( errcode_ret )
        {
            errcode_ret[0] = CL_INVALID_VALUE;
        }
        return NULL;
    }

    void*   ptr = new char[size];
#endif
    if( ptr == NULL )
    {
        if( errcode_ret )
        {
            errcode_ret[0] = CL_OUT_OF_RESOURCES;// TODO: Which error?
        }
        return NULL;
    }

    // Record this allocation in the alloc map:
    SUSMContextInfo&    usmContextInfo = m_USMContextInfoMap[context];
    SUSMAllocInfo&      allocInfo = usmContextInfo.AllocMap[ptr];
    allocInfo.Type = CL_MEM_TYPE_SHARED_INTEL;
    allocInfo.Device = device;
    allocInfo.BaseAddress = ptr;
    allocInfo.Size = size;
    allocInfo.Alignment = alignment;

    usmContextInfo.SharedAllocVector.push_back( ptr );

    if( errcode_ret )
    {
        errcode_ret[0] = CL_SUCCESS;
    }
    return ptr;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::emulatedMemFree(
    cl_context context,
    const void* ptr )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    SUSMContextInfo&    usmContextInfo = m_USMContextInfoMap[context];

    CUSMAllocMap::iterator iter = usmContextInfo.AllocMap.find( ptr );
    if( iter != usmContextInfo.AllocMap.end() )
    {
        const SUSMAllocInfo &allocInfo = iter->second;

        switch( allocInfo.Type )
        {
        case CL_MEM_TYPE_HOST_INTEL:
            usmContextInfo.HostAllocVector.erase(
                std::find(
                    usmContextInfo.HostAllocVector.begin(),
                    usmContextInfo.HostAllocVector.end(),
                    ptr ) );
            break;
        case CL_MEM_TYPE_DEVICE_INTEL:
            usmContextInfo.DeviceAllocVector.erase(
                std::find(
                    usmContextInfo.DeviceAllocVector.begin(),
                    usmContextInfo.DeviceAllocVector.end(),
                    ptr ) );
            break;
        case CL_MEM_TYPE_SHARED_INTEL:
            usmContextInfo.SharedAllocVector.erase(
                std::find(
                    usmContextInfo.SharedAllocVector.begin(),
                    usmContextInfo.SharedAllocVector.end(),
                    ptr ) );
        default:
            CLI_ASSERT( 0 );
            break;
        }

        usmContextInfo.AllocMap.erase( ptr );

#ifdef USE_DRIVER_SVM
        dispatch().clSVMFree(
            context,
            (void*)ptr );
        ptr = NULL;
#else
        delete [] ptr;
        ptr = NULL;
#endif

        return CL_SUCCESS;
    }

    return CL_INVALID_MEM_OBJECT;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::emulatedGetMemAllocInfoINTEL(
    cl_context context,
    const void* ptr,
    cl_mem_info_intel param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret)
{
    if( ptr == NULL )
    {
        return CL_INVALID_VALUE;
    }

    SUSMContextInfo&    usmContextInfo = m_USMContextInfoMap[context];

    if( usmContextInfo.AllocMap.size() == 0 )
    {
        // No pointers allocated?
        return CL_INVALID_MEM_OBJECT;   // TODO: new error code?
    }

    CUSMAllocMap::iterator iter = usmContextInfo.AllocMap.lower_bound( ptr );

    if( iter->first != ptr )
    {
        if( iter == usmContextInfo.AllocMap.begin() )
        {
            // This pointer is not in the map.
            return CL_INVALID_MEM_OBJECT;
        }

        // Go to the previous iterator.
        --iter;
    }

    const SUSMAllocInfo &allocInfo = iter->second;

    const void* startPtr = allocInfo.BaseAddress;
    const void* endPtr = (const char*)allocInfo.BaseAddress + allocInfo.Size;
    //logf("start = %p, ptr = %p, end = %p\n",
    //    startPtr,
    //    ptr,
    //    endPtr );

    if( ptr < startPtr || ptr >= endPtr )
    {
        return CL_INVALID_MEM_OBJECT;
    }

    //logf("For ptr = %p: base = %p, size = %p\n",
    //    ptr,
    //    allocInfo.BaseAddress,
    //    allocInfo.Size );

    switch( param_name )
    {
    case CL_MEM_ALLOC_TYPE_INTEL:
        {
            cl_unified_shared_memory_type_intel* ptr =
                (cl_unified_shared_memory_type_intel*)param_value;
            return writeParamToMemory(
                param_value_size,
                allocInfo.Type,
                param_value_size_ret,
                ptr);
        }
    case CL_MEM_ALLOC_BASE_PTR_INTEL:
        {
            const void** ptr =
                (const void**)param_value;
            return writeParamToMemory(
                param_value_size,
                allocInfo.BaseAddress,
                param_value_size_ret,
                ptr);
        }
    case CL_MEM_ALLOC_SIZE_INTEL:
        {
            size_t* ptr =
                (size_t*)param_value;
            return writeParamToMemory(
                param_value_size,
                allocInfo.Size,
                param_value_size_ret,
                ptr);
        }
    case CL_MEM_ALLOC_DEVICE_INTEL:
        {
            cl_device_id* ptr =
                (cl_device_id*)param_value;
            return writeParamToMemory(
                param_value_size,
                allocInfo.Device,
                param_value_size_ret,
                ptr);
        }
    default:
        break;
    }

    return CL_INVALID_VALUE;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::trackUSMKernelExecInfo(
    cl_kernel kernel,
    cl_kernel_exec_info param_name,
    size_t param_value_size,
    const void* param_value)
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_int  retVal = CL_INVALID_VALUE;

    switch( param_name )
    {
    case CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL:
        if( param_value_size == sizeof(cl_bool) )
        {
            SUSMKernelInfo& kernelInfo = m_USMKernelInfoMap[kernel];
            cl_bool*    pBool = (cl_bool*)param_value;

            kernelInfo.IndirectHostAccess = ( pBool[0] == CL_TRUE );
            retVal = CL_SUCCESS;
        }
        break;
    case CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL:
        if( param_value_size == sizeof(cl_bool) )
        {
            SUSMKernelInfo& kernelInfo = m_USMKernelInfoMap[kernel];
            cl_bool*    pBool = (cl_bool*)param_value;

            kernelInfo.IndirectDeviceAccess = ( pBool[0] == CL_TRUE );
            retVal = CL_SUCCESS;
        }
        break;
    case CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL:
        if( param_value_size == sizeof(cl_bool) )
        {
            SUSMKernelInfo& kernelInfo = m_USMKernelInfoMap[kernel];
            cl_bool*    pBool = (cl_bool*)param_value;

            kernelInfo.IndirectSharedAccess = ( pBool[0] == CL_TRUE );
            retVal = CL_SUCCESS;
        }
        break;
    case CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL:
        {
            SUSMKernelInfo& kernelInfo = m_USMKernelInfoMap[kernel];
            void**  pPtrs = (void**)param_value;
            size_t  numPtrs = param_value_size / sizeof(void*);

            kernelInfo.USMPtrs.clear();
            kernelInfo.USMPtrs.reserve(numPtrs);
            kernelInfo.USMPtrs.insert(
                kernelInfo.USMPtrs.begin(),
                pPtrs,
                pPtrs + numPtrs );
        }
        break;
    case CL_KERNEL_EXEC_INFO_SVM_PTRS:
        {
            SUSMKernelInfo& kernelInfo = m_USMKernelInfoMap[kernel];
            void**  pPtrs = (void**)param_value;
            size_t  numPtrs = param_value_size / sizeof(void*);

            kernelInfo.SVMPtrs.clear();
            kernelInfo.SVMPtrs.reserve(numPtrs);
            kernelInfo.SVMPtrs.insert(
                kernelInfo.SVMPtrs.begin(),
                pPtrs,
                pPtrs + numPtrs );

            // Don't set CL_SUCCESS so the call passes through.
        }
        break;
    default: break;
    }

    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::setUSMKernelExecInfo(
    cl_command_queue commandQueue,
    cl_kernel kernel )
{
    const SUSMKernelInfo& usmKernelInfo = m_USMKernelInfoMap[ kernel ];

    cl_int  errorCode = CL_SUCCESS;

    if( usmKernelInfo.IndirectHostAccess ||
        usmKernelInfo.IndirectDeviceAccess ||
        usmKernelInfo.IndirectSharedAccess )
    {
        cl_context  context = NULL;
        dispatch().clGetCommandQueueInfo(
            commandQueue,
            CL_QUEUE_CONTEXT,
            sizeof( context ),
            &context,
            NULL );

        const SUSMContextInfo& usmContextInfo = m_USMContextInfoMap[context];

        std::lock_guard<std::mutex> lock(m_Mutex);

        bool    hasSVMPtrs =
                    !usmKernelInfo.SVMPtrs.empty();
        bool    hasUSMPtrs =
                    !usmKernelInfo.USMPtrs.empty();
        bool    setHostAllocs =
                    !usmContextInfo.HostAllocVector.empty() &&
                    usmKernelInfo.IndirectHostAccess;
        bool    setDeviceAllocs =
                    !usmContextInfo.DeviceAllocVector.empty() &&
                    usmKernelInfo.IndirectDeviceAccess;
        bool    setSharedAllocs =
                    !usmContextInfo.SharedAllocVector.empty() &&
                    usmKernelInfo.IndirectSharedAccess;

        bool    fastPath =
                    ( hasSVMPtrs == false ) &&
                    ( hasUSMPtrs == false ) &&
                    ( ( !setHostAllocs && !setDeviceAllocs && !setSharedAllocs ) ||
                      (  setHostAllocs && !setDeviceAllocs && !setSharedAllocs ) ||
                      ( !setHostAllocs &&  setDeviceAllocs && !setSharedAllocs ) ||
                      ( !setHostAllocs && !setDeviceAllocs &&  setSharedAllocs ) );

        if( fastPath )
        {
            if( setHostAllocs )
            {
                size_t  count = usmContextInfo.HostAllocVector.size();

                logf("Indirect USM Allocs for kernel %s: Fast path for %zu host allocs\n",
                    getShortKernelName(kernel).c_str(),
                    count );

                errorCode = dispatch().clSetKernelExecInfo(
                    kernel,
                    CL_KERNEL_EXEC_INFO_SVM_PTRS,
                    count * sizeof(void*),
                    usmContextInfo.HostAllocVector.data() );
            }
            if( setDeviceAllocs )
            {
                size_t  count = usmContextInfo.DeviceAllocVector.size();

                logf("Indirect USM Allocs for kernel %s: Fast path for %zu device allocs\n",
                    getShortKernelName(kernel).c_str(),
                    count );

                errorCode = dispatch().clSetKernelExecInfo(
                    kernel,
                    CL_KERNEL_EXEC_INFO_SVM_PTRS,
                    count * sizeof(void*),
                    usmContextInfo.DeviceAllocVector.data() );
            }
            if( setSharedAllocs )
            {
                size_t  count = usmContextInfo.SharedAllocVector.size();

                logf("Indirect USM Allocs for kernel %s: Fast path for %zu shared allocs\n",
                    getShortKernelName(kernel).c_str(),
                    count );

                errorCode = dispatch().clSetKernelExecInfo(
                    kernel,
                    CL_KERNEL_EXEC_INFO_SVM_PTRS,
                    count * sizeof(void*),
                    usmContextInfo.SharedAllocVector.data() );
            }
        }
        else
        {
            logf("Indirect USM allocs for kernel %s: %zu svm ptrs, %zu usm ptrs, %zu host allocs, %zu device allocs, %zu shared allocs\n",
                getShortKernelName(kernel).c_str(),
                usmKernelInfo.SVMPtrs.size(),
                usmKernelInfo.USMPtrs.size(),
                setHostAllocs ? usmContextInfo.HostAllocVector.size() : 0,
                setDeviceAllocs ? usmContextInfo.DeviceAllocVector.size() : 0,
                setSharedAllocs ? usmContextInfo.SharedAllocVector.size() : 0 );

            size_t  count =
                usmKernelInfo.SVMPtrs.size() +
                usmKernelInfo.USMPtrs.size() +
                ( setHostAllocs ? usmContextInfo.HostAllocVector.size() : 0 ) +
                ( setDeviceAllocs ? usmContextInfo.DeviceAllocVector.size() : 0 ) +
                ( setSharedAllocs ? usmContextInfo.SharedAllocVector.size() : 0 );

            std::vector<const void*>  combined( count );

            combined.insert(
                combined.end(),
                usmKernelInfo.SVMPtrs.begin(),
                usmKernelInfo.SVMPtrs.end() );
            combined.insert(
                combined.end(),
                usmKernelInfo.USMPtrs.begin(),
                usmKernelInfo.USMPtrs.end() );
            if( setHostAllocs )
            {
                combined.insert(
                    combined.end(),
                    usmContextInfo.HostAllocVector.begin(),
                    usmContextInfo.HostAllocVector.end() );
            }
            if( setDeviceAllocs )
            {
                combined.insert(
                    combined.end(),
                    usmContextInfo.DeviceAllocVector.begin(),
                    usmContextInfo.DeviceAllocVector.end() );
            }
            if( setSharedAllocs )
            {
                combined.insert(
                    combined.end(),
                    usmContextInfo.SharedAllocVector.begin(),
                    usmContextInfo.SharedAllocVector.end() );
            }

            errorCode = dispatch().clSetKernelExecInfo(
                kernel,
                CL_KERNEL_EXEC_INFO_SVM_PTRS,
                count * sizeof(void*),
                combined.data() );
        }

        if( errorCode != CL_SUCCESS )
        {
            logf("clSetKernelExecInfo to set indirect USM allocations returned %s (%d)!\n",
                enumName().name(errorCode).c_str(),
                errorCode );
        }
    }

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_int CLIntercept::finishAll(
    cl_context context )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    const CQueueList& queues = m_ContextQueuesMap[context];

    cl_int  errorCode = CL_SUCCESS;

    for( auto queue : queues )
    {
        cl_int  tempErrorCode = dispatch().clFinish( queue );
        if( tempErrorCode != CL_SUCCESS )
        {
            logf("clFinish on queue %p returned %s (%d)!\n",
                queue,
                enumName().name(errorCode).c_str(),
                errorCode );
            errorCode = tempErrorCode;
        }
    }

    return errorCode;
}
