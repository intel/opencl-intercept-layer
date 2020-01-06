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

#include <stdint.h>

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <time.h>
#include <sys/time.h>
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

class Timer
{
public:
    Timer() {};
    ~Timer() {};

    bool    Init( void );

    uint64_t    GetTimer( void ) const;
    uint64_t    TickToNS( uint64_t delta ) const;

private:
#if defined(_WIN32)
    LARGE_INTEGER       m_Freq;
#endif

    DISALLOW_COPY_AND_ASSIGN( Timer );
};

inline bool Timer::Init( void )
{
#if defined(_WIN32)
    if( ::QueryPerformanceFrequency( &m_Freq ) == FALSE )
    {
        return false;
    }
#endif
    return true;
}

inline uint64_t Timer::GetTimer( void ) const
{
#if defined(_WIN32)
    LARGE_INTEGER   i;
    ::QueryPerformanceCounter( &i );
    return (uint64_t)i.QuadPart;
#elif defined(__linux__)
#ifdef USE_OLD_TIMER
    timeval i;
    gettimeofday( &i, NULL );
    return i.tv_sec * 1000000 + i.tv_usec;
#else
    struct timespec t;
    clock_gettime( CLOCK_MONOTONIC, &t );
    return t.tv_sec * 1000000000 + t.tv_nsec;
#endif
#else
#error Need to implement Timer::GetTimer!
#endif
}

inline uint64_t Timer::TickToNS( uint64_t delta ) const
{
#if defined(_WIN32)
    double  ns = delta * ( 1000000000.0 / m_Freq.QuadPart );
    return (uint64_t)ns;
#elif defined(__linux__)
#ifdef USE_OLD_TIMER
    double  ns = delta * 1000.0;
    return (uint64_t)ns;
#else
    return delta;
#endif
#else
#error Need to implement Timer::TickToNS!
#endif
}

}
