/*
// Copyright (c) 2018-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#include "metrics_discovery_api.h"

#include <ostream>
#include <map>
#include <vector>

#include <limits.h>
#include <stdint.h>

namespace MetricsDiscovery
{

struct SMetricAggregationData
{
    SMetricAggregationData() :
        Count(0),
        Sum(0),
        Min(ULLONG_MAX),
        Max(0) {}

    uint64_t Count;
    uint64_t Sum;
    uint64_t Min;
    uint64_t Max;
};

// This is a map of a metric name to sum/min/max/count data:
typedef std::map<const std::string, SMetricAggregationData> CMetricAggregationsForKernel;

// This is a map of kernel names to aggregated metrics:
typedef std::map<const std::string, CMetricAggregationsForKernel> CMetricAggregations;

class MDHelper
{
public:
    static MDHelper* CreateEBS(
        const std::string& metricsLibraryName,
        const std::string& metricSetSymbolName,
        const std::string& metricsFileName,
        uint32_t adapterIndex,
        bool includeMaxValues );
    static MDHelper* CreateTBS(
        const std::string& metricsLibraryName,
        const std::string& metricSetSymbolName,
        const std::string& metricsFileName,
        uint32_t adapterIndex,
        bool includeMaxValues );
    static void Delete( MDHelper*& pMDHelper );

    uint32_t GetMetricsConfiguration();
    uint32_t GetQueryReportSize();

    bool    ActivateMetricSet();
    void    DeactivateMetricSet();

    void    SetMetricSetFiltering(
                TMetricApiType apiMask );

    uint32_t GetMetricsFromReports(
                const uint32_t numReports,
                const char* pData,
                std::vector<TTypedValueLatest>& results,
                std::vector<TTypedValueLatest>& maxValues );
    void    GetIOMeasurementInformation(
                std::vector<TTypedValueLatest>& ioInfoValues );

    void    OpenStream(
                uint32_t timerPeriod,
                uint32_t bufferSize,
                uint32_t pid );
    bool    SaveReportsFromStream( void );
    uint32_t GetMetricsFromSavedReports(
                std::vector<TTypedValueLatest>& results,
                std::vector<TTypedValueLatest>& maxValues );
    void    ResetSavedReports( void );
    void    CloseStream( void );

    void    PrintMetricNames(
                std::ostream& os );
    void    PrintMetricUnits(
                std::ostream& os );

    void    PrintMetricValues(
                std::ostream& os,
                const std::string& name,
                const uint32_t numResults,
                const std::vector<TTypedValueLatest>& results,
                const std::vector<TTypedValueLatest>& maxValues,
                const std::vector<TTypedValueLatest>& ioInfoValues );

    void    AggregateMetrics(
                CMetricAggregations& aggregations,
                const std::string& name,
                const std::vector<TTypedValueLatest>& results );

private:
    MDHelper(uint32_t apiMask);
    ~MDHelper();

    bool InitMetricsDiscovery(
        const std::string& metricsLibraryName,
        const std::string& metricSetSymbolName,
        const std::string& metricsFileName,
        uint32_t adapterIndex,
        bool includeMaxValues );

    bool InitMetricsDiscoveryAdapterGroup(
        const std::string& metricSetSymbolName,
        const std::string& metricsFileName,
        uint32_t adapterIndex );
    bool InitMetricsDiscoveryLegacy(
        const std::string& metricSetSymbolName,
        const std::string& metricsFileName );

    bool FindMetricSetForDevice(
        IMetricsDeviceLatest* pMetricsDevice,
        const std::string& metricSetSymbolName );

    void    PrintValue(
                std::ostream& os,
                const TTypedValueLatest& value );

    TTypedValueLatest* GetGlobalSymbolValue(
                const char* symbolName );

    static uint64_t CastToUInt64(TTypedValueLatest value );

    OpenAdapterGroup_fn             OpenAdapterGroup;
    OpenMetricsDevice_fn            OpenMetricsDevice;
    OpenMetricsDeviceFromFile_fn    OpenMetricsDeviceFromFile;
    CloseMetricsDevice_fn           CloseMetricsDevice;

    bool                    m_Initialized;
    bool                    m_Activated;
    bool                    m_IncludeMaxValues;
    uint32_t                m_APIMask;
    uint32_t                m_CategoryMask;

    IAdapterGroupLatest*    m_AdapterGroup;
    IAdapterLatest*         m_Adapter;
    IMetricsDeviceLatest*   m_MetricsDevice;
    IConcurrentGroupLatest* m_ConcurrentGroup;
    IMetricSetLatest*       m_MetricSet;

    // Report data for time based sampling:
    std::vector<char>       m_SavedReportData;
    uint32_t                m_NumSavedReports;

private:
    MDHelper(MDHelper const&);
    void operator=(MDHelper const&);
};

/************************************************************************/
/* GetMetricsConfiguration                                              */
/************************************************************************/
inline uint32_t MDHelper::GetMetricsConfiguration()
{
    return ( m_MetricSet != NULL ) ? m_MetricSet->GetParams()->ApiSpecificId.OCL : 0;
}

/************************************************************************/
/* GetQueryReportSize                                                   */
/************************************************************************/
inline uint32_t MDHelper::GetQueryReportSize()
{
    return ( m_MetricSet != NULL ) ? m_MetricSet->GetParams()->QueryReportSize : 0;
}

}
