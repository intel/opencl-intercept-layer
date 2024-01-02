/*
// Copyright (c) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#include <stdio.h>
#include <string>
#include "mdapi/metrics_discovery_api.h"

#if defined(_WIN32)
static const wchar_t* cMDLibFileName =
sizeof(void*) == 8 ?
    L"igdmd64.dll" :
    L"igdmd32.dll";

#include <Windows.h>
#include "mdapi/DriverStorePath.h"

static void* OpenLibrary()
{
    return (void*)LoadDynamicLibrary(cMDLibFileName);
}

#define GetFunctionAddress(_handle, _name)  GetProcAddress((HMODULE)_handle, _name)

#elif defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
#if defined(__linux__) || defined(__FreeBSD__)
static const char* cMDLibFileName = "libigdmd.so";
#else
static const char* cMDLibFileName = "libigdmd.dylib";
#endif

#include <dlfcn.h>
#include <stdarg.h>
#include <string.h>

static void* OpenLibrary()
{
    void* ret = dlopen(cMDLibFileName, RTLD_LAZY | RTLD_LOCAL);
#if !defined(__APPLE__)
    if (ret == NULL)
    {
        // old alternate name, may eventually be removed
        ret = dlopen("libmd.so", RTLD_LAZY | RTLD_LOCAL);
    }
#endif
    return ret;
}

#define GetFunctionAddress(_handle, _name)  dlsym(_handle, _name)

#endif

namespace MetricsDiscovery
{

static bool printMetricsHelper(const std::string& metricsFileName)
{
    OpenMetricsDevice_fn            OpenMetricsDevice;
    CloseMetricsDevice_fn           CloseMetricsDevice;
    OpenMetricsDeviceFromFile_fn    OpenMetricsDeviceFromFile;

    TCompletionCode res = CC_OK;

    IMetricsDevice_1_5* metricsDevice;

    void* pLibrary = OpenLibrary();
    if (pLibrary == NULL)
    {
        fprintf(stderr, "Couldn't load metrics discovery library!\n");
        return false;
    }

    CloseMetricsDevice = (CloseMetricsDevice_fn)GetFunctionAddress(pLibrary, "CloseMetricsDevice");
    if (CloseMetricsDevice == NULL)
    {
        fprintf(stderr, "Couldn't get pointer to CloseMetricsDevice!\n");
        return false;
    }

    OpenMetricsDevice = (OpenMetricsDevice_fn)GetFunctionAddress(pLibrary, "OpenMetricsDevice");
    if (OpenMetricsDevice == NULL)
    {
        fprintf(stderr, "Couldn't get pointer to OpenMetricsDevice!\n");
        return false;
    }

    OpenMetricsDeviceFromFile = (OpenMetricsDeviceFromFile_fn)GetFunctionAddress(pLibrary, "OpenMetricsDeviceFromFile");
    if (OpenMetricsDeviceFromFile == NULL)
    {
        fprintf(stderr, "Couldn't get pointer to OpenMetricsDeviceFromFile!\n");
        return false;
    }

    if (!metricsFileName.empty())
    {
        res = OpenMetricsDeviceFromFile(metricsFileName.c_str(), (void*)"", &metricsDevice);
        if (res != CC_OK)
        {
            res = OpenMetricsDeviceFromFile(metricsFileName.c_str(), (void*)"abcdefghijklmnop", &metricsDevice);
        }
        if (res != CC_OK)
        {
            fprintf(stderr, "OpenMetricsDeviceFromFile failed, res: %d\n", res);
            return false;
        }
    }
    else
    {
        res = OpenMetricsDevice(&metricsDevice);
        if (res != CC_OK)
        {
            fprintf(stderr, "OpenMetricsDevice failed, res: %d\n", res);
            return false;
        }
    }

    TMetricsDeviceParams_1_0* deviceParams = metricsDevice->GetParams();
    if (NULL == deviceParams)
    {
        fprintf(stderr, "DeviceParams null\n");
        return false;
    }

    fprintf(stderr, "MDAPI Headers: v%d.%d.%d, MDAPI Lib: v%d.%d.%d\n",
        MD_API_MAJOR_NUMBER_CURRENT,
        MD_API_MINOR_NUMBER_CURRENT,
        MD_API_BUILD_NUMBER_CURRENT,
        deviceParams->Version.MajorNumber,
        deviceParams->Version.MinorNumber,
        deviceParams->Version.BuildNumber);
    if (deviceParams->Version.MajorNumber < 1 ||
        (deviceParams->Version.MajorNumber == 1 && deviceParams->Version.MinorNumber < 1))
    {
        fprintf(stderr, "MDAPI Lib version must be at least v1.1!\n");
        return false;
    }

    bool found = false;
    for (uint32_t cg = 0; !found && cg < deviceParams->ConcurrentGroupsCount; cg++)
    {
        IConcurrentGroup_1_1* group = metricsDevice->GetConcurrentGroup(cg);
        TConcurrentGroupParams_1_0* groupParams = group->GetParams();

        if (groupParams)
        {
            fprintf(stderr, "\nMetric Group: %s (%d Metric Set%s)\n",
                groupParams->Description,
                groupParams->MetricSetsCount,
                groupParams->MetricSetsCount > 1 ? "s" : "");
            fprintf(stderr, "========================================\n\n");

            for (uint32_t ms = 0; ms < groupParams->MetricSetsCount; ms++)
            {
                IMetricSet_1_1* metricSet = group->GetMetricSet(ms);
                TMetricSetParams_1_0* setParams = metricSet->GetParams();

                if (setParams)
                {
                    fprintf(stderr, "Metric Set: %s (%d Metric%s)\n",
                        setParams->ShortName,
                        setParams->MetricsCount,
                        setParams->MetricsCount > 1 ? "s" : "");
                    fprintf(stderr, "----------------------------------------\n\n");

                    for (uint32_t m = 0; m < setParams->MetricsCount; m++)
                    {
                        TMetricParams_1_0* metricParams = metricSet->GetMetric(m)->GetParams();

                        if (metricParams)
                        {
                            fprintf(stderr,
                                "%s\\%s (%s):\n"
                                "%s\n\n",
                                setParams->SymbolName,
                                metricParams->SymbolName,
                                metricParams->ShortName,
                                metricParams->LongName);
                        }
                    }
                }
            }
        }
    }

    res = CloseMetricsDevice(metricsDevice);
    if (res != CC_OK)
    {
        fprintf(stderr, "CloseMetricsDevice failed, res: %d\n", res);
        return false;
    }

    return true;
}

};

static void printMetrics()
{
    MetricsDiscovery::printMetricsHelper("");
}
