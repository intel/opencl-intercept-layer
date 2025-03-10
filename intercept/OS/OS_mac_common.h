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

/*****************************************************************************\

MACRO:
    DISALLOW_COPY_AND_ASSIGN

Description:
    A macro to disallow the copy constructor and operator= functions
    This should be used in the private: declarations for a class

\*****************************************************************************/
#if !defined(DISALLOW_COPY_AND_ASSIGN)
#define DISALLOW_COPY_AND_ASSIGN( TypeName ) \
    TypeName(const TypeName&); \
    void operator=(const TypeName&)
#endif

namespace OS
{

class Services_Common
{
public:
    static const char* ENV_PREFIX;
    static const char* CONFIG_FILE;
    static const char* SYSTEM_DIR;
    static const char* LOG_DIR;
    static bool        APPEND_PID;

    Services_Common();
    ~Services_Common();

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

private:
    bool    GetControlFromFile(
                const std::string& fileName,
                const std::string& controlName,
                void* pValue,
                size_t size ) const;

    DISALLOW_COPY_AND_ASSIGN( Services_Common );
};

inline bool Services_Common::Init()
{
    return true;
}

inline uint64_t Services_Common::GetProcessID() const
{
    return getpid();
}

inline uint64_t Services_Common::GetThreadID() const
{
    uint64_t tid = 0;
    pthread_threadid_np(NULL, &tid);
    return tid;
}

inline std::string Services_Common::GetProcessName() const
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

inline void Services_Common::OutputDebugString(
    const std::string& str ) const
{
    syslog( LOG_USER | LOG_INFO, "%s", str.c_str() );
}

inline void* Services_Common::LoadLibrary(
    const std::string& libraryName ) const
{
    void* pLibrary = dlopen( libraryName.c_str(), RTLD_NOW );
    return pLibrary;
}

inline void Services_Common::UnloadLibrary(
    void*& pLibrary ) const
{
    dlclose( pLibrary );
    pLibrary = NULL;
}

inline void* Services_Common::GetFunctionPointer(
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

inline void Services_Common::GetDumpDirectoryNameWithoutPid(
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

inline void Services_Common::GetDumpDirectoryName(
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

inline void Services_Common::MakeDumpDirectories(
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

}
