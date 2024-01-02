/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/
#pragma once

#include "OS_mac_common.h"

void CLIntercept_Load(void);

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

    bool    CheckMDAPIPermissions(
                std::string& str ) const;

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
    if( dladdr( (void*)CLIntercept_Load, &info ) )
    {
        name = info.dli_fname;
    }
    return false;
}

// TODO: We currently don't support any of the kernels overrides on OSX.

inline bool Services::GetPrecompiledKernelString(
    const char*& str,
    size_t& length ) const
{
    return false;
}

inline bool Services::GetBuiltinKernelString(
    const char*& str,
    size_t& length ) const
{
    return false;
}

inline bool Services::GetReplayScriptString(
    const char*& str,
    size_t& length ) const
{
    return false;
}

// TODO

inline bool Services::ExecuteCommand( const std::string& command ) const
{
    return false;
}

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

inline bool Services::CheckMDAPIPermissions(
    std::string& str ) const
{
    return true;
}

}
