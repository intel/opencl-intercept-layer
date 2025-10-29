/*
// Copyright (c) 2018-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/
#pragma once

#include <sys/stat.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <libproc.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>

void CLIntercept_Load(void);

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

    bool    Init();

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

inline bool Services::Init()
{
    return true;
}

inline uint64_t Services::GetProcessID() const
{
    return getpid();
}

inline uint64_t Services::GetThreadID() const
{
    uint64_t tid = 0;
    pthread_threadid_np(NULL, &tid);
    return tid;
}

inline std::string Services::GetProcessName() const
{
    char    processName[ 1024 ];
    char*   pProcessName = processName;

    pid_t   pid = getpid();
    int     ret = proc_pidpath( pid, processName, sizeof(processName) );
    if( ret > 0 )
    {
        pProcessName = strrchr( processName, '/' );
    }
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
    void* pLibrary = dlopen( libraryName.c_str(), RTLD_NOW );
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
    // Get the home directory and add our directory name.
    {
        directoryName = getenv("HOME");
        directoryName += "/";
        directoryName += subDir;
        directoryName += "/";
    }
    // Add the process name to the directory name.
    {
        char    processName[ 1024 ];
        char*   pProcessName = processName;

        pid_t   pid = getpid();
        int     ret = proc_pidpath( pid, processName, sizeof(processName) );
        if( ret > 0 )
        {
            pProcessName = strrchr( processName, '/' );
        }
        else
        {
            strncpy( processName, "process.exe", sizeof( processName ) );
            processName[ sizeof( processName ) - 1 ] = 0;
        }

        directoryName += pProcessName;
    }
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
    if( dladdr( (void*)CLIntercept_Load, &info ) )
    {
        name = info.dli_fname;
    }
    return false;
}

// TODO: We currently don't support any of the kernels overrides on OSX.

inline bool Services::GetPrecompiledKernelString(
    const char*& str,
    size_t& length ) const
{
    return false;
}

inline bool Services::GetBuiltinKernelString(
    const char*& str,
    size_t& length ) const
{
    return false;
}

inline bool Services::GetReplayScriptString(
    const char*& str,
    size_t& length ) const
{
    return false;
}

// TODO

inline bool Services::ExecuteCommand( const std::string& command ) const
{
    return false;
}

inline bool Services::StartAubCapture(
    const std::string& fileName,
    uint64_t delay ) const
{
    return false;
}

inline bool Services::StopAubCapture(
    uint64_t delay ) const
{
    return false;
}

inline bool Services::CheckMDAPIPermissions(
    std::string& str ) const
{
    return true;
}

}
