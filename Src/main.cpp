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

#include "common.h"
#include "intercept.h"
#ifdef __ANDROID__
#include <android/log.h>
#endif

CLIntercept*    g_pIntercept = NULL;

#if defined(_WIN32)

#include <windows.h>

BOOL APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReasonForCall, LPVOID lpReserved) 
{
    switch(dwReasonForCall) { 
    case DLL_PROCESS_ATTACH:
        if( CLIntercept::Create( hInstance, g_pIntercept ) == false )
        {
            return FALSE;
        }
        break;

    case DLL_PROCESS_DETACH:
        CLIntercept::Delete( g_pIntercept );
        break; 

    case DLL_THREAD_ATTACH:
        break; 

    case DLL_THREAD_DETACH:
        break;

    default:
        CLI_ASSERT(0);
        break;
    }

    return TRUE;
}

#elif defined(__linux__) || defined(__APPLE__)

void __attribute__((constructor)) CLIntercept_Load(void);
void __attribute__((destructor))  CLIntercept_Unload(void);

void CLIntercept_Load(void)
{
#ifdef __ANDROID__
    __android_log_print( ANDROID_LOG_INFO, "clIntercept", ">>Load.pid=%d\n", getpid() );
#endif
    CLIntercept::Create( NULL, g_pIntercept );
#ifdef __ANDROID__
    __android_log_print( ANDROID_LOG_INFO, "clIntercept", "<<Load\n" );
#endif
}

void CLIntercept_Unload(void)
{
    CLIntercept::Delete( g_pIntercept );
}

#else
#error Unknown OS!
#endif
