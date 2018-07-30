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

// Enables logs
#define MD_DEBUG 1

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
    m_Initialized(false),
    m_APIMask( API_TYPE_OCL | API_TYPE_OGL | API_TYPE_DX12 ),
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
    DebugPrint("MDHelper Destructor");

    if(CloseMetricsDevice != NULL && m_MetricsDevice)
    {
        CloseMetricsDevice( m_MetricsDevice );
        m_MetricsDevice = NULL;
    }

    m_Initialized = false;
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

    if (m_APIMask & API_TYPE_IOSTREAM )
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

    {
        TTypedValue_1_0* euCores = GetGlobalSymbolValue("EuCoresTotalCount");
        if (euCores == NULL)
        {
            DebugPrint("EuCoresTotalCount null, maybe support_enable");
            return false;
        }
        m_EUCoresCount = euCores->ValueUInt32;
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

    m_Initialized = true;

    DebugPrint("MetricsDiscoveryInit End\n");
    return m_Initialized;
}

/************************************************************************/
/* ActivateMetricSet                                                    */
/************************************************************************/
void MDHelper::ActivateMetricSet()
{
    if( !m_Initialized || !m_MetricSet ) return;

    TCompletionCode res = m_MetricSet->Activate();
    if( res != CC_OK ) DebugPrint("ActivateMetricSet failed!\n");
}

/************************************************************************/
/* DeactivateMetricSet                                                  */
/************************************************************************/
void MDHelper::DeactivateMetricSet()
{
    if( !m_Initialized || !m_MetricSet ) return;

    TCompletionCode res = m_MetricSet->Deactivate();
    if( res != CC_OK ) DebugPrint("DeactivateMetricSet failed!\n");
}

/************************************************************************/
/* SetMetricSetFiltering                                                */
/************************************************************************/
void MDHelper::SetMetricSetFiltering( TMetricApiType apiMask )
{
    if( !m_Initialized || !m_MetricSet ) return;

    TCompletionCode res = m_MetricSet->SetApiFiltering( apiMask );
    if( res != CC_OK ) DebugPrint("SetMetricSetFiltering failed!\n");
}

/************************************************************************/
/* GetMetricsFromReport                                                 */
/************************************************************************/
void MDHelper::GetMetricsFromReport(
    const char* pReportData,
    std::vector<TTypedValue_1_0>& results,
    std::vector<TTypedValue_1_0>& information )
{
    if( !m_Initialized || !m_MetricSet ) return;

    const uint32_t metricsCount     = m_MetricSet->GetParams()->MetricsCount;
    const uint32_t informationCount = m_MetricSet->GetParams()->InformationCount;

    std::vector<TTypedValue_1_0>    deltaValues;

    deltaValues.resize( metricsCount );
    results.resize( metricsCount );
    information.resize( informationCount );

    // Read metrics from the report data and normalize:
    ReadMetrics( pReportData, deltaValues );
    NormalizeMetrics( deltaValues, results );

    // Read informational from the report data:
    ReadInformation( pReportData, information );
}

/************************************************************************/
/* PrintMetricNames                                                     */
/************************************************************************/
void MDHelper::PrintMetricNames( std::ostream& os )
{
    if( !m_Initialized || !m_MetricSet || !os.good() ) return;

    os << "kernel,";

    for( uint32_t i = 0; i < m_MetricSet->GetParams( )->MetricsCount; i++ )
    {
        // Skip if not supported
        if ( 0 == ( m_MetricSet->GetMetric( i )->GetParams()->ApiMask & m_APIMask )) continue;
        os << m_MetricSet->GetMetric( i )->GetParams()->SymbolName << ",";
    }

    os << ",";

    for(uint32_t i = 0; i < m_MetricSet->GetParams()->InformationCount; i++)
    {
        // Skip if not supported
        if ( 0 == ( m_MetricSet->GetInformation( i )->GetParams()->ApiMask & m_APIMask )) continue;
        os << m_MetricSet->GetInformation( i )->GetParams()->SymbolName << ",";
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
    const std::vector<TTypedValue_1_0>& information )
{
    if( !m_Initialized || !m_MetricSet || !os.good() ) return;

    os << name << ",";

    uint32_t metricsCount = m_MetricSet->GetParams()->MetricsCount;
    for( uint32_t i = 0; i < metricsCount; i++ )
    {
        // Skip if not supported
        if ( 0 == ( m_MetricSet->GetMetric( i )->GetParams()->ApiMask & m_APIMask )) continue;

        PrintValue( os, results[ i ] );

    }

    os << ",";

    for( uint32_t i = 0; i < m_MetricSet->GetParams()->InformationCount; i++ )
    {
        // Skip if not supported
        if ( 0 == ( m_MetricSet->GetInformation( i )->GetParams()->ApiMask & m_APIMask )) continue;

        PrintValue( os, information[ i ] );
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
    if( !m_Initialized || !m_MetricSet ) return;

    CMetricAggregationsForKernel& kernelMetrics = aggregations[ name ];

    uint32_t metricsCount = m_MetricSet->GetParams()->MetricsCount;
    for( uint32_t i = 0; i < metricsCount; i++ )
    {
        TMetricParams_1_0* metricParams = m_MetricSet->GetMetric( i )->GetParams();

        // Skip if not supported
        if( 0 == ( metricParams->ApiMask & m_APIMask ) ) continue;

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
/* ReadMetrics                                                          */
/************************************************************************/
void MDHelper::ReadMetrics(
    const char* pReportData,
    std::vector<TTypedValue_1_0>& deltaValues )
{
    if( !pReportData ) return;

    uint32_t metricsCount = m_MetricSet->GetParams()->MetricsCount;
    m_GPUCoreClocks             = 0;

    for( uint32_t i = 0; i < metricsCount; i++ )
    {
        TMetricParams_1_0* metricParams = m_MetricSet->GetMetric( i )->GetParams();
        if ( 0 == ( metricParams->ApiMask & m_APIMask )) continue;

        IEquation_1_0* equation = metricParams->QueryReadEquation;

        if( equation )
        {
            deltaValues[ i ] = CalculateReadEquation( equation, pReportData );
        }
        else
        {
            deltaValues[ i ].ValueType = VALUE_TYPE_UINT64;
            deltaValues[ i ].ValueUInt64 = 0ULL;
        }

        if( strcmp( metricParams->SymbolName, "GpuCoreClocks" ) == 0 )
        {
            m_GPUCoreClocks = deltaValues[ i ].ValueUInt64;
        }
    }
}

/************************************************************************/
/* NormalizeMetrics                                                     */
/************************************************************************/
void MDHelper::NormalizeMetrics(
    std::vector<TTypedValue_1_0>& deltaValues,
    std::vector<TTypedValue_1_0>& results )
{
    uint32_t metricsCount = m_MetricSet->GetParams()->MetricsCount;

    for( uint32_t i = 0; i < metricsCount; i++ )
    {
        TMetricParams_1_0* metricParams = m_MetricSet->GetMetric( i )->GetParams();
        if ( 0 == ( metricParams->ApiMask & m_APIMask )) continue;

        IEquation_1_0* normEquation = metricParams->NormEquation;

        if ( normEquation )
        {
            // do final calculation, may refer to global symbols, local delta results and local normalization results
            results[ i ] = CalculateLocalNormalizationEquation(
                normEquation,
                deltaValues.data(),
                results.data(),
                i );
        }
        else
        {
            results[ i ] = deltaValues[ i ];
        }
    }
}

/************************************************************************/
/* ReadInformation                                                      */
/************************************************************************/
void MDHelper::ReadInformation(
    const char* pReportData,
    std::vector<TTypedValue_1_0>& results )
{
    if( !pReportData ) return;

    uint32_t informationCount = m_MetricSet->GetParams()->InformationCount;

    for( uint32_t i = 0; i < informationCount; i++ )
    {
        IInformation_1_0* information = m_MetricSet->GetInformation( i );
        ReadSingleInformation( pReportData, information, &results[ i ] );
    }
}

/************************************************************************/
/* ReadSingleInformation                                                */
/************************************************************************/
void MDHelper::ReadSingleInformation(
    const char* pReportData,
    IInformation_1_0* information,
    TTypedValue_1_0* result )
{
    if( !pReportData || !information || !result ) return;

    TInformationParams_1_0* informationParams = information->GetParams();
    if ( 0 == ( informationParams->ApiMask & m_APIMask )) return;

    IEquation_1_0* equation = informationParams->QueryReadEquation;

    if( equation )
    {
        *result = CalculateReadEquation( equation, pReportData );
    }
    else
    {
        result->ValueUInt64 = 0ULL;
    }

    if( informationParams->InfoType == INFORMATION_TYPE_FLAG )
    {
        result->ValueType = VALUE_TYPE_BOOL;
    }
    else
    {
        result->ValueType = VALUE_TYPE_UINT64;
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
    for( uint32_t i = 0; i < m_MetricsDevice->GetParams( )->GlobalSymbolsCount; i++ )
    {
        TGlobalSymbol_1_0* symbol = m_MetricsDevice->GetGlobalSymbol( i );
        if( strcmp( symbol->SymbolName, SymbolName ) == 0 )
        {
            return &( symbol->SymbolTypedValue );
        }
    }
    return NULL;
}

/*******************************************************************************/
/* CalculateReadEquation                                                       */
/*******************************************************************************/
TTypedValue_1_0 MDHelper::CalculateReadEquation(
    IEquation_1_0* equation,
    const char* pReportData )
{
    std::stack<TTypedValue_1_0> equationStack;
    TTypedValue_1_0 typedValue;

    for( uint32_t i = 0; i < equation->GetEquationElementsCount(); i++ )
    {
        TEquationElement_1_0* element = equation->GetEquationElement( i );
        switch( element->Type )
        {
        case EQUATION_ELEM_RD_BITFIELD:

            typedValue.ValueUInt64 = ReadBitfield( (const char *)(pReportData + element->ReadParams.ByteOffset),
                element->ReadParams.BitOffset, element->ReadParams.BitsCount );
            typedValue.ValueType   = VALUE_TYPE_UINT64;

            equationStack.push( typedValue );
            break;

        case EQUATION_ELEM_RD_UINT8:
            {
                uint8_t byteValue = *((const uint8_t *)(pReportData + element->ReadParams.ByteOffset));
                typedValue.ValueUInt64    = (uint64_t) byteValue;
                typedValue.ValueType      = VALUE_TYPE_UINT64;
            }
            equationStack.push( typedValue );
            break;

        case EQUATION_ELEM_RD_UINT16:
            {
                uint16_t shortValue = *((const uint16_t *)(pReportData + element->ReadParams.ByteOffset));
                typedValue.ValueUInt64      = (uint64_t) shortValue;
                typedValue.ValueType        = VALUE_TYPE_UINT64;
            }
            equationStack.push( typedValue );
            break;

        case EQUATION_ELEM_RD_UINT32:
            {
                uint32_t dwordValue = *((const uint32_t *)(pReportData + element->ReadParams.ByteOffset));
                typedValue.ValueUInt64  = (uint64_t) dwordValue;
                typedValue.ValueType    = VALUE_TYPE_UINT64;
            }
            equationStack.push( typedValue );
            break;

        case EQUATION_ELEM_RD_UINT64:
            typedValue.ValueUInt64 = *((const uint64_t *)(pReportData + element->ReadParams.ByteOffset));
            typedValue.ValueType   = VALUE_TYPE_UINT64;
            equationStack.push( typedValue );
            break;

        case EQUATION_ELEM_RD_FLOAT:
            typedValue.ValueFloat = *((const float *)(pReportData + element->ReadParams.ByteOffset));
            typedValue.ValueType  = VALUE_TYPE_FLOAT;
            equationStack.push( typedValue );
            break;

        case EQUATION_ELEM_RD_40BIT_CNTR:
            {
                typedValue.ValueUInt64Fields.Low = *((const uint32_t *)(pReportData + element->ReadParams.ByteOffset));
                typedValue.ValueUInt64Fields.High = (uint32_t)*((const unsigned char *)(pReportData + element->ReadParams.ByteOffsetExt));
                typedValue.ValueType = VALUE_TYPE_UINT64;
                equationStack.push( typedValue );
            }
            break;

        case EQUATION_ELEM_IMM_UINT64:
            typedValue.ValueUInt64 = element->ImmediateUInt64;
            typedValue.ValueType   = VALUE_TYPE_UINT64;
            equationStack.push( typedValue );
            break;

        case EQUATION_ELEM_IMM_FLOAT:
            typedValue.ValueFloat = element->ImmediateFloat;
            typedValue.ValueType   = VALUE_TYPE_FLOAT;
            equationStack.push( typedValue );
            break;

        case EQUATION_ELEM_GLOBAL_SYMBOL:
            {
                TTypedValue_1_0* pValue = GetGlobalSymbolValue( element->SymbolName );
                if( pValue )
                {
                    typedValue = *pValue;
                }
                else
                {
                    typedValue.ValueUInt64 = 0;
                    typedValue.ValueType   = VALUE_TYPE_UINT64;
                }
                equationStack.push( typedValue );
            }
            break;

        case EQUATION_ELEM_OPERATION:
            {
                //pop two values from stack
                TTypedValue_1_0 valueLast = equationStack.top();
                equationStack.pop();
                TTypedValue_1_0 valuePrev = equationStack.top();
                equationStack.pop();

                typedValue = CalculateEquationElemOperation( element->Operation, valuePrev, valueLast );
                equationStack.push( typedValue );
            }
            break;

        default:
            CLI_ASSERT( false );
            break;
        }
    }

    typedValue = equationStack.top();
    equationStack.pop();

    CLI_ASSERT( equationStack.empty() );

    return typedValue;
}

/************************************************************************/
/* CalculateLocalNormalizationEquation                                  */
/************************************************************************/
TTypedValue_1_0 MDHelper::CalculateLocalNormalizationEquation(
    IEquation_1_0* equation,        // can't be const but should
    TTypedValue_1_0* deltaValues,   // could be const
    TTypedValue_1_0* results,
    uint32_t metricIndex )
{
    TTypedValue_1_0 typedValue;

    std::stack<TTypedValue_1_0> equationStack;

    for( uint32_t i = 0; i < equation->GetEquationElementsCount(); i++ )
    {
        const TEquationElement_1_0* element = equation->GetEquationElement( i );
        switch( element->Type )
        {
        case EQUATION_ELEM_RD_BITFIELD:
        case EQUATION_ELEM_RD_UINT8:
        case EQUATION_ELEM_RD_UINT16:
        case EQUATION_ELEM_RD_UINT32:
        case EQUATION_ELEM_RD_UINT64:
        case EQUATION_ELEM_RD_FLOAT:
        case EQUATION_ELEM_RD_40BIT_CNTR:
            //not allowed in norm equation
            CLI_ASSERT( false );
            break;

        case EQUATION_ELEM_IMM_FLOAT:
            typedValue.ValueFloat = element->ImmediateFloat;
            typedValue.ValueType = VALUE_TYPE_FLOAT;
            equationStack.push( typedValue );

            break;

        case EQUATION_ELEM_IMM_UINT64:
            typedValue.ValueUInt64 = element->ImmediateUInt64;
            typedValue.ValueType = VALUE_TYPE_UINT64;
            equationStack.push( typedValue );
            break;

        case EQUATION_ELEM_SELF_COUNTER_VALUE:
            //get result of delta equation
            typedValue = deltaValues[ metricIndex ];
            equationStack.push( typedValue );
            break;

        case EQUATION_ELEM_LOCAL_COUNTER_SYMBOL:
            {
                bool found = false;
                for( uint32_t j = 0; j < m_MetricSet->GetParams()->MetricsCount; j++ )
                {
                    //find symbol by name in the set
                    IMetric_1_0* metric = m_MetricSet->GetMetric( j );
                    if( strcmp( element->SymbolName, metric->GetParams()->SymbolName ) == 0 )
                    {
                        found = true;
                        typedValue = deltaValues[ metric->GetParams()->IdInSet ];
                    }
                }
                if( !found )
                {
                    typedValue.ValueUInt64 = 0;
                    typedValue.ValueType   = VALUE_TYPE_UINT64;
                }
                equationStack.push( typedValue );
            }
            break;

        case EQUATION_ELEM_LOCAL_METRIC_SYMBOL:
            {
                bool found = false;
                for( uint32_t j = 0; j < metricIndex; j++ )
                {
                    //find symbol by name in the set
                    IMetric_1_0* metric = m_MetricSet->GetMetric( j );
                    if( strcmp( element->SymbolName, metric->GetParams()->SymbolName ) == 0 )
                    {
                        found = true;
                        typedValue = results[ metric->GetParams()->IdInSet ];
                    }
                }
                if( !found )
                {
                    typedValue.ValueUInt64 = 0;
                    typedValue.ValueType   = VALUE_TYPE_UINT64;
                }
                equationStack.push( typedValue );
            }
            break;

        case EQUATION_ELEM_GLOBAL_SYMBOL:
            {
                TTypedValue_1_0* pValue = GetGlobalSymbolValue( element->SymbolName );
                if( pValue )
                {
                    typedValue = *pValue;
                }
                else
                {
                    typedValue.ValueUInt64 = 0;
                    typedValue.ValueType   = VALUE_TYPE_UINT64;
                }
                equationStack.push( typedValue );
            }
            break;

        case EQUATION_ELEM_OPERATION:
            {
                TTypedValue_1_0 valueLast = equationStack.top();
                equationStack.pop();
                TTypedValue_1_0 valuePrev = equationStack.top();
                equationStack.pop();

                typedValue = CalculateEquationElemOperation( element->Operation, valuePrev, valueLast );
                equationStack.push( typedValue );
            }
            break;

        case EQUATION_ELEM_STD_NORM_GPU_DURATION:
            CLI_ASSERT( equationStack.empty() );

            if( m_GPUCoreClocks != 0 )
            {
                float self = CastToFloat( deltaValues[ metricIndex ] );
                float gpuCoreClocks = (float) m_GPUCoreClocks;

                typedValue.ValueFloat = 100.0f * self / gpuCoreClocks;
                typedValue.ValueType = VALUE_TYPE_FLOAT;
                return typedValue;
            }
            else
            {
                DebugPrint( "Waring: GpuCoreClocks is 0" );
                typedValue.ValueFloat = 0.0f;
                typedValue.ValueType = VALUE_TYPE_FLOAT;
                return typedValue;
            }

        case EQUATION_ELEM_STD_NORM_EU_AGGR_DURATION:
            CLI_ASSERT( equationStack.empty() );

            if( m_GPUCoreClocks != 0 )
            {
                float self = CastToFloat( deltaValues[ metricIndex ] );
                float gpuCoreClocks = (float) (m_GPUCoreClocks * m_EUCoresCount);

                typedValue.ValueFloat = 100.0f * self / gpuCoreClocks;
                typedValue.ValueType = VALUE_TYPE_FLOAT;
                return typedValue;
            }
            else
            {
                DebugPrint( "Warning: GpuCoreClocks is 0" );
                typedValue.ValueFloat = 0.0f;
                typedValue.ValueType = VALUE_TYPE_FLOAT;
                return typedValue;
            }

        default:
            CLI_ASSERT( false );
            break;
        }
    }

    typedValue = equationStack.top();
    equationStack.pop();

    CLI_ASSERT( equationStack.empty() );

    return typedValue;
}

/************************************************************************/
/* CalculateEquationElemOperation                                       */
/************************************************************************/
TTypedValue_1_0 MDHelper::CalculateEquationElemOperation(
    TEquationOperation operation,
    TTypedValue_1_0 valuePrev,
    TTypedValue_1_0 valueLast )
{
    TTypedValue_1_0 value;
    value.ValueUInt64 = 0;
    value.ValueType   = VALUE_TYPE_UINT64;

    switch( operation )
    {
    case EQUATION_OPER_AND:
        value.ValueUInt64 = CastToUInt64(valuePrev) & CastToUInt64(valueLast);
        value.ValueType = VALUE_TYPE_UINT64;
        break;

    case EQUATION_OPER_OR:
        value.ValueUInt64 = CastToUInt64(valuePrev) | CastToUInt64(valueLast);
        value.ValueType = VALUE_TYPE_UINT64;
        break;

    case EQUATION_OPER_RSHIFT:
        value.ValueUInt64 = CastToUInt64(valuePrev) >> CastToUInt64(valueLast);
        value.ValueType = VALUE_TYPE_UINT64;
        break;

    case EQUATION_OPER_LSHIFT:
        value.ValueUInt64 = CastToUInt64(valuePrev) << CastToUInt64(valueLast);
        value.ValueType = VALUE_TYPE_UINT64;
        break;

    case EQUATION_OPER_XOR:
        value.ValueUInt64 = CastToUInt64( valuePrev ) ^ CastToUInt64( valueLast );
        value.ValueType = VALUE_TYPE_UINT64;
        break;

    case EQUATION_OPER_XNOR:
        value.ValueUInt64 = ~(CastToUInt64( valuePrev ) ^ CastToUInt64( valueLast ));
        value.ValueType = VALUE_TYPE_UINT64;
        break;

    case EQUATION_OPER_AND_L:
        value.ValueBool = CastToUInt64( valuePrev ) && CastToUInt64( valueLast );
        value.ValueType = VALUE_TYPE_BOOL;
        break;

    case EQUATION_OPER_EQUALS:
        value.ValueBool = CastToUInt64( valuePrev ) == CastToUInt64( valueLast );
        value.ValueType = VALUE_TYPE_BOOL;
        break;

    case EQUATION_OPER_UADD:
        value.ValueUInt64 = CastToUInt64(valuePrev) + CastToUInt64(valueLast);
        value.ValueType = VALUE_TYPE_UINT64;
        break;

    case EQUATION_OPER_USUB:
        value.ValueUInt64 = CastToUInt64(valuePrev) - CastToUInt64(valueLast);
        value.ValueType = VALUE_TYPE_UINT64;
        break;

    case EQUATION_OPER_UDIV:
        if(CastToUInt64(valueLast) != 0LL)
        {
            value.ValueUInt64 = CastToUInt64(valuePrev) / CastToUInt64(valueLast);
            value.ValueType = VALUE_TYPE_UINT64;
        }
        else
        {
            value.ValueUInt64 = 0ULL;
            value.ValueType = VALUE_TYPE_UINT64;
        }
        break;

    case EQUATION_OPER_UMUL:
        value.ValueUInt64 = CastToUInt64(valuePrev) * CastToUInt64(valueLast);
        value.ValueType = VALUE_TYPE_UINT64;
        break;

    case EQUATION_OPER_FADD:
        value.ValueFloat = CastToFloat(valuePrev) + CastToFloat(valueLast);
        value.ValueType = VALUE_TYPE_FLOAT;
        break;

    case EQUATION_OPER_FSUB:
        value.ValueFloat = CastToFloat(valuePrev) - CastToFloat(valueLast);
        value.ValueType = VALUE_TYPE_FLOAT;
        break;

    case EQUATION_OPER_FMUL:
        value.ValueFloat = CastToFloat(valuePrev) * CastToFloat(valueLast);
        value.ValueType = VALUE_TYPE_FLOAT;
        break;

    case EQUATION_OPER_FDIV:
        if( CastToFloat(valueLast) != 0LL )
        {
            value.ValueFloat = CastToFloat( valuePrev ) / CastToFloat( valueLast );
            value.ValueType = VALUE_TYPE_FLOAT;
        }
        else
        {
            value.ValueFloat = 0.0f;
            value.ValueType = VALUE_TYPE_FLOAT;
        }
        break;

    case EQUATION_OPER_UGT:
        value.ValueBool = CastToUInt64( valuePrev ) > CastToUInt64( valueLast );
        value.ValueType = VALUE_TYPE_BOOL;
        break;

    case EQUATION_OPER_ULT:
        value.ValueBool = CastToUInt64( valuePrev ) < CastToUInt64( valueLast );
        value.ValueType = VALUE_TYPE_BOOL;
        break;

    case EQUATION_OPER_UGTE:
        value.ValueBool = CastToUInt64( valuePrev ) >= CastToUInt64( valueLast );
        value.ValueType = VALUE_TYPE_BOOL;
        break;

    case EQUATION_OPER_ULTE:
        value.ValueBool = CastToUInt64( valuePrev ) <= CastToUInt64( valueLast );
        value.ValueType = VALUE_TYPE_BOOL;
        break;

    case EQUATION_OPER_FGT:
        value.ValueBool = CastToFloat( valuePrev ) > CastToFloat( valueLast );
        value.ValueType = VALUE_TYPE_BOOL;
        break;

    case EQUATION_OPER_FLT:
        value.ValueBool = CastToFloat( valuePrev ) < CastToFloat( valueLast );
        value.ValueType = VALUE_TYPE_BOOL;
        break;

    case EQUATION_OPER_FGTE:
        value.ValueBool = CastToFloat( valuePrev ) >= CastToFloat( valueLast );
        value.ValueType = VALUE_TYPE_BOOL;
        break;

    case EQUATION_OPER_FLTE:
        value.ValueBool = CastToFloat( valuePrev ) <= CastToFloat( valueLast );
        value.ValueType = VALUE_TYPE_BOOL;
        break;

    case EQUATION_OPER_UMIN:
        value.ValueUInt64 = std::min<uint64_t>(CastToUInt64( valuePrev ), CastToUInt64( valueLast ));
        value.ValueType = VALUE_TYPE_UINT64;
        break;

    case EQUATION_OPER_UMAX:
        value.ValueUInt64 = std::max<uint64_t>(CastToUInt64( valuePrev ), CastToUInt64( valueLast ));
        value.ValueType = VALUE_TYPE_UINT64;
        break;

    case EQUATION_OPER_FMIN:
        value.ValueFloat = std::min<float>(CastToFloat( valuePrev ), CastToFloat( valueLast ));
        value.ValueType = VALUE_TYPE_FLOAT;
        break;

    case EQUATION_OPER_FMAX:
        value.ValueFloat = std::max<float>(CastToFloat( valuePrev ), CastToFloat( valueLast ));
        value.ValueType = VALUE_TYPE_FLOAT;
        break;

    default:
        CLI_ASSERT( false );
        value.ValueUInt64 = 0;
        value.ValueType   = VALUE_TYPE_UINT64;
        break;
    }

    return value;
}

/************************************************************************/
/* ReadBitfield                                                         */
/************************************************************************/
uint64_t MDHelper::ReadBitfield(
    const char *pUnalignedData,
    uint32_t bitOffset,
    uint32_t bitsCount )
{
    if( (bitsCount > 32) || (bitsCount == 0) || (bitsCount + bitOffset > 32) ) return 0;

    uint32_t mask = 0;
    for( uint32_t i = 0; i < bitsCount; i++) mask |= 1 << i;
    for( uint32_t i = 0; i < bitOffset; i++) mask = mask << 1;

    uint32_t data =
        (*pUnalignedData) |
        ((*(pUnalignedData + 1)) << 8) |
        ((*(pUnalignedData + 2)) << 16) |
        ((*(pUnalignedData + 3)) << 24);

    return (uint64_t)((data & mask) >> bitOffset);
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

/************************************************************************/
/* CastToFloat                                                          */
/************************************************************************/
float MDHelper::CastToFloat(TTypedValue_1_0 value)
{
    switch( value.ValueType )
    {
    case VALUE_TYPE_BOOL:
        return ( value.ValueBool ) ? 1.0f : 0.0f;

    case VALUE_TYPE_UINT32:
        return (float)value.ValueUInt32;

    case VALUE_TYPE_UINT64:
        return (float)value.ValueUInt64;

    case VALUE_TYPE_FLOAT:
        return value.ValueFloat;

    default:
        CLI_ASSERT( false );
        break;
    }

    return 0.0f;
}

}
