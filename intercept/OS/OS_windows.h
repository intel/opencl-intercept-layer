/*
// Copyright (c) 2018-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#include <Windows.h>
#include <string>
#include <stdint.h>

#include "resource/clIntercept_resource.h"

namespace OS
{

class Services
{
public:
    static const char* ENV_PREFIX;
    static const char* REGISTRY_KEY;
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
    bool    StartAubCaptureKDC(
                const std::string& fileName,
                uint64_t delay ) const;
    bool    StopAubCaptureKDC(
                uint64_t delay ) const;

    bool    CheckMDAPIPermissions(
                std::string& str ) const;

    bool    CheckConditionalEnable(
                const char* name) const;

private:
    HINSTANCE   m_hInstance;
};

inline uint64_t Services::GetProcessID() const
{
    return GetCurrentProcessId();
}

inline uint64_t Services::GetThreadID() const
{
    return GetCurrentThreadId();
}

inline std::string Services::GetProcessName() const
{
    char    processName[ MAX_PATH ] = "";
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

inline bool Services::GetControl(
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

inline void Services::OutputDebugString(
    const std::string& str ) const
{
    ::OutputDebugString( str.c_str() );
}

inline void* Services::LoadLibrary(
    const std::string& libraryName ) const
{
    HMODULE hModule = ::LoadLibraryA( libraryName.c_str() );
    return hModule;
}

inline void Services::UnloadLibrary(
    void*& pLibrary ) const
{
    HMODULE hModule = (HMODULE)pLibrary;
    ::FreeLibrary( hModule );
    pLibrary = NULL;
}

inline void* Services::GetFunctionPointer(
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

inline void Services::GetDumpDirectoryNameWithoutPid(
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

        directoryName = systemDrive ? systemDrive : "";
        directoryName += "/Intel/";
        directoryName += subDir;
        directoryName += "/";

        free( systemDrive );
    }

    // Add the process name to the directory name.
    directoryName += GetProcessName();
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
        CreateDirectoryA(
            fileName.substr( 0, pos ).c_str(),
            NULL );

        pos = fileName.find( "/", ++pos );
    }
}
inline bool Services::GetCLInterceptName(
    std::string& name ) const
{
    char    dllName[ MAX_PATH ] = "";

    if( GetModuleFileNameA( m_hInstance, dllName, MAX_PATH - 1 ) )
    {
        name = dllName;
        return true;
    }

    return false;
}

inline bool Services::GetPrecompiledKernelString(
    const char*& str,
    size_t& length ) const
{
    bool    success = false;

    HRSRC hrsrc = ::FindResource(
        m_hInstance,
        MAKEINTRESOURCE(IDR_TEXT_PRECOMPILED_KERNELS),
        "TEXT" );

    if( hrsrc != NULL )
    {
        length = ::SizeofResource(
            m_hInstance,
            hrsrc );

        HGLOBAL hres = ::LoadResource(
            m_hInstance,
            hrsrc );
        if( hres != NULL )
        {
            void*   pVoid = ::LockResource( hres );
            if( pVoid )
            {
                str = (const char*)pVoid;
                success = true;
            }
        }
    }

    return success;
}

inline bool Services::GetBuiltinKernelString(
    const char*& str,
    size_t& length ) const
{
    bool    success = false;

    HRSRC hrsrc = ::FindResource(
        m_hInstance,
        MAKEINTRESOURCE(IDR_TEXT_BUILTIN_KERNELS),
        "TEXT" );

    if( hrsrc != NULL )
    {
        length = ::SizeofResource(
            m_hInstance,
            hrsrc );

        HGLOBAL hres = ::LoadResource(
            m_hInstance,
            hrsrc );
        if( hres != NULL )
        {
            void*   pVoid = ::LockResource( hres );
            if( pVoid )
            {
                str = (const char*)pVoid;
                success = true;
            }
        }
    }

    return success;
}

inline bool Services::GetReplayScriptString(
    const char*& str,
    size_t& length ) const
{
    bool    success = false;

    HRSRC hrsrc = ::FindResource(
        m_hInstance,
        MAKEINTRESOURCE(IDR_TEXT_REPLAY_SCRIPT),
        "TEXT" );

    if( hrsrc != NULL )
    {
        length = ::SizeofResource(
            m_hInstance,
            hrsrc );

        HGLOBAL hres = ::LoadResource(
            m_hInstance,
            hrsrc );
        if( hres != NULL )
        {
            void*   pVoid = ::LockResource( hres );
            if( pVoid )
            {
                str = (const char*)pVoid;
                success = true;
            }
        }
    }

    return success;
}

inline bool Services::ExecuteCommand( const std::string& command ) const
{
    int res = system( command.c_str() );
    return res != -1;
}

static inline bool SetAubCaptureRegistryKeys(
    const std::string& fileName,
    DWORD dwValue )
{
    // For NEO AubCapture:
    // As setup, need to set AUBDumpSubCaptureMode = 2.
    // This will be the client's responsibility.
    //
    // To start/stop AubCapture:
    //  set AUBDumpToggleCaptureOnOff = 1/0
    //  set AUBDumpToggleFileName appropriately
    // This is CLIntercept's responsibility.

    // Registry keys are written to:
    const char* const AUB_CAPTURE_REGISTRY_KEY = "SOFTWARE\\INTEL\\IGFX\\OCL";
    const char* const AUB_CAPTURE_TOGGLE_SUBKEY = "AUBDumpToggleCaptureOnOff";
    const char* const AUB_CAPTURE_FILE_NAME_SUBKEY = "AUBDumpToggleFileName";

    LSTATUS success = ERROR_SUCCESS;
    HKEY    key = NULL;

    if( success == ERROR_SUCCESS )
    {
        success = RegCreateKeyEx(
            HKEY_CURRENT_USER,
            AUB_CAPTURE_REGISTRY_KEY,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE,
            NULL,
            &key,
            NULL );
        if( success == ERROR_SUCCESS )
        {
            success = RegSetValueEx(
                key,
                AUB_CAPTURE_TOGGLE_SUBKEY,
                0,
                REG_DWORD,
                (CONST BYTE *)&dwValue,
                sizeof(DWORD));
        }
        if( success == ERROR_SUCCESS )
        {
            if( fileName.empty() )
            {
                success = RegDeleteValue(
                    key,
                    AUB_CAPTURE_FILE_NAME_SUBKEY );
            }
            else
            {
                success = RegSetValueEx(
                    key,
                    AUB_CAPTURE_FILE_NAME_SUBKEY,
                    0,
                    REG_SZ,
                    (const BYTE*)fileName.c_str(),
                    (DWORD)fileName.length() );
            }
        }
        RegCloseKey(key);
    }

    if( success != ERROR_SUCCESS )
    {
        LPVOID lpMsgBuf = NULL;

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            success,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&lpMsgBuf,
            0, NULL );

        OutputDebugString((LPCSTR)lpMsgBuf);

        LocalFree(lpMsgBuf);
    }

    return success == ERROR_SUCCESS;
}

inline bool Services::StartAubCapture(
    const std::string& fileName,
    uint64_t delay ) const
{
    if( delay )
    {
        Sleep( (DWORD)delay );
    }

    // This is the newer NEO method of AubCapture:
    bool success = SetAubCaptureRegistryKeys( fileName, 1 );

    return success;
}

inline bool Services::StopAubCapture(
    uint64_t delay ) const
{
    if( delay )
    {
        Sleep( (DWORD)delay );
    }

    // This is the newer NEO method of AubCapture:
    bool success = SetAubCaptureRegistryKeys( "", 0 );

    return success;
}

inline bool Services::StartAubCaptureKDC(
    const std::string& fileName,
    uint64_t delay ) const
{
    if( delay )
    {
        Sleep( (DWORD)delay );
    }

    // This is the old kdc method of AubCapture:
    std::string command = "kdc.exe " + fileName;
    int res = system(command.c_str());
    //fprintf(stderr, "Running the command: %s returned %d\n", command.c_str(), res );

    return res != -1;
}

inline bool Services::StopAubCaptureKDC(
    uint64_t delay ) const
{
    if( delay )
    {
        Sleep( (DWORD)delay );
    }

    // This is the old kdc method of AubCapture:
    std::string command = "kdc.exe -off";
    int res = system(command.c_str());
    //fprintf(stderr, "Running the command: %s returned %d\n", command.c_str(), res );

    return res != -1;
}

inline bool Services::CheckMDAPIPermissions(
    std::string& str ) const
{
    return true;
}

inline bool Services::CheckConditionalEnable(
    const char* name) const
{
    bool enabled = false;
    char* envVal = NULL;
    size_t  len = 0;
    errno_t err = _dupenv_s( &envVal, &len, name );
    if( !err && envVal )
    {
        if( strcmp(envVal, "0") != 0 )
        {
            enabled = true;
        }
        free( envVal );
    }
    return enabled;
}

}
