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

#include "MetricsDiscoveryHelper.h"

#include "common.h"

#include <algorithm>
#include <stack>
#include <string>

#include <stdio.h>

// Enables logs:
//#define MD_DEBUG

#if defined(_WIN32)
static const wchar_t* cMDLibFileName =
    sizeof(void*) == 8 ?
    L"igdmd64.dll" :
    L"igdmd32.dll";

#include "DriverStorePath.h"

#define OpenLibrary(_filename)              (void*)LoadDynamicLibrary( _filename )
#define GetFunctionAddress(_handle, _name)  GetProcAddress((HMODULE)_handle, _name)

#elif defined(__linux__) || defined(__APPLE__)
static const char* cMDLibFileName = "libmd.so";

#include <dlfcn.h>
#include <stdarg.h>
#include <string.h>

#define OpenLibrary(_filename)              dlopen( _filename, RTLD_LAZY | RTLD_LOCAL )
#define GetFunctionAddress(_handle, _name)  dlsym(_handle, _name)
#define GetLastError()                      dlerror()
#define OutputDebugString(_buf)             fprintf(stderr, "%s\n", _buf);

#endif

namespace MetricsDiscovery
{

#ifdef MD_DEBUG
/************************************************************************/
/* DebugPrint                                                           */
/************************************************************************/
static void DebugPrint(const char* formatString, ...)
{
    const int BUFF_SIZE = 1024;
    const int FORMAT_SIZE = 512;

    va_list ap;
    char buf[BUFF_SIZE];
    char format[FORMAT_SIZE];

    CLI_SPRINTF(format, FORMAT_SIZE, "MDAPI Helper: %s", formatString);
    va_start(ap, formatString);
    CLI_VSPRINTF(buf, BUFF_SIZE, format, ap);
    OutputDebugString(buf);
    va_end(ap);
}
#else
#define DebugPrint(...)
#endif // MD_DEBUG

/************************************************************************/
/* MDHelper constructor                                                 */
/************************************************************************/
MDHelper::MDHelper() :
    m_Initialized (false ),
    m_APIMask( API_TYPE_OCL ),
    m_CategoryMask( GPU_RENDER | GPU_COMPUTE | GPU_MEDIA | GPU_GENERIC ),
    m_MetricsDevice( NULL ),
    m_MetricSet( NULL )
{
}

/************************************************************************/
/* ~MDHelper                                                            */
/************************************************************************/
MDHelper::~MDHelper()
{
    if(CloseMetricsDevice != NULL && m_MetricsDevice)
    {
        CloseMetricsDevice( m_MetricsDevice );
        m_MetricsDevice = NULL;
    }

    m_Initialized = false;
}

/************************************************************************/
/* Create                                                               */
/************************************************************************/
MDHelper* MDHelper::Create(
    const std::string& metricSetSymbolName,
    const std::string& metricsFileName )
{
    MDHelper*   pMDHelper = new MDHelper();
    if( pMDHelper )
    {
        if( pMDHelper->InitMetricsDiscovery(
                metricSetSymbolName,
                metricsFileName ) == false )
        {
            Delete( pMDHelper );
        }
    }
    return pMDHelper;
}

/************************************************************************/
/* Delete                                                               */
/************************************************************************/
void MDHelper::Delete( MDHelper*& pMDHelper )
{
    delete pMDHelper;
    pMDHelper = NULL;
}

/************************************************************************/
/* InitMetricsDiscovery                                                 */
/************************************************************************/
bool MDHelper::InitMetricsDiscovery(
    const std::string& metricSetSymbolName,
    const std::string& metricsFileName )
{
    m_Initialized = false;

    CloseMetricsDevice = NULL;
    OpenMetricsDevice = NULL;
    OpenMetricsDeviceFromFile = NULL;

    TCompletionCode res = CC_OK;

    if (m_APIMask & API_TYPE_IOSTREAM)
    {
        DebugPrint("API type must not be IOSTREAM.");
        return false;
    }

    void* pLibrary = OpenLibrary(cMDLibFileName);
    if (pLibrary == NULL)
    {
        DebugPrint("Couldn't load metrics discovery library!");
        return false;
    }

    CloseMetricsDevice = (CloseMetricsDevice_fn)GetFunctionAddress(pLibrary, "CloseMetricsDevice");
    if (CloseMetricsDevice == NULL)
    {
        DebugPrint("CloseMetricsDevice NULL, error: %d", GetLastError());
        return false;
    }

    OpenMetricsDevice = (OpenMetricsDevice_fn)GetFunctionAddress(pLibrary, "OpenMetricsDevice");
    if (OpenMetricsDevice == NULL)
    {
        DebugPrint("OpenMetricsDevice NULL, error: %d", GetLastError());
        return false;
    }

    OpenMetricsDeviceFromFile = (OpenMetricsDeviceFromFile_fn)GetFunctionAddress(pLibrary, "OpenMetricsDeviceFromFile");
    if (OpenMetricsDeviceFromFile == NULL)
    {
        DebugPrint("OpenMetricsDeviceFromFile NULL, error: %d", GetLastError());
        return false;
    }

    if (!metricsFileName.empty())
    {
        res = OpenMetricsDeviceFromFile(metricsFileName.c_str(), (void*)"", &m_MetricsDevice);
        if (res != CC_OK)
        {
            DebugPrint("OpenMetricsDeviceFromFile failed, res: %d", res);
            return false;
        }
    }
    else
    {
        res = OpenMetricsDevice(&m_MetricsDevice);
        if (res != CC_OK)
        {
            DebugPrint("OpenMetricsDevice failed, res: %d", res);
            return false;
        }
    }

    TMetricsDeviceParams_1_0* deviceParams = m_MetricsDevice->GetParams();
    if (NULL == deviceParams)
    {
        DebugPrint("DeviceParams null");
        return false;
    }

    DebugPrint("MDAPI Headers: v%d.%d.%d, MDAPI Lib: v%d.%d.%d\n",
        MD_API_MAJOR_NUMBER_CURRENT,
        MD_API_MINOR_NUMBER_CURRENT,
        MD_API_BUILD_NUMBER_CURRENT,
        deviceParams->Version.MajorNumber,
        deviceParams->Version.MinorNumber,
        deviceParams->Version.BuildNumber);
    if( deviceParams->Version.MajorNumber < 1 ||
        ( deviceParams->Version.MajorNumber == 1 && deviceParams->Version.MinorNumber < 1 ) )
    {
        DebugPrint("MDAPI Lib version must be at least v1.1!\n");
        return false;
    }

    bool found = false;
    for( uint32_t cg = 0; !found && cg < deviceParams->ConcurrentGroupsCount; cg++ )
    {
        IConcurrentGroup_1_1 *group = m_MetricsDevice->GetConcurrentGroup(cg);
        TConcurrentGroupParams_1_0* groupParams = group->GetParams();
        if( groupParams )
        {
            for( uint32_t ms = 0; !found && ms < groupParams->MetricSetsCount; ms++)
            {
                IMetricSet_1_1* metricSet = group->GetMetricSet(ms);
                TMetricSetParams_1_0* setParams = metricSet->GetParams();

                if( setParams &&
                    ( setParams->ApiMask & m_APIMask ) &&
                    ( setParams->CategoryMask & m_CategoryMask ) &&
                    ( metricSetSymbolName == setParams->SymbolName ) )
                {
                    DebugPrint("Matched Group: %s MetricSet: %s MetricCount: %d API: %X, Category: %X\n",
                        groupParams->SymbolName,
                        setParams->SymbolName,
                        setParams->MetricsCount,
                        setParams->ApiMask,
                        setParams->CategoryMask );

                    found = true;
                    m_MetricSet = metricSet;
                }
                else if( setParams )
                {
                    DebugPrint("Skipped Group: %s MetricSet: %s MetricCount: %d API: %X, Category: %X\n",
                        groupParams->SymbolName,
                        setParams->SymbolName,
                        setParams->MetricsCount,
                        setParams->ApiMask,
                        setParams->CategoryMask );
                }
            }
        }
    }

    if (m_MetricSet == NULL)
    {
        DebugPrint("MetricSet is null\n");
        return false;
    }

    m_MetricSet->SetApiFiltering( m_APIMask );

    m_Initialized = true;

    DebugPrint("MetricsDiscoveryInit End\n");
    return m_Initialized;
}

/************************************************************************/
/* ActivateMetricSet                                                    */
/************************************************************************/
bool MDHelper::ActivateMetricSet()
{
    if( !m_Initialized || !m_MetricSet )
    {
        DebugPrint("Can't ActivateMetricSet!\n");
        return false;
    }

    TCompletionCode res = m_MetricSet->Activate();
    if( res != CC_OK ) { DebugPrint("ActivateMetricSet failed!\n"); }

    return res == CC_OK;
}

/************************************************************************/
/* DeactivateMetricSet                                                  */
/************************************************************************/
void MDHelper::DeactivateMetricSet()
{
    if( !m_Initialized || !m_MetricSet )
    {
        DebugPrint("Can't DeactivateMetricSet!\n");
        return;
    }

    TCompletionCode res = m_MetricSet->Deactivate();
    if( res != CC_OK ) { DebugPrint("DeactivateMetricSet failed!\n"); }
}

/************************************************************************/
/* SetMetricSetFiltering                                                */
/************************************************************************/
void MDHelper::SetMetricSetFiltering( TMetricApiType apiMask )
{
    if( !m_Initialized || !m_MetricSet )
    {
        DebugPrint("Can't SetMetricSetFiltering!\n");
        return;
    }

    TCompletionCode res = m_MetricSet->SetApiFiltering( apiMask );
    if( res != CC_OK ) { DebugPrint("SetMetricSetFiltering failed!\n"); }
}

/************************************************************************/
/* GetMetricsFromReport                                                 */
/************************************************************************/
void MDHelper::GetMetricsFromReport(
    const char* pReportData,
    std::vector<TTypedValue_1_0>& results,
	std::vector<TTypedValue_1_0>* maxValues)
{
    if( !m_Initialized || !m_MetricSet )
    {
        DebugPrint("Can't GetMetricsFromReport!\n");
        return;
    }

    const uint32_t reportSize       = m_MetricSet->GetParams()->QueryReportSize;

    const uint32_t metricsCount     = m_MetricSet->GetParams()->MetricsCount;
    const uint32_t informationCount = m_MetricSet->GetParams()->InformationCount;

    results.resize( metricsCount + informationCount );
	if ( maxValues != NULL ) (*maxValues).resize(metricsCount);

    uint32_t    outReportCount = 0;
	TCompletionCode res = MetricsDiscovery::CC_ERROR_GENERAL;
	if( maxValues != NULL )
    res = ((MetricsDiscovery::IMetricSet_1_5*)m_MetricSet)->CalculateMetrics(
        (const unsigned char*)pReportData,
        reportSize,
        results.data(),
        (uint32_t)(results.size() * sizeof(TTypedValue_1_0)),
        &outReportCount,
        (*maxValues).data(),
		(uint32_t)((*maxValues).size() * sizeof(TTypedValue_1_0))
	);
	else
	res = ((MetricsDiscovery::IMetricSet_1_1*)m_MetricSet)->CalculateMetrics(
		(const unsigned char*)pReportData,
		reportSize,
		results.data(),
		(uint32_t)(results.size() * sizeof(TTypedValue_1_0)),
		&outReportCount,
		false
	);
    if( res != CC_OK ) DebugPrint("CalculateMetrics failed!\n");
}

/************************************************************************/
/* PrintMetricNames                                                     */
/************************************************************************/
void MDHelper::PrintMetricNames( std::ostream& os , bool printMax )
{
    if( !m_Initialized || !m_MetricSet || !os.good() )
    {
        DebugPrint("Can't PrintMetricNames!\n");
        return;
    }

    os << "kernel,";

    for( uint32_t i = 0; i < m_MetricSet->GetParams( )->MetricsCount; i++ )
    {
        os << m_MetricSet->GetMetric( i )->GetParams()->SymbolName << ",";
		if(printMax) os << "max_" << m_MetricSet->GetMetric( i )->GetParams()->SymbolName << ",";
    }

    os << ",";

    for(uint32_t i = 0; i < m_MetricSet->GetParams()->InformationCount; i++)
    {
        os << m_MetricSet->GetInformation( i )->GetParams()->SymbolName << ",";
    }

    os << std::endl;
}

/************************************************************************/
/* PrintMetricUnits                                                 */
/************************************************************************/
void MDHelper::PrintMetricUnits(std::ostream& os, bool printMax )
{
	if (!m_Initialized || !m_MetricSet || !os.good()) return;

	os << " ,";

	for (uint32_t i = 0; i < m_MetricSet->GetParams()->MetricsCount; i++)
	{
		const char* unit = m_MetricSet->GetMetric(i)->GetParams()->MetricResultUnits;
		if (unit == NULL)
			os << (printMax ? " , ," : " ,");
		else os << unit << (printMax ? ", ," : ",");
	}

	os << ",";

	for (uint32_t i = 0; i < m_MetricSet->GetParams()->InformationCount; i++)
	{
		const char* unit = m_MetricSet->GetInformation(i)->GetParams()->InfoUnits;
		if (unit == NULL)
			os << " ,";
		else os << unit << ",";
	}

	os << std::endl;
}

/************************************************************************/
/* PrintMetricValues                                                    */
/************************************************************************/
void MDHelper::PrintMetricValues(
    std::ostream& os,
    const std::string& name,
    const std::vector<TTypedValue_1_0>& results,
	const std::vector<TTypedValue_1_0>* maxValues)
{
    if( !m_Initialized || !m_MetricSet || !os.good() )
    {
        DebugPrint("Can't PrintMetricValues!\n");
        return;
    }

    os << name << ",";

    uint32_t metricsCount = m_MetricSet->GetParams()->MetricsCount;
    for( uint32_t i = 0; i < metricsCount; i++ )
    {
        PrintValue( os, results[ i ] );
		if( maxValues != NULL ) PrintValue( os, (*maxValues)[ i ] );
    }

    os << ",";

    for( uint32_t i = 0; i < m_MetricSet->GetParams()->InformationCount; i++ )
    {
        PrintValue( os, results[ metricsCount + i ] );
    }

    os << std::endl;
}

/************************************************************************/
/* AggregateMetrics                                                     */
/************************************************************************/
void MDHelper::AggregateMetrics(
    CMetricAggregations& aggregations,
    const std::string& name,
    const std::vector<TTypedValue_1_0>& results )
{
    if( !m_Initialized || !m_MetricSet )
    {
        DebugPrint("Can't AggregateMetrics!\n");
        return;
    }

    CMetricAggregationsForKernel& kernelMetrics = aggregations[ name ];

    uint32_t metricsCount = m_MetricSet->GetParams()->MetricsCount;
    for( uint32_t i = 0; i < metricsCount; i++ )
    {
        TMetricParams_1_0* metricParams = m_MetricSet->GetMetric( i )->GetParams();

        // Find profile data for metric
        const char* metricName = metricParams->SymbolName;
        SMetricAggregationData& aggregationData =
            kernelMetrics[ metricName ];

        // Add data to metricData
        uint64_t value = CastToUInt64( results[ i ] );

        aggregationData.Count++;
        aggregationData.Sum += value;
        aggregationData.Min = std::min<uint64_t>( aggregationData.Min, value );
        aggregationData.Max = std::max<uint64_t>( aggregationData.Max, value );
    }
}

/************************************************************************/
/* PrintValue                                                           */
/************************************************************************/
void MDHelper::PrintValue( std::ostream& os, const TTypedValue_1_0& value )
{
    switch( value.ValueType )
    {
    case VALUE_TYPE_UINT64:
        os << value.ValueUInt64 << ",";
        break;

    case VALUE_TYPE_FLOAT:
        os << value.ValueFloat << ",";
        break;

    case VALUE_TYPE_BOOL:
        os << (value.ValueBool ? "TRUE" : "FALSE") << ",";
        break;

    case VALUE_TYPE_UINT32:
        os << value.ValueUInt32 << ",";
        break;

    default:
        CLI_ASSERT(false);
    }
}

/************************************************************************/
/* GetGlobalSymbolValue                                                 */
/************************************************************************/
TTypedValue_1_0* MDHelper::GetGlobalSymbolValue(
    const char* SymbolName )
{
    for( uint32_t i = 0; i < m_MetricsDevice->GetParams()->GlobalSymbolsCount; i++ )
    {
        TGlobalSymbol_1_0* symbol = m_MetricsDevice->GetGlobalSymbol( i );
        if( strcmp( symbol->SymbolName, SymbolName ) == 0 )
        {
            return &( symbol->SymbolTypedValue );
        }
    }
    return NULL;
}

/************************************************************************/
/* CastToUInt64                                                         */
/************************************************************************/
uint64_t MDHelper::CastToUInt64(TTypedValue_1_0 value)
{
    switch( value.ValueType )
    {
    case VALUE_TYPE_BOOL:
        return ( value.ValueBool ) ? 1LL : 0LL;

    case VALUE_TYPE_UINT32:
        return (uint64_t)value.ValueUInt32;

    case VALUE_TYPE_UINT64:
        return value.ValueUInt64;

    case VALUE_TYPE_FLOAT:
        return (uint64_t)value.ValueFloat;

    default:
        CLI_ASSERT( false );
        break;
    }

    return 0;
}

}
