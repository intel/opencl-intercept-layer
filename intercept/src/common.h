/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_2_APIS
#define CL_TARGET_OPENCL_VERSION 300

#if defined(__ANDROID__)
#include <GLES/gl.h>
#elif defined(_WIN32) || defined(__linux__) || defined(__FreeBSD__)
#include "GL/glcorearb.h"
#elif defined(__APPLE__)
#include <OpenGL/GL.h>
#else
#error Unknown OS!
#endif

// Note: This is purposefully including the CLIntercept version of cl.h
// and cl_gl.h, not the system header files.
#include "CL/cl.h"
#include "CL/cl_gl.h"

#include "cli_ext.h"

#if defined(_WIN32)
    #define CLI_DEBUG_BREAK()   __debugbreak();
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
    #include <limits.h>
    #include <signal.h>
    #define CLI_DEBUG_BREAK()   raise(SIGTRAP);
#else
    #error Unknown OS!
#endif

#ifdef _DEBUG
    #define CLI_ASSERT(x)       \
    {                           \
        if (!(x))               \
        {                       \
            CLI_DEBUG_BREAK();  \
        }                       \
    }
#else
    #define CLI_ASSERT(x)
#endif

#if defined(_WIN32) || defined(__linux__) || defined(__FreeBSD__)
    #define CLIRN( _funcname )  _funcname
#elif defined(__APPLE__)
    #define CLIRN( _funcname )  i ## _funcname
#else
    #error Unknown OS!
#endif

#if defined(_WIN32)
    #define CLI_SPRINTF(_s, _sz, _f, ...)   sprintf_s(_s, _TRUNCATE, _f, ##__VA_ARGS__)
    #define CLI_VSPRINTF(_s, _sz, _f, _a)   vsnprintf_s(_s, _TRUNCATE, _f, _a)
    #define CLI_MEMCPY(_d, _dsz, _s, _sz)   memcpy_s(_d, _dsz, _s, _sz)
    #define CLI_STRCAT(_d, _dsz, _s)        strcat_s(_d, _dsz, _s)
    #define CLI_STRTOK(_s, _d, _c)          strtok_s(_s, _d, _c)
    #define CLI_C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#else
    #if !defined(MAX_PATH)
        #define MAX_PATH    256
    #endif
    #define CLI_SPRINTF(_s, _sz, _f, ...)   snprintf(_s, _sz, _f, ##__VA_ARGS__)
    #define CLI_VSPRINTF(_s, _sz, _f, _a)   vsnprintf(_s, _sz, _f, _a)
    // TODO: Investigate how to reliably use memcpy_s on Linux:
    #define CLI_MEMCPY(_d, _dsz, _s, _sz)   ((void)(_dsz), memcpy(_d, _s, _sz))
    #define CLI_STRCAT(_d, _dsz, _s)        ((void)(_dsz), strcat(_d, _s))
    #define CLI_STRTOK(_s, _d, _c)          strtok_r(_s, _d, _c)
    #define CLI_C_ASSERT(e) typedef char __attribute__((unused)) __C_ASSERT__[(e)?1:-1]
#endif

#define CLI_STRING_BUFFER_SIZE (16 * 1024)

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
