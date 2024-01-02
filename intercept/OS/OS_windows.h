/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#include "OS_windows_common.h"

#include "resource/clIntercept_resource.h"

namespace OS
{

class Services : public Services_Common
{
public:
    Services( void* pGlobalData );
    ~Services();

    bool    Init();

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
                const std::string& filename ) const;
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

private:
    HINSTANCE   m_hInstance;

    DISALLOW_COPY_AND_ASSIGN( Services );
};

inline bool Services::Init()
{
    if( m_hInstance == NULL )
    {
        return false;
    }

    return Services_Common::Init();
}

inline bool Services::GetCLInterceptName(
    std::string& name ) const
{
    char    dllName[ MAX_PATH ];

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

}
