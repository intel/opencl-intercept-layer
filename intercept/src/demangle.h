/*
// Copyright (c) 2022-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#if defined(USE_DEMANGLE)
#include <cxxabi.h>
#endif
#include <string>

static inline std::string demangle(const std::string& in)
{
#if defined(USE_DEMANGLE)
    std::string ret;
    int status = 0;
    char* demangled = abi::__cxa_demangle(in.c_str(), NULL, NULL, &status);
    if( status != 0 )
    {
        ret = in;
    }
    else
    {
        const std::string skip("typeinfo name for ");
        ret = demangled;
        if (ret.rfind(skip, 0) == 0) {
            ret.erase(0, skip.length());
        }
    }
    free(demangled);
    return ret;
#else
    return in;
#endif
}