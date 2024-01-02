/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
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

#elif defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)

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
