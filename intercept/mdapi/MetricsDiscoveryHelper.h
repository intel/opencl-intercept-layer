/*
// Copyright (c) 2018-2026 Intel Corporation
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

    std::vector<char>&  GetWorkingReportData();

    uint32_t GetMetricsConfiguration();

    bool    ActivateMetricSet();
    void    DeactivateMetricSet();

    void    SetMetricSetFiltering(
                TMetricApiType apiMask );

    uint32_t GetMetricsFromReport();
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
                const uint32_t numResults );
    void    PrintMetricValues(
                std::ostream& os,
                const std::string& name,
                const uint32_t numResults,
                const std::vector<TTypedValueLatest>& results,
                const std::vector<TTypedValueLatest>& maxValues,
                const std::vector<TTypedValueLatest>& ioInfoValues );

    void    AggregateMetrics(
                CMetricAggregations& aggregations,
                const std::string& name );
    void    AggregateMetrics(
                CMetricAggregations& aggregations,
                const std::string& name,
                const std::vector<TTypedValueLatest>& results );

private:
    MDHelper(uint32_t apiMask);
    ~MDHelper();
    MDHelper(MDHelper const&) = delete;
    MDHelper& operator=(MDHelper const&) = delete;

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

    // Working data for event based sampling:
    std::vector<char>       m_WorkingReportData;    // passed to clGetEventProfilingInfo
    std::vector<TTypedValueLatest>  m_WorkingResults;
    std::vector<TTypedValueLatest>  m_WorkingMaxValues;

    // Report data for time based sampling:
    std::vector<char>       m_SavedReportData;
    uint32_t                m_NumSavedReports;
};

/************************************************************************/
/* GetWorkingReportData                                                 */
/************************************************************************/
inline std::vector<char>& MDHelper::GetWorkingReportData()
{
    return m_WorkingReportData;
}

/************************************************************************/
/* GetMetricsConfiguration                                              */
/************************************************************************/
inline uint32_t MDHelper::GetMetricsConfiguration()
{
    return ( m_MetricSet != NULL ) ? m_MetricSet->GetParams()->ApiSpecificId.OCL : 0;
}

}
