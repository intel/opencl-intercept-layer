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

#include <fstream>
#include <iomanip>

#include "common.h"
#include "intercept.h"

#define CL_PROFILING_COMMAND_PERFCOUNTERS_INTEL 0x407F

///////////////////////////////////////////////////////////////////////////////
//
static bool convertPropertiesToOCL1_2(
    const cl_queue_properties* properties,
    cl_command_queue_properties& ocl1_2_properties )
{
    if( properties )
    {
        // Convert properties from array of pairs (OCL2.0) to bitfield (OCL1.2)
        for( int i = 0; properties[ i ] != 0; i += 2 )
        {
            if( properties[ i ] == CL_QUEUE_PROPERTIES )
            {
                switch( properties[ i + 1 ] )
                {
                case CL_QUEUE_PROFILING_ENABLE:
                    ocl1_2_properties |= CL_QUEUE_PROFILING_ENABLE;
                    break;
                case CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE:
                    ocl1_2_properties |= CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
                    break;
                default:
                    return false;
                }
            }
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_uint CLIntercept::initCustomPerfCounters()
{
    const std::string& metricSetSymbolName = config().DevicePerfCounterCustom;
    const std::string& metricsFileName = config().DevicePerfCounterFile;

    cl_uint configuration = 0;

    CLI_ASSERT( !metricSetSymbolName.empty() );

    if( m_pMDHelper == NULL )
    {
        m_pMDHelper = new MetricsDiscovery::MDHelper();
        if( m_pMDHelper )
        {
            bool initialized = m_pMDHelper->InitMetricsDiscovery(
                metricSetSymbolName,
                metricsFileName );
            if( initialized  )
            {
                log( "Metric Discovery initialized.\n" );
            }
            else
            {
                log( "Metric Discovery failed to initialize.\n" );
            }
        }
    }

    if( m_pMDHelper )
    {
        configuration = m_pMDHelper->GetMetricsConfiguration();
        // TODO: This doesn't appear to be necessary?
        //m_pMDHelper->SetMetricSetFiltering( API_TYPE_OCL );
        m_pMDHelper->ActivateMetricSet( );

        // Get the dump directory name and register dump file name
        {
            std::string fileName = "";
            OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
            fileName += '/';
            fileName += sc_DumpPerfCountersFileNamePrefix;
            fileName += "_";
            fileName += metricSetSymbolName;
            fileName += ".csv";

            OS().MakeDumpDirectories( fileName );

            m_MetricDump.open( fileName.c_str(), std::ios::out );

            m_pMDHelper->PrintMetricNames( m_MetricDump );
        }
    }

    return configuration;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_command_queue CLIntercept::createMDAPICommandQueue(
    cl_context context,
    cl_device_id device,
    cl_command_queue_properties properties,
    cl_int* errcode_ret )
{
    cl_command_queue    retVal = NULL;

    getExtensionFunctionAddress(
        NULL,
        "clCreatePerfCountersCommandQueueINTEL" );

    if( dispatch().clCreatePerfCountersCommandQueueINTEL )
    {
        m_OS.EnterCriticalSection();

        cl_uint configuration = initCustomPerfCounters();

        retVal = dispatch().clCreatePerfCountersCommandQueueINTEL(
            context,
            device,
            properties,
            configuration,
            errcode_ret );
        if( retVal == NULL )
        {
            log( "clCreatePerfCountersCommandQueueINTEL() returned NULL!\n" );
        }

        if( m_pMDHelper )
        {
            m_pMDHelper->DeactivateMetricSet();
        }

        m_OS.LeaveCriticalSection();
    }

    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
cl_command_queue CLIntercept::createMDAPICommandQueue(
    cl_context context,
    cl_device_id device,
    const cl_queue_properties* properties,
    cl_int* errcode_ret )
{
    cl_command_queue    retVal = NULL;

    // This is a temporary workaround until we have a
    // clCreatePerfCountersCommandQueueWithPropertiesINTEL API.
    // It converts the OpenCL 2.0 command queue properties to
    // OpenLC 1.2 command queue properties, unless an unsupported
    // command queue property is specified.  If an unsupported
    // property is specified then we cannot create an MDAPI command
    // queue.

    cl_command_queue_properties ocl1_2_properties = 0;
    if( convertPropertiesToOCL1_2( properties, ocl1_2_properties ) )
    {
        retVal = createMDAPICommandQueue(
            context,
            device,
            ocl1_2_properties,
            errcode_ret );
    }

    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::saveMDAPICounters(
    const std::string& name,
    const cl_event event )
{
    if( m_pMDHelper )
    {
        const size_t reportSize = std::max< unsigned int >(
            1024,
            m_pMDHelper->GetQueryReportSize() ); //work around for driver/MDAPI bug

        char*   pReport = new char[ reportSize ];
        if( pReport )
        {
            size_t  outputSize = 0;
            cl_int  errorCode = dispatch().clGetEventProfilingInfo(
                event,
                CL_PROFILING_COMMAND_PERFCOUNTERS_INTEL,
                reportSize,
                pReport,
                &outputSize );

            if( errorCode == CL_SUCCESS )
            {
                std::vector<MetricsDiscovery::TTypedValue_1_0> results;
                std::vector<MetricsDiscovery::TTypedValue_1_0> information;
                m_pMDHelper->GetMetricsFromReport(
                    pReport,
                    results,
                    information );
                m_pMDHelper->PrintMetricValues(
                    m_MetricDump,
                    name,
                    results,
                    information );
                m_pMDHelper->AggregateMetrics(
                    m_MetricAggregations,
                    name,
                    results );
            }
            else
            {
                logf("Couldn't get MDAPI data!  clGetEventProfilingInfo returned '%s' (%08X)!\n",
                    enumName().name(errorCode).c_str(),
                    errorCode );
            }

            delete [] pReport;
            pReport = NULL;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::reportMDAPICounters( std::ostream& os )
{
    if( config().DevicePerfCounterTiming &&
        !m_MetricAggregations.empty() )
    {
        std::string header;
        std::vector<uint32_t> headerWidths;
        for( auto& metric : m_MetricAggregations.begin()->second )
        {
            const std::string& metricName = metric.first;

            header += metricName + ", ";
            headerWidths.push_back((uint32_t)metricName.length());
        }

        os << std::endl << "Device Performance Counter Timing: (Average metric per enqueue)" << std::endl;
        os << "                                FunctionName,  Calls, " << header;

        for( auto& metricsForKernel : m_MetricAggregations )
        {
            const std::string& kernelName = metricsForKernel.first;
            const MetricsDiscovery::CMetricAggregationsForKernel& kernelMetrics = metricsForKernel.second;

            uint64_t count = kernelMetrics.begin()->second.Count;
            os << std::endl << std::right << std::setw( 44 ) << kernelName << ", ";
            os << std::right << std::setw( 6 ) << count << ", ";

            int numMetric = 0;
            for( auto& metric : kernelMetrics )
            {
                const MetricsDiscovery::SMetricAggregationData& aggregationData = metric.second;
                os << std::right << std::setw( headerWidths[ numMetric++ ] );
                os << aggregationData.Sum / aggregationData.Count << ", ";
            }
        }

        os << std::endl;
    }
}
