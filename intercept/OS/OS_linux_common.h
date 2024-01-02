/*
// Copyright (c) 2018-2024 Intel Corporation
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
    void    GetDumpDirectoryNameWithoutProcessName(
                const std::string& subDir,
                std::string& directoryName) const;
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
#if defined(__linux__)
    return gettid();
#elif defined(__FreeBSD__)
    return pthread_getthreadid_np();
#else
#pragma message("Not sure how to implement GetThreadID()")
    return 0;
#endif
}

inline std::string Services_Common::GetProcessName() const
{
    char    processName[ 1024 ];
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

inline void Services_Common::OutputDebugString(
    const std::string& str ) const
{
    syslog( LOG_USER | LOG_INFO, "%s", str.c_str() );
}

inline void* Services_Common::LoadLibrary(
    const std::string& libraryName ) const
{
    void* pLibrary = dlopen( libraryName.c_str(), RTLD_NOW | RTLD_GLOBAL );
    //if( pLibrary == NULL )
    //{
    //    fprintf(stderr, "dlopen() error: %s\n", dlerror());
    //}
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
    if( LOG_DIR )
    {
        // Return log dir override if set in regkeys
        directoryName = LOG_DIR;
    }
    else
    {
        {
#ifndef __ANDROID__
            directoryName = getenv("HOME");
#else
            const char *envVal = getenv("HOME");
            if( envVal == NULL )
            {
                directoryName = "/sdcard/Intel";
            }
            else
            {
                directoryName = envVal;
            }
#endif
            directoryName += "/";
            directoryName += subDir;
            directoryName += "/";
        }
        // Add the process name to the directory name.
        directoryName += GetProcessName();
    }

#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "clIntercept", "dumpDir=%s\n", directoryName.c_str());
#endif
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

inline void Services_Common::GetDumpDirectoryNameWithoutProcessName(
    const std::string& subDir,
    std::string& directoryName) const
{
    // Get the home directory and add our directory name.
    if( LOG_DIR )
    {
       // Return log dir override if set in regkeys
        directoryName = LOG_DIR;
    }
    else
    {
        directoryName = getenv("HOME");
        directoryName += "/";
        directoryName += subDir;
        directoryName += "/";
    }
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "clIntercept", "dumpDir=%s\n", directoryName.c_str());
#endif
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
