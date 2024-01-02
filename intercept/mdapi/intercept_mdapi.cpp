/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include <fstream>
#include <iomanip>

#include "common.h"
#include "intercept.h"

#define CL_PROFILING_COMMAND_PERFCOUNTERS_INTEL 0x407F

///////////////////////////////////////////////////////////////////////////////
//
static bool convertPropertiesArrayToBitfield(
    const cl_queue_properties* properties,
    cl_command_queue_properties& propertiesBits )
{
    if( properties )
    {
        for( int i = 0; properties[ i ] != 0; i += 2 )
        {
            switch( properties[ i ] )
            {
            case CL_QUEUE_PROPERTIES:
                switch( properties[ i + 1 ] )
                {
                case 0: // no special queue properties
                case CL_QUEUE_PROFILING_ENABLE:
                case CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE:
                case CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE:
                    propertiesBits |= properties[ i + 1 ];
                    break;
                default:
                    return false;
                }
                break;
            default:
                return false;
            }
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
static void createMDAPICommandQueueProperties(
    cl_uint configuration,
    const cl_queue_properties* properties,
    cl_queue_properties*& pLocalQueueProperties )
{
    bool    addMDAPIProperties = true;
    bool    addMDAPIConfiguration = true;

    size_t  numProperties = 0;
    if( properties )
    {
        while( properties[ numProperties ] != 0 )
        {
            switch( properties[ numProperties ] )
            {
            case CL_QUEUE_MDAPI_PROPERTIES_INTEL:
                addMDAPIProperties = false;
                break;
            case CL_QUEUE_MDAPI_CONFIGURATION_INTEL:
                addMDAPIConfiguration = false;
                break;
            default:
                break;
            }
            numProperties += 2;
        }
    }

    if( addMDAPIProperties )
    {
        numProperties += 2;
    }
    if( addMDAPIConfiguration )
    {
        numProperties += 2;
    }

    // Allocate a new array of properties.  We need to allocate two
    // properties for each pair, plus one property for the terminating
    // zero.
    pLocalQueueProperties = new cl_queue_properties[ numProperties + 1 ];
    if( pLocalQueueProperties )
    {
        // Copy the old properties array to the new properties array,
        // if the new properties array exists.
        numProperties = 0;
        if( properties )
        {
            while( properties[ numProperties ] != 0 )
            {
                pLocalQueueProperties[ numProperties ] = properties[ numProperties ];
                if( properties[ numProperties ] == CL_QUEUE_MDAPI_PROPERTIES_INTEL )
                {
                    CLI_ASSERT( addMDAPIProperties == false );
                    pLocalQueueProperties[ numProperties + 1 ] = CL_QUEUE_MDAPI_ENABLE_INTEL;
                }
                else if( properties[ numProperties ] == CL_QUEUE_MDAPI_CONFIGURATION_INTEL )
                {
                    CLI_ASSERT( addMDAPIConfiguration == false );
                    pLocalQueueProperties[ numProperties + 1 ] = configuration;
                }
                else
                {
                    pLocalQueueProperties[ numProperties + 1 ] =
                        properties[ numProperties + 1 ];
                }
                numProperties += 2;
            }
        }
        if( addMDAPIProperties )
        {
            pLocalQueueProperties[ numProperties] = CL_QUEUE_MDAPI_PROPERTIES_INTEL;
            pLocalQueueProperties[ numProperties + 1 ] = CL_QUEUE_MDAPI_ENABLE_INTEL;
            numProperties += 2;
        }
        if( addMDAPIConfiguration )
        {
            pLocalQueueProperties[ numProperties] = CL_QUEUE_MDAPI_CONFIGURATION_INTEL;
            pLocalQueueProperties[ numProperties + 1 ] = configuration;
            numProperties += 2;
        }

        // Add the terminating zero.
        pLocalQueueProperties[ numProperties ] = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::initCustomPerfCounters()
{
    const std::string& metricSetSymbolName = config().DevicePerfCounterCustom;
    const std::string& metricsFileName = config().DevicePerfCounterFile;
    const bool includeMaxValues = config().DevicePerfCounterReportMax;

    if( m_pMDHelper == NULL )
    {
        std::string permissionString;
        if( OS().CheckMDAPIPermissions(permissionString) == false )
        {
            log( permissionString );
        }
        else if( config().DevicePerfCounterEventBasedSampling )
        {
            m_pMDHelper = MetricsDiscovery::MDHelper::CreateEBS(
                config().DevicePerfCounterLibName,
                metricSetSymbolName,
                metricsFileName,
                includeMaxValues );
        }
        else if( config().DevicePerfCounterTimeBasedSampling )
        {
            m_pMDHelper = MetricsDiscovery::MDHelper::CreateTBS(
                config().DevicePerfCounterLibName,
                metricSetSymbolName,
                metricsFileName,
                includeMaxValues );
        }
        else
        {
            CLI_ASSERT( 0 );
        }
        if( m_pMDHelper )
        {
            log( "Metric Discovery initialized.\n" );
        }
        else
        {
            log( "Metric Discovery failed to initialize.\n" );
        }
    }

    if( m_pMDHelper )
    {
        // Open the metric stream for time based sampling, if needed.
        if( config().DevicePerfCounterTimeBasedSampling )
        {
            uint32_t    timerNS =
                config().DevicePerfCounterTimeBasedSamplingPeriod * 1000;
            uint32_t    bufferSizeBytes =
                config().DevicePerfCounterTimeBasedBufferSize;
            m_pMDHelper->OpenStream(
                timerNS,        // timer period, in nanoseconds
                bufferSizeBytes,// buffer size in bytes, 0 = device maximum
                0 );            // pid -> sample all processes
        }

        // Get the dump directory name and create the dump file for
        // metrics, if we haven't created it already.
        if ( !m_MetricDump.is_open() )
        {
            std::string fileName = "";
            OS().GetDumpDirectoryName( sc_DumpDirectoryName, fileName );
            fileName += '/';
            fileName += sc_DumpPerfCountersFileNamePrefix;
            fileName += "_";
            fileName += metricSetSymbolName;
            fileName += ".csv";

            OS().MakeDumpDirectories( fileName );

            m_MetricDump.open( fileName.c_str(), std::ios::out | std::ios::binary );

            m_pMDHelper->PrintMetricNames( m_MetricDump );
            m_pMDHelper->PrintMetricUnits( m_MetricDump );
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
cl_command_queue CLIntercept::createMDAPICommandQueue(
    cl_context context,
    cl_device_id device,
    cl_command_queue_properties properties,
    cl_int* errcode_ret )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    cl_command_queue    retVal = NULL;

    cl_platform_id  platform = getPlatform(device);
    if( dispatchX(platform).clCreatePerfCountersCommandQueueINTEL == NULL )
    {
        getExtensionFunctionAddress(
            platform,
            "clCreatePerfCountersCommandQueueINTEL" );
    }

    const auto& dispatchX = this->dispatchX(platform);
    if( dispatchX.clCreatePerfCountersCommandQueueINTEL == NULL )
    {
        log( "Couldn't get pointer to clCreatePerfCountersCommandQueueINTEL!\n" );
    }
    else if( m_pMDHelper == NULL )
    {
        log( "Metrics discovery is not initialized!\n" );
    }
    else if( m_pMDHelper->ActivateMetricSet() )
    {
        cl_int  errorCode = CL_SUCCESS;
        cl_uint configuration = m_pMDHelper->GetMetricsConfiguration();
        logf( "Calling clCreatePerfCountersCommandQueueINTEL with configuration %u....\n",
            configuration);

        retVal = dispatchX.clCreatePerfCountersCommandQueueINTEL(
            context,
            device,
            properties,
            configuration,
            &errorCode );
        if( retVal == NULL )
        {
            logf( "clCreatePerfCountersCommandQueueINTEL returned %s (%d)!\n",
                enumName().name( errorCode ).c_str(),
                errorCode );
        }
        else
        {
            log( "clCreatePerfCountersCommandQueueINTEL succeeded.\n" );
        }
        if( errcode_ret )
        {
            errcode_ret[0] = errorCode;
        }
    }
    else
    {
        log( "Metric Discovery: Couldn't activate metric set!\n" );
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

    // Some drivers only support creating MDAPI command queues via
    // clCreatePerfCountersCommandQueueINTEL.  So, for maximum compatibility,
    // first try to convert the passed-in properties array to a properties
    // bitfield and use clCreatePerfCountersCommandQueueINTEL.  If this
    // fails, we will instead try a newer codepath that creates an MDAPI
    // command queue using new property-value pairs.

    cl_command_queue_properties propertiesBits = 0;
    if( convertPropertiesArrayToBitfield( properties, propertiesBits ) )
    {
        retVal = createMDAPICommandQueue(
            context,
            device,
            propertiesBits,
            errcode_ret );
    }

    if( retVal == NULL )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);

        if( m_pMDHelper == NULL )
        {
            log( "Metrics discovery is not initialized!\n" );
        }
        else if( m_pMDHelper->ActivateMetricSet() )
        {
            cl_int  errorCode = CL_SUCCESS;
            cl_uint configuration = m_pMDHelper->GetMetricsConfiguration();

            logf( "Creating MDAPI command queue properties for configuration %u....\n",
                configuration);

            cl_queue_properties*    newProperties = NULL;
            createMDAPICommandQueueProperties(
                configuration,
                properties,
                newProperties );

            retVal = createCommandQueueWithProperties(
                context,
                device,
                newProperties,
                &errorCode );
            if( retVal == NULL )
            {
                logf( "MDAPI clCreateCommandQueueWithProperties returned %s (%d)!\n",
                    enumName().name( errorCode ).c_str(),
                    errorCode );
            }
            else
            {
                log( "MDAPI clCreateCommandQueueWithProperties succeeded.\n" );
            }
            if( errcode_ret )
            {
                errcode_ret[0] = errorCode;
            }
        }
        else
        {
            log( "Metric Discovery: Couldn't activate metric set!\n" );
        }
    }

    return retVal;
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getMDAPICountersFromStream( void )
{
    // We should only get here when time based sampling is enabled.
    CLI_ASSERT( config().DevicePerfCounterTimeBasedSampling );

    if( m_pMDHelper )
    {
        std::vector<MetricsDiscovery::TTypedValue_1_0> results;
        std::vector<MetricsDiscovery::TTypedValue_1_0> maxValues;
        std::vector<MetricsDiscovery::TTypedValue_1_0> ioInfoValues;

        while( true )
        {
            bool report = m_pMDHelper->SaveReportsFromStream();
            if( report )
            {
                uint32_t numResults = m_pMDHelper->GetMetricsFromSavedReports(
                    results,
                    maxValues );
                m_pMDHelper->GetIOMeasurementInformation(
                    ioInfoValues );

                m_pMDHelper->PrintMetricValues(
                    m_MetricDump,
                    "TBS",
                    numResults,
                    results,
                    maxValues,
                    ioInfoValues );

                m_pMDHelper->ResetSavedReports();
            }
            else
            {
                break;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
//
void CLIntercept::getMDAPICountersFromEvent(
    const std::string& name,
    const cl_event event )
{
    // We should only get here when event based sampling is enabled.
    CLI_ASSERT( config().DevicePerfCounterEventBasedSampling );

    if( m_pMDHelper )
    {
        const size_t reportSize = m_pMDHelper->GetQueryReportSize();

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
                // Check: The size of the queried report should be the expected size.
                CLI_ASSERT( outputSize == reportSize );

                std::vector<MetricsDiscovery::TTypedValue_1_0> results;
                std::vector<MetricsDiscovery::TTypedValue_1_0> maxValues;
                std::vector<MetricsDiscovery::TTypedValue_1_0> ioInfoValues; // unused

                uint32_t numResults = m_pMDHelper->GetMetricsFromReports(
                    1,
                    pReport,
                    results,
                    maxValues );

                if( numResults )
                {
                    m_pMDHelper->PrintMetricValues(
                        m_MetricDump,
                        name,
                        numResults,
                        results,
                        maxValues,
                        ioInfoValues );
                    m_pMDHelper->AggregateMetrics(
                        m_MetricAggregations,
                        name,
                        results );
                }
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
        config().DevicePerfCounterEventBasedSampling &&
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
