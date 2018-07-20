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

#include <algorithm>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdarg.h>
#include <sstream>
#include <time.h>       // strdate

#include "common.h"
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
    const unsigned int *data,
    size_t count )
{
    unsigned int    a = 0x428a2f98, hi = 0x71374491, lo = 0xb5c0fbcf;
    while( count-- )
    {
        a ^= *(data++);
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
    m_Dispatch = dummyDispatch;

    m_OpenCLLibraryHandle = NULL;

    m_LoggedCLInfo = false;

    m_EnqueueCounter = 1;
    m_StartTime = 0;

    m_ProgramNumber = 0;

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

    m_OS.EnterCriticalSection();

    log( "CLIntercept is shutting down...\n" );

    // Set the dispatch to the dummy dispatch.  The destructor is called
    // as the process is terminating.  We don't know when each DLL gets
    // unloaded, so it's not safe to call into any OpenCL functions in
    // our destructor.  Setting to the dummy dispatch ensures that no
    // OpenCL functions get called.  Note that this means we do potentially
    // leave some events, kernels, or programs un-released, but since
    // the process is terminating, that's probably OK.
    m_Dispatch = dummyDispatch;

    if( m_OpenCLLibraryHandle != NULL )
    {
        OS().UnloadLibrary( m_OpenCLLibraryHandle );
    }

    {
        CCpuTimingStatsMap::iterator i = m_CpuTimingStatsMap.begin();
        while( i != m_CpuTimingStatsMap.end() )
        {
            SCpuTimingStats* pCpuTimingStats = (*i).second;

            if( pCpuTimingStats )
            {
                delete pCpuTimingStats;
            }

            (*i).second = NULL;
            ++i;
        }
    }

    {
        CDeviceTimingStatsMap::iterator i = m_DeviceTimingStatsMap.begin();
        while( i != m_DeviceTimingStatsMap.end() )
        {
            SDeviceTimingStats* pDeviceTimingStats = (*i).second;

            if( pDeviceTimingStats )
            {
                delete pDeviceTimingStats;
            }

            (*i).second = NULL;
            ++i;
        }
    }

    {
        CEventList::iterator i = m_EventList.begin();
        while( i != m_EventList.end() )
        {
            SEventListNode* pEventListNode = (*i);

            if( pEventListNode )
            {
                // If we were able to release events, we'd release
                // pEventListNode->Event here.

                delete pEventListNode;
            }

            (*i) = NULL;
            ++i;
        }
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

    log( "... shutdown complete.\n" );

    m_InterceptLog.close();
    m_InterceptTrace.close();

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
template <class T>
static bool ReadRegistry(
    const OS::Services& OS,
    const char* name,
    T& value )
{
    unsigned int    readValue = 0;
    bool success = OS.ReadRegistry( name, &readValue, sizeof(readValue) );
    if( success )
    {
        value = readValue;
    }

    return success;
}
template <>
bool ReadRegistry<bool>(
    const OS::Services& OS,
    const char* name,
    bool& value )
{
    unsigned int    readValue = 0;
    bool success = OS.ReadRegistry( name, &readValue, sizeof(readValue) );
    if( success )
    {
        value = ( readValue != 0 );
    }

    return success;
}
template <>
bool ReadRegistry<std::string>(
    const OS::Services& OS,
    const char* name,
    std::string& value )
{
    char    readValue[256] = "";
    bool success = OS.ReadRegistry( name, readValue, sizeof(readValue) );
    if( success )
    {
        value = readValue;
    }

    return success;
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::init()
{
    if( m_OS.Init() == false )
    {
#ifdef __ANDROID__
         __android_log_print(ANDROID_LOG_INFO, "clIntercept", "OS.Init FAILED!\n" );
#endif
        return false;
    }

    m_OS.EnterCriticalSection();

#if defined(_WIN32)
    OS::Services_Common::ENV_PREFIX = "CLI_";
    OS::Services_Common::REGISTRY_KEY = "SOFTWARE\\INTEL\\IGFX\\CLINTERCEPT";
#elif defined(__linux__) || defined(__APPLE__)
    OS::Services_Common::ENV_PREFIX = "CLI_";
    OS::Services_Common::CONFIG_FILE = "clintercept.conf";
#endif

    m_kernelId = 0;
    m_maxKernelLength = 32;
    bool    breakOnLoad = false;
    ReadRegistry( m_OS, "BreakOnLoad", breakOnLoad );

    if( breakOnLoad )
    {
        CLI_DEBUG_BREAK();
    }

    std::string dllName = "";
    ReadRegistry( m_OS, "DllName", dllName );

    ReadRegistry( m_OS, "SimpleDumpProgram",         m_Config.SimpleDumpProgramSource );     // backwards compatible, replaced by SimpleDumpProgramSource
    ReadRegistry( m_OS, "DumpProgramsScript",        m_Config.DumpProgramSourceScript );     // backwards compatible, replaced by DumpProgramSourceScript
    ReadRegistry( m_OS, "DumpProgramsInject",        m_Config.DumpProgramSource );           // backwards compatible, replaced by DumpProgramSource
    ReadRegistry( m_OS, "InjectPrograms",            m_Config.InjectProgramSource );         // backwards compatible, replaced by InjectProgramSource

#define CLI_CONTROL( _type, _name, _init, _desc ) ReadRegistry( m_OS, #_name, m_Config . _name );
#include "controls.h"
#undef CLI_CONTROL

    if( m_Config.LogToFile )
    {
        std::string fileName = "";

#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
        if( !m_Config.LogDir.empty() )
        {
            std::replace( m_Config.LogDir.begin(), m_Config.LogDir.end(), '\\', '/' );
            OS::Services_Common::LOG_DIR = m_Config.LogDir.c_str();
        }
#endif
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
        fileName += "/";
        fileName += sc_LogFileName;

        OS().MakeDumpDirectories( fileName );

        if( m_Config.AppendFiles )
        {
            m_InterceptLog.open( fileName.c_str(), std::ios::out | std::ios::app );
        }
        else
        {
            m_InterceptLog.open( fileName.c_str(), std::ios::out );
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
        m_InterceptTrace.open( fileName.c_str(), std::ios::out );
        m_InterceptTrace << "[\n";

        uint64_t    processId = OS().GetProcessID();
        uint64_t    threadId = OS().GetThreadID();
        std::string processName = OS().GetProcessName();
        m_InterceptTrace
            << "{\"ph\":\"M\", \"name\":\"process_name\", \"pid\":" << processId
            << ", \"tid\":" << threadId
            << ", \"args\":{\"name\":\"" << processName
            << "\"}},\n";
        //m_InterceptTrace
        //    << "{\"ph\":\"M\", \"name\":\"thread_name\", \"pid\":" << processId
        //    << ", \"tid\":" << threadId
        //    << ", \"args\":{\"name\":\"Host APIs\"}},\n";
    }

    std::string name = "";
    OS().GetCLInterceptName( name );

    std::string bits =
        ( sizeof(void*) == 8 ) ? "64-bit" :
        ( sizeof(void*) == 4 ) ? "32-bit" :
        "XX-bit";

    log( "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n" );
    log( "CLIntercept (" + bits + ") is loading...\n" );
    log( "CLintercept file location: " + name + "\n" );
    log( "CLIntercept URL: " + std::string(sc_URL) + "\n" );
#if defined(CLINTERCEPT_CMAKE)
    log( "CLIntercept git description: " + std::string(sc_GitDescribe) + "\n" );
    log( "CLIntercept git refspec: " + std::string(sc_GitRefSpec) + "\n" );
    log( "CLInterecpt git hash: " + std::string(sc_GitHash) + "\n" );
#endif
    log( "CLIntercept optional features:\n"
#if defined(CLINTERCEPT_CLIPROF) || !defined(_WIN32)    // extra code only needed for Windows
        "    cliprof(supported)\n"
#else
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
    );
#if defined(_WIN32)
    log( "CLIntercept environment variable prefix: " + std::string( OS::Services_Common::ENV_PREFIX ) + "\n"  );
    log( "CLIntercept registry key: " + std::string( OS::Services_Common::REGISTRY_KEY ) + "\n" );
#elif defined(__linux__) || defined(__APPLE__)
    log( "CLIntercept environment variable prefix: " + std::string( OS::Services_Common::ENV_PREFIX ) + "\n"  );
    log( "CLIntercept config file: " + std::string( OS::Services_Common::CONFIG_FILE ) + "\n" );
#endif

    // Windows and Linux load the real OpenCL library and retrieve
    // the OpenCL entry points from the real library dynamically.
#if defined(_WIN32) || defined(__linux__)
    if( dllName != "" )
    {
        log( "Read DLL name from user parameters: " + dllName + "\n" );
        log( "Trying to load dispatch from: " + dllName + "\n" );

        if( initDispatch( dllName ) )
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
        const std::string dllNames[] =
        {
            "real_opencl.dll",
        #if defined(WIN32)
            std::string(windir) + "/syswow64/opencl.dll",
        #endif
            std::string(windir) + "/system32/opencl.dll",
        };

        free( windir );

#elif defined(__ANDROID__)

        const std::string dllNames[] =
        {
            "/system/vendor/lib/real_libOpenCL.so",
            "real_libOpenCL.so",
        };

#elif defined(__linux__)

        const std::string dllNames[] =
        {
            "./real_libOpenCL.so",
            "/usr/lib/x86_64-linux-gnu/libOpenCL.so",
            "/opt/intel/opencl/lib64/libOpenCL.so",
        };

#else
#error Unknown OS!
#endif

        const int numNames = sizeof(dllNames) / sizeof(dllNames[0]);
        int i = 0;

        for( i = 0; i < numNames; i++ )
        {
            log( "Trying to load dispatch from: " + dllNames[i] + "\n" );

            if( initDispatch( dllNames[i] ) )
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

#define CLI_CONTROL( _type, _name, _init, _desc ) if( m_Config . _name != _init ) { log( #_name " is set to a non-default value!\n" ); }
#include "controls.h"
#undef CLI_CONTROL

    m_StartTime = m_OS.GetTimer();
    log( "Timer Started!\n" );

    log( "... loading complete.\n" );

    m_OS.LeaveCriticalSection();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::report()
{
    m_OS.EnterCriticalSection();

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
            os.open( filepath, std::ios::out | std::ios::binary | std::ios::app );
        }
        else
        {
            os.open( filepath, std::ios::out | std::ios::binary );
        }
        if( os.good() )
        {
            writeReport( os );
            os.close();
        }
    }

    m_OS.LeaveCriticalSection();
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

    uint64_t    numEnqueues = m_EnqueueCounter - 1;
    if( numEnqueues > 0 )
    {
        os << "Total Enqueues: " << numEnqueues << std::endl << std::endl;
    }

    if( config().LeakChecking )
    {
        os << std::endl << "Leak Checking:" << std::endl;
        m_ObjectTracker.writeReport( os );
    }

    if( config().HostPerformanceTiming &&
        !m_CpuTimingStatsMap.empty() )
    {
        os << std::endl << "Host Performance Timing Results:" << std::endl;

        os << std::endl
            << std::right << std::setw(44) << "Function Name" << ", "
            << std::right << std::setw( 6) << "Calls" << ", "
            << std::right << std::setw(13) << "Average (ns)" << ", "
            << std::right << std::setw(13) << "Min (ns)" << ", "
            << std::right << std::setw(13) << "Max (ns)" << std::endl;

        uint64_t overallTotalTicks = 0;
        CCpuTimingStatsMap::const_iterator i = m_CpuTimingStatsMap.begin();
        while( i != m_CpuTimingStatsMap.end() )
        {
            SCpuTimingStats* pCpuTimingStats = (*i).second;
            const std::string& name = (*i).first;

            if( !name.empty() && pCpuTimingStats )
            {
                os << std::right << std::setw(44) << name << ", "
                    << std::right << std::setw( 6) << pCpuTimingStats->NumberOfCalls << ", "
                    << std::right << std::setw(13) << OS().TickToNS( pCpuTimingStats->TotalTicks ) / pCpuTimingStats->NumberOfCalls << ", "
                    << std::right << std::setw(13) << OS().TickToNS( pCpuTimingStats->MinTicks ) << ", "
                    << std::right << std::setw(13) << OS().TickToNS( pCpuTimingStats->MaxTicks ) << std::endl;

                overallTotalTicks += pCpuTimingStats->TotalTicks;
            }

            ++i;
        }

        os << std::endl
            << std::right << std::setw(44) << "Function Name" << ", "
            << std::right << std::setw( 6) << "Calls" << ", "
            << std::right << std::setw(13) << "Ticks" << ", "
            << std::right << std::setw(13) << "Min Ticks" << ", "
            << std::right << std::setw(13) << "Max Ticks" << ", "
            << std::right << std::setw(13) << "% Ticks" << std::endl;

        i = m_CpuTimingStatsMap.begin();
        while( i != m_CpuTimingStatsMap.end() )
        {
            SCpuTimingStats* pCpuTimingStats = (*i).second;
            const std::string& name = (*i).first;

            if( !name.empty() && pCpuTimingStats )
            {
                os << std::right << std::setw(44) << name << ", "
                    << std::right << std::setw( 6) << pCpuTimingStats->NumberOfCalls << ", "
                    << std::right << std::setw(13) << pCpuTimingStats->TotalTicks << ", "
                    << std::right << std::setw(13) << pCpuTimingStats->MinTicks << ", "
                    << std::right << std::setw(13) << pCpuTimingStats->MaxTicks << ", "
                    << std::right << std::setw(13)
                        << std::fixed << std::setprecision(2)
                        << ( pCpuTimingStats->TotalTicks * 100.0 ) / ( overallTotalTicks ) << std::endl;
            }

            ++i;
        }
    }

    if( config().DevicePerformanceTiming &&
        !m_DeviceTimingStatsMap.empty() )
    {
        os << std::endl << "Device Performance Timing Results:" << std::endl;

        cl_ulong    totalTotalNS = 0;
        size_t      longestName = 32;

        CDeviceTimingStatsMap::const_iterator i = m_DeviceTimingStatsMap.begin();
        while( i != m_DeviceTimingStatsMap.end() )
        {
            const std::string& name = (*i).first;
            SDeviceTimingStats* pDeviceTimingStats = (*i).second;

            if( !name.empty() && pDeviceTimingStats )
            {
                totalTotalNS += pDeviceTimingStats->TotalNS;
                longestName = std::max< size_t >( name.length(), longestName );
            }

            ++i;
        }

        os << std::endl << "Total Time (ns): " << totalTotalNS << std::endl;

        i = m_DeviceTimingStatsMap.begin();
        if( m_Config.IndexLongKernelNames )
        {
            bool isHeaderEmpty = true;
            while( i != m_DeviceTimingStatsMap.end() )
            {
                const std::string& name = (*i).first;
                SDeviceTimingStats* pDeviceTimingStats = (*i).second;
                if( name.length() > m_maxKernelLength ) {
                  if( isHeaderEmpty )
                  {
                    os << "Function Name Mapping:" << std::endl
                        << std::right << std::setw( 11) << "Function Id" << ", "
                        << std::right << std::setw( 1) << "Function Long Name" << std::endl;
                    isHeaderEmpty = false;
                  }
                  os << std::right << std::setw( 11) << pDeviceTimingStats->KernelId <<  ", "
                     << std::right << std::setw( 1) << name << std::endl;
                }
                ++i;
            }
            longestName = m_maxKernelLength;
        }
        os << std::endl
            << std::right << std::setw(longestName) << "Function Name" << ", "
            << std::right << std::setw( 6) << "Calls" << ", "
            << std::right << std::setw(13) << "Time (ns)" << ", "
            << std::right << std::setw( 8) << "Time (%)" << ", "
            << std::right << std::setw(13) << "Average (ns)" << ", "
            << std::right << std::setw(13) << "Min (ns)" << ", "
            << std::right << std::setw(13) << "Max (ns)" << std::endl;

        i = m_DeviceTimingStatsMap.begin();
        while( i != m_DeviceTimingStatsMap.end() )
        {
            const std::string& name = (*i).first;
            SDeviceTimingStats* pDeviceTimingStats = (*i).second;

            if( !name.empty() && pDeviceTimingStats )
            {
                os << std::right << std::setw(longestName) << ( ( m_Config.IndexLongKernelNames && name.length() > m_maxKernelLength ) ? pDeviceTimingStats->KernelId : name ) << ", "
                   << std::right << std::setw( 6) << pDeviceTimingStats->NumberOfCalls << ", "
                   << std::right << std::setw(13) << pDeviceTimingStats->TotalNS << ", "
                   << std::right << std::setw( 7) << std::fixed << std::setprecision(2) << pDeviceTimingStats->TotalNS * 100.0f / totalTotalNS << "%, "
                   << std::right << std::setw(13) << pDeviceTimingStats->TotalNS / pDeviceTimingStats->NumberOfCalls << ", "
                   << std::right << std::setw(13) << pDeviceTimingStats->MinNS << ", "
                   << std::right << std::setw(13) << pDeviceTimingStats->MaxNS << std::endl;
            }

            ++i;
        }
    }

#if defined(USE_MDAPI)
    if( !config().DevicePerfCounterCustom.empty() )
    {
        reportMDAPICounters( os );
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
std::string CLIntercept::getKernelName(
    const cl_kernel kernel )
{
    m_OS.EnterCriticalSection();

    std::string kernelName = "";
    if( m_Config.IndexLongKernelNames )
    {
        if( kernelName.length() > m_maxKernelLength )
        {
            kernelName += m_KernelNameMap[ kernel ].kernelId;
        }
        else
        {
            kernelName += m_KernelNameMap[ kernel ].kernelName;
        }
    }
    else
    {
        kernelName += m_KernelNameMap[ kernel ].kernelName;
    }

    m_OS.LeaveCriticalSection();

    return kernelName;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getCallLoggingPrefix(
    std::string& str )
{
    if( m_Config.CallLoggingElapsedTime )
    {
        uint64_t    tickDelta =
            OS().GetTimer() -
            m_StartTime;
        uint64_t    usDelta =
            OS().TickToNS( tickDelta ) / 1000;
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
            unsigned int    threadNum = 0;
            if( m_ThreadNumberMap.find( threadId ) != m_ThreadNumberMap.end() )
            {
                threadNum = m_ThreadNumberMap[ threadId ];
            }
            else
            {
                threadNum = (unsigned int)m_ThreadNumberMap.size();
                m_ThreadNumberMap[ threadId ] = threadNum;
            }
            ss << "TNum = ";
            ss << threadNum;
            ss << " ";
        }

        str += ss.str();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::callLoggingEnter(
    const std::string& functionName,
    const cl_kernel kernel )
{
    m_OS.EnterCriticalSection();

    std::string str;
    getCallLoggingPrefix( str );

    str += functionName;

    if( kernel )
    {
        const std::string& kernelName = getKernelName(kernel);
        str += "( ";
        str += kernelName;
        str += " )";
    }

    if( m_Config.CallLoggingEnqueueCounter )
    {
        std::ostringstream  ss;
        ss << ", EnqueueCounter: ";
        ss << m_EnqueueCounter;
        str += ss.str();
    }

    log( ">>>> " + str + "\n" );

    m_OS.LeaveCriticalSection();
}
void CLIntercept::callLoggingEnter(
    const std::string& functionName,
    const cl_kernel kernel,
    const char* formatStr,
    ... )
{
    va_list args;
    va_start( args, formatStr );

    std::string str = functionName;

    if( kernel )
    {
        const std::string& kernelName = getKernelName(kernel);
        str += "( ";
        str += kernelName;
        str += " )";
    }

    char temp[ CLI_MAX_STRING_SIZE ] = "";
    int size = CLI_VSPRINTF( temp, CLI_MAX_STRING_SIZE, formatStr, args );
    if( size >= 0 && size < CLI_MAX_STRING_SIZE )
    {
        str += ": ";
        str += temp;
    }
    else
    {
        str += ": too long";
    }
    callLoggingEnter( str, NULL );

    va_end( args );
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::callLoggingInfo(
    const std::string& str )
{
    m_OS.EnterCriticalSection();

    log( "---- " + str + "\n" );

    m_OS.LeaveCriticalSection();
}

void CLIntercept::callLoggingInfo(
    const char* formatStr,
    ... )
{
    va_list args;
    va_start( args, formatStr );

    char temp[ CLI_MAX_STRING_SIZE ] = "";
    int size = CLI_VSPRINTF( temp, CLI_MAX_STRING_SIZE, formatStr, args );
    if( size >= 0 && size < CLI_MAX_STRING_SIZE )
    {
        callLoggingInfo( std::string( temp ) );
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
    const std::string& functionName,
    const cl_kernel kernel,
    const cl_event* event )
{
    m_OS.EnterCriticalSection();

    std::string str;
    getCallLoggingPrefix( str );

    str += functionName;

    if( kernel )
    {
        const std::string& kernelName = getKernelName(kernel);
        str += "( ";
        str += kernelName;
        str += " )";
    }

    if( event )
    {
        char temp[ CLI_MAX_STRING_SIZE ] = "";
        CLI_SPRINTF( temp, CLI_MAX_STRING_SIZE, " created event = %p", *event );
        str += temp;
    }

    log( "<<<< " + str + "\n" );

    m_OS.LeaveCriticalSection();
}
void CLIntercept::callLoggingExit(
    const std::string& functionName,
    const cl_kernel kernel,
    const cl_event* event,
    const char* formatStr,
    ... )
{
    va_list args;
    va_start( args, formatStr );

    std::string str = functionName;

    if( kernel )
    {
        const std::string& kernelName = getKernelName(kernel);
        str += "( ";
        str += kernelName;
        str += " )";
    }

    char temp[ CLI_MAX_STRING_SIZE ] = "";

    if( event )
    {
        CLI_SPRINTF( temp, CLI_MAX_STRING_SIZE, " created event = %p", *event );
        str += temp;
    }

    int size = CLI_VSPRINTF( temp, CLI_MAX_STRING_SIZE, formatStr, args );
    if( size >= 0 && size < CLI_MAX_STRING_SIZE )
    {
        str += ": ";
        str += temp;
    }
    else
    {
        str += ": too long";
    }

    callLoggingExit( str, NULL, NULL );

    va_end( args );
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
        str += platformName;
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

    unsigned int    i = 0;
    for( i = 0; i < numDevices; i++ )
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
                str += " | ";
            }

            if( deviceName )
            {
                str += deviceName;
            }
            str += " (";
            str += enumName().name_device_type( deviceType );
            str += ")";
        }

        delete [] deviceName;
        deviceName = NULL;
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
                char temp[ CLI_MAX_STRING_SIZE ] = "";
                CLI_SPRINTF( temp, CLI_MAX_STRING_SIZE, "%p", eventList[i] );
                str += temp;
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
        while( properties[0] != 0 )
        {
            char    temp_str[ CLI_MAX_STRING_SIZE ];

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
                {
                    const void** pp = (const void**)( properties + 1 );
                    const void*  value = pp[0];
                    CLI_SPRINTF( temp_str, CLI_MAX_STRING_SIZE, "%p", value );
                    str += temp_str;
                }
                break;
            case CL_CONTEXT_INTEROP_USER_SYNC:
                {
                    const cl_bool*  pb = (const cl_bool*)( properties + 1);
                    cl_bool value = pb[0];
                    str += enumName().name_bool( value );
                }
                break;
            default:
                str += "<Unknown!>";
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
#if 0
                    if( property == CL_SAMPLER_LOD_MAX )
                    {
                        cl_float*   pFixup = (cl_float*)( properties + 1);
                        if( pFixup[0] < 0.5f )
                        {
                            pFixup[0] = 100.0f;
                        }
                    }
#endif

                    const cl_float* pf = (const cl_float*)( properties + 1 );

                    cl_float    value = pf[0];

                    char    fstr[ CLI_MAX_STRING_SIZE ];
                    CLI_SPRINTF( fstr, CLI_MAX_STRING_SIZE, "%.2f", value );
                    str += fstr;
                }
                break;
            default:
                str += "<Unexpected!>";
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
        while( properties[0] != 0 )
        {
            cl_int  property = (cl_int)properties[0];
            str += enumName().name( property ) + " = ";

            switch( property )
            {
            case CL_QUEUE_PROPERTIES:
                {
                    str += "<TODO>";
                }
                break;
            case CL_QUEUE_SIZE:
                {
                    const cl_uint*  pu = (const cl_uint*)( properties + 1);
                    cl_uint value = pu[0];
                    str += value;
                }
                break;
            default:
                str += "<Unexpected!>";
                break;
            }

            properties += 2;
            if( properties[0] != 0 )
            {
                str += ", ";
            }
        }
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
        cl_uint numKernels = num_kernels_ret[0];

        str += "kernels = [ ";
        for( cl_uint i = 0; i < numKernels; i++ )
        {
            char    s[256];
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
    char    s[CLI_MAX_STRING_SIZE] = "";

    if( getSampler(
            arg_size,
            arg_value,
            str ) )
    {
        CLI_SPRINTF( s, CLI_MAX_STRING_SIZE, "index = %d, size = %d, value = %s\n",
            arg_index,
            (unsigned int)arg_size,
            str.c_str() );
    }
    else if( ( arg_value != NULL ) &&
             ( arg_size == sizeof(cl_mem) ) )
    {
        cl_mem* pMem = (cl_mem*)arg_value;
        CLI_SPRINTF( s, CLI_MAX_STRING_SIZE, "index = %d, size = %d, value = %p",
            arg_index,
            (unsigned int)arg_size,
            pMem[0] );
    }
    else if( ( arg_value != NULL ) &&
             ( arg_size == sizeof(cl_uint) ) )
    {
        cl_uint*    pData = (cl_uint*)arg_value;
        CLI_SPRINTF( s, CLI_MAX_STRING_SIZE, "index = %d, size = %d, value = 0x%x",
            arg_index,
            (unsigned int)arg_size,
            pData[0] );
    }
    else if( ( arg_value != NULL ) &&
             ( arg_size == sizeof(cl_ulong) ) )
    {
        cl_ulong*   pData = (cl_ulong*)arg_value;
        CLI_SPRINTF( s, CLI_MAX_STRING_SIZE, "index = %d, size = %d, value = 0x%jx",
            arg_index,
            (unsigned int)arg_size,
            pData[0] );
    }
    else if( ( arg_value != NULL ) &&
             ( arg_size == sizeof(cl_int4) ) )
    {
        cl_int4*   pData = (cl_int4*)arg_value;
        CLI_SPRINTF( s, CLI_MAX_STRING_SIZE, "index = %d, size = %d, valueX = 0x%0x, valueY = 0x%0x, valueZ = 0x%0x, valueW = 0x%0x",
            arg_index,
            (unsigned int)arg_size,
            pData->s[0],
            pData->s[1],
            pData->s[2],
            pData->s[3]);
    }
    else
    {
        CLI_SPRINTF( s, CLI_MAX_STRING_SIZE, "index = %d, size = %d",
            arg_index,
            (unsigned int)arg_size );
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
                ss << ", ";
            }
        }
    }
    else
    {
        ss << "NULL?";
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
                ss << ", ";
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
        {
            cl_buffer_region*   pRegion = (cl_buffer_region*)createInfo;
            ss << "origin = "
                << pRegion->origin
                << " size = "
                << pRegion->size;
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
        m_OS.EnterCriticalSection();

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

        m_OS.LeaveCriticalSection();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::logBuild(
    uint64_t buildTimeStart,
    const cl_program program,
    cl_uint numDevices,
    const cl_device_id* deviceList )
{
    uint64_t    buildTimeEnd = m_OS.GetTimer();

    m_OS.EnterCriticalSection();

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
        unsigned int    programNumber = m_ProgramNumberMap[ program ];
        unsigned int    compileCount = m_ProgramNumberCompileCountMap[ programNumber ];

        logf( "Build Info for program %p, number %u, compile %u, for %u device(s):\n",
            program,
            programNumber,
            compileCount,
            numDevices );

        float   buildTimeMS = m_OS.TickToNS( buildTimeEnd - buildTimeStart ) / 1e6f;
        logf( "    Build finished in %.2f ms.\n", buildTimeMS );
    }

    if( errorCode == CL_SUCCESS )
    {
        size_t i = 0;
        for( i = 0; i < numDevices; i++ )
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

                    CLI_SPRINTF( str, 256, "Build Status for device %u = ",
                        (unsigned int)i );

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

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::logError(
    const std::string& functionName,
    cl_int errorCode )
{
    std::ostringstream  ss;
    ss << "ERROR! " << functionName << " returned " << enumName().name(errorCode) << " (" << errorCode << ")\n";

    m_OS.EnterCriticalSection();

    log( ss.str() );

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::logFlushOrFinishAfterEnqueueStart(
    const std::string& flushOrFinish,
    const std::string& functionName )
{
    m_OS.EnterCriticalSection();

    log( "Calling " + flushOrFinish + " after " + functionName + "...\n" );

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::logFlushOrFinishAfterEnqueueEnd(
    const std::string& flushOrFinish,
    const std::string& functionName,
    cl_int errorCode )
{
    std::ostringstream  ss;
    ss << "... " << flushOrFinish << " after " << functionName << " returned " << enumName().name( errorCode ) << " (" << errorCode << ")\n";

    m_OS.EnterCriticalSection();

    log( ss.str() );

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::logPreferredWorkGroupSizeMultiple(
    const cl_kernel* kernels,
    cl_uint numKernels )
{
    if( numKernels > 0 )
    {
        m_OS.EnterCriticalSection();

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
        while( numKernels-- )
        {
            cl_kernel   kernel = kernels[ numKernels ];

            if( errorCode == CL_SUCCESS )
            {
                log( "Preferred Work Group Size Multiple for: '" + getKernelName(kernel) + "':\n" );
            }
            if( errorCode == CL_SUCCESS )
            {
                size_t i = 0;
                for( i = 0; i < numDevices; i++ )
                {
                    size_t  kernelPreferredWorkGroupSizeMultiple = 0;
                    errorCode = dispatch().clGetKernelWorkGroupInfo(
                        kernel,
                        deviceList[i],
                        CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                        sizeof(kernelPreferredWorkGroupSizeMultiple),
                        &kernelPreferredWorkGroupSizeMultiple,
                        NULL );
                    if( errorCode == CL_SUCCESS )
                    {
                        char*   deviceName = NULL;

                        errorCode = allocateAndGetDeviceInfoString(
                            deviceList[i],
                            CL_DEVICE_NAME,
                            deviceName );
                        if( errorCode == CL_SUCCESS )
                        {
                            logf( "    for device %s: %u\n",
                                deviceName,
                                (unsigned int)kernelPreferredWorkGroupSizeMultiple );
                        }

                        delete [] deviceName;
                    }
                }
            }
        }

        delete [] deviceList;

        m_OS.LeaveCriticalSection();
    }
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
    m_OS.EnterCriticalSection();

    char    str[256] = "";
    CLI_SPRINTF( str, 256, "=======> Context Callback (private_info = %p, cb = %u):\n",
        private_info,
        (unsigned int)cb );

    log( str + errinfo + "\n" + "<======= End of Context Callback\n" );

    m_OS.LeaveCriticalSection();
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
        int     numProperties = 0;
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
        m_OS.EnterCriticalSection();

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

        m_OS.LeaveCriticalSection();
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

    CALL_LOGGING_ENTER( "event = %p, status = %s (%d)",
        event,
        pIntercept->enumName().name_command_exec_status( status ).c_str(),
        status );

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

    CALL_LOGGING_EXIT();

    delete pEventCallbackInfo;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::eventCallback(
    cl_event event,
    int status )
{
    // TODO: Since we call log the eventCallbackCaller, do we need to do
    //       anything here?
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::incrementEnqueueCounter()
{
    m_OS.EnterCriticalSection();

    m_EnqueueCounter++;

    m_OS.LeaveCriticalSection();
}

uint64_t CLIntercept::getEnqueueCounter()
{
    return m_EnqueueCounter;
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
                    m_OS.EnterCriticalSection();
                    logf( "Couldn't override NULL local work size: < %u > %% < %u > != 0!\n",
                        (unsigned int)global_work_size[0],
                        (unsigned int)m_Config.NullLocalWorkSizeX );
                    m_OS.LeaveCriticalSection();
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
                    m_OS.EnterCriticalSection();
                    logf( "Couldn't override NULL local work size: < %u, %u > %% < %u, %u > != 0!\n",
                        (unsigned int)global_work_size[0],
                        (unsigned int)global_work_size[1],
                        (unsigned int)m_Config.NullLocalWorkSizeX,
                        (unsigned int)m_Config.NullLocalWorkSizeY );
                    m_OS.LeaveCriticalSection();
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
                    m_OS.EnterCriticalSection();
                    logf( "Couldn't override NULL local work size: < %u, %u, %u > %% < %u, %u, %u > != 0!\n",
                        (unsigned int)global_work_size[0],
                        (unsigned int)global_work_size[1],
                        (unsigned int)global_work_size[2],
                        (unsigned int)m_Config.NullLocalWorkSizeX,
                        (unsigned int)m_Config.NullLocalWorkSizeY,
                        (unsigned int)m_Config.NullLocalWorkSizeZ );
                    m_OS.LeaveCriticalSection();
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
            length = strlen( strings[i] );
        }
        else
        {
            length = lengths[i];
        }
        allocSize += length;
    }

    // Allocate a multiple of four bytes.
    // Allocate some extra to make sure we're null terminated.
    allocSize = ( allocSize + ( 4 + 4 - 1 ) ) & ~( 4 - 1 );

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
                length = strlen( strings[i] );
            }
            else
            {
                length = lengths[i];
            }
            CLI_MEMCPY(
                pDst,
                remaining,
                strings[i],
                length );
            pDst += length;
            remaining -= length;
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
    m_OS.EnterCriticalSection();

    unsigned int    programNumber = m_ProgramNumberMap[ program ];
    unsigned int    compileCount = m_ProgramNumberCompileCountMap[ programNumber ];

    ++compileCount;

    m_ProgramNumberCompileCountMap[ programNumber ] = compileCount;

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
uint64_t CLIntercept::hashString(
    const char* singleString,
    size_t length )
{
    uint64_t    hash = 0;

    if( singleString != NULL )
    {
        const unsigned int* dwProgramSource = (const unsigned int*)singleString;
        size_t  dwProgramSize = length;

        dwProgramSize = ( dwProgramSize + ( 4 - 1 ) ) & ~( 4 - 1 );
        dwProgramSize /= 4;

        hash = Hash(
            dwProgramSource,
            dwProgramSize );
    }

    return hash;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::saveProgramHash(
    const cl_program program,
    uint64_t hash )
{
    m_OS.EnterCriticalSection();

    if( program != NULL )
    {
        m_ProgramHashMap[ program ] = hash;
    }

    m_OS.LeaveCriticalSection();
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

    m_OS.EnterCriticalSection();

    bool    injected = false;

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
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

    m_OS.LeaveCriticalSection();
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

    m_OS.EnterCriticalSection();

    bool    injected = false;

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
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

    m_OS.LeaveCriticalSection();
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
    m_OS.EnterCriticalSection();

    bool    injected = false;

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
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

    m_OS.LeaveCriticalSection();
    return injected;
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::injectProgramOptions(
    const cl_program program,
    const char*& options,
    char*& newOptions )
{
    m_OS.EnterCriticalSection();

    CLI_ASSERT( newOptions == NULL );

    bool    injected = false;

    unsigned int    programNumber = m_ProgramNumberMap[ program ];
    uint64_t        programHash = m_ProgramHashMap[ program ];
    unsigned int    compileCount = m_ProgramNumberCompileCountMap[ programNumber ];

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
        fileName += "/Inject";
    }
    // Make four candidate filenames.  They will have the form:
    //   CLI_<program number>_<hash>_<count>_options.txt, or
    //   CLI_<hash>_<count>_options.txt, or
    //   CLI_<hash>_options.txt, or
    //   CLI_options.txt
    {
        char    numberString1[256] = "";
        CLI_SPRINTF( numberString1, 256, "%04u_%08X_%04u",
            programNumber,
            (unsigned int)programHash,
            compileCount );

        char    numberString2[256] = "";
        CLI_SPRINTF( numberString2, 256, "%08X_%04u",
            (unsigned int)programHash,
            compileCount );

        char    numberString3[256] = "";
        CLI_SPRINTF( numberString3, 256, "%08X",
            (unsigned int)programHash );

        std::string fileName1;
        fileName1 = fileName;
        fileName1 += "/CLI_";
        fileName1 += numberString1;
        fileName1 += "_options.txt";

        std::string fileName2;
        fileName2 = fileName;
        fileName2 += "/CLI_";
        fileName2 += numberString2;
        fileName2 += "_options.txt";

        std::string fileName3;
        fileName3 = fileName;
        fileName3 += "/CLI_";
        fileName3 += numberString3;
        fileName3 += "_options.txt";

        std::string fileName4;
        fileName4 = fileName;
        fileName4 += "/CLI_options.txt";

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

                options = newOptions;

                injected = true;
            }

            is.close();
        }
    }

    m_OS.LeaveCriticalSection();
    return injected;
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::appendBuildOptions(
    const char*& options,
    char*& newOptions )
{
    m_OS.EnterCriticalSection();

    bool    modified = false;

    if( options == NULL )
    {
        // If the options string does not exist, we can simply point it at the
        // options we'd like to "append" to it.  We don't need to allocate any
        // new memory in this case.  We also expect that we haven't allocated
        // any new options in this case, because if we did, we would have
        // pointed the options string to the new options.

        CLI_ASSERT( newOptions == NULL );
        options = config().AppendBuildOptions.c_str();

        modified = true;
    }
    else
    {
        // If the options string does exist, we have two possibilities:
        // Either we've already modified the options so we've already
        // allocated new options, or we're still working on the application
        // provided options.

        size_t  newSize =
            strlen(options)
            + 1     // for a space
            + config().AppendBuildOptions.length()
            + 1;    // for the null terminator

        char* newNewOptions = new char[ newSize ];
        if( newNewOptions )
        {
            memset( newNewOptions, 0, newSize );

            CLI_STRCAT( newNewOptions, newSize, options );
            CLI_STRCAT( newNewOptions, newSize, " " );
            CLI_STRCAT( newNewOptions, newSize, config().AppendBuildOptions.c_str() );

            // If we have already allocated new options, we can free them
            // now.
            if( newOptions )
            {
                delete [] newOptions;
                newOptions = NULL;
            }

            // Either way, the new new options are now the new options.
            newOptions = newNewOptions;
            options = newOptions;

            modified = true;
        }
    }

    m_OS.LeaveCriticalSection();
    return modified;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpProgramSourceScript(
    cl_program program,
    const char* singleString )
{
#if defined(_WIN32)

    m_OS.EnterCriticalSection();

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
    }

    m_ProgramNumberMap[ program ] = m_ProgramNumber;
    m_ProgramNumberCompileCountMap[ m_ProgramNumber ] = 0;
    m_ProgramNumber++;

    m_OS.LeaveCriticalSection();

#else
    CLI_ASSERT( 0 );
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpProgramSource(
    uint64_t hash,
    cl_program program,
    const char* singleString )
{
    m_OS.EnterCriticalSection();

    CLI_ASSERT( config().DumpProgramSource || config().AutoCreateSPIRV );

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
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
    }

    m_ProgramNumberMap[ program ] = m_ProgramNumber;
    m_ProgramNumberCompileCountMap[ m_ProgramNumber ] = 0;
    m_ProgramNumber++;

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpInputProgramBinaries(
    uint64_t hash,
    const cl_program program,
    cl_uint num_devices,
    const cl_device_id* device_list,
    const size_t* lengths,
    const unsigned char** binaries )
{
    m_OS.EnterCriticalSection();

    CLI_ASSERT( config().DumpInputProgramBinaries );

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
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
            outputFileName += "_ACCELERATOR";
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
    }

    m_ProgramNumberMap[ program ] = m_ProgramNumber;
    m_ProgramNumberCompileCountMap[ m_ProgramNumber ] = 0;
    m_ProgramNumber++;

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpProgramSPIRV(
    uint64_t hash,
    cl_program program,
    const size_t length,
    const void* il )
{
    m_OS.EnterCriticalSection();

    CLI_ASSERT( config().DumpProgramSPIRV );

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
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

    // Dump the program source to a .cl file.
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
    }

    m_ProgramNumberMap[ program ] = m_ProgramNumber;
    m_ProgramNumberCompileCountMap[ m_ProgramNumber ] = 0;
    m_ProgramNumber++;

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpProgramOptionsScript(
    const cl_program program,
    const char* options )
{
#if defined(_WIN32)

    m_OS.EnterCriticalSection();

    CLI_ASSERT( config().DumpProgramSource || config().SimpleDumpProgramSource );

    unsigned int    programNumber = m_ProgramNumberMap[ program ];

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

            CLI_SPRINTF( curPos, remaining, "_%8.8x", programNumber );
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
    }

    m_OS.LeaveCriticalSection();

#else
    CLI_ASSERT( 0 );
#endif
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpProgramOptions(
    const cl_program program,
    const char* options )
{
    m_OS.EnterCriticalSection();

    CLI_ASSERT( config().DumpProgramSource || config().DumpProgramBinaries || config().DumpProgramSPIRV );

    unsigned int    programNumber = m_ProgramNumberMap[ program ];
    uint64_t        programHash = m_ProgramHashMap[ program ];
    unsigned int    compileCount = m_ProgramNumberCompileCountMap[ programNumber ];

    if( options )
    {
        std::string fileName;

        // Get the dump directory name.
        {
            OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
        }
        // Make the filename.  It will have the form:
        //   CLI_<program number>_<hash>_<compile count>
        // Leave off the extension for now.
        {
            char    numberString[256] = "";

            if( config().OmitProgramNumber )
            {
                CLI_SPRINTF( numberString, 256, "%08X_%04u",
                    (unsigned int)programHash,
                    compileCount );
            }
            else
            {
                CLI_SPRINTF( numberString, 256, "%04u_%08X_%04u",
                    programNumber,
                    (unsigned int)programHash,
                    compileCount );
            }

            fileName += "/CLI_";
            fileName += numberString;
        }
        // Dump the program source to a .txt file.
        {
            fileName += "_options.txt";
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
        }
    }

    m_OS.LeaveCriticalSection();
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

    unsigned int    programNumber = m_ProgramNumberMap[ program ];
    uint64_t        programHash = m_ProgramHashMap[ program ];
    unsigned int    compileCount = m_ProgramNumberCompileCountMap[ programNumber ];

    std::string fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
    }
    // Make the filename.  It will have the form:
    //   CLI_<program number>_<hash>_<compile count>
    // Leave off the extension for now.
    {
        char    numberString[256] = "";

        if( config().OmitProgramNumber )
        {
            CLI_SPRINTF( numberString, 256, "%08X_%04u",
                (unsigned int)programHash,
                compileCount );
        }
        else
        {
            CLI_SPRINTF( numberString, 256, "%04u_%08X_%04u",
                programNumber,
                (unsigned int)programHash,
                compileCount );
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
        fileName += "_ACCELERATOR";
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
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::updateHostTimingStats(
    const std::string& functionName,
    cl_kernel kernel,
    uint64_t start,
    uint64_t end )
{
    m_OS.EnterCriticalSection();

    std::string key( functionName );
    if( kernel )
    {
        const std::string& kernelName = getKernelName(kernel);
        key += "( ";
        key += kernelName;
        key += " )";
    }

    SCpuTimingStats* pCpuTimingStats = m_CpuTimingStatsMap[ key ];
    if( pCpuTimingStats == NULL )
    {
        pCpuTimingStats = new SCpuTimingStats;
        if( pCpuTimingStats == NULL )
        {
            // Memory allocation failure.
        }
        else
        {
            pCpuTimingStats->NumberOfCalls = 0;
            pCpuTimingStats->TotalTicks = 0;
            pCpuTimingStats->MinTicks = UINT_MAX;
            pCpuTimingStats->MaxTicks = 0;

            m_CpuTimingStatsMap[ key ] = pCpuTimingStats;
        }
    }

    uint64_t    numberOfCalls = 0;
    uint64_t    tickDelta = end - start;

    if( pCpuTimingStats != NULL )
    {
        pCpuTimingStats->NumberOfCalls++;
        pCpuTimingStats->TotalTicks += tickDelta;
        pCpuTimingStats->MinTicks = std::min< uint64_t >( pCpuTimingStats->MinTicks, tickDelta );
        pCpuTimingStats->MaxTicks = std::max< uint64_t >( pCpuTimingStats->MaxTicks, tickDelta );

        numberOfCalls = pCpuTimingStats->NumberOfCalls;
    }

    if( config().HostPerformanceTimeLogging )
    {
        uint64_t    nsDelta = OS().TickToNS( tickDelta );
        logf( "Host Time for call %u: %s = %u\n",
            (unsigned int)numberOfCalls,
            key.c_str(),
            (unsigned int)nsDelta );
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::modifyCommandQueueProperties(
    cl_command_queue_properties& props ) const
{
    if( config().DevicePerformanceTiming ||
        config().ITTPerformanceTiming ||
        config().ChromePerformanceTiming ||
        config().SIMDSurvey ||
        !config().DevicePerfCounterCustom.empty() )
    {
        props |= (cl_command_queue_properties)CL_QUEUE_PROFILING_ENABLE;
    }
    if( config().InOrderQueue )
    {
        props &= ~(cl_command_queue_properties)CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::createCommandQueueOverrideInit(
    const cl_queue_properties* properties,
    cl_queue_properties*& pLocalQueueProperties ) const
{
    // We want to add command queue properties, unless command queue
    // properties already exist (requesting the same property twice is an
    // error).  So, look through the queue properties for the command queue
    // properties enum.  We need to do this anyways to count the number of
    // property pairs.
    bool    foundCommandQueuePropertiesEnum = false;
    int     numProperties = 0;
    if( properties )
    {
        while( properties[ numProperties ] != 0 )
        {
            if( properties[ numProperties ] == CL_QUEUE_PROPERTIES )
            {
                foundCommandQueuePropertiesEnum = true;
            }
            numProperties += 2;
        }
    }

    if( foundCommandQueuePropertiesEnum == false )
    {
        // The performance hint property isn't already set, so we'll
        // need to allocate an extra pair of properties for it.
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
                    CLI_ASSERT( foundCommandQueuePropertiesEnum );

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
        // Add command queue properties if they aren't already set.
        if( foundCommandQueuePropertiesEnum == false )
        {
            cl_command_queue_properties props = 0;

            modifyCommandQueueProperties( props );

            pLocalQueueProperties[ numProperties ] = CL_QUEUE_PROPERTIES;
            pLocalQueueProperties[ numProperties + 1 ] = props;
            numProperties += 2;
        }
        // Add the terminating zero.
        pLocalQueueProperties[ numProperties ] = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::createCommandQueueOverrideCleanup(
    cl_queue_properties*& pLocalQueueProperties ) const
{
    if( pLocalQueueProperties )
    {
        delete pLocalQueueProperties;
        pLocalQueueProperties = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addTimingEvent(
    const std::string& functionName,
    const uint64_t queuedTime,
    const cl_kernel kernel,
    const cl_uint workDim,
    const size_t* gwo,
    const size_t* gws,
    const size_t* lws,
    cl_event event )
{
    m_OS.EnterCriticalSection();

    SEventListNode* pNode = new SEventListNode;
    if( pNode )
    {
        pNode->FunctionName = functionName;
        if( kernel )
        {
            pNode->KernelName = m_KernelNameMap[ kernel ].kernelName;
            if( m_Config.IndexLongKernelNames)
            {
                pNode->KernelId = m_KernelNameMap[ kernel ].kernelId;
            }

            if( config().DevicePerformanceTimeHashTracking )
            {
                cl_program program = NULL;
                dispatch().clGetKernelInfo(
                    kernel,
                    CL_KERNEL_PROGRAM,
                    sizeof(program),
                    &program,
                    NULL );
                if( program )
                {
                    unsigned int    programNumber = m_ProgramNumberMap[ program ];
                    uint64_t        programHash = m_ProgramHashMap[ program ];
                    unsigned int    compileCount = m_ProgramNumberCompileCountMap[ programNumber ];

                    char    hashString[256] = "";
                    if( config().OmitProgramNumber )
                    {
                        CLI_SPRINTF( hashString, 256, "(%08X_%04u)",
                            (unsigned int)programHash,
                            compileCount );
                    }
                    else
                    {
                        CLI_SPRINTF( hashString, 256, "(%04u_%08X_%04u)",
                            programNumber,
                            (unsigned int)programHash,
                            compileCount );
                    }
                    pNode->KernelName += hashString;
                    if( m_Config.IndexLongKernelNames )
                    {
                        pNode->KernelId += hashString;
                    }
                }
            }

            if( config().DevicePerformanceTimeKernelInfoTracking )
            {
                cl_command_queue queue = NULL;
                dispatch().clGetEventInfo(
                    event,
                    CL_EVENT_COMMAND_QUEUE,
                    sizeof(queue),
                    &queue,
                    NULL );
                if( queue )
                {
                    cl_device_id device = NULL;
                    dispatch().clGetCommandQueueInfo(
                        queue,
                        CL_QUEUE_DEVICE,
                        sizeof(device),
                        &device,
                        NULL );
                    if( device )
                    {
                        std::ostringstream  ss;
                        {
                            size_t  pwgsm = 0;
                            dispatch().clGetKernelWorkGroupInfo(
                                kernel,
                                device,
                                CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                sizeof(pwgsm),
                                &pwgsm,
                                NULL );
                            if( pwgsm )
                            {
                                ss << " SIMD" << (unsigned int)pwgsm;
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
                                ss << " SLM=" << (unsigned int)slm;
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
                                ss << " TPM=" << (unsigned int)tpm;
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
                                ss << " SPILL=" << (unsigned int)spill;
                            }
                        }
                        pNode->KernelName += ss.str();
                    }
                }
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
                        ss << " x " << gwo[1];
                    }
                    if( workDim >= 3 )
                    {
                        ss << " x " << gwo[2];
                    }
                }
                else
                {
                    ss << "NULL";
                }
                ss << " ]";
                pNode->KernelName += ss.str();
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
                pNode->KernelName += ss.str();
            }

            if( config().DevicePerformanceTimeLWSTracking )
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
                pNode->KernelName += ss.str();
            }
        }
        pNode->QueuedTime = queuedTime;
        pNode->Kernel = kernel; // Note: no retain, so cannot count on this value...
        pNode->Event = event;

        m_EventList.push_back( pNode );
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::checkTimingEvents()
{
    m_OS.EnterCriticalSection();

    CEventList::iterator    current = m_EventList.begin();
    CEventList::iterator    next;

    while( current != m_EventList.end() )
    {
        cl_int  errorCode = CL_SUCCESS;
        cl_int  eventStatus = 0;

        next = current;
        ++next;

        SEventListNode* pNode = *current;

        errorCode = dispatch().clGetEventInfo(
            pNode->Event,
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
                    config().ChromePerformanceTiming ||
                    config().SIMDSurvey )
                {
                    cl_ulong    commandQueued = 0;
                    cl_ulong    commandSubmit = 0;
                    cl_ulong    commandStart = 0;
                    cl_ulong    commandEnd = 0;

                    uint64_t    numberOfCalls = 0;

                    errorCode |= dispatch().clGetEventProfilingInfo(
                        pNode->Event,
                        CL_PROFILING_COMMAND_QUEUED,
                        sizeof( commandQueued ),
                        &commandQueued,
                        NULL );
                    errorCode |= dispatch().clGetEventProfilingInfo(
                        pNode->Event,
                        CL_PROFILING_COMMAND_SUBMIT,
                        sizeof( commandSubmit ),
                        &commandSubmit,
                        NULL );
                    errorCode |= dispatch().clGetEventProfilingInfo(
                        pNode->Event,
                        CL_PROFILING_COMMAND_START,
                        sizeof( commandStart ),
                        &commandStart,
                        NULL );
                    errorCode |= dispatch().clGetEventProfilingInfo(
                        pNode->Event,
                        CL_PROFILING_COMMAND_END,
                        sizeof( commandEnd ),
                        &commandEnd,
                        NULL );
                    if( errorCode == CL_SUCCESS )
                    {
                        cl_ulong delta = commandEnd - commandStart;

                        const std::string&  key =
                            pNode->KernelName.empty() ?
                            pNode->FunctionName :
                            pNode->KernelName;

                        SDeviceTimingStats* pDeviceTimingStats = m_DeviceTimingStatsMap[ key ];
                        if( pDeviceTimingStats == NULL )
                        {
                            pDeviceTimingStats = new SDeviceTimingStats;
                            if( pDeviceTimingStats == NULL )
                            {
                                // Memory allocation failure.
                            }
                            else
                            {
                                pDeviceTimingStats->KernelId= "";
                                pDeviceTimingStats->NumberOfCalls = 0;
                                pDeviceTimingStats->TotalNS = 0;
                                pDeviceTimingStats->MinNS = CL_ULONG_MAX;
                                pDeviceTimingStats->MaxNS = 0;

                                m_DeviceTimingStatsMap[ key ] = pDeviceTimingStats;
                            }
                        }

                        if( pDeviceTimingStats != NULL )
                        {
                            if( m_Config.IndexLongKernelNames )
                            {
                                pDeviceTimingStats->KernelId = pNode->KernelId;
                            }
                            pDeviceTimingStats->NumberOfCalls++;
                            pDeviceTimingStats->TotalNS += delta;
                            pDeviceTimingStats->MinNS = std::min< cl_ulong >( pDeviceTimingStats->MinNS, delta );
                            pDeviceTimingStats->MaxNS = std::max< cl_ulong >( pDeviceTimingStats->MaxNS, delta );

                            numberOfCalls = pDeviceTimingStats->NumberOfCalls;
                        }

                        if( config().DevicePerformanceTimeLogging )
                        {
                            cl_ulong    queuedDelta = commandSubmit - commandQueued;
                            cl_ulong    submitDelta = commandStart - commandSubmit;

                            std::ostringstream  ss;

                            ss << "Device Time for call " << numberOfCalls << " to " << key << " = "
                                << queuedDelta << " ns (queued -> submit), "
                                << submitDelta << " ns (submit -> start), "
                                << delta << " ns (start -> end)\n";

                            log( ss.str() );
                        }

                        if( config().DevicePerformanceTimelineLogging )
                        {
                            std::ostringstream  ss;

                            ss << "Device Timeline for call " << numberOfCalls << " to " << key << " = "
                                << commandQueued << " ns (queued), "
                                << commandSubmit << " ns (submit), "
                                << commandStart << " ns (start), "
                                << commandEnd << " ns (end)\n";

                            log( ss.str() );
                        }

                        if( config().SIMDSurvey &&
                            pNode->Kernel )
                        {
                            SSIMDSurveyKernel*  pSIMDSurveyKernel =
                                m_SIMDSurveyKernelMap[ pNode->Kernel ];
                            if( pSIMDSurveyKernel )
                            {
                                if( pNode->Kernel == pSIMDSurveyKernel->SIMD8Kernel &&
                                    pSIMDSurveyKernel->SIMD8ExecutionTimeNS > delta )
                                {
                                    pSIMDSurveyKernel->SIMD8ExecutionTimeNS = delta;
                                    logf( "SIMD Survey: Results: New min SIMD8 Time for kernel %s is: %lu\n",
                                        pNode->KernelName.c_str(),
                                        pSIMDSurveyKernel->SIMD8ExecutionTimeNS );
                                }
                                if( pNode->Kernel == pSIMDSurveyKernel->SIMD16Kernel &&
                                    pSIMDSurveyKernel->SIMD16ExecutionTimeNS > delta )
                                {
                                    pSIMDSurveyKernel->SIMD16ExecutionTimeNS = delta;
                                    logf( "SIMD Survey: Results: New min SIMD16 Time for kernel %s is: %lu\n",
                                        pNode->KernelName.c_str(),
                                        pSIMDSurveyKernel->SIMD16ExecutionTimeNS );
                                }
                                if( pNode->Kernel == pSIMDSurveyKernel->SIMD32Kernel &&
                                    pSIMDSurveyKernel->SIMD32ExecutionTimeNS > delta )
                                {
                                    pSIMDSurveyKernel->SIMD32ExecutionTimeNS = delta;
                                    logf( "SIMD Survey: Results: New min SIMD32 Time for kernel %s is: %lu\n",
                                        pNode->KernelName.c_str(),
                                        pSIMDSurveyKernel->SIMD32ExecutionTimeNS );
                                }
                                if( pNode->Kernel != pSIMDSurveyKernel->SIMD8Kernel &&
                                    pNode->Kernel != pSIMDSurveyKernel->SIMD16Kernel &&
                                    pNode->Kernel != pSIMDSurveyKernel->SIMD32Kernel )
                                {
                                    logf( "SIMD Survey: Results: Default Time for kernel %s is: %lu\n",
                                        pNode->KernelName.c_str(),
                                        delta );
                                }
                            }
                            else
                            {
                                logf( "SIMD Survey: Results: Don't have any information kernel %p!?!?\n",
                                    pNode->Kernel );
                            }
                        }
                    }
                }

#if defined(USE_ITT)
                if( config().ITTPerformanceTiming )
                {
                    const std::string& name =
                        pNode->KernelName.empty() ?
                        pNode->FunctionName :
                        pNode->KernelName;

                    ittTraceEvent(
                        name,
                        pNode->Event,
                        pNode->QueuedTime );
                }
#endif

                if( config().ChromePerformanceTiming )
                {
                    const std::string& name =
                        pNode->KernelName.empty() ?
                        pNode->FunctionName :
                        (m_Config.IndexLongKernelNames) ?
                        pNode->KernelId :
                        pNode->KernelName;

                    chromeTraceEvent(
                        name,
                        pNode->Event,
                        pNode->QueuedTime );
                }

#if defined(USE_MDAPI)
                if( !config().DevicePerfCounterCustom.empty() )
                {
                    const std::string& name =
                        pNode->KernelName.empty() ?
                        pNode->FunctionName :
                        pNode->KernelName;

                    saveMDAPICounters(
                        name,
                        pNode->Event );
                }
#endif

                dispatch().clReleaseEvent( pNode->Event );
                delete pNode;

                m_EventList.erase( current );
            }
            break;
        case CL_INVALID_EVENT:
            {
                // This is unexpected.  We retained the event when we
                // added it to the list.  Remove the event from the
                // list.
                logf( "Unexpectedly got CL_INVALID_EVENT for an event from %s!\n",
                    pNode->FunctionName.c_str() );

                delete pNode;

                m_EventList.erase( current );
            }
            break;
        default:
            // nothing
            break;
        }

        current = next;
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addKernelName(
    cl_kernel kernel,
    const std::string& kernelName )
{
    m_OS.EnterCriticalSection();

    m_KernelNameMap[ kernel ].kernelName = kernelName;
    if( m_Config.IndexLongKernelNames)
    {
      m_KernelNameMap[ kernel ].kernelId   = "k_" + std::to_string(m_kernelId++);
    }
    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addKernelNames(
    cl_kernel* kernels,
    cl_uint numKernels )
{
    m_OS.EnterCriticalSection();

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
                    m_KernelNameMap[ kernel ].kernelName = kernelName;
                    if( m_Config.IndexLongKernelNames)
                    {
                      m_KernelNameMap[ kernel ].kernelId   = "k_" + std::to_string(m_kernelId++);
                    }
                }

                delete [] kernelName;
            }
        }
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::removeKernel(
    cl_kernel kernel )
{
    m_OS.EnterCriticalSection();

    cl_uint     refCount = 0;
    cl_int      errorCode = CL_SUCCESS;

    errorCode = dispatch().clGetKernelInfo(
        kernel,
        CL_KERNEL_REFERENCE_COUNT,
        sizeof( refCount ),
        &refCount,
        NULL );
    if( errorCode == CL_SUCCESS )
    {
        if( refCount == 1 )
        {
            m_KernelNameMap.erase( kernel );

            SSIMDSurveyKernel*  pSIMDSurveyKernel =
                m_SIMDSurveyKernelMap[ kernel ];
            if( pSIMDSurveyKernel )
            {
                errorCode = dispatch().clReleaseKernel( pSIMDSurveyKernel->SIMD8Kernel );
                errorCode = dispatch().clReleaseKernel( pSIMDSurveyKernel->SIMD16Kernel );
                errorCode = dispatch().clReleaseKernel( pSIMDSurveyKernel->SIMD32Kernel );

                // Remove the parent kernel and each of the child kernels from the map.
                m_SIMDSurveyKernelMap.erase( kernel );

                m_SIMDSurveyKernelMap.erase( pSIMDSurveyKernel->SIMD8Kernel );
                m_SIMDSurveyKernelMap.erase( pSIMDSurveyKernel->SIMD16Kernel );
                m_SIMDSurveyKernelMap.erase( pSIMDSurveyKernel->SIMD32Kernel );

                // Also clean up the kernel name map.
                m_KernelNameMap.erase( pSIMDSurveyKernel->SIMD8Kernel );
                m_KernelNameMap.erase( pSIMDSurveyKernel->SIMD16Kernel );
                m_KernelNameMap.erase( pSIMDSurveyKernel->SIMD32Kernel );

                // Done!
                delete pSIMDSurveyKernel;
                pSIMDSurveyKernel = NULL;
            }
        }
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addBuffer(
    cl_mem buffer )
{
    if( buffer )
    {
        m_OS.EnterCriticalSection();

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

        m_OS.LeaveCriticalSection();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addSampler(
    cl_sampler sampler,
    const std::string& str )
{
    if( sampler )
    {
        m_OS.EnterCriticalSection();
        m_SamplerDataMap[sampler] = str;
        m_OS.LeaveCriticalSection();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::removeSampler(
    cl_sampler sampler )
{
    if( sampler )
    {
        m_OS.EnterCriticalSection();

        CSamplerDataMap::iterator iter = m_SamplerDataMap.find( sampler );
        if( iter != m_SamplerDataMap.end() )
        {
            m_SamplerDataMap.erase( iter );
        }

        m_OS.LeaveCriticalSection();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
bool CLIntercept::getSampler(
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
void CLIntercept::dumpArgument(
    cl_kernel kernel,
    cl_int arg_index,
    size_t size,
    const void *pBuffer )
{
    if( kernel )
    {
        m_OS.EnterCriticalSection();

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
                (unsigned int)m_EnqueueCounter );
            fileName += "SetKernelArg_";
            fileName += enqueueCount;
        }

        // Add the kernel name to the filename
        {
            fileName += "_Kernel_";
            fileName += getKernelName(kernel);
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
            }
        }

        m_OS.LeaveCriticalSection();
    }
}
///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addImage(
    cl_mem image )
{
    if( image )
    {
        m_OS.EnterCriticalSection();

        cl_int  errorCode = CL_SUCCESS;

        size_t  width = 0;
        size_t  height = 0;
        size_t  depth = 0;
        size_t  arraySize = 0;
        size_t  elementSize = 0;

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

        if( errorCode == CL_SUCCESS )
        {
            SImageInfo  imageInfo;

            imageInfo.Region[0] = width;
            if( height == 0 )
            {
                if( arraySize == 0 )
                {
                    imageInfo.Region[1] = 1;            // 1D iamge
                }
                else
                {
                    imageInfo.Region[1] = arraySize;    // 1D image array
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
                }
                else
                {
                    imageInfo.Region[2] = arraySize;    // 2D image array
                }
            }
            else
            {
                imageInfo.Region[2] = depth;            // 3D image
            }

            imageInfo.ElementSize = elementSize;

            m_MemAllocNumberMap[ image ] = m_MemAllocNumber;
            m_ImageInfoMap[ image ] = imageInfo;
            m_MemAllocNumber++;
        }

        m_OS.LeaveCriticalSection();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::removeMemObj(
    cl_mem memobj )
{
    m_OS.EnterCriticalSection();

    cl_uint refCount = 0;
    cl_int  errorCode = CL_SUCCESS;

    errorCode = dispatch().clGetMemObjectInfo(
        memobj,
        CL_MEM_REFERENCE_COUNT,
        sizeof( refCount ),
        &refCount,
        NULL );
    if( errorCode == CL_SUCCESS )
    {
        if( refCount == 1 )
        {
            m_MemAllocNumberMap.erase( memobj );
            m_BufferInfoMap.erase( memobj );
            m_ImageInfoMap.erase( memobj );
        }
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::addSVMAllocation(
    void* svmPtr,
    size_t size )
{
    if( svmPtr )
    {
        m_OS.EnterCriticalSection();

        m_MemAllocNumberMap[ svmPtr ] = m_MemAllocNumber;
        m_SVMAllocInfoMap[ svmPtr ] = size;
        m_MemAllocNumber++;

        m_OS.LeaveCriticalSection();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::removeSVMAllocation(
    void* svmPtr )
{
    m_OS.EnterCriticalSection();

    m_MemAllocNumberMap.erase( svmPtr );
    m_SVMAllocInfoMap.erase( svmPtr );

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::setKernelArg(
    cl_kernel kernel,
    cl_uint arg_index,
    cl_mem memobj )
{
    m_OS.EnterCriticalSection();

    if( m_MemAllocNumberMap.find( memobj ) != m_MemAllocNumberMap.end() )
    {
        CKernelArgMemMap&   kernelArgMap = m_KernelArgMap[ kernel ];
        kernelArgMap[ arg_index ] = memobj;
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::setKernelArgSVMPointer(
    cl_kernel kernel,
    cl_uint arg_index,
    const void* arg )
{
    m_OS.EnterCriticalSection();

    // Unlike clSetKernelArg(), which must pass a cl_mem, clSetKernelArgSVMPointer
    // can pass a pointer to the base of a SVM allocation or anywhere inside of
    // an SVM allocation.  As a result, we may need to search the SVM map to find
    // the base address and size of the SVM allocation.  Still, try to just lookup
    // the SVM allocation in the map, just in case the app sets the base address
    // (this may be the common case?).

    CKernelArgMemMap&   kernelArgMap = m_KernelArgMap[ kernel ];

    if( m_SVMAllocInfoMap.find( arg ) != m_SVMAllocInfoMap.end() )
    {
        // Got it, the pointer was the base address of an SVM allocation.
        kernelArgMap[ arg_index ] = arg;
    }
    else
    {
        intptr_t    iarg = (intptr_t)arg;
        for( CSVMAllocInfoMap::iterator i = m_SVMAllocInfoMap.begin();
             i != m_SVMAllocInfoMap.end();
             ++i )
        {
            const void* ptr = (*i).first;
            size_t      size = (*i).second;

            intptr_t    start = (intptr_t)ptr;
            intptr_t    end = start + size;
            if( start <= iarg &&
                iarg < end )
            {
                kernelArgMap[ arg_index ] = ptr;
                break;
            }
        }
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpBuffersForKernel(
    const std::string& name,
    cl_kernel kernel,
    cl_command_queue command_queue )
{
    m_OS.EnterCriticalSection();

    std::string fileNamePrefix = "";

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

    CKernelArgMemMap&   kernelArgMemMap = m_KernelArgMap[ kernel ];
    CKernelArgMemMap::iterator  i = kernelArgMemMap.begin();
    while( i != kernelArgMemMap.end() )
    {
        CLI_C_ASSERT( sizeof(void*) == sizeof(cl_mem) );
        cl_uint arg_index = (*i).first;
        void*   allocation = (void*)(*i).second;
        cl_mem  memobj = (cl_mem)allocation;
        ++i;
        if( ( m_SVMAllocInfoMap.find( allocation ) != m_SVMAllocInfoMap.end() ) ||
            ( m_BufferInfoMap.find( memobj ) != m_BufferInfoMap.end() ) )
        {
            unsigned int        number = m_MemAllocNumberMap[ memobj ];

            std::string fileName = fileNamePrefix;
            char    tmpStr[ MAX_PATH ];

            // Add the enqueue count to file name
            {
                CLI_SPRINTF( tmpStr, MAX_PATH, "%04u",
                    (unsigned int)m_EnqueueCounter );

                fileName += "Enqueue_";
                fileName += tmpStr;
            }

            // Add the kernel name to the filename
            {
                fileName += "_Kernel_";
                fileName += getKernelName(kernel);
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

            // Dump the buffer contents to the file.
            if( m_SVMAllocInfoMap.find( allocation ) != m_SVMAllocInfoMap.end() )
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

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpImagesForKernel(
    const std::string& name,
    cl_kernel kernel,
    cl_command_queue command_queue )
{
    m_OS.EnterCriticalSection();

    std::string fileNamePrefix = "";

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
            char    tmpStr[ MAX_PATH ];

            // Add the enqueue count to file name
            {
                CLI_SPRINTF( tmpStr, MAX_PATH, "%04u",
                    (unsigned int)m_EnqueueCounter );

                fileName += "Enqueue_";
                fileName += tmpStr;
            }

            // Add the kernel name to the filename
            {
                fileName += "_Kernel_";
                fileName += getKernelName(kernel);
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
                CLI_SPRINTF( tmpStr, MAX_PATH, "_%ux%ux%u_%ubpp",
                    (unsigned int)info.Region[0],
                    (unsigned int)info.Region[1],
                    (unsigned int)info.Region[2],
                    (unsigned int)info.ElementSize * 8 );

                fileName += tmpStr;
            }

            // Add extension to file name
            {
                fileName += ".raw";
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
                    }

                    delete [] readImageData;
                }
            }
        }
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpBuffer(
    const std::string& name,
    cl_mem memobj,
    cl_command_queue command_queue,
    void* ptr,
    size_t offset,
    size_t size )
{
    m_OS.EnterCriticalSection();

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

            CLI_SPRINTF( offsetName, MAX_PATH, "%04u",
                (unsigned int)offset );

            fileName += "_Offset_";
            fileName += offsetName;
        }

        // Add the enqueue count to file name
        {
            char    enqueueCount[ MAX_PATH ];

            CLI_SPRINTF( enqueueCount, MAX_PATH, "%04u",
                (unsigned int)m_EnqueueCounter );

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

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::checkEventList(
    const std::string& functionName,
    cl_uint numEvents,
    const cl_event* eventList )
{
    if( numEvents != 0 && eventList == NULL )
    {
        m_OS.EnterCriticalSection();
        logf( "Check Events for %s: Num Events is %d, but Event List is NULL!\n",
            functionName.c_str(),
            numEvents );
        m_OS.LeaveCriticalSection();
    }
    else
    {
        for( cl_uint i = 0; i < numEvents; i++ )
        {
            cl_int  eventCommandExecutionStatus = 0;
            cl_int  errorCode = dispatch().clGetEventInfo(
                eventList[i],
                CL_EVENT_COMMAND_EXECUTION_STATUS,
                sizeof(eventCommandExecutionStatus),
                &eventCommandExecutionStatus,
                NULL );
            if( errorCode != CL_SUCCESS )
            {
                m_OS.EnterCriticalSection();
                logf( "Check Events for %s: clGetEventInfo for event %p returned %s (%d)!\n",
                    functionName.c_str(),
                    eventList[i],
                    enumName().name(errorCode).c_str(),
                    errorCode );
                m_OS.LeaveCriticalSection();
            }
            else if( eventCommandExecutionStatus < 0 )
            {
                m_OS.EnterCriticalSection();
                logf( "Check Events for %s: event %p is in an error state (%d)!\n",
                    functionName.c_str(),
                    eventList[i],
                    eventCommandExecutionStatus );
                m_OS.LeaveCriticalSection();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::startAubCapture(
    const std::string& functionName,
    const cl_kernel kernel,
    const cl_uint workDim,
    const size_t* gws,
    const size_t* lws,
    cl_command_queue command_queue )
{
    if( m_AubCaptureStarted == false )
    {
        m_OS.EnterCriticalSection();

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

                    CLI_SPRINTF( charBuf, MAX_PATH, "%08u", (cl_uint)m_EnqueueCounter );

                    fileName += charBuf;
                    fileName += "_";

                    if( kernel )
                    {
                        fileName += "kernel_";
                        fileName += getKernelName(kernel);

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

                fileName += ".daf";
            }

            // Now make directories as appropriate.
            {
                OS().MakeDumpDirectories( fileName );
            }

            OS().StartAubCapture(
                fileName,
                config().AubCaptureStartWait );
            log( "AubCapture started... maybe.  Filename is: " + fileName + "\n" );

            // No matter what, set the flag that aubcapture is started, so we
            // don't try again.
            m_AubCaptureStarted = true;
        }

        m_OS.LeaveCriticalSection();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::stopAubCapture(
    cl_command_queue command_queue )
{
    if( m_AubCaptureStarted == true )
    {
        m_OS.EnterCriticalSection();

        if( m_AubCaptureStarted == true )
        {
            if( command_queue )
            {
                dispatch().clFinish( command_queue );
            }

            OS().StopAubCapture(
                config().AubCaptureEndWait );
            log( "AubCapture stopped.\n" );

            // No matter what, clar the flag that aubcapture is started, so we
            // don't try again.
            m_AubCaptureStarted = false;
        }

        m_OS.LeaveCriticalSection();
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::initPrecompiledKernelOverrides(
    const cl_context context )
{
    m_OS.EnterCriticalSection();
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

        // Get the program string from the resource embedded into this DLL.
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
                            cl_uint i = 0;
                            for( i = 0; i < numDevices; i++ )
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
    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::initBuiltinKernelOverrides(
    const cl_context context )
{
    m_OS.EnterCriticalSection();
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

        // Get the program string from the resource embedded into this DLL.
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
                            cl_uint i = 0;
                            for( i = 0; i < numDevices; i++ )
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
    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
cl_program CLIntercept::createProgramWithInjectionBinaries(
    uint64_t hash,
    cl_context context,
    cl_int* errcode_ret )
{
    m_OS.EnterCriticalSection();

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
            OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
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
                    suffix += "_ACCELERATOR";
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
                    log( "Injecting binaries failed: clCreateProgramWithBinary() returned %s\n" +
                        enumName().name( errorCode ) + "\n" );
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

    m_OS.LeaveCriticalSection();
    return program;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpProgramBinary(
    const cl_program program )
{
    m_OS.EnterCriticalSection();

    unsigned int    programNumber = m_ProgramNumberMap[ program ];
    uint64_t        programHash = m_ProgramHashMap[ program ];
    unsigned int    compileCount = m_ProgramNumberCompileCountMap[ programNumber ];

    std::string     fileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
    }
    // Make the filename.  It will have the form:
    //   CLI_<program number>_<hash>_<compile count>
    // Leave off the extension for now.
    {
        char    numberString[256] = "";

        if( config().OmitProgramNumber )
        {
            CLI_SPRINTF( numberString, 256, "%08X_%04u",
                (unsigned int)programHash,
                compileCount );
        }
        else
        {
            CLI_SPRINTF( numberString, 256, "%04u_%08X_%04u",
                programNumber,
                (unsigned int)programHash,
                compileCount );
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
                    outputFileName += "_ACCELERATOR";
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

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::dumpKernelISABinaries(
    const cl_program program )
{
    m_OS.EnterCriticalSection();

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
        unsigned int    programNumber = m_ProgramNumberMap[ program ];
        uint64_t        programHash = m_ProgramHashMap[ program ];
        unsigned int    compileCount = m_ProgramNumberCompileCountMap[ programNumber ];

        std::string fileNamePrefix;

        // Get the dump directory name.
        {
            OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileNamePrefix );
        }
        // Make the filename prefix.  It will have the form:
        //   CLI_<program number>_<hash>_<compile count>_<device type>_<kernel name>.isabin
        // We'll fill in the device type and kernel name later.
        {
            char    numberString[256] = "";

            if( config().OmitProgramNumber )
            {
                CLI_SPRINTF( numberString, 256, "%08X_%04u_",
                    (unsigned int)programHash,
                    compileCount );
            }
            else
            {
                CLI_SPRINTF( numberString, 256, "%04u_%08X_%04u_",
                    programNumber,
                    (unsigned int)programHash,
                    compileCount );
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
                        fileName += "ACCELERATOR_";
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

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
cl_program CLIntercept::createProgramWithInjectionSPIRV(
    uint64_t hash,
    cl_context context,
    cl_int* errcode_ret )
{
    m_OS.EnterCriticalSection();

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
            OS().GetDumpDirectoryName(sc_DumpDirectoryName, fileName);
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

    m_OS.LeaveCriticalSection();
    return program;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::autoCreateSPIRV(
    const cl_program program,
    const char* raw_options )
{
    m_OS.EnterCriticalSection();

    unsigned int    programNumber = m_ProgramNumberMap[ program ];
    uint64_t        programHash = m_ProgramHashMap[ program ];
    unsigned int    compileCount = m_ProgramNumberCompileCountMap[ programNumber ];

    std::string     dumpDirectoryName;
    std::string     inputFileName;
    std::string     outputFileName;

    // Get the dump directory name.
    {
        OS().GetDumpDirectoryName( sc_DumpDirectoryName, dumpDirectoryName );
    }

    // Re-create the input file name.  This will be a program source file we dumped
    // earlier.  It will have the form:
    //   CLI_<program number>_<hash>_source.cl
    {
        char    numberString[256] = "";

        if( config().OmitProgramNumber )
        {
            CLI_SPRINTF( numberString, 256, "%08X",
                (unsigned int)programHash );
        }
        else
        {
            CLI_SPRINTF( numberString, 256, "%04u_%08X",
                programNumber,
                (unsigned int)programHash );
        }

        inputFileName = dumpDirectoryName;
        inputFileName += "/CLI_";
        inputFileName += numberString;
        inputFileName += "_source.cl";
    }

    // Make the output file name.  It will have the form:
    //   CLI_<program number>_<hash>_<compile count>.spv
    {
        char    numberString[256] = "";

        if( config().OmitProgramNumber )
        {
            CLI_SPRINTF( numberString, 256, "%08X_%04u",
                (unsigned int)programHash,
                compileCount );
        }
        else
        {
            CLI_SPRINTF( numberString, 256, "%04u_%08X_%04u",
                programNumber,
                (unsigned int)programHash,
                compileCount );
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

    m_OS.LeaveCriticalSection();
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
bool CLIntercept::overrideGetPlatformInfo(
    cl_platform_info param_name,
    size_t param_value_size,
    void* param_value,
    size_t* param_value_size_ret,
    cl_int& errorCode )
{
    bool    override = false;

    m_OS.EnterCriticalSection();

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
    default:
        break;
    }

    m_OS.LeaveCriticalSection();

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
    cl_int& errorCode )
{
    bool    override = false;

    m_OS.EnterCriticalSection();

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
#if 0
    // This is a hack to get Sandra to try to compile fp64
    // kernels on devices that do not report fp64 capabilities.
    case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
        {
            cl_uint*    ptr = (cl_uint*)param_value;
            errorCode = writeParamToMemory(
                param_value_size,
                (cl_uint)1,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
    case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
        {
            cl_uint*    ptr = (cl_uint*)param_value;
            errorCode = writeParamToMemory(
                param_value_size,
                (cl_uint)1,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
#endif
#if 0
    // This is a hack to get fp16 conformance tests to run on
    // Broadwell.
    case CL_DEVICE_HALF_FP_CONFIG:
        {
            cl_device_fp_config value =
                CL_FP_ROUND_TO_NEAREST |
                CL_FP_ROUND_TO_ZERO |
                CL_FP_INF_NAN |
                CL_FP_ROUND_TO_INF;

            cl_device_fp_config*    ptr = (cl_device_fp_config*)param_value;
            errorCode = writeParamToMemory(
                param_value_size,
                value,
                param_value_size_ret,
                ptr );
            override = true;
        }
        break;
#endif
#if 0
    // This is a hack to get fp32 denormal tests to run on Broadwell.
    case CL_DEVICE_SINGLE_FP_CONFIG:
        {
            cl_device_fp_config value = 0;
            errorCode = dispatch().clGetDeviceInfo(
                device,
                param_name,
                sizeof(value),
                &value,
                NULL );
            if( errorCode == CL_SUCCESS )
            {
                value |= CL_FP_DENORM;

                cl_device_fp_config*    ptr = (cl_device_fp_config*)param_value;
                errorCode = writeParamToMemory(
                    param_value_size,
                    value,
                    param_value_size_ret,
                    ptr );
                override = true;
            }

        }
        break;
#endif
    default:
        break;
    }

    m_OS.LeaveCriticalSection();

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
    m_OS.EnterCriticalSection();

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

    m_OS.LeaveCriticalSection();

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
    m_OS.EnterCriticalSection();

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

    m_OS.LeaveCriticalSection();

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
    m_OS.EnterCriticalSection();

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

    m_OS.LeaveCriticalSection();

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
    m_OS.EnterCriticalSection();

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

    m_OS.LeaveCriticalSection();

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
    m_OS.EnterCriticalSection();

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

    m_OS.LeaveCriticalSection();

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
    m_OS.EnterCriticalSection();

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

    m_OS.LeaveCriticalSection();

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
    m_OS.EnterCriticalSection();

    cl_program  program = NULL;

    SBuiltinKernelOverrides*    pOverrides = m_BuiltinKernelOverridesMap[ context ];
    if( pOverrides )
    {
        program = pOverrides->Program;
        dispatch().clRetainProgram( program );
    }

    m_OS.LeaveCriticalSection();
    return program;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_kernel CLIntercept::createBuiltinKernel(
    cl_program program,
    const std::string& kernel_name,
    cl_int* errcode_ret )
{
    m_OS.EnterCriticalSection();

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

    m_OS.LeaveCriticalSection();

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
    m_OS.EnterCriticalSection();

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

    m_OS.LeaveCriticalSection();

    return errorCode;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::SIMDSurveyCreateProgramFromSource(
    const cl_program program,
    cl_context context,
    cl_uint count,
    const char** strings,
    const size_t* lengths )
{
    m_OS.EnterCriticalSection();

    cl_int  errorCode = CL_SUCCESS;

    SSIMDSurveyProgram* pSIMDSurveyProgram =
        m_SIMDSurveyProgramMap[ program ];
    if( pSIMDSurveyProgram )
    {
        errorCode = dispatch().clReleaseProgram( pSIMDSurveyProgram->SIMD8Program );
        errorCode = dispatch().clReleaseProgram( pSIMDSurveyProgram->SIMD16Program );
        errorCode = dispatch().clReleaseProgram( pSIMDSurveyProgram->SIMD32Program );

        delete pSIMDSurveyProgram;
        pSIMDSurveyProgram = NULL;

        m_SIMDSurveyProgramMap[ program ] = NULL;
    }

    pSIMDSurveyProgram = new SSIMDSurveyProgram;
    if( pSIMDSurveyProgram )
    {
        log( "SIMD Survey: CreateProgramFromSource\n" );

        pSIMDSurveyProgram->SIMD8Program = dispatch().clCreateProgramWithSource(
            context,
            count,
            strings,
            lengths,
            &errorCode );
        pSIMDSurveyProgram->SIMD16Program = dispatch().clCreateProgramWithSource(
            context,
            count,
            strings,
            lengths,
            &errorCode );
        pSIMDSurveyProgram->SIMD32Program = dispatch().clCreateProgramWithSource(
            context,
            count,
            strings,
            lengths,
            &errorCode );

        m_SIMDSurveyProgramMap[ program ] = pSIMDSurveyProgram;
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::SIMDSurveyBuildProgram(
    const cl_program program,
    cl_uint numDevices,
    const cl_device_id* deviceList,
    const char* options )
{
    m_OS.EnterCriticalSection();

    cl_int  errorCode = CL_SUCCESS;

    SSIMDSurveyProgram* pSIMDSurveyProgram =
        m_SIMDSurveyProgramMap[ program ];
    if( pSIMDSurveyProgram )
    {
        // Pre-pend the required subgroup size build option.  This assumes that
        // if the required subgroup size options string is already in the program
        // options string then the later option will have precedence.
        std::string userOptions( options ? options : "" );
        std::string simd8Options  = config().SIMDSurveySIMD8Option  + " " + userOptions;
        std::string simd16Options = config().SIMDSurveySIMD16Option + " " + userOptions;
        std::string simd32Options = config().SIMDSurveySIMD32Option + " " + userOptions;

        log( "SIMD Survey: Building SIMD8 kernel with options: " + simd8Options + "\n" );
        errorCode |= dispatch().clBuildProgram(
            pSIMDSurveyProgram->SIMD8Program,
            numDevices,
            deviceList,
            simd8Options.c_str(),
            NULL,
            NULL );
        log( "SIMD Survey: Building SIMD16 kernel with options: " + simd16Options + "\n" );
        errorCode |= dispatch().clBuildProgram(
            pSIMDSurveyProgram->SIMD16Program,
            numDevices,
            deviceList,
            simd16Options.c_str(),
            NULL,
            NULL );
        log( "SIMD Survey: Building SIMD32 kernel with options: " + simd16Options + "\n" );
        errorCode |= dispatch().clBuildProgram(
            pSIMDSurveyProgram->SIMD32Program,
            numDevices,
            deviceList,
            simd32Options.c_str(),
            NULL,
            NULL );
        if( errorCode != CL_SUCCESS )
        {
            log( "SIMD Survey: Building done (with errors).\n" );
        }
        else
        {
            log( "SIMD Survey: Building done.\n" );
        }
    }
    else
    {
        logf( "SIMD Survey: BuildProgram: Couldn't find info for program %p!?!?\n",
            program );
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::SIMDSurveyCreateKernel(
    const cl_program program,
    const cl_kernel kernel,
    const std::string& kernelName )
{
    m_OS.EnterCriticalSection();

    cl_int  errorCode = CL_SUCCESS;

    SSIMDSurveyKernel*  pSIMDSurveyKernel =
        m_SIMDSurveyKernelMap[ kernel ];
    if( pSIMDSurveyKernel )
    {
        // I don't think this should happen, assuming we've cleaned up
        // correctly when the kernel is released....
        CLI_ASSERT( 0 );

        // Remove the parent kernel and each of the child kernels from the map.
        m_SIMDSurveyKernelMap.erase( kernel );

        m_SIMDSurveyKernelMap.erase( pSIMDSurveyKernel->SIMD8Kernel );
        m_SIMDSurveyKernelMap.erase( pSIMDSurveyKernel->SIMD16Kernel );
        m_SIMDSurveyKernelMap.erase( pSIMDSurveyKernel->SIMD32Kernel );

        errorCode = dispatch().clReleaseKernel( pSIMDSurveyKernel->SIMD8Kernel );
        errorCode = dispatch().clReleaseKernel( pSIMDSurveyKernel->SIMD16Kernel );
        errorCode = dispatch().clReleaseKernel( pSIMDSurveyKernel->SIMD32Kernel );

        delete pSIMDSurveyKernel;
        pSIMDSurveyKernel = NULL;
    }

    SSIMDSurveyProgram* pSIMDSurveyProgram =
        m_SIMDSurveyProgramMap[ program ];
    if( pSIMDSurveyProgram )
    {
        pSIMDSurveyKernel = new SSIMDSurveyKernel;
        if( pSIMDSurveyKernel )
        {
            logf( "SIMD Survey: Creating kernels for %s\n",
                kernelName.c_str() );

            pSIMDSurveyKernel->SIMD8Kernel = dispatch().clCreateKernel(
                pSIMDSurveyProgram->SIMD8Program,
                kernelName.c_str(),
                &errorCode );
            pSIMDSurveyKernel->SIMD16Kernel = dispatch().clCreateKernel(
                pSIMDSurveyProgram->SIMD16Program,
                kernelName.c_str(),
                &errorCode );
            pSIMDSurveyKernel->SIMD32Kernel = dispatch().clCreateKernel(
                pSIMDSurveyProgram->SIMD32Program,
                kernelName.c_str(),
                &errorCode );

            pSIMDSurveyKernel->SIMD8ExecutionTimeNS = CL_ULONG_MAX;
            pSIMDSurveyKernel->SIMD16ExecutionTimeNS = CL_ULONG_MAX;
            pSIMDSurveyKernel->SIMD32ExecutionTimeNS = CL_ULONG_MAX;

            pSIMDSurveyKernel->ExecutionNumber = 0;

            // We'll install the same pointer into the map for the real
            // parent kernel and for each of the child kernels compiled
            // for specific SIMD sizes.  The parent kernel is used to
            // look up the kernel to execute, and the child kernels are
            // used to aggregate the results.

            m_SIMDSurveyKernelMap[ kernel ] = pSIMDSurveyKernel;

            m_SIMDSurveyKernelMap[ pSIMDSurveyKernel->SIMD8Kernel ] = pSIMDSurveyKernel;
            m_SIMDSurveyKernelMap[ pSIMDSurveyKernel->SIMD16Kernel ] = pSIMDSurveyKernel;
            m_SIMDSurveyKernelMap[ pSIMDSurveyKernel->SIMD32Kernel ] = pSIMDSurveyKernel;

            // Also, keep the kernel name map up-to-date.  This is necessary to
            // print the right kernel names in e.g. device timing reports.  The
            // other maps, such as the kernel arg map, don't need to know about
            // child kernels, so we don't add anything for them here.
            m_KernelNameMap[ pSIMDSurveyKernel->SIMD8Kernel ].kernelName = kernelName;
            m_KernelNameMap[ pSIMDSurveyKernel->SIMD16Kernel ].kernelName = kernelName;
            m_KernelNameMap[ pSIMDSurveyKernel->SIMD32Kernel ].kernelName = kernelName;
        }
    }
    else
    {
        logf( "SIMD Survey: CreateKernel: Couldn't find info for program %p!?!?\n",
            program );
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::SIMDSurveySetKernelArg(
    cl_kernel kernel,
    cl_uint argIndex,
    size_t argSize,
    const void* argValue )
{
    m_OS.EnterCriticalSection();

    SSIMDSurveyKernel*  pSIMDSurveyKernel =
        m_SIMDSurveyKernelMap[ kernel ];
    if( pSIMDSurveyKernel )
    {
        dispatch().clSetKernelArg(
            pSIMDSurveyKernel->SIMD8Kernel,
            argIndex,
            argSize,
            argValue );
        dispatch().clSetKernelArg(
            pSIMDSurveyKernel->SIMD16Kernel,
            argIndex,
            argSize,
            argValue );
        dispatch().clSetKernelArg(
            pSIMDSurveyKernel->SIMD32Kernel,
            argIndex,
            argSize,
            argValue );
    }
    else
    {
        logf( "SIMD Survey: SerKernelArg: Couldn't find info for kernel %p!?!?\n",
            kernel );
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::SIMDSurveyNDRangeKernel(
    cl_kernel& kernel )
{
    m_OS.EnterCriticalSection();

    SSIMDSurveyKernel*  pSIMDSurveyKernel =
        m_SIMDSurveyKernelMap[ kernel ];
    if( pSIMDSurveyKernel )
    {
        const std::string& kernelName = m_KernelNameMap[ kernel ].kernelName;

        const uint32_t  cWarmupIterations = config().SIMDSurveyWarmupIterations;
        if( pSIMDSurveyKernel->ExecutionNumber >= cWarmupIterations )
        {
            const uint32_t  cSample =
                pSIMDSurveyKernel->ExecutionNumber - cWarmupIterations;

            // This just tries the three kernels in order from
            // 8 -> 16 -> 32, one time each.
            //
            // Other things we can try:
            // - executing each kernel multiple times
            // - different orders
            switch( cSample )
            {
            case 0:
                if( pSIMDSurveyKernel->SIMD8Kernel )
                {
                    log( "SIMD Survey: NDRange: Sampling SIMD8 kernel for " + kernelName + "\n" );
                    kernel = pSIMDSurveyKernel->SIMD8Kernel;
                }
                else
                {
                    log( "SIMD Survey: NDRange: Skipping sample, no SIMD8 kernel exists for " + kernelName + ".\n" );
                }
                break;
            case 1:
                if( pSIMDSurveyKernel->SIMD16Kernel )
                {
                    log( "SIMD Survey: NDRange: Sampling SIMD16 kernel for " + kernelName + "\n" );
                    kernel = pSIMDSurveyKernel->SIMD16Kernel;
                }
                else
                {
                    log( "SIMD Survey: NDRange: Skipping sample, no SIMD16 kernel exists for " + kernelName + ".\n" );
                }
                break;
            case 2:
                if( pSIMDSurveyKernel->SIMD32Kernel )
                {
                    log( "SIMD Survey: NDRange: Sampling SIMD32 kernel for " + kernelName + "\n" );
                    kernel = pSIMDSurveyKernel->SIMD32Kernel;
                }
                else
                {
                    log( "SIMD Survey: NDRange: Skipping sample, no SIMD32 kernel exists for " + kernelName + ".\n" );
                }
                break;
            default:
                if( pSIMDSurveyKernel->SIMD8ExecutionTimeNS != CL_ULONG_MAX ||
                    pSIMDSurveyKernel->SIMD16ExecutionTimeNS != CL_ULONG_MAX ||
                    pSIMDSurveyKernel->SIMD32ExecutionTimeNS != CL_ULONG_MAX )
                {
                    cl_ulong    fastestTimeNS = CL_ULONG_MAX;
                    cl_uint     fastestSIMD = 0;
                    if( pSIMDSurveyKernel->SIMD8ExecutionTimeNS < fastestTimeNS )
                    {
                        fastestTimeNS = pSIMDSurveyKernel->SIMD8ExecutionTimeNS;
                        fastestSIMD = 8;
                        kernel = pSIMDSurveyKernel->SIMD8Kernel;
                    }
                    if( pSIMDSurveyKernel->SIMD16ExecutionTimeNS < fastestTimeNS )
                    {
                        fastestTimeNS = pSIMDSurveyKernel->SIMD16ExecutionTimeNS;
                        fastestSIMD = 16;
                        kernel = pSIMDSurveyKernel->SIMD16Kernel;
                    }
                    if( pSIMDSurveyKernel->SIMD32ExecutionTimeNS < fastestTimeNS )
                    {
                        fastestTimeNS = pSIMDSurveyKernel->SIMD32ExecutionTimeNS;
                        fastestSIMD = 32;
                        kernel = pSIMDSurveyKernel->SIMD32Kernel;
                    }

                    logf( "SIMD Survey: NDRange: Picking SIMD%u kernel for %s: SIMD8 Time = %u, SIMD16 Time = %u, SIMD32 Time = %u\n",
                        fastestSIMD,
                        kernelName.c_str(),
                        (cl_uint)pSIMDSurveyKernel->SIMD8ExecutionTimeNS,
                        (cl_uint)pSIMDSurveyKernel->SIMD16ExecutionTimeNS,
                        (cl_uint)pSIMDSurveyKernel->SIMD32ExecutionTimeNS );
                }
                else
                {
                    log( "SIMD Survey: NDRange: No samples for kernel " + kernelName + " (yet?)\n" );
                }
                break;
            }
        }
        else
        {
            logf( "SIMD Survey: NDRange: Executing warmup iteration %d of %d for kernel %s\n",
                pSIMDSurveyKernel->ExecutionNumber + 1,
                cWarmupIterations,
                kernelName.c_str() );
        }

        pSIMDSurveyKernel->ExecutionNumber++;
    }
    else
    {
        logf( "SIMD Survey NDRange: Couldn't find info for kernel %p!?!?\n",
            kernel );
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
#define CHECK_RETURN_EXTENSION_FUNCTION(funcname)                           \
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

///////////////////////////////////////////////////////////////////////////////
//
void* CLIntercept::getExtensionFunctionAddress(
    cl_platform_id platform,
    const std::string& func_name ) const
{
    // KHR Extensions

    // cl_khr_gl_sharing
    // Even though all of these functions except for clGetGLContextInfoKHR()
    // are exported from the ICD DLL, still call CHECK_RETURN_EXTENSION_FUNCTION
    // to handle the case where an intercepted DLL supports the extension but
    // does not export the entry point.  This will probably never happen in
    // practice, but better safe than sorry.
#if defined(_WIN32) || defined(__linux__)
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromGLBuffer );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromGLTexture );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromGLTexture2D );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromGLTexture3D );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromGLRenderbuffer );
    CHECK_RETURN_EXTENSION_FUNCTION( clGetGLObjectInfo );
    CHECK_RETURN_EXTENSION_FUNCTION( clGetGLTextureInfo );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueAcquireGLObjects );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueReleaseGLObjects );
#endif
    CHECK_RETURN_EXTENSION_FUNCTION( clGetGLContextInfoKHR );
    // cl_khr_gl_event
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateEventFromGLsyncKHR );
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
    // cl_khr_il_program
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateProgramWithILKHR );
    // cl_khr_subgroups
    CHECK_RETURN_EXTENSION_FUNCTION( clGetKernelSubGroupInfoKHR );
    // cl_khr_create_command_queue
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateCommandQueueWithPropertiesKHR );

    // Intel Extensions

#if defined(_WIN32)
    // cl_intel_dx9_media_sharing
    CHECK_RETURN_EXTENSION_FUNCTION( clGetDeviceIDsFromDX9INTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromDX9MediaSurfaceINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueAcquireDX9ObjectsINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueReleaseDX9ObjectsINTEL );
#endif

    // Unofficial MDAPI extension:
    CHECK_RETURN_EXTENSION_FUNCTION( clCreatePerfCountersCommandQueueINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clSetPerformanceConfigurationINTEL );

    // cl_intel_accelerator
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateAcceleratorINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clGetAcceleratorInfoINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clRetainAcceleratorINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clReleaseAcceleratorINTEL );

    // cl_intel_va_api_media_sharing
    CHECK_RETURN_EXTENSION_FUNCTION( clGetDeviceIDsFromVA_APIMediaAdapterINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clCreateFromVA_APIMediaSurfaceINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueAcquireVA_APIMediaSurfacesINTEL );
    CHECK_RETURN_EXTENSION_FUNCTION( clEnqueueReleaseVA_APIMediaSurfacesINTEL );

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
            m_InterceptLog.flush();
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

    char temp[ CLI_MAX_STRING_SIZE ] = "";
    int size = CLI_VSPRINTF( temp, CLI_MAX_STRING_SIZE, formatStr, args );
    if( size >= 0 && size < CLI_MAX_STRING_SIZE )
    {
        log( std::string( temp ) );
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
#if defined(_WIN32) || defined(__linux__)
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
bool CLIntercept::initDispatch( const std::string& dllName )
{
    bool success = true;

    if( success )
    {
        m_OpenCLLibraryHandle = OS().LoadLibrary( dllName.c_str() );
        if( m_OpenCLLibraryHandle == NULL )
        {
            log( std::string("Couldn't load library from: ") + dllName + "\n");
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

        // Optional features?
        INIT_EXPORTED_FUNC(clGetExtensionFunctionAddress);
        INIT_EXPORTED_FUNC(clGetExtensionFunctionAddressForPlatform);

        // OpenCL 1.1 Entry Points (optional)
        INIT_EXPORTED_FUNC(clCreateSubBuffer);
        INIT_EXPORTED_FUNC(clSetMemObjectDestructorCallback);
        INIT_EXPORTED_FUNC(clCreateUserEvent);
        INIT_EXPORTED_FUNC(clSetUserEventStatus);
        INIT_EXPORTED_FUNC(clSetEventCallback);
        INIT_EXPORTED_FUNC(clEnqueueReadBufferRect);
        INIT_EXPORTED_FUNC(clEnqueueWriteBufferRect);
        INIT_EXPORTED_FUNC(clEnqueueCopyBufferRect);

        // OpenCL 1.2 Entry Points (optional)
        INIT_EXPORTED_FUNC(clCompileProgram);
        INIT_EXPORTED_FUNC(clCreateFromGLTexture);
        INIT_EXPORTED_FUNC(clCreateImage);
        INIT_EXPORTED_FUNC(clCreateProgramWithBuiltInKernels);
        INIT_EXPORTED_FUNC(clCreateSubDevices);
        INIT_EXPORTED_FUNC(clEnqueueBarrierWithWaitList);
        INIT_EXPORTED_FUNC(clEnqueueFillBuffer);
        INIT_EXPORTED_FUNC(clEnqueueFillImage);
        INIT_EXPORTED_FUNC(clEnqueueMarkerWithWaitList);
        INIT_EXPORTED_FUNC(clEnqueueMigrateMemObjects);
        INIT_EXPORTED_FUNC(clGetKernelArgInfo);
        INIT_EXPORTED_FUNC(clLinkProgram);
        INIT_EXPORTED_FUNC(clReleaseDevice);
        INIT_EXPORTED_FUNC(clRetainDevice);
        INIT_EXPORTED_FUNC(clUnloadPlatformCompiler);

        // OpenCL 2.0 Entry Points (optional)
        INIT_EXPORTED_FUNC(clSVMAlloc);
        INIT_EXPORTED_FUNC(clSVMFree);
        INIT_EXPORTED_FUNC(clEnqueueSVMFree);
        INIT_EXPORTED_FUNC(clEnqueueSVMMemcpy);
        INIT_EXPORTED_FUNC(clEnqueueSVMMemFill);
        INIT_EXPORTED_FUNC(clEnqueueSVMMap);
        INIT_EXPORTED_FUNC(clEnqueueSVMUnmap);
        INIT_EXPORTED_FUNC(clSetKernelArgSVMPointer);
        INIT_EXPORTED_FUNC(clSetKernelExecInfo);
        INIT_EXPORTED_FUNC(clCreatePipe);
        INIT_EXPORTED_FUNC(clGetPipeInfo);
        INIT_EXPORTED_FUNC(clCreateCommandQueueWithProperties);
        INIT_EXPORTED_FUNC(clCreateSamplerWithProperties);

        // OpenCL 2.1 Entry Points (optional)
        INIT_EXPORTED_FUNC(clSetDefaultDeviceCommandQueue);
        INIT_EXPORTED_FUNC(clGetDeviceAndHostTimer);
        INIT_EXPORTED_FUNC(clGetHostTimer);
        INIT_EXPORTED_FUNC(clCreateProgramWithIL);
        INIT_EXPORTED_FUNC(clCloneKernel);
        INIT_EXPORTED_FUNC(clGetKernelSubGroupInfo);
        INIT_EXPORTED_FUNC(clEnqueueSVMMigrateMem);

        // OpenCL 2.2 Entry Points (optional)
        INIT_EXPORTED_FUNC(clSetProgramReleaseCallback);
        INIT_EXPORTED_FUNC(clSetProgramSpecializationConstant);

        // CL-GL Entry Points (optional)
        INIT_EXPORTED_FUNC(clCreateFromGLBuffer);
        INIT_EXPORTED_FUNC(clCreateFromGLTexture);
        INIT_EXPORTED_FUNC(clCreateFromGLTexture2D);
        INIT_EXPORTED_FUNC(clCreateFromGLTexture3D);
        INIT_EXPORTED_FUNC(clCreateFromGLRenderbuffer);
        INIT_EXPORTED_FUNC(clGetGLObjectInfo);
        INIT_EXPORTED_FUNC(clGetGLTextureInfo );
        INIT_EXPORTED_FUNC(clEnqueueAcquireGLObjects);
        INIT_EXPORTED_FUNC(clEnqueueReleaseGLObjects);

        // Extensions (optional)
        // Extensions get loaded into the dispatch table on the fly.

        success = savedSuccess;
    }

    if( !success )
    {
        if( m_OpenCLLibraryHandle != NULL )
        {
            OS().UnloadLibrary( m_OpenCLLibraryHandle );
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

    // Optional features?
    INIT_CL_FUNC(clGetExtensionFunctionAddress);
    INIT_CL_FUNC(clGetExtensionFunctionAddressForPlatform);

    // OpenCL 1.1 Entry Points (optional)
    INIT_CL_FUNC(clCreateSubBuffer);
    INIT_CL_FUNC(clSetMemObjectDestructorCallback);
    INIT_CL_FUNC(clCreateUserEvent);
    INIT_CL_FUNC(clSetUserEventStatus);
    INIT_CL_FUNC(clSetEventCallback);
    INIT_CL_FUNC(clEnqueueReadBufferRect);
    INIT_CL_FUNC(clEnqueueWriteBufferRect);
    INIT_CL_FUNC(clEnqueueCopyBufferRect);

    // OpenCL 1.2 Entry Points (optional)
    INIT_CL_FUNC(clCompileProgram);
    INIT_CL_FUNC(clCreateFromGLTexture);
    INIT_CL_FUNC(clCreateImage);
    INIT_CL_FUNC(clCreateProgramWithBuiltInKernels);
    INIT_CL_FUNC(clCreateSubDevices);
    INIT_CL_FUNC(clEnqueueBarrierWithWaitList);
    INIT_CL_FUNC(clEnqueueFillBuffer);
    INIT_CL_FUNC(clEnqueueFillImage);
    INIT_CL_FUNC(clEnqueueMarkerWithWaitList);
    INIT_CL_FUNC(clEnqueueMigrateMemObjects);
    INIT_CL_FUNC(clGetKernelArgInfo);
    INIT_CL_FUNC(clLinkProgram);
    INIT_CL_FUNC(clReleaseDevice);
    INIT_CL_FUNC(clRetainDevice);
    INIT_CL_FUNC(clUnloadPlatformCompiler);

    // CL-GL Entry Points (optional)
    INIT_CL_FUNC(clCreateFromGLBuffer);
    INIT_CL_FUNC(clCreateFromGLTexture);   // OpenCL 1.2
    INIT_CL_FUNC(clCreateFromGLTexture2D);
    INIT_CL_FUNC(clCreateFromGLTexture3D);
    INIT_CL_FUNC(clCreateFromGLRenderbuffer);
    INIT_CL_FUNC(clGetGLObjectInfo);
    INIT_CL_FUNC(clGetGLTextureInfo);
    INIT_CL_FUNC(clEnqueueAcquireGLObjects);
    INIT_CL_FUNC(clEnqueueReleaseGLObjects);

    // Extensions (optional)
    // Extensions get loaded into the dispatch table on the fly.

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
        m_OS.EnterCriticalSection();

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

        m_OS.LeaveCriticalSection();
    }
}

void CLIntercept::ittCallLoggingEnter(
    const std::string& functionName,
    const cl_kernel kernel )
{
    std::string str( functionName );
    if( kernel )
    {
        const std::string& kernelName = getKernelName(kernel);
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
    m_OS.EnterCriticalSection();

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

    SITTQueueInfo*  pITTQueueInfo = NULL;
    if( errorCode == CL_SUCCESS )
    {
        pITTQueueInfo = new SITTQueueInfo;
        if( pITTQueueInfo == NULL )
        {
            errorCode = CL_OUT_OF_HOST_MEMORY;
        }
        else
        {
            pITTQueueInfo->pIntercept = this;
            pITTQueueInfo->SupportsPerfCounters = supportsPerfCounters;

            pITTQueueInfo->itt_track = NULL;
            pITTQueueInfo->itt_clock_domain = NULL;
            pITTQueueInfo->CPUReferenceTime = 0;
            pITTQueueInfo->CLReferenceTime = 0;
        }
    }

    if( errorCode == CL_SUCCESS )
    {
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
            trackName += " ACCELERATOR";
        }
        if( deviceType & CL_DEVICE_TYPE_CUSTOM )
        {
            trackName += " CUSTOM";
        }

        trackName += " Queue, ";

        {
            char str[CLI_MAX_STRING_SIZE] = "";
            CLI_SPRINTF( str, CLI_MAX_STRING_SIZE, "Handle = %p", queue );
            trackName = trackName + str;
        }

        // Don't fail if the track cannot be created, it just means we
        // won't be as detailed in our tracking.
        //__itt_track* track = __itt_track_create(
        //    m_ITTQueueTrackGroup,
        //    __itt_string_handle_create(trackName.c_str()),
        //    __itt_track_type_queue );
        //if( track != NULL )
        //{
        //    pITTQueueInfo->itt_track = track;
        //
        //    __itt_set_track(track);
        //
        //    __ittx_set_default_state(
        //        m_ITTDomain,
        //        m_ITTQueuedState );
        //
        //    __itt_set_track(NULL);
        //}

        dispatch().clRetainCommandQueue( queue );

        m_ITTQueueInfoMap[ queue ] = pITTQueueInfo;
    }

    if( errorCode != CL_SUCCESS )
    {
        delete pITTQueueInfo;
        pITTQueueInfo = NULL;
    }

    m_OS.LeaveCriticalSection();
}

void CLIntercept::ittReleaseCommandQueue(
    cl_command_queue queue )
{
    m_OS.EnterCriticalSection();

    cl_int  errorCode = CL_SUCCESS;
    cl_uint refCount = 0;

    SITTQueueInfo*  pITTQueueInfo = m_ITTQueueInfoMap[ queue ];
    if( pITTQueueInfo )
    {
        if( errorCode == CL_SUCCESS )
        {
            errorCode = dispatch().clGetCommandQueueInfo(
                queue,
                CL_QUEUE_REFERENCE_COUNT,
                sizeof( refCount ),
                &refCount,
                NULL );
        }

        if( ( errorCode == CL_SUCCESS ) &&
            ( refCount == 1 ) )
        {
            dispatch().clReleaseCommandQueue( queue );

            // I guess we don't delete a track after we've created it?
            // Or a clock domain?

            delete pITTQueueInfo;
            pITTQueueInfo = NULL;

            m_ITTQueueInfoMap.erase( queue );
        }
    }

    m_OS.LeaveCriticalSection();
}

void ITTAPI CLIntercept::ittClockInfoCallback(
    __itt_clock_info* pClockInfo,
    void* pData )
{
    const SITTQueueInfo*    pQueueInfo = (const SITTQueueInfo*)pData;

    uint64_t    cpuTickDelta =
        pQueueInfo->pIntercept->OS().GetTimer() -
        pQueueInfo->CPUReferenceTime;

    uint64_t    cpuDeltaNS = pQueueInfo->pIntercept->OS().TickToNS( cpuTickDelta );

    pClockInfo->clock_base = pQueueInfo->CLReferenceTime + cpuDeltaNS;
    pClockInfo->clock_freq = 1000000000;    // NS
}

void CLIntercept::ittTraceEvent(
    const std::string& name,
    cl_event event,
    uint64_t queuedTime )
{
    cl_int  errorCode = CL_SUCCESS;

    cl_command_queue    queue = NULL;
    cl_command_type     type = 0;

    cl_ulong    commandQueued = 0;
    cl_ulong    commandSubmit = 0;
    cl_ulong    commandStart = 0;
    cl_ulong    commandEnd = 0;

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

    errorCode |= dispatch().clGetEventProfilingInfo(
        event,
        CL_PROFILING_COMMAND_QUEUED,
        sizeof( commandQueued ),
        &commandQueued,
        NULL );
    errorCode |= dispatch().clGetEventProfilingInfo(
        event,
        CL_PROFILING_COMMAND_SUBMIT,
        sizeof( commandSubmit ),
        &commandSubmit,
        NULL );
    errorCode |= dispatch().clGetEventProfilingInfo(
        event,
        CL_PROFILING_COMMAND_START,
        sizeof( commandStart ),
        &commandStart,
        NULL );
    errorCode |= dispatch().clGetEventProfilingInfo(
        event,
        CL_PROFILING_COMMAND_END,
        sizeof( commandEnd ),
        &commandEnd,
        NULL );

    if( errorCode == CL_SUCCESS )
    {
        // It's possible we don't have any ITT info for this queue.
        SITTQueueInfo*  pITTQueueInfo = m_ITTQueueInfoMap[ queue ];
        if( pITTQueueInfo != NULL )
        {
            __itt_clock_domain* clockDomain = pITTQueueInfo->itt_clock_domain;
            if( clockDomain == NULL )
            {
                pITTQueueInfo->CPUReferenceTime = queuedTime;
                pITTQueueInfo->CLReferenceTime = commandQueued;

                clockDomain = __itt_clock_domain_create(
                    ittClockInfoCallback,
                    pITTQueueInfo );
                if( clockDomain == NULL )
                {
                    log( "__itt_clock_domain_create() returned NULL!\n");
                }

                pITTQueueInfo->itt_clock_domain = clockDomain;
            }

            __itt_track*    track = pITTQueueInfo->itt_track;
            uint64_t        clockOffset = 0;

            if( commandQueued == 0 )
            {
                clockOffset = queuedTime;
                clockOffset -= pITTQueueInfo->CPUReferenceTime;
                clockOffset = OS().TickToNS( clockOffset );
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

            if( pITTQueueInfo->SupportsPerfCounters )
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
    const std::string& functionName,
    const cl_kernel kernel,
    uint64_t tickStart,
    uint64_t tickEnd )
{
    std::string str;
    str += functionName;

    if( kernel )
    {
        const std::string& kernelName = getKernelName(kernel);
        str += "( ";
        str += kernelName;
        str += " )";
    }

    uint64_t    processId =
        OS().GetProcessID();
    uint64_t    threadId =
        OS().GetThreadID();

    uint64_t    usStart =
        OS().TickToNS( tickStart - m_StartTime ) / 1000;
    uint64_t    usDelta =
        OS().TickToNS( tickEnd - tickStart ) / 1000;

    m_InterceptTrace
        << "{\"ph\":\"X\", \"pid\":" << processId
        << ", \"tid\":" << threadId
        << ", \"name\":\"" << str
        << "\", \"ts\":" << usStart
        << ", \"dur\":" << usDelta
        << "},\n";
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::chromeRegisterCommandQueue(
    cl_command_queue queue )
{
    m_OS.EnterCriticalSection();

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
            trackName += " ACCELERATOR";
        }
        if( deviceType & CL_DEVICE_TYPE_CUSTOM )
        {
            trackName += " CUSTOM";
        }

        trackName += " Queue";

        //{
        //    char str[CLI_MAX_STRING_SIZE] = "";
        //    CLI_SPRINTF( str, CLI_MAX_STRING_SIZE, ", Handle = %p", queue );
        //    trackName = trackName + str;
        //}

        uint64_t    processId = OS().GetProcessID();
        m_InterceptTrace
            << "{\"ph\":\"M\", \"name\":\"thread_name\", \"pid\":" << processId
            << ", \"tid\":-" << (uintptr_t)queue
            << ", \"args\":{\"name\":\"" << trackName
            << "\"}},\n";
    }

    m_OS.LeaveCriticalSection();
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::chromeTraceEvent(
    const std::string& name,
    cl_event event,
    uint64_t queuedTime )
{
    cl_int  errorCode = CL_SUCCESS;

    cl_command_queue    queue = NULL;
    cl_command_type     type = 0;

    cl_ulong    commandQueued = 0;
    cl_ulong    commandSubmit = 0;
    cl_ulong    commandStart = 0;
    cl_ulong    commandEnd = 0;

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

    errorCode |= dispatch().clGetEventProfilingInfo(
        event,
        CL_PROFILING_COMMAND_QUEUED,
        sizeof( commandQueued ),
        &commandQueued,
        NULL );
    errorCode |= dispatch().clGetEventProfilingInfo(
        event,
        CL_PROFILING_COMMAND_SUBMIT,
        sizeof( commandSubmit ),
        &commandSubmit,
        NULL );
    errorCode |= dispatch().clGetEventProfilingInfo(
        event,
        CL_PROFILING_COMMAND_START,
        sizeof( commandStart ),
        &commandStart,
        NULL );
    errorCode |= dispatch().clGetEventProfilingInfo(
        event,
        CL_PROFILING_COMMAND_END,
        sizeof( commandEnd ),
        &commandEnd,
        NULL );

    if( errorCode == CL_SUCCESS )
    {
        uint64_t    normalizedQueuedTimeNS =
            OS().TickToNS( queuedTime - m_StartTime );
        uint64_t    normalizedStartTimeNS =
            ( commandStart - commandQueued ) + normalizedQueuedTimeNS;

        uint64_t    usStart = normalizedStartTimeNS / 1000;
        uint64_t    usDelta = ( commandEnd - commandStart ) / 1000;

        uint64_t    processId = OS().GetProcessID();
        m_InterceptTrace
            << "{\"ph\":\"X\", \"pid\":" << processId
            << ", \"tid\":-" << (uintptr_t)queue
            << ", \"name\":\"" << name
            << "\", \"ts\":" << usStart
            << ", \"dur\":" << usDelta
            << "},\n";
    }
    else
    {
        log( "chromeTraceEvent(): OpenCL error\n" );
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
    m_OS.EnterCriticalSection();

    bool    match = true;

    // If the aubcapture kernel name is set, make sure it matches the name
    // of the passed-in kernel:

    if( match &&
        m_Config.AubCaptureKernelName != "" &&
        m_KernelNameMap[ kernel ].kernelName != m_Config.AubCaptureKernelName )
    {
        //logf( "Skipping aub capture: kernel name '%s' doesn't match the requested kernel name '%s'.\n",
        //    m_KernelNameMap[ kernel ].kernelName.c_str(),
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
        std::string& key = m_KernelNameMap[ kernel ].kernelName;
        {
            cl_program program = NULL;
            dispatch().clGetKernelInfo(
                kernel,
                CL_KERNEL_PROGRAM,
                sizeof(program),
                &program,
                NULL );
            if( program )
            {
                unsigned int    programNumber = m_ProgramNumberMap[ program ];
                uint64_t        programHash = m_ProgramHashMap[ program ];
                unsigned int    compileCount = m_ProgramNumberCompileCountMap[ programNumber ];

                char    hashString[256] = "";
                if( config().OmitProgramNumber )
                {
                    CLI_SPRINTF( hashString, 256, "(%08X_%04u)",
                        (unsigned int)programHash,
                        compileCount );
                }
                else
                {
                    CLI_SPRINTF( hashString, 256, "(%04u_%08X_%04u)",
                        programNumber,
                        (unsigned int)programHash,
                        compileCount );
                }
                key += hashString;
            }
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

    m_OS.LeaveCriticalSection();

    return match;
}
