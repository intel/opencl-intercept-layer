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

static const char* adapterTypeToString(TAdapterType type)
{
    switch (type) {
    case ADAPTER_TYPE_UNDEFINED: return "UNDEFINED";
    case ADAPTER_TYPE_INTEGRATED: return "INTEGRATED";
    case ADAPTER_TYPE_DISCRETE: return "DISCRETE";
    default: break;
    }
    return "Unknown";
}

static bool printMetricsForDevice(IMetricsDeviceLatest* pMetricsDevice)
{
    TMetricsDeviceParamsLatest* deviceParams = pMetricsDevice->GetParams();
    if (NULL == deviceParams)
    {
        fprintf(stderr, "MetricsDevice->GetParams() returned NULL\n");
        return false;
    }

    for (uint32_t cg = 0; cg < deviceParams->ConcurrentGroupsCount; cg++)
    {
        IConcurrentGroupLatest* group = pMetricsDevice->GetConcurrentGroup(cg);
        TConcurrentGroupParamsLatest* groupParams = group->GetParams();

        if (NULL == groupParams)
        {
            continue;
        }

        fprintf(stderr, "\nMetric Group: %s (%d Metric Set%s)\n",
            groupParams->Description,
            groupParams->MetricSetsCount,
            groupParams->MetricSetsCount > 1 ? "s" : "");
        fprintf(stderr, "========================================\n\n");

        for (uint32_t ms = 0; ms < groupParams->MetricSetsCount; ms++)
        {
            IMetricSetLatest* metricSet = group->GetMetricSet(ms);
            TMetricSetParamsLatest* setParams = metricSet->GetParams();

            if (NULL == setParams)
            {
                continue;
            }

            fprintf(stderr, "Metric Set: %s (%d Metric%s)\n",
                setParams->ShortName,
                setParams->MetricsCount,
                setParams->MetricsCount > 1 ? "s" : "");
            fprintf(stderr, "----------------------------------------\n\n");

            for (uint32_t m = 0; m < setParams->MetricsCount; m++)
            {
                TMetricParamsLatest* metricParams = metricSet->GetMetric(m)->GetParams();

                if (NULL == metricParams)
                {
                    continue;
                }

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

    return true;
}

static bool printMetricsForAdapterGroup(void* pLibrary)
{
    TCompletionCode res = CC_OK;

    OpenAdapterGroup_fn             OpenAdapterGroup;
    OpenAdapterGroup = (OpenAdapterGroup_fn)GetFunctionAddress(pLibrary, "OpenAdapterGroup");
    if (OpenAdapterGroup == NULL)
    {
        fprintf(stderr, "Couldn't get pointer to OpenAdapterGroup!\n");
        return false;
    }

    IAdapterGroupLatest* pAdapterGroup = NULL;
    res = OpenAdapterGroup(&pAdapterGroup);
    if (res != CC_OK)
    {
        fprintf(stderr, "OpenAdapterGroup failed, res: %d\n", res);
        return false;
    }

    const TAdapterGroupParamsLatest* pAdapterGroupParams = pAdapterGroup->GetParams();
    if (NULL == pAdapterGroupParams)
    {
        fprintf(stderr, "AdapterGroup->GetParams() returned NULL\n");
        return false;
    }

    fprintf(stderr, "MDAPI Headers: v%d.%d.%d, MDAPI Lib: v%d.%d.%d\n",
        MD_API_MAJOR_NUMBER_CURRENT,
        MD_API_MINOR_NUMBER_CURRENT,
        MD_API_BUILD_NUMBER_CURRENT,
        pAdapterGroupParams->Version.MajorNumber,
        pAdapterGroupParams->Version.MinorNumber,
        pAdapterGroupParams->Version.BuildNumber);
    if (pAdapterGroupParams->Version.MajorNumber < 1 ||
        (pAdapterGroupParams->Version.MajorNumber == 1 && pAdapterGroupParams->Version.MinorNumber < 1))
    {
        fprintf(stderr, "MDAPI Lib version must be at least v1.1!\n");
    }
    else
    {
        fprintf(stderr, "Found %u MDAPI Adapter%s.\n\n",
            pAdapterGroupParams->AdapterCount,
            pAdapterGroupParams->AdapterCount > 1 ? "s" : "");
        for (uint32_t a = 0; a < pAdapterGroupParams->AdapterCount; a++)
        {
            IAdapterLatest* pAdapter = pAdapterGroup->GetAdapter(a);
            if (NULL == pAdapter)
            {
                fprintf(stderr, "AdapterGroup->GetAdapter() returned NULL, skipping adapter.\n");
                continue;
            }

            const TAdapterParamsLatest* pAdapterParams = pAdapter->GetParams();
            if (NULL == pAdapterParams)
            {
                fprintf(stderr, "Adapter->GetParams() returned NULL, skipping adapter.\n");
                continue;
            }

            fprintf(stderr, "\nAdapter: %s (%s)\n",
                pAdapterParams->ShortName,
                adapterTypeToString(pAdapterParams->Type));
            fprintf(stderr, "PCI Vendor Id: %04X, Device Id: %04X, Bus Info: %02X:%02X.%02X\n",
                pAdapterParams->VendorId,
                pAdapterParams->DeviceId,
                pAdapterParams->BusNumber,
                pAdapterParams->DeviceNumber,
                pAdapterParams->FunctionNumber);
            fprintf(stderr, "########################################\n\n");

            IMetricsDeviceLatest* pMetricsDevice = NULL;
            res = pAdapter->OpenMetricsDevice(&pMetricsDevice);
            if (res != CC_OK)
            {
                fprintf(stderr, "OpenMetricsDevice failed, res: %d, skipping adapter.\n", res);
                continue;
            }

            printMetricsForDevice(pMetricsDevice);

            res = pAdapter->CloseMetricsDevice(pMetricsDevice);
            if (res != CC_OK)
            {
                fprintf(stderr, "CloseMetricsDevice failed, res: %d\n", res);
            }
        }
    }

    res = pAdapterGroup->Close();
    if (res != CC_OK)
    {
        fprintf(stderr, "AdapterGroup->Close() failed, res: %d\n", res);
    }

    return true;
}

static bool printMetricsForLegacyDevice(void* pLibrary)
{
    TCompletionCode res = CC_OK;

    OpenMetricsDevice_fn            OpenMetricsDevice;
    CloseMetricsDevice_fn           CloseMetricsDevice;
    OpenMetricsDeviceFromFile_fn    OpenMetricsDeviceFromFile;

    OpenMetricsDevice = (OpenMetricsDevice_fn)GetFunctionAddress(pLibrary, "OpenMetricsDevice");
    if (OpenMetricsDevice == NULL)
    {
        fprintf(stderr, "Couldn't get pointer to OpenMetricsDevice!\n");
        return false;
    }

    CloseMetricsDevice = (CloseMetricsDevice_fn)GetFunctionAddress(pLibrary, "CloseMetricsDevice");
    if (CloseMetricsDevice == NULL)
    {
        fprintf(stderr, "Couldn't get pointer to CloseMetricsDevice!\n");
        return false;
    }

    IMetricsDeviceLatest* pMetricsDevice;
    res = OpenMetricsDevice(&pMetricsDevice);
    if (res != CC_OK)
    {
        fprintf(stderr, "OpenMetricsDevice failed, res: %d\n", res);
        return false;
    }

    TMetricsDeviceParams_1_0* deviceParams = pMetricsDevice->GetParams();
    if (NULL == deviceParams)
    {
        fprintf(stderr, "MetricsDevice->GetParams() returned NULL\n");
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
    else
    {
        printMetricsForDevice(pMetricsDevice);
    }

    res = CloseMetricsDevice(pMetricsDevice);
    if (res != CC_OK)
    {
        fprintf(stderr, "CloseMetricsDevice failed, res: %d\n", res);
    }

    return true;
}

static bool printMetricsHelper()
{
    TCompletionCode res = CC_OK;

    void* pLibrary = OpenLibrary();
    if (pLibrary == NULL)
    {
        fprintf(stderr, "Couldn't load metrics discovery library!\n");
        return false;
    }

    bool success = printMetricsForAdapterGroup(pLibrary);
    if (!success)
    {
        success = printMetricsForLegacyDevice(pLibrary);
    }

    return success;
}

};

static void printMetrics()
{
    MetricsDiscovery::printMetricsHelper();
}
