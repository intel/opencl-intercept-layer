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
    static MDHelper* Create(
        const std::string& metricSetSymbolName,
        const std::string& metricsFileName );
    static void Delete( MDHelper*& pMDHelper );

    uint32_t GetMetricsConfiguration();
    uint32_t GetQueryReportSize();

    bool    ActivateMetricSet();
    void    DeactivateMetricSet();

    void    SetMetricSetFiltering(
                TMetricApiType apiMask );

    void    GetMetricsFromReport(
                const char* pData,
                std::vector<TTypedValue_1_0>& results,
				std::vector<TTypedValue_1_0>& maxValues,
				bool printMax);

	void	PrintMetricUnits(
				std::ostream& os, bool printMax );
    void    PrintMetricNames(
                std::ostream& os, bool printMax );
    void    PrintMetricValues(
                std::ostream& os,
                const std::string& name,
                const std::vector<TTypedValue_1_0>& results,
                const std::vector<TTypedValue_1_0>& maxValues,
                bool printMax);

    void    AggregateMetrics(
                CMetricAggregations& aggregations,
                const std::string& name,
                const std::vector<TTypedValue_1_0>& results );

private:
    MDHelper();
    ~MDHelper();

    bool InitMetricsDiscovery(
        const std::string& metricSetSymbolName,
        const std::string& metricsFileName );

    void    PrintValue(
                std::ostream& os,
                const TTypedValue_1_0& value );

    TTypedValue_1_0* GetGlobalSymbolValue(
        const char*         symbolName );

    static uint64_t CastToUInt64(TTypedValue_1_0 value );

    OpenMetricsDevice_fn            OpenMetricsDevice;
    CloseMetricsDevice_fn           CloseMetricsDevice;
    OpenMetricsDeviceFromFile_fn    OpenMetricsDeviceFromFile;

    bool                    m_Initialized;
    uint32_t                m_APIMask;
    uint32_t                m_CategoryMask;

    IMetricsDevice_1_5*     m_MetricsDevice;
    IMetricSet_1_1*         m_MetricSet;

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
