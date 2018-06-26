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

    bool    ExecuteCommand(
                const std::string& filename ) const;
    bool    StartAubCapture(
                const std::string& fileName,
                uint64_t delay ) const;
    bool    StopAubCapture(
                uint64_t delay ) const;

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

inline bool Services::ExecuteCommand( const std::string& command ) const
{
    int res = system( command.c_str() );
    return res != -1;
}

static inline bool SetAubcaptureRegistryKey(
    DWORD dwValue )
{
    // For NEO Aubcapture:
    // As setup, need to set AUBDumpSubcaptureMode = 2.  This will be the client's responsibility.
    //
    // To start/stop aubcapture, set AUBDumpToggleCaptureOnOff = 1/0.  This is CLIntercept's responsibility.

    // There is no way to set the aubcapture file name at the moment.

    // Registry keys are written to:
    const char* const AUBCAPTURE_REGISTRY_KEY = "SOFTWARE\\INTEL\\IGFX\\OCL";

    LSTATUS success = ERROR_SUCCESS;
    HKEY    key;

    if( success == ERROR_SUCCESS )
    {
        success = RegCreateKeyEx(
            HKEY_LOCAL_MACHINE,
            AUBCAPTURE_REGISTRY_KEY,
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
                "AUBDumpToggleCaptureOnOff",
                0,
                REG_DWORD,
                (CONST BYTE *)&dwValue,
                sizeof(DWORD));
            RegCloseKey(key);
        }
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

#if 0
    std::string command = "kdc.exe " + fileName;
    int res = system(command.c_str());
    //fprintf(stderr, "Running the command: %s returned %d\n", command.c_str(), res );
    return res != -1;
#else
    return SetAubcaptureRegistryKey( 1 );
#endif
}

inline bool Services::StopAubCapture(
    uint64_t delay ) const
{
    if( delay )
    {
        Sleep( (DWORD)delay );
    }

#if 0
    std::string command = "kdc.exe -off";
    int res = system(command.c_str());
    //fprintf(stderr, "Running the command: %s returned %d\n", command.c_str(), res );
    return res != -1;
#else
    return SetAubcaptureRegistryKey( 0 );
#endif
}

}
