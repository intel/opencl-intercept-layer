/*
// Copyright (c) 2018-2024 Intel Corporation
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
        const bool includeMaxValues );
    static MDHelper* CreateTBS(
        const std::string& metricsLibraryName,
        const std::string& metricSetSymbolName,
        const std::string& metricsFileName,
        const bool includeMaxValues );
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
                std::vector<TTypedValue_1_0>& results,
                std::vector<TTypedValue_1_0>& maxValues );
    void    GetIOMeasurementInformation(
                std::vector<TTypedValue_1_0>& ioInfoValues );

    void    OpenStream(
                uint32_t timerPeriod,
                uint32_t bufferSize,
                uint32_t pid );
    bool    SaveReportsFromStream( void );
    uint32_t GetMetricsFromSavedReports(
                std::vector<TTypedValue_1_0>& results,
                std::vector<TTypedValue_1_0>& maxValues );
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
                const std::vector<TTypedValue_1_0>& results,
                const std::vector<TTypedValue_1_0>& maxValues,
                const std::vector<TTypedValue_1_0>& ioInfoValues );

    void    AggregateMetrics(
                CMetricAggregations& aggregations,
                const std::string& name,
                const std::vector<TTypedValue_1_0>& results );

private:
    MDHelper(uint32_t apiMask);
    ~MDHelper();

    bool InitMetricsDiscovery(
        const std::string& metricsLibraryName,
        const std::string& metricSetSymbolName,
        const std::string& metricsFileName,
        const bool includeMaxValues );

    void    PrintValue(
                std::ostream& os,
                const TTypedValue_1_0& value );

    TTypedValue_1_0* GetGlobalSymbolValue(
                const char* symbolName );

    static uint64_t CastToUInt64(TTypedValue_1_0 value );

    OpenMetricsDevice_fn            OpenMetricsDevice;
    CloseMetricsDevice_fn           CloseMetricsDevice;
    OpenMetricsDeviceFromFile_fn    OpenMetricsDeviceFromFile;

    bool                    m_Initialized;
    bool                    m_Activated;
    bool                    m_IncludeMaxValues;
    uint32_t                m_APIMask;
    uint32_t                m_CategoryMask;

    IMetricsDevice_1_5*     m_MetricsDevice;
    IConcurrentGroup_1_1*   m_ConcurrentGroup;
    IMetricSet_1_1*         m_MetricSet;

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
