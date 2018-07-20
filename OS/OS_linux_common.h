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

#include "OS_timer.h"

#include <sys/stat.h>
#include <dlfcn.h>
#include <pthread.h>
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
    void    GetDumpDirectoryNameWithoutProcessName(
                const std::string& subDir,
                std::string& directoryName) const;
    void    MakeDumpDirectories(
                const std::string& fileName ) const;

private:
    Timer           m_Timer;
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
    // TODO: Is this the thread ID we should be returning?
    return pthread_self();
}

inline std::string Services_Common::GetProcessName() const
{
    char    processName[ 1024 ];
    char*   pProcessName = processName;

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
    return m_Timer.GetTimer();
}

inline uint64_t Services_Common::TickToNS(
    uint64_t delta ) const
{
    return m_Timer.TickToNS( delta );
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

inline void Services_Common::GetDumpDirectoryName(
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
