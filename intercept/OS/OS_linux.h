/*
// Copyright (c) 2018-2020 Intel Corporation
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

#include "OS_linux_common.h"

// Since we just need the symbol clGetPlatformIDs, we can use any OpenCL
// version.
#if !defined(CL_TARGET_OPENCL_VERSION)
#define CL_TARGET_OPENCL_VERSION 100
#endif
#include "CL/cl.h"

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
    DISALLOW_COPY_AND_ASSIGN( Services );
};

inline bool Services::Init()
{
    return Services_Common::Init();
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
#endif

    return true;
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
#endif

    return true;
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
    // As setup, need to set AUBDumpSubcaptureMode = 2.
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

}
