/*
// Copyright (c) 2018-2019 Intel Corporation
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
    static const char* LOG_DIR;
    static bool        APPEND_PID;

    Services_Common();
    ~Services_Common();

    bool    Init();

    void    EnterCriticalSection();
    void    LeaveCriticalSection();

    uint64_t    GetProcessID() const;
    uint64_t    GetThreadID() const;

    std::string GetProcessName() const;

    bool    ReadRegistry(
                const std::string& name,
                void* pValue,
                size_t size ) const;

    void    OutputDebugString(
                const std::string& str ) const;

    uint64_t    GetTimer() const;
    uint64_t    TickToNS(
                    uint64_t delta ) const;

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
    pthread_mutex_t m_CriticalSection;

    DISALLOW_COPY_AND_ASSIGN( Services_Common );
};

inline bool Services_Common::Init()
{
    if( pthread_mutex_init(
            &m_CriticalSection,
            NULL ) )
    {
        return false;
    }

    return true;
}

inline void Services_Common::EnterCriticalSection()
{
    pthread_mutex_lock( &m_CriticalSection );
}

inline void Services_Common::LeaveCriticalSection()
{
    pthread_mutex_unlock( &m_CriticalSection );
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

inline uint64_t Services_Common::GetTimer() const
{
    timeval i;
    gettimeofday( &i, NULL );
    return i.tv_sec * 1000000 + i.tv_usec;
}

inline uint64_t Services_Common::TickToNS(
    uint64_t delta ) const
{
    double  ns = delta * 1000.0;
    return (uint64_t)ns;
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

inline void Services_Common::GetDumpDirectoryNameWithoutProcessName(
    const std::string& subDir,
    std::string& directoryName) const
{
    // Get the home directory and add our directory name.
    {
        directoryName = getenv("HOME");
        directoryName += "/";
        directoryName += subDir;
        directoryName += "/";
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
