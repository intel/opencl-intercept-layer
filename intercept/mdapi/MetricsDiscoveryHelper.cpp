/*
// Copyright (c) 2018-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include "MetricsDiscoveryHelper.h"

#include "common.h"

#include <algorithm>
#include <stack>
#include <string>

#include <stdio.h>

// Enables logs:
//#ifdef _DEBUG
//#define MD_DEBUG
//#endif

#if defined(_WIN32)
static const wchar_t* cMDLibFileName =
    sizeof(void*) == 8 ?
    L"igdmd64.dll" :
    L"igdmd32.dll";

#include "DriverStorePath.h"

static void* OpenLibrary( const std::string& metricsLibraryName )
{
    return !metricsLibraryName.empty() ?
        (void*)LoadLibraryA(metricsLibraryName.c_str()) :
        (void*)LoadDynamicLibrary(cMDLibFileName);
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

static void* OpenLibrary( const std::string& metricsLibraryName )
{
    void* ret = NULL;
    if( !metricsLibraryName.empty() )
    {
        ret = dlopen(metricsLibraryName.c_str(), RTLD_LAZY | RTLD_LOCAL);
    }
    else
    {
        if( ret == NULL )
        {
            ret = dlopen(cMDLibFileName, RTLD_LAZY | RTLD_LOCAL);
        }
#if !defined(__APPLE__)
        if (ret == NULL)
        {
            // try adding an explicit soversion
            std::string check(cMDLibFileName); check += ".1";
            ret = dlopen(check.c_str(), RTLD_LAZY | RTLD_LOCAL);
        }
        if( ret == NULL )
        {
            // old alternate name, may eventually be removed
            ret = dlopen("libmd.so", RTLD_LAZY | RTLD_LOCAL);
        }
#endif
    }
    return ret;
}

#define GetFunctionAddress(_handle, _name)  dlsym(_handle, _name)
#define OutputDebugString(_buf)             fprintf(stderr, "%s", _buf);

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
MDHelper::MDHelper(uint32_t apiMask) :
    m_Initialized( false ),
    m_Activated( false ),
    m_APIMask( apiMask ),
    m_CategoryMask( GPU_RENDER | GPU_COMPUTE | GPU_MEDIA | GPU_GENERIC ),
    m_AdapterGroup( NULL ),
    m_MetricsDevice( NULL ),
    m_ConcurrentGroup( NULL ),
    m_MetricSet( NULL ),
    m_NumSavedReports( 0 )
{
}

/************************************************************************/
/* ~MDHelper                                                            */
/************************************************************************/
MDHelper::~MDHelper()
{
    if( m_Activated )
    {
        DeactivateMetricSet();
    }

    if( m_AdapterGroup != NULL )
    {
        if( m_Adapter && m_MetricsDevice )
        {
            m_Adapter->CloseMetricsDevice( m_MetricsDevice );
            m_MetricsDevice = NULL;
        }

        m_AdapterGroup->Close();
        m_AdapterGroup = NULL;
        m_Adapter = NULL;
    }
    else if( CloseMetricsDevice != NULL && m_MetricsDevice )
    {
        CloseMetricsDevice( m_MetricsDevice );
        m_MetricsDevice = NULL;
    }

    m_Initialized = false;
}

/************************************************************************/
/* CreateEBS                                                            */
/************************************************************************/
MDHelper* MDHelper::CreateEBS(
    const std::string& metricsLibraryName,
    const std::string& metricSetSymbolName,
    const std::string& metricsFileName,
    uint32_t adapterIndex,
    bool includeMaxValues )
{
#if defined(__linux__) || defined(__FreeBSD__)
    // This is a temporary workaround until the Linux MDAPI is updated
    // to expose metrics for OpenCL.
    MDHelper*   pMDHelper = new MDHelper(API_TYPE_OCL|API_TYPE_OGL4_X);
#else
    MDHelper*   pMDHelper = new MDHelper(API_TYPE_OCL);
#endif
    if( pMDHelper )
    {
        if( pMDHelper->InitMetricsDiscovery(
                metricsLibraryName,
                metricSetSymbolName,
                metricsFileName,
                adapterIndex,
                includeMaxValues ) == false )
        {
            Delete( pMDHelper );
        }
    }
    return pMDHelper;
}

/************************************************************************/
/* CreateTBS                                                            */
/************************************************************************/
MDHelper* MDHelper::CreateTBS(
    const std::string& metricsLibraryName,
    const std::string& metricSetSymbolName,
    const std::string& metricsFileName,
    uint32_t adapterIndex,
    bool includeMaxValues )
{
    MDHelper*   pMDHelper = new MDHelper(API_TYPE_IOSTREAM);
    if( pMDHelper )
    {
        if( pMDHelper->InitMetricsDiscovery(
                metricsLibraryName,
                metricSetSymbolName,
                metricsFileName,
                adapterIndex,
                includeMaxValues ) == false )
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
    const std::string& metricsLibraryName,
    const std::string& metricSetSymbolName,
    const std::string& metricsFileName,
    uint32_t adapterIndex,
    bool includeMaxValues )
{
    m_Initialized = false;
    m_IncludeMaxValues = includeMaxValues;

    OpenAdapterGroup = NULL;
    OpenMetricsDevice = NULL;
    OpenMetricsDeviceFromFile = NULL;
    CloseMetricsDevice = NULL;

    if (m_APIMask & API_TYPE_IOSTREAM && m_APIMask != API_TYPE_IOSTREAM)
    {
        DebugPrint("API type IOSTREAM cannot be combined with any other API type.\n");
        return false;
    }

    // Open the MDAPI library from the passed-in file name if provided, or from
    // a default file name otherwise.
    void* pLibrary = OpenLibrary(metricsLibraryName);
    if (pLibrary == NULL)
    {
        DebugPrint("Couldn't load metrics discovery library!\n");
        return false;
    }

    OpenAdapterGroup = (OpenAdapterGroup_fn)GetFunctionAddress(pLibrary, "OpenAdapterGroup");
    if (OpenAdapterGroup == NULL)
    {
        DebugPrint("Couldn't get pointer to OpenAdapterGroup!\n");
        // OpenAdapterGroup is a newer function that is not supported by older MDAPI.
    }

    OpenMetricsDevice = (OpenMetricsDevice_fn)GetFunctionAddress(pLibrary, "OpenMetricsDevice");
    if (OpenMetricsDevice == NULL)
    {
        DebugPrint("Couldn't get pointer to OpenMetricsDevice!\n");
        return false;
    }

    OpenMetricsDeviceFromFile = (OpenMetricsDeviceFromFile_fn)GetFunctionAddress(pLibrary, "OpenMetricsDeviceFromFile");
    if (OpenMetricsDeviceFromFile == NULL)
    {
        DebugPrint("Couldn't get pointer to OpenMetricsDeviceFromFile!\n");
        return false;
    }

    CloseMetricsDevice = (CloseMetricsDevice_fn)GetFunctionAddress(pLibrary, "CloseMetricsDevice");
    if (CloseMetricsDevice == NULL)
    {
        DebugPrint("Couldn't get pointer to CloseMetricsDevice!\n");
        return false;
    }

    DebugPrint("InitMetricsDiscoveryAdapterGroup for adapter index %u...\n", adapterIndex);
    bool success = InitMetricsDiscoveryAdapterGroup(
        metricSetSymbolName,
        metricsFileName,
        adapterIndex );
    if (!success)
    {
        DebugPrint("InitMetricsDiscoveryLegacy...\n", adapterIndex);
        success = InitMetricsDiscoveryLegacy(
            metricSetSymbolName,
            metricsFileName );
    }

    if (success)
    {
        m_MetricSet->SetApiFiltering( m_APIMask );
        m_Initialized = true;
    }

    DebugPrint("MetricsDiscoveryInit End\n");
    return m_Initialized;
}

/************************************************************************/
/* InitMetricsDiscoveryAdapterGroup                                     */
/************************************************************************/
bool MDHelper::InitMetricsDiscoveryAdapterGroup(
    const std::string& metricSetSymbolName,
    const std::string& metricsFileName,
    uint32_t adapterIndex )
{
    TCompletionCode res = CC_OK;

    res = OpenAdapterGroup(&m_AdapterGroup);
    if (res != CC_OK)
    {
        DebugPrint("OpenAdapterGroup failed, res: %d\n", res);
        return false;
    }

    const TAdapterGroupParamsLatest* pAdapterGroupParams = m_AdapterGroup->GetParams();
    if (NULL == pAdapterGroupParams)
    {
        DebugPrint("AdapterGroup->GetParams() returned NULL\n");
        return false;
    }

    DebugPrint("MDAPI Headers: v%d.%d.%d, MDAPI Lib: v%d.%d.%d\n",
        MD_API_MAJOR_NUMBER_CURRENT,
        MD_API_MINOR_NUMBER_CURRENT,
        MD_API_BUILD_NUMBER_CURRENT,
        pAdapterGroupParams->Version.MajorNumber,
        pAdapterGroupParams->Version.MinorNumber,
        pAdapterGroupParams->Version.BuildNumber);
    if (pAdapterGroupParams->Version.MajorNumber < 1 ||
        (pAdapterGroupParams->Version.MajorNumber == 1 && pAdapterGroupParams->Version.MinorNumber < 1))
    {
        DebugPrint("MDAPI Lib version must be at least v1.1!\n");
        return false;
    }
    if( pAdapterGroupParams->Version.MajorNumber < 1 ||
        ( pAdapterGroupParams->Version.MajorNumber == 1 && pAdapterGroupParams->Version.MinorNumber < 5 ) )
    {
        if( m_IncludeMaxValues )
        {
            DebugPrint("MDAPI Lib version must be at least v1.5 for maximum value tracking - disabling.\n");
            m_IncludeMaxValues = false;
        }
    }

    if (adapterIndex >= pAdapterGroupParams->AdapterCount)
    {
        DebugPrint("Requested adapter index is %u but only %u adapters were found.  Using adapter 0.\n");
        adapterIndex = 0;
    }

    m_Adapter = m_AdapterGroup->GetAdapter(adapterIndex);
    if (NULL == m_Adapter)
    {
        DebugPrint("AdapterGroup->GetAdapter() returned NULL\n");
        return false;
    }

    const TAdapterParamsLatest* pAdapterParams = m_Adapter->GetParams();
    if (pAdapterParams)
    {
        DebugPrint("Adapter: %s\n",
            pAdapterParams->ShortName);
        DebugPrint("PCI Vendor Id: %04X, Device Id: %04X, Bus Info: %02X:%02X.%02X\n",
            pAdapterParams->VendorId,
            pAdapterParams->DeviceId,
            pAdapterParams->BusNumber,
            pAdapterParams->DeviceNumber,
            pAdapterParams->FunctionNumber);
    }

    if (!metricsFileName.empty())
    {
        res = m_Adapter->OpenMetricsDeviceFromFile(metricsFileName.c_str(), (void*)"", &m_MetricsDevice);
        if (res != CC_OK)
        {
            res = m_Adapter->OpenMetricsDeviceFromFile(metricsFileName.c_str(), (void*)"abcdefghijklmnop", &m_MetricsDevice);
        }
        if (res != CC_OK)
        {
            DebugPrint("OpenMetricsDeviceFromFile failed, res: %d\n", res);
            return false;
        }
    }
    else
    {
        res = m_Adapter->OpenMetricsDevice(&m_MetricsDevice);
        if (res != CC_OK)
        {
            DebugPrint("OpenMetricsDevice failed, res: %d\n", res);
            return false;
        }
    }

    bool found = FindMetricSetForDevice(
        m_MetricsDevice,
        metricSetSymbolName );

    return found;
}

/************************************************************************/
/* InitMetricsDiscoveryLegacy                                           */
/************************************************************************/
bool MDHelper::InitMetricsDiscoveryLegacy(
    const std::string& metricSetSymbolName,
    const std::string& metricsFileName )
{
    TCompletionCode res = CC_OK;

    if (!metricsFileName.empty())
    {
        res = OpenMetricsDeviceFromFile(metricsFileName.c_str(), (void*)"", &m_MetricsDevice);
        if (res != CC_OK)
        {
            res = OpenMetricsDeviceFromFile(metricsFileName.c_str(), (void*)"abcdefghijklmnop", &m_MetricsDevice);
        }
        if (res != CC_OK)
        {
            DebugPrint("OpenMetricsDeviceFromFile failed, res: %d\n", res);
            return false;
        }
    }
    else
    {
        res = OpenMetricsDevice(&m_MetricsDevice);
        if (res != CC_OK)
        {
            DebugPrint("OpenMetricsDevice failed, res: %d\n", res);
            return false;
        }
    }

    TMetricsDeviceParamsLatest* deviceParams = m_MetricsDevice->GetParams();
    if (NULL == deviceParams)
    {
        DebugPrint("MetricsDevice->GetParams() returned NULL\n");
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
    if( deviceParams->Version.MajorNumber < 1 ||
        ( deviceParams->Version.MajorNumber == 1 && deviceParams->Version.MinorNumber < 5 ) )
    {
        if( m_IncludeMaxValues )
        {
            DebugPrint("MDAPI Lib version must be at least v1.5 for maximum value tracking - disabling.\n");
            m_IncludeMaxValues = false;
        }
    }

    bool found = FindMetricSetForDevice(
        m_MetricsDevice,
        metricSetSymbolName );

    return found;
}

/************************************************************************/
/* FindMetricSetForDevice                                               */
/************************************************************************/
bool MDHelper::FindMetricSetForDevice(
    IMetricsDeviceLatest* pMetricsDevice,
    const std::string& metricSetSymbolName )
{
    DebugPrint("Looking for MetricSet: %s, API: %X, Category: %X\n",
        metricSetSymbolName.c_str(),
        m_APIMask,
        m_CategoryMask );

    TMetricsDeviceParamsLatest* pDeviceParams = pMetricsDevice->GetParams();
    if (NULL == pDeviceParams)
    {
        DebugPrint("MetricsDevice->GetParams returned NULL\n");
        return false;
    }

    bool found = false;
    for( uint32_t cg = 0; !found && cg < pDeviceParams->ConcurrentGroupsCount; cg++ )
    {
        IConcurrentGroupLatest *pGroup = pMetricsDevice->GetConcurrentGroup(cg);
        TConcurrentGroupParamsLatest* pGroupParams = pGroup->GetParams();

        if (NULL == pGroupParams)
        {
            continue;
        }

        for( uint32_t ms = 0; !found && ms < pGroupParams->MetricSetsCount; ms++)
        {
            IMetricSetLatest* pMetricSet = pGroup->GetMetricSet(ms);
            TMetricSetParamsLatest* pSetParams = pMetricSet->GetParams();

            if( pSetParams &&
                ( pSetParams->ApiMask & m_APIMask ) &&
                ( pSetParams->CategoryMask & m_CategoryMask ) &&
                ( metricSetSymbolName == pSetParams->SymbolName ) )
            {
                DebugPrint("Matched Group: %s MetricSet: %s MetricCount: %d API: %X, Category: %X\n",
                    pGroupParams->SymbolName,
                    pSetParams->SymbolName,
                    pSetParams->MetricsCount,
                    pSetParams->ApiMask,
                    pSetParams->CategoryMask );

                found = true;
                m_ConcurrentGroup = pGroup;
                m_MetricSet = pMetricSet;
            }
            else if( pSetParams )
            {
                DebugPrint("Skipped Group: %s MetricSet: %s MetricCount: %d API: %X, Category: %X\n",
                    pGroupParams->SymbolName,
                    pSetParams->SymbolName,
                    pSetParams->MetricsCount,
                    pSetParams->ApiMask,
                    pSetParams->CategoryMask );
            }
        }
    }

    if (m_MetricSet == NULL)
    {
        DebugPrint("MetricSet not found.\n");
    }

    return found;
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

    if( m_Activated )
    {
        DebugPrint("Skipping ActivateMetricSet - already active.\n");
        return true;
    }

    TCompletionCode res = m_MetricSet->Activate();
    if( res != CC_OK ) { DebugPrint("ActivateMetricSet failed!\n"); }

    m_Activated = res == CC_OK;

    return res == CC_OK;
}

/************************************************************************/
/* DeactivateMetricSet                                                  */
/************************************************************************/
void MDHelper::DeactivateMetricSet()
{
    if( !m_Initialized || !m_Activated || !m_MetricSet )
    {
        DebugPrint("Can't DeactivateMetricSet!\n");
        return;
    }

    TCompletionCode res = m_MetricSet->Deactivate();
    if( res != CC_OK ) { DebugPrint("DeactivateMetricSet failed!\n"); }

    m_Activated = res != CC_OK;
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
/* GetMetricsFromReports                                                */
/************************************************************************/
uint32_t MDHelper::GetMetricsFromReports(
    const uint32_t numReports,
    const char* pReportData,
    std::vector<TTypedValueLatest>& results,
    std::vector<TTypedValueLatest>& maxValues )
{
    if( !m_Initialized || !m_MetricSet )
    {
        DebugPrint("Can't GetMetricsFromReports!\n");
        return 0;
    }

    const uint32_t reportSize       =
        m_APIMask & API_TYPE_IOSTREAM ?
        m_MetricSet->GetParams()->RawReportSize * numReports :
        m_MetricSet->GetParams()->QueryReportSize * numReports;

    const uint32_t metricsCount     = m_MetricSet->GetParams()->MetricsCount;
    const uint32_t informationCount = m_MetricSet->GetParams()->InformationCount;

    results.resize( ( metricsCount + informationCount ) * (size_t)numReports );

    TCompletionCode res = MetricsDiscovery::CC_ERROR_GENERAL;
    uint32_t    outReportCount = 0;
    if( m_IncludeMaxValues )
    {
        maxValues.resize( metricsCount * (size_t)numReports );
        res = m_MetricSet->CalculateMetrics(
            (const unsigned char*)pReportData,
            reportSize,
            results.data(),
            (uint32_t)(results.size() * sizeof(TTypedValueLatest)),
            &outReportCount,
            maxValues.data(),
            (uint32_t)(maxValues.size() * sizeof(TTypedValueLatest)) );
    }
    else
    {
        res = m_MetricSet->CalculateMetrics(
            (const unsigned char*)pReportData,
            reportSize,
            results.data(),
            (uint32_t)(results.size() * sizeof(TTypedValueLatest)),
            &outReportCount,
            false );
    }

    if( res != CC_OK )
    {
        DebugPrint("CalculateMetrics failed!\n");
        return 0;
    }
    else
    {
        DebugPrint("CalculateMetrics: got %d reports out.\n", outReportCount);
        return outReportCount;
    }
}

/************************************************************************/
/* GetIOMeasurementInformation                                          */
/************************************************************************/
void MDHelper::GetIOMeasurementInformation(
    std::vector<TTypedValueLatest>& ioInfoValues )
{
    if (!m_Initialized || !m_ConcurrentGroup || !m_MetricSet )
    {
        DebugPrint("Can't GetIOMeasurementInformation!\n");
        return;
    }

    if( !( m_APIMask & API_TYPE_IOSTREAM ) )
    {
        DebugPrint("GetIOMeasurementInformation requires API_TYPE_IOSTREAM!\n");
        return;
    }

    const uint32_t ioInformationCount =
        m_ConcurrentGroup->GetParams()->IoMeasurementInformationCount;

    ioInfoValues.resize(ioInformationCount);
    TCompletionCode res = m_MetricSet->CalculateIoMeasurementInformation(
        ioInfoValues.data(),
        (uint32_t)(ioInfoValues.size() * sizeof(TTypedValueLatest)) );
    if( res != CC_OK )
    {
        DebugPrint("CalculateIoMeasurementInformation failed!\n");
        return;
    }
}

/************************************************************************/
/* OpenStream                                                           */
/************************************************************************/
void MDHelper::OpenStream( uint32_t timerPeriod, uint32_t bufferSize, uint32_t pid )
{
    if( !m_Initialized || !m_ConcurrentGroup || !m_MetricSet )
    {
        DebugPrint("Can't OpenStream!\n");
        return;
    }

    if( !( m_APIMask & API_TYPE_IOSTREAM ) )
    {
        DebugPrint("OpenStream requires API_TYPE_IOSTREAM!\n");
        return;
    }

    if( bufferSize == 0 )
    {
        TTypedValueLatest* oaBufferSize = m_MetricsDevice->
            GetGlobalSymbolValueByName( "OABufferMaxSize" );
        if( oaBufferSize )
        {
            bufferSize = oaBufferSize->ValueUInt32;
            DebugPrint("Trying device maximum buffer size = %u bytes.\n", bufferSize);
        }
        else
        {
            bufferSize = 4 * 1024 * 1024;   // 4MB
            DebugPrint("Trying default maximum buffer size = %u bytes.\n", bufferSize);
        }
    }

    TCompletionCode res = m_ConcurrentGroup->OpenIoStream(
        m_MetricSet,
        pid,
        &timerPeriod,
        &bufferSize );
    if( res != CC_OK )
    {
        DebugPrint("OpenIoStream failed %d\n", res);
        return;
    }
    else
    {
        DebugPrint("OpenIoStream succeeded: timer period = %u ns, buffer size = %u bytes.\n",
            timerPeriod,
            bufferSize);
    }

    // Read a dummy report from the stream, to populate the metric names and units.
    const uint32_t  reportSize = m_MetricSet->GetParams()->RawReportSize;

    std::vector<char>   reportData;
    reportData.resize( reportSize );

    uint32_t    numReports = 1;
    res = m_ConcurrentGroup->ReadIoStream(
        &numReports,
        reportData.data(),
        IO_READ_FLAG_DROP_OLD_REPORTS );
    if( res != CC_OK && res != CC_READ_PENDING )
    {
        DebugPrint("Dummy ReadIoStream failed %d\n", res);
        return;
    }
}

/************************************************************************/
/* SaveReportsFromStream                                                */
/************************************************************************/
bool MDHelper::SaveReportsFromStream( void )
{
    if( !m_Initialized || !m_ConcurrentGroup || !m_MetricSet )
    {
        DebugPrint("Can't GetReportFromStream!\n");
        return false;
    }

    if( !( m_APIMask & API_TYPE_IOSTREAM ) )
    {
        DebugPrint("SaveReportsFromStream requires API_TYPE_IOSTREAM!\n");
        return false;
    }

    const uint32_t  cMaxNumReports = 256;
    const uint32_t  cMinNumReports = 16;

    const uint32_t  reportSize = m_MetricSet->GetParams()->RawReportSize;

    if( m_SavedReportData.capacity() < reportSize * cMaxNumReports )
    {
        m_SavedReportData.resize( reportSize * cMaxNumReports );
        m_NumSavedReports = 0;
    }

    uint32_t    reportsToRead = cMaxNumReports - m_NumSavedReports;
    char*       pNextReportData = m_SavedReportData.data() + reportSize * m_NumSavedReports;

    DebugPrint("SaveReportsFromStream: currently have %d reports, reading up to %d more reports.\n", m_NumSavedReports, reportsToRead);

    TCompletionCode res = m_ConcurrentGroup->ReadIoStream(
        &reportsToRead,
        pNextReportData,
        0 );

    if( res == CC_READ_PENDING )
    {
        DebugPrint("CC_READ_PENDING: Read %d reports from the stream.\n", reportsToRead);
    }
    else if( res == CC_OK )
    {
        DebugPrint("CC_OK: Read %d reports from the stream.\n", reportsToRead);
    }
    else
    {
        DebugPrint("Error reading from stream: res = %d\n", (int)res);
    }

    m_NumSavedReports += reportsToRead;

    DebugPrint("SaveReportsFromStream: read %d reports, now there are %d saved reports.\n", reportsToRead, m_NumSavedReports);

    return m_NumSavedReports >= cMinNumReports;
}

/************************************************************************/
/* GetMetricsFromSavedReports                                           */
/************************************************************************/
uint32_t MDHelper::GetMetricsFromSavedReports(
    std::vector<TTypedValueLatest>& results,
    std::vector<TTypedValueLatest>& maxValues )
{
    DebugPrint("Getting metrics from %d saved reports...\n", m_NumSavedReports);

    return GetMetricsFromReports(
        m_NumSavedReports,
        m_SavedReportData.data(),
        results,
        maxValues );
}

/************************************************************************/
/* ResetSavedReports                                                    */
/************************************************************************/
void MDHelper::ResetSavedReports( void )
{
    m_NumSavedReports = 0;
}

/************************************************************************/
/* CloseStream                                                          */
/************************************************************************/
void MDHelper::CloseStream()
{
    if( !m_Initialized || !m_ConcurrentGroup )
    {
        DebugPrint("Can't CloseStream!\n");
        return;
    }

    if( !( m_APIMask & API_TYPE_IOSTREAM ) )
    {
        DebugPrint("CloseStream requires API_TYPE_IOSTREAM!\n");
        return;
    }

    TCompletionCode res = m_ConcurrentGroup->CloseIoStream();
    if( res != CC_OK )
    {
        DebugPrint( "CloseStream failed: %d\n", res );
        return;
    }
}

/************************************************************************/
/* PrintMetricNames                                                     */
/************************************************************************/
void MDHelper::PrintMetricNames( std::ostream& os )
{
    if( !m_Initialized || !m_ConcurrentGroup || !m_MetricSet || !os.good() )
    {
        DebugPrint("Can't PrintMetricNames!\n");
        return;
    }

    os << "kernel,";

    for( uint32_t i = 0; i < m_MetricSet->GetParams()->MetricsCount; i++ )
    {
        os << m_MetricSet->GetMetric( i )->GetParams()->SymbolName << ",";
        if( m_IncludeMaxValues )
        {
            os << "max_" << m_MetricSet->GetMetric( i )->GetParams()->SymbolName << ",";
        }
    }

    os << ",";

    for(uint32_t i = 0; i < m_MetricSet->GetParams()->InformationCount; i++)
    {
        os << m_MetricSet->GetInformation( i )->GetParams()->SymbolName << ",";
    }

    if( m_APIMask & API_TYPE_IOSTREAM )
    {
        os << ",";

        const uint32_t ioInfoCount =
            m_ConcurrentGroup->GetParams()->IoMeasurementInformationCount;
        for( uint32_t i = 0; i < ioInfoCount; i++ )
        {
            os << m_ConcurrentGroup->GetIoMeasurementInformation( i )->GetParams()->SymbolName << ",";
        }
    }

    os << std::endl;
}

/************************************************************************/
/* PrintMetricUnits                                                 */
/************************************************************************/
void MDHelper::PrintMetricUnits(std::ostream& os )
{
    if (!m_Initialized || !m_ConcurrentGroup || !m_MetricSet || !os.good())
    {
        DebugPrint("Can't PrintMetricUnits!\n");
        return;
    }

    os << " ,";

    for (uint32_t i = 0; i < m_MetricSet->GetParams()->MetricsCount; i++)
    {
        const char* unit = m_MetricSet->GetMetric(i)->GetParams()->MetricResultUnits;
        os << ( unit ? unit : " " ) << ( m_IncludeMaxValues ? ", ," : "," );
    }

    os << ",";

    for (uint32_t i = 0; i < m_MetricSet->GetParams()->InformationCount; i++)
    {
        const char* unit = m_MetricSet->GetInformation(i)->GetParams()->InfoUnits;
        os << ( unit ? unit : " " ) << ",";
    }

    if( m_APIMask & API_TYPE_IOSTREAM )
    {
        os << ",";

        const uint32_t ioInfoCount =
            m_ConcurrentGroup->GetParams()->IoMeasurementInformationCount;
        for( uint32_t i = 0; i < ioInfoCount; i++ )
        {
            const char* unit = m_ConcurrentGroup->GetIoMeasurementInformation( i )->GetParams()->InfoUnits;
            os << ( unit ? unit : " " ) << ",";
        }
    }

    os << std::endl;
}

/************************************************************************/
/* PrintMetricValues                                                    */
/************************************************************************/
void MDHelper::PrintMetricValues(
    std::ostream& os,
    const std::string& name,
    const uint32_t numResults,
    const std::vector<TTypedValueLatest>& results,
    const std::vector<TTypedValueLatest>& maxValues,
    const std::vector<TTypedValueLatest>& ioInfoValues )
{
    if( !m_Initialized || !m_ConcurrentGroup || !m_MetricSet || !os.good() )
    {
        DebugPrint("Can't PrintMetricValues!\n");
        return;
    }

    const uint32_t metricsCount = m_MetricSet->GetParams()->MetricsCount;
    const uint32_t infoCount = m_MetricSet->GetParams()->InformationCount;

    const uint32_t resultsCount = metricsCount + infoCount;

    for( uint32_t result = 0; result < numResults; result++ )
    {
        os << name << ",";

        for( uint32_t i = 0; i < metricsCount; i++ )
        {
            PrintValue( os, results[ resultsCount * result + i ] );
            if( m_IncludeMaxValues )
            {
                PrintValue( os, maxValues[ metricsCount * result + i ] );
            }
        }

        os << ",";

        for( uint32_t i = 0; i < infoCount; i++ )
        {
            PrintValue( os, results[ resultsCount * result + metricsCount + i ] );
        }

        if( m_APIMask & API_TYPE_IOSTREAM )
        {
            os << ",";

            const uint32_t ioInfoCount =
                m_ConcurrentGroup->GetParams()->IoMeasurementInformationCount;
            for( uint32_t i = 0; i < ioInfoCount; i++ )
            {
                PrintValue( os, ioInfoValues[ i ] );
            }
        }

        os << std::endl;
    }
}

/************************************************************************/
/* AggregateMetrics                                                     */
/************************************************************************/
void MDHelper::AggregateMetrics(
    CMetricAggregations& aggregations,
    const std::string& name,
    const std::vector<TTypedValueLatest>& results )
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
        TMetricParamsLatest* metricParams = m_MetricSet->GetMetric( i )->GetParams();

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
void MDHelper::PrintValue( std::ostream& os, const TTypedValueLatest& value )
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
TTypedValueLatest* MDHelper::GetGlobalSymbolValue(
    const char* SymbolName )
{
    for( uint32_t i = 0; i < m_MetricsDevice->GetParams()->GlobalSymbolsCount; i++ )
    {
        TGlobalSymbolLatest* symbol = m_MetricsDevice->GetGlobalSymbol( i );
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
uint64_t MDHelper::CastToUInt64(TTypedValueLatest value)
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
