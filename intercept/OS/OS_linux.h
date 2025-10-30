/*
// Copyright (c) 2018-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#include <sys/stat.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#ifdef __FreeBSD__
#include <pthread_np.h>
#include <sys/user.h>
#include <libutil.h>
#endif

#if defined(__linux__)
// Workaround for older glibc versions that do not have gettid:
#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
    #include <sys/syscall.h>
    #define gettid() syscall(SYS_gettid)
#endif
#endif

// Since we just need the symbol clGetPlatformIDs, we can use any OpenCL
// version.
#if !defined(CL_TARGET_OPENCL_VERSION)
#define CL_TARGET_OPENCL_VERSION 100
#endif
#include "CL/cl.h"

namespace OS
{

class Services
{
public:
    static const char* ENV_PREFIX;
    static const char* CONFIG_FILE;
    static const char* SYSTEM_DIR;
    static const char* LOG_DIR;
    static bool        APPEND_PID;

    Services( void* pGlobalData );
    Services( const Services& ) = delete;
    Services& operator=( const Services& ) = delete;

    uint64_t    GetProcessID() const;
    uint64_t    GetThreadID() const;

    std::string GetProcessName() const;

    bool    GetControl(
                const std::string& name,
                void* pValue,
                size_t size ) const;

    void    OutputDebugString(
                const std::string& str ) const;

    void*   LoadLibrary(
                const std::string& libraryName ) const;
    void    UnloadLibrary(
                void*& pLibrary ) const;

    void*   GetFunctionPointer(
                void* pLibrary,
                const std::string& functionName ) const;

    void    GetDumpDirectoryName(
                const std::string& subDir,
                std::string& directoryName ) const;
    void    GetDumpDirectoryNameWithoutPid(
                const std::string& subDir,
                std::string& directoryName ) const;
    void    MakeDumpDirectories(
                const std::string& fileName ) const;

    bool    GetCLInterceptName(
                std::string& name ) const;

    bool    GetPrecompiledKernelString(
                const char*& str,
                size_t& length ) const;

    bool    GetBuiltinKernelString(
                const char*& str,
                size_t& length ) const;

    bool    GetReplayScriptString(
                const char*& str,
                size_t& length ) const;

    bool    ExecuteCommand(
                const std::string& fileName ) const;
    bool    StartAubCapture(
                const std::string& fileName,
                uint64_t delay ) const;
    bool    StopAubCapture(
                uint64_t delay ) const;

    bool    CheckMDAPIPermissions(
                std::string& str ) const;

private:
    bool    GetControlFromFile(
                const std::string& fileName,
                const std::string& controlName,
                void* pValue,
                size_t size ) const;
};

inline uint64_t Services::GetProcessID() const
{
    return getpid();
}

inline uint64_t Services::GetThreadID() const
{
#if defined(__linux__)
    return gettid();
#elif defined(__FreeBSD__)
    return pthread_getthreadid_np();
#else
#pragma message("Not sure how to implement GetThreadID()")
    return 0;
#endif
}

inline std::string Services::GetProcessName() const
{
    char    processName[ 1024 ] = "";
    char*   pProcessName = processName;

#if defined(__linux__)
    size_t  bytes = readlink(
        "/proc/self/exe",
        processName,
        sizeof( processName ) - 1 );
    if( bytes )
    {
        processName[ bytes ] = '\0';

        pProcessName = strrchr( processName, '/' );
        pProcessName++;
    }
#elif defined(__FreeBSD__)
    struct kinfo_proc* proc = kinfo_getproc(getpid());
    if( proc )
    {
        strncpy( processName, proc->ki_comm, sizeof( processName ) );
        processName[ sizeof( processName ) - 1 ] = '\0';

        free(proc);
    }
#else
#pragma message("Not sure how to implement GetProcessName()")
    if (false) {}
#endif
    else
    {
        strncpy( processName, "process.exe", sizeof( processName ) );
        processName[ sizeof( processName ) - 1 ] = 0;
    }

    return std::string(pProcessName);
}

inline void Services::OutputDebugString(
    const std::string& str ) const
{
    syslog( LOG_USER | LOG_INFO, "%s", str.c_str() );
}

inline void* Services::LoadLibrary(
    const std::string& libraryName ) const
{
    void* pLibrary = dlopen( libraryName.c_str(), RTLD_NOW | RTLD_GLOBAL );
    //if( pLibrary == NULL )
    //{
    //    fprintf(stderr, "dlopen() error: %s\n", dlerror());
    //}
    return pLibrary;
}

inline void Services::UnloadLibrary(
    void*& pLibrary ) const
{
    dlclose( pLibrary );
    pLibrary = NULL;
}

inline void* Services::GetFunctionPointer(
    void* pLibrary,
    const std::string& functionName ) const
{
    if( pLibrary )
    {
        return dlsym( pLibrary, functionName.c_str() );
    }
    else
    {
        return dlsym( RTLD_NEXT, functionName.c_str() );
    }
}

inline void Services::GetDumpDirectoryNameWithoutPid(
    const std::string& subDir,
    std::string& directoryName ) const
{
    if( LOG_DIR )
    {
        // Return log dir override if set in regkeys
        directoryName = LOG_DIR;
    }
    else
    {
        // Get the home directory and add our directory name.
        const char *envVal = getenv("HOME");
        char* resolved_path = realpath(envVal, nullptr);
        if( resolved_path )
        {
            directoryName = resolved_path;
            free(resolved_path);
            resolved_path = nullptr;
        }
        else
        {
#ifdef __ANDROID__
            directoryName = "/sdcard/Intel";
#else
            directoryName = "/tmp/Intel";
#endif
        }
        directoryName += "/";
        directoryName += subDir;
        directoryName += "/";

        // Add the process name to the directory name.
        directoryName += GetProcessName();
    }

#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "clIntercept", "dumpDir=%s\n", directoryName.c_str());
#endif
}

inline void Services::GetDumpDirectoryName(
    const std::string& subDir,
    std::string& directoryName ) const
{
    GetDumpDirectoryNameWithoutPid(subDir, directoryName);
    if( APPEND_PID )
    {
        directoryName += ".";
        directoryName += std::to_string(GetProcessID());
    }
}

inline void Services::MakeDumpDirectories(
    const std::string& fileName ) const
{
    // The first directory name is the root.  We don't
    // have to make a directory for it.
    std::string::size_type  pos = fileName.find( "/" );

    pos = fileName.find( "/", ++pos );
    while( pos != std::string::npos )
    {
        mkdir(
            fileName.substr( 0, pos ).c_str(),
            0777 );

        pos = fileName.find( "/", ++pos );
    }
}

inline bool Services::GetCLInterceptName(
    std::string& name ) const
{
    Dl_info info;
    if( dladdr( (void*)clGetPlatformIDs, &info ) )
    {
        name = info.dli_fname;
    }
    return false;
}

#if defined(USE_KERNEL_OVERRIDES)
extern "C" char _binary_kernels_precompiled_kernels_cl_start;
extern "C" char _binary_kernels_precompiled_kernels_cl_end;
#endif

inline bool Services::GetPrecompiledKernelString(
    const char*& str,
    size_t& length ) const
{
#if defined(USE_KERNEL_OVERRIDES)
    str = &_binary_kernels_precompiled_kernels_cl_start;
    length = &_binary_kernels_precompiled_kernels_cl_end - &_binary_kernels_precompiled_kernels_cl_start;
    return true;
#else
    return false;
#endif
}

#if defined(USE_KERNEL_OVERRIDES)
extern "C" char _binary_kernels_builtin_kernels_cl_start;
extern "C" char _binary_kernels_builtin_kernels_cl_end;
#endif

inline bool Services::GetBuiltinKernelString(
    const char*& str,
    size_t& length ) const
{
#if defined(USE_KERNEL_OVERRIDES)
    str = &_binary_kernels_builtin_kernels_cl_start;
    length = &_binary_kernels_builtin_kernels_cl_end - &_binary_kernels_builtin_kernels_cl_start;
    return true;
#else
    return false;
#endif
}

#if defined(USE_SCRIPTS)
extern "C" char _binary_scripts_run_py_start;
extern "C" char _binary_scripts_run_py_end;
#endif

inline bool Services::GetReplayScriptString(
    const char*& str,
    size_t& length ) const
{
#if defined(USE_SCRIPTS)
    str = &_binary_scripts_run_py_start;
    length = &_binary_scripts_run_py_end - &_binary_scripts_run_py_start;
    return true;
#else
    return false;
#endif
}

inline bool Services::ExecuteCommand( const std::string& command ) const
{
    int res = system( command.c_str() );
    return res != -1;
}

static inline bool SetAubCaptureEnvironmentVariables(
    const std::string& fileName,
    bool start )
{
    // For NEO AubCapture:
    // As setup, need to set AUBDumpSubCaptureMode = 2.
    // This will be the client's responsibility.
    //
    // To start/stop AubCapture:
    //  set AUBDumpToggleCaptureOnOff = 1/0
    //  set AUBDumpToggleFileName appropriately
    // This is CLIntercept's responsibility.

    const char* const AUB_CAPTURE_TOGGLE_ENV_VAR = "AUBDumpToggleCaptureOnOff";
    const char* const AUB_CAPTURE_FILE_NAME_ENV_VAR = "AUBDumpToggleFileName";

    int status = 0;

    if( start )
    {
        status = setenv( AUB_CAPTURE_TOGGLE_ENV_VAR, "1", 1 );
    }
    else
    {
        status = setenv( AUB_CAPTURE_TOGGLE_ENV_VAR, "0", 1 );
    }
    if( status == 0 )
    {
        if( fileName.empty() )
        {
            status = unsetenv( AUB_CAPTURE_FILE_NAME_ENV_VAR );
        }
        else
        {
            status = setenv( AUB_CAPTURE_FILE_NAME_ENV_VAR, fileName.c_str(), 1 );
        }
    }

    return status == 0;
}

inline bool Services::StartAubCapture(
    const std::string& fileName,
    uint64_t delay ) const
{
    if( delay )
    {
        usleep( delay );
    }

    return SetAubCaptureEnvironmentVariables( fileName, true );
}

inline bool Services::StopAubCapture(
    uint64_t delay ) const
{
    if( delay )
    {
        usleep( delay );
    }

    return SetAubCaptureEnvironmentVariables( "", false );
}

inline bool Services::CheckMDAPIPermissions(
    std::string& str ) const
{
    const char* i915_path = "/proc/sys/dev/i915/perf_stream_paranoid";
    const char* xe_path = "/proc/sys/dev/xe/observation_paranoid";

    str.clear();

    const auto readValueFromFile = [](const char* path) -> uint64_t {
        uint64_t value = 1;
        int fd = open(path, 0);
        if( fd > 0 )
        {
            char buf[32] = "";
            int n = read(fd, buf, sizeof(buf) - 1);
            close(fd);

            if( n > 0 )
            {
                buf[n] = 0;
                value = strtoull(buf, NULL, 0);
            }
        }
        return value;
    };

    if( geteuid() != 0 )
    {
        uint64_t i915_value = readValueFromFile(i915_path);
        if (i915_value != 0)
        {
            str += "Warning: possibly insufficient permissions for MDAPI!"
                "  Consider: sysctl dev.i915.perf_stream_paranoid=0\n";
        }

        uint64_t xe_value = readValueFromFile(xe_path);
        if (xe_value != 0)
        {
            str += "Warning: possibly insufficient permissions for MDAPI!"
                "  Consider: sysctl dev.xe.observation_paranoid=0\n";
        }
    }

    return str.empty();
}

}
