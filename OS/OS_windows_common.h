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

#include "OS_timer.h"

#include <Windows.h>
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

// Visual Studio 2008 doesn't support stdint.h, but
// Visual Studio 2010 does.  When CLIntercept stops
// supporting Visual Studio 2008 we can remove this
// typedef and include stdint.h instead.
typedef unsigned __int64 uint64_t;

namespace OS
{

class Services_Common
{
public:
    static const char* ENV_PREFIX;
    static const char* REGISTRY_KEY;
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
    Timer               m_Timer;
    CRITICAL_SECTION    m_CriticalSection;

    DISALLOW_COPY_AND_ASSIGN( Services_Common );
};

inline bool Services_Common::Init()
{
    if( m_Timer.Init() == false )
    {
        return false;
    }

    if( ::InitializeCriticalSectionAndSpinCount(
            &m_CriticalSection,
            0x400 ) == FALSE )
    {
        return false;
    }

    return true;
}

inline void Services_Common::EnterCriticalSection()
{
    ::EnterCriticalSection( &m_CriticalSection );
}

inline void Services_Common::LeaveCriticalSection()
{
    ::LeaveCriticalSection( &m_CriticalSection );
}

inline uint64_t Services_Common::GetProcessID() const
{
    return GetCurrentProcessId();
}

inline uint64_t Services_Common::GetThreadID() const
{
    return GetCurrentThreadId();
}

inline std::string Services_Common::GetProcessName() const
{
    char    processName[ MAX_PATH ];
    char*   pProcessName = processName;

    if( GetModuleFileNameA( NULL, processName, MAX_PATH - 1 ) )
    {
        pProcessName = strrchr( processName, '\\' );
        pProcessName++;
    }
    else
    {
        strcpy_s( processName, MAX_PATH, "process.exe" );
    }

    return std::string(pProcessName);
}

inline bool Services_Common::ReadRegistry(
    const std::string& name,
    void* pValue,
    size_t size ) const
{
    // Look at environment variables first:
    {
        std::string envName(ENV_PREFIX);
        envName += name;

        char* envVal = NULL;
        size_t  len = 0;
        errno_t err = _dupenv_s( &envVal, &len, envName.c_str() );
        if( !err )
        {
            if( ( envVal != NULL ) && ( size == sizeof(unsigned int) ) )
            {
                unsigned int *puVal = (unsigned int *)pValue;
                *puVal = atoi(envVal);
                free( envVal );
                return true;
            }
            else if( ( envVal != NULL ) && ( strlen(envVal) < size ) )
            {
                char* pStr = (char*)pValue;
                strcpy_s( pStr, size, envVal );
                free( envVal );
                return true;
            }
            free( envVal );
        }
    }

    LONG    success = ERROR_SUCCESS;
    HKEY    cliKey;

    // Try HKEY_CURRENT_USER first.

    success = ::RegOpenKeyEx(
        HKEY_CURRENT_USER,
        REGISTRY_KEY,
        0,
        KEY_READ,
        &cliKey );
    if( ERROR_SUCCESS == success )
    {
        DWORD   dwSize = (DWORD)size;

        success = ::RegQueryValueEx(
            cliKey,
            name.c_str(),
            NULL,
            NULL,
            (LPBYTE)pValue,
            &dwSize );

        ::RegCloseKey( cliKey );
    }

    // Only try HKEY_LOCAL_MACHINE if we didn't find the
    // control in HKEY_CURRENT_USER.  This way we maintain
    // backwards compatibility with existing installations
    // of CLIntercept, but controls in HKEY_CURRENT_USER
    // "win".

    if( ERROR_SUCCESS != success )
    {
        success = ::RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            REGISTRY_KEY,
            0,
            KEY_READ,
            &cliKey );
        if( ERROR_SUCCESS == success )
        {
            DWORD   dwSize = (DWORD)size;

            success = ::RegQueryValueEx(
                cliKey,
                name.c_str(),
                NULL,
                NULL,
                (LPBYTE)pValue,
                &dwSize );

            ::RegCloseKey( cliKey );
        }
    }

    return ( ERROR_SUCCESS == success );
}

inline void Services_Common::OutputDebugString(
    const std::string& str ) const
{
    ::OutputDebugString( str.c_str() );
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
    HMODULE hModule = ::LoadLibraryA( libraryName.c_str() );
    return hModule;
}

inline void Services_Common::UnloadLibrary(
    void*& pLibrary ) const
{
    HMODULE hModule = (HMODULE)pLibrary;
    ::FreeLibrary( hModule );
    pLibrary = NULL;
}

inline void* Services_Common::GetFunctionPointer(
    void* pLibrary,
    const std::string& functionName ) const
{
    if( pLibrary )
    {
        HMODULE hModule = (HMODULE)pLibrary;
        return ::GetProcAddress( hModule, functionName.c_str() );
    }
    else
    {
        return NULL;
    }
}

inline void Services_Common::GetDumpDirectoryNameWithoutPid(
    const std::string& subDir,
    std::string& directoryName ) const
{
    // Return log dir override if set in regkeys
    if( LOG_DIR )
    {
        directoryName = LOG_DIR;
        return;
    }

    // Get the system root and add our directory name.
    {
        char*   systemDrive = NULL;
        size_t  length = 0;

        _dupenv_s( &systemDrive, &length, "SystemDrive" );

        directoryName = systemDrive;
        directoryName += "/Intel/";
        directoryName += subDir;
        directoryName += "/";

        free( systemDrive );
    }

    // Add the process name to the directory name.
    directoryName += GetProcessName();
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
    // Return log dir override if set in regkeys
    if( LOG_DIR )
    {
        directoryName = LOG_DIR;
        return;
    }

    // Get the system root and add our directory name.
    {
        char*   systemDrive = NULL;
        size_t  length = 0;

        _dupenv_s(&systemDrive, &length, "SystemDrive");

        directoryName = systemDrive;
        directoryName += "/Intel/";
        directoryName += subDir;
        directoryName += "/";

        free(systemDrive);
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
        CreateDirectoryA(
            fileName.substr( 0, pos ).c_str(),
            NULL );

        pos = fileName.find( "/", ++pos );
    }
}

}
