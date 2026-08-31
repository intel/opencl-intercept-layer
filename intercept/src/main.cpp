/*
// Copyright (c) 2018-2026 Intel Corporation
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

typedef cl_uint cl_layer_info;
typedef cl_uint cl_layer_api_version;

#define CL_LAYER_API_VERSION 0x4240
#define CL_LAYER_NAME 0x4241
#define CL_LAYER_API_VERSION_100 100

static cl_icd_dispatch layer_dispatch;

extern "C" CL_API_ENTRY cl_int CL_API_CALL clGetLayerInfo(
    cl_layer_info param_name,
    size_t param_value_size,
    void *param_value,
    size_t *param_value_size_ret)
{
    switch (param_name) {
        case CL_LAYER_API_VERSION: {
            const cl_layer_api_version version = CL_LAYER_API_VERSION_100;
            if (param_value_size_ret)
                *param_value_size_ret = sizeof(version);

            if (param_value) {
                if (param_value_size < sizeof(version))
                    return CL_INVALID_VALUE;

                CLI_MEMCPY(param_value, param_value_size, &version, sizeof(version));
            }
            return CL_SUCCESS;
        }
        case CL_LAYER_NAME: {
            const char name[] = "CLIntercept";
            if (param_value_size_ret)
                *param_value_size_ret = sizeof(name);

            if (param_value) {
                if (param_value_size < sizeof(name))
                    return CL_INVALID_VALUE;

                CLI_MEMCPY(param_value, param_value_size, name, sizeof(name));
            }
            return CL_SUCCESS;
        }

        default:
            return CL_INVALID_VALUE;
    }
}

extern "C" CL_API_ENTRY cl_int CL_API_CALL clInitLayer(
    cl_uint num_entries,
    const cl_icd_dispatch *target_dispatch,
    cl_uint *num_entries_out,
    const cl_icd_dispatch **layer_dispatch_ret)
{
    if ( target_dispatch == NULL || num_entries_out == NULL || layer_dispatch_ret == NULL ) {
        return CL_INVALID_VALUE;
    }

    g_pIntercept->initLayer( layer_dispatch, target_dispatch );
    *layer_dispatch_ret = &layer_dispatch;
    *num_entries_out = num_entries;
    return CL_SUCCESS;
}
