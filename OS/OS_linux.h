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

#include "OS_linux_common.h"

#include "CL/cl.h"  // for clGetPlatformIDs

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

#ifndef __ANDROID__
extern "C" char _binary_Kernels_precompiled_kernels_cl_start;
extern "C" char _binary_Kernels_precompiled_kernels_cl_end;
#endif

inline bool Services::GetPrecompiledKernelString(
    const char*& str,
    size_t& length ) const
{
#ifndef __ANDROID__
    str = &_binary_Kernels_precompiled_kernels_cl_start;
    length = &_binary_Kernels_precompiled_kernels_cl_end - &_binary_Kernels_precompiled_kernels_cl_start;
#endif

    return true;
}

#ifndef __ANDROID__
extern "C" char _binary_Kernels_builtin_kernels_cl_start;
extern "C" char _binary_Kernels_builtin_kernels_cl_end;
#endif

inline bool Services::GetBuiltinKernelString(
    const char*& str,
    size_t& length ) const
{
#ifndef __ANDROID__
    str = &_binary_Kernels_builtin_kernels_cl_start;
    length = &_binary_Kernels_builtin_kernels_cl_end - &_binary_Kernels_builtin_kernels_cl_start;
#endif

    return true;
}

inline bool Services::ExecuteCommand( const std::string& command ) const
{
    int res = system( command.c_str() );
    return res != -1;
}

// TODO

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

}
