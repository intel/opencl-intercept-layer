/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include "git_version.h"
#include <string>

#include "printcontrols.h"
#include "printmetrics.h"

bool debug = false;

#define DEBUG(_s, ...) if(debug) fprintf(stderr, "[cliloader debug] " _s, ##__VA_ARGS__ );

#if defined(_WIN32)

#include <windows.h>

static std::string commandLine = "";

static bool checkWow64(HANDLE parent, HANDLE child)
{
    BOOL parentWow64 = FALSE;
    IsWow64Process(parent, &parentWow64);

    BOOL childWow64 = FALSE;
    IsWow64Process(child, &childWow64);

    if( parentWow64 != childWow64 )
    {
        fprintf(stderr, "This is the %d-bit version of cliloader, but the target application is a %d-bit application.\n",
            parentWow64 ? 32 : 64,
            childWow64 ? 32 : 64 );
        fprintf(stderr, "Execution will continue, but intercepting and profiling will be disabled.\n");
        return false;
    }

    return true;
}

static void die(const char *op)
{
    DWORD err = GetLastError();
    char description[1024] = "";
    FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        description,
        sizeof(description),
        NULL );
    fprintf(stderr, "cliloader Error: %s: %s\n",
        op,
        description );
    exit(1);
}

static void getCommandLine(char *argv[], int startArg)
{
    std::string rawCommandLine(GetCommandLineA());
    std::string::size_type startPos = 0;

    // Skip all cliloader arguments.
    for( int i = 0; i < startArg; i++ )
    {
        std::string arg(argv[i]);

        std::string::size_type pos = rawCommandLine.find(arg, startPos);
        if( pos == std::string::npos )
        {
            die("creating child process command line");
        }
        else
        {
            startPos = pos + arg.length();
            DEBUG("position after parsing arg '%s' is %zu\n", arg.c_str(), startPos);
        }
    }

    // Skip any remaining non-whitespace characters.
    startPos = rawCommandLine.find_first_of("\t ", startPos);
    DEBUG("position after skipping non-whitespace characters is %zu\n", startPos);

    // Skip any remaining whitespace characters.
    startPos = rawCommandLine.find_first_not_of("\t ", startPos);
    DEBUG("position after skipping whitespace characters is %zu\n", startPos);

    // Everything else should be considered the command line for the child process.
    commandLine = rawCommandLine.substr(startPos);
}

#define SETENV( _name, _value ) _putenv_s( _name, _value )
#define GETENV( _name, _value ) _dupenv_s( &_value, NULL, _name )
#define FREEENV( _value ) free( _value )

#else

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef __APPLE__
#include <libproc.h>
#include <mach-o/dyld.h>
#endif
#ifdef __FreeBSD__
#include <pthread_np.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <libprocstat.h>
#include <libutil.h>
#endif

#ifdef __APPLE__
// Note: OSX has not been tested and may not work!
#define LIB_EXTENSION "dylib"
#define LD_LIBRARY_PATH_ENV "DYLD_LIBRARY_PATH"
#define LD_PRELOAD_ENV "DYLD_INSERT_LIBRARIES"
#else
#define LIB_EXTENSION "so"
#define LD_LIBRARY_PATH_ENV "LD_LIBRARY_PATH"
#define LD_PRELOAD_ENV "LD_PRELOAD"
#endif

#ifndef CLILOADER_LIB_DIR
#define CLILOADER_LIB_DIR "lib"
#endif

static char **appArgs = NULL;

static void die(const char *op)
{
    fprintf(stderr, "cliloader Error: %s\n",
        op );
    exit(1);
}

static bool fileExists( const std::string& name )
{
    if( FILE* fp = fopen(name.c_str(), "r"))
    {
        fclose(fp);
        return true;
    }
    else
    {
        return false;
    }
}

static bool getEnvVars(
    const std::string& path,
    std::string& ld_preload,
    std::string& ld_library_path )
{
    std::string name = path + "/libOpenCL." LIB_EXTENSION;
    bool found = fileExists( name );
    if( found )
    {
        // Construct new LD_LIBRARY_PATH:
        ld_library_path = path;
        const char *old_ld_library_path = getenv(LD_LIBRARY_PATH_ENV);
        if( old_ld_library_path )
        {
            ld_library_path += ":";
            ld_library_path += old_ld_library_path;
        }

        // Add intercept library to LD_PRELOAD:
        ld_preload = path + "/libOpenCL." + LIB_EXTENSION;
        const char *old_ld_preload = getenv(LD_PRELOAD_ENV);
        if( old_ld_preload )
        {
            ld_preload += ":";
            ld_preload += old_ld_preload;
        }
    }
    return found;
}

#define SETENV( _name, _value ) setenv( _name, _value, 1 )
#define GETENV( _name, _value ) _value = getenv(_name)
#define FREEENV( _value ) (void)_value

bool set_LD_LIBRARY_PATH = true;
bool set_LD_PRELOAD = true;

#endif

static void checkSetEnv(const char* name, const char* value)
{
    char* oldValue = NULL;

    GETENV(name, oldValue);

    if (oldValue != NULL && value != NULL && strcmp(value, oldValue)) {
        fprintf(stderr, "cliloader warning: forcing environment variable %s from %s to %s\n",
            name,
            oldValue,
            value);
    } else {
        DEBUG("setting environment variable %s to %s\n", name, value);
    }

    SETENV(name, value);

    FREEENV( oldValue );
}

// Note: This assumes that the CLIntercept DLL/so is in the same directory
// as the executable!
static std::string getProcessDirectory()
{
#if defined(_WIN32)

    // Get full path to executable:
    char    processName[MAX_PATH];
    if( GetModuleFileNameA(GetModuleHandle(NULL), processName, MAX_PATH) == 0 )
    {
        die("Couldn't get the path to the cliloader executable");
    }

    char*   pProcessName = processName;
    pProcessName = strrchr( processName, '\\' );
    if( pProcessName != NULL )
    {
        DEBUG("pProcessName is non-NULL: %s\n", pProcessName);
        *pProcessName = '\0';
    }

    DEBUG("process directory is: %s\n", processName);
    return std::string(processName);

#elif defined(__APPLE__)

    // Get full path to executable:
    char    processName[ 1024 ];
    pid_t   pid = getpid();
    int     ret = proc_pidpath( pid, processName, sizeof(processName) );
    if( ret <= 0 )
    {
        die("Couldn't get the path to the cliloader executable");
    }

    char*   pProcessName = processName;
    pProcessName = strrchr( processName, '/' );
    if( pProcessName != NULL )
    {
        DEBUG("pProcessName is non-NULL: %s\n", pProcessName);
        *pProcessName = '\0';
    }

    DEBUG("process directory is %s\n", pProcessName);
    return std::string(processName);

#elif defined(__linux__)

    // Get full path to executable:
    char    processName[ 1024 ];
    size_t  bytes = readlink(
        "/proc/self/exe",
        processName,
        sizeof( processName ) - 1 );
    if( bytes == 0 )
    {
        die("Couldn't get the path to the cliloader executable");
    }

    processName[ bytes] = '\0';
    DEBUG("full path to executable is: %s\n", processName);

    char*   pProcessName = processName;
    pProcessName = strrchr( processName, '/' );
    if( pProcessName != NULL )
    {
        DEBUG("pProcessName is non-NULL: %s\n", pProcessName);
        *pProcessName = '\0';
    }

    DEBUG("process directory is %s\n", processName);
    return std::string(processName);

#elif defined(__FreeBSD__)

    // Get full path to executable:
    char    processName[ 1024 ];
    struct procstat *prstat = procstat_open_sysctl();
    if( prstat == NULL )
    {
        die("procstat_open_sysctl returned NULL");
    }
    unsigned int count = 0;
    struct kinfo_proc *kp = procstat_getprocs(prstat, KERN_PROC_PID, getpid(), &count);
    if( count != 1 )
    {
        die("Unexpected count returned from procstat_getprocs");
    }
    int ret = procstat_getpathname(prstat, kp, processName, sizeof(processName));
    if (ret != 0) {
        die("procstat_getpathname returned an error");
    }
    procstat_close(prstat);

    processName[ sizeof( processName ) - 1 ] = '\0';
    DEBUG("full path to executable is: %s\n", processName);

    char*   pProcessName = processName;
    pProcessName = strrchr( processName, '/' );
    if( pProcessName != NULL )
    {
        DEBUG("pProcessName is non-NULL: %s\n", pProcessName);
        *pProcessName = '\0';
    }

    DEBUG("process directory is %s\n", processName);
    return std::string(processName);

#else
#pragma message("Need to implement getProcessDirectory()")
    return std::string();
#endif
}

// Important: This needs to stay in sync with GetDumpDirectoryName!
static std::string getDefaultDumpDirectory()
{
    const char* cDumpDirectoryName = "CLIntercept_Dump";

    std::string dumpDir;
#if defined(_WIN32)
    char* systemDrive = NULL;
    size_t  length = 0;

    _dupenv_s(&systemDrive, &length, "SystemDrive");

    dumpDir = systemDrive;
    dumpDir += "/Intel/";
    dumpDir += cDumpDirectoryName;
    dumpDir += "/<executable name>";

    free(systemDrive);
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
    dumpDir = getenv("HOME");
    dumpDir += "/";
    dumpDir += cDumpDirectoryName;
    dumpDir += "/<executable name>";
#else
#pragma message("Need to implement getDefaultDumpDirectory()")
    dumpDir = "unknown";
#endif
    return dumpDir;
}

static bool parseArguments(int argc, char *argv[])
{
    // Defer setting these controls, since they may be overridden by explicit options.
    const char* mdapiGroup = NULL;
    const char* reportToStderr = "1";

    bool    unknownOption = false;

    for (int i = 1; i < argc; i++)
    {
        if( !strcmp(argv[i], "--debug") )
        {
            debug = true;
        }
        else if (!strcmp(argv[i], "--controls") )
        {
            printControls();
            return false;
        }
        else if (!strcmp(argv[i], "--metrics"))
        {
            printMetrics();
            return false;
        }
#if !defined(_WIN32)
        else if( !strcmp(argv[i], "--no-LD_PRELOAD") )
        {
            set_LD_PRELOAD = false;
        }
        else if( !strcmp(argv[i], "--no-LD_LIBRARY_PATH") )
        {
            set_LD_LIBRARY_PATH = false;
        }
#endif
        else if( !strcmp(argv[i], "-q") || !strcmp(argv[i], "--quiet") )
        {
            checkSetEnv("CLI_SuppressLogging", "1");
        }
        else if( !strcmp(argv[i], "-c") || !strcmp(argv[i], "--call-logging") )
        {
            checkSetEnv("CLI_CallLogging", "1");
        }
        else if( !strcmp(argv[i], "-e") || !strcmp(argv[i], "--error-logging") )
        {
            checkSetEnv("CLI_ErrorLogging", "1");
        }
        else if( !strcmp(argv[i], "--tid") )
        {
            checkSetEnv("CLI_CallLoggingThreadId", "1");
        }
        else if( !strcmp(argv[i], "--appendpid") )
        {
            checkSetEnv("CLI_AppendPid", "1");
        }
        else if( !strcmp(argv[i], "--demangle") )
        {
            checkSetEnv("CLI_DemangleKernelNames", "1");
        }
        else if( !strcmp(argv[i], "-dsrc") || !strcmp(argv[i], "--dump-source") )
        {
            checkSetEnv("CLI_DumpProgramSource", "1");
        }
        else if( !strcmp(argv[i], "-dspv") || !strcmp(argv[i], "--dump-spirv") )
        {
            checkSetEnv("CLI_DumpProgramSPIRV", "1");
        }
        else if( !strcmp(argv[i], "--dump-output-binaries") )
        {
            checkSetEnv("CLI_DumpProgramBinaries", "1");
        }
        else if( !strcmp(argv[i], "--dump-kernel-isa-binaries") )
        {
            checkSetEnv("CLI_DumpKernelISABinaries", "1");
        }
        else if( !strcmp(argv[i], "-d") || !strcmp(argv[i], "--device-timing") )
        {
            checkSetEnv("CLI_DevicePerformanceTiming", "1");
        }
        else if( !strcmp(argv[i], "-dv") || !strcmp(argv[i], "--device-timing-verbose") )
        {
            checkSetEnv("CLI_DevicePerformanceTiming", "1");
            checkSetEnv("CLI_DevicePerformanceTimeKernelInfoTracking", "1");
            checkSetEnv("CLI_DevicePerformanceTimeGWSTracking", "1");
            checkSetEnv("CLI_DevicePerformanceTimeLWSTracking", "1");
            checkSetEnv("CLI_DevicePerformanceTimeTransferTracking", "1");
        }
        else if( !strcmp(argv[i], "-ccl") || !strcmp(argv[i], "--chrome-call-logging") )
        {
            checkSetEnv("CLI_ChromeCallLogging", "1");
        }
        else if( !strcmp(argv[i], "-cdt") || !strcmp(argv[i], "--chrome-device-timeline") )
        {
            checkSetEnv("CLI_ChromePerformanceTiming", "1");
        }
        else if( !strcmp(argv[i], "-ckt") || !strcmp(argv[i], "--chrome-kernel-timeline") )
        {
            checkSetEnv("CLI_ChromePerformanceTiming", "1");
            checkSetEnv("CLI_ChromePerformanceTimingPerKernel", "1");
        }
        else if( !strcmp(argv[i], "-cds") || !strcmp(argv[i], "--chrome-device-stages") )
        {
            checkSetEnv("CLI_ChromePerformanceTiming", "1");
            checkSetEnv("CLI_ChromePerformanceTimingInStages", "1");
        }
        else if( !strcmp(argv[i], "--driver-diagnostics") || !strcmp(argv[i], "-ddiag") )
        {
            checkSetEnv("CLI_ContextCallbackLogging", "1");
            checkSetEnv("CLI_ContextHintLevel", "7");    // GOOD, BAD, and NEUTRAL
        }
        else if( !strcmp(argv[i], "--mdapi-ebs") )
        {
            if( mdapiGroup == NULL )
            {
                mdapiGroup = "ComputeBasic";
            }
            checkSetEnv("CLI_DevicePerfCounterEventBasedSampling", "1" );
            checkSetEnv("CLI_DevicePerfCounterTiming", "1");
        }
        else if( !strcmp(argv[i], "--mdapi-tbs") )
        {
            if( mdapiGroup == NULL )
            {
                mdapiGroup = "ComputeBasic";
            }
            checkSetEnv("CLI_DevicePerfCounterTimeBasedSampling", "1" );
        }
        else if( !strcmp(argv[i], "--mdapi-group") )
        {
            ++i;
            if( i < argc )
            {
                mdapiGroup = argv[i];
            }
        }
        else if( !strcmp(argv[i], "-h") || !strcmp(argv[i], "--host-timing") )
        {
            checkSetEnv("CLI_HostPerformanceTiming", "1");
        }
        else if( !strcmp(argv[i], "-l") || !strcmp(argv[i], "--leak-checking") )
        {
            checkSetEnv("CLI_LeakChecking", "1");
        }
        else if( !strcmp(argv[i], "-f") || !strcmp(argv[i], "--output-to-file") )
        {
            checkSetEnv("CLI_LogToFile", "1");
            reportToStderr = "0";
        }
        else if( !strcmp(argv[i], "--dump-dir") )
        {
            ++i;
            if( i < argc )
            {
                checkSetEnv("CLI_DumpDir", argv[i]);
            }
        }
        else if (argv[i][0] == '-')
        {
            unknownOption = true;
        }
        else
        {
            if( mdapiGroup != NULL )
            {
                checkSetEnv("CLI_DevicePerfCounterCustom", mdapiGroup);
            }
            if (reportToStderr)
            {
                checkSetEnv("CLI_ReportToStderr", reportToStderr);
            }

#if defined(_WIN32)
            getCommandLine(argv, i);
#else // not Windows
            // Build command line argv for target application:
            appArgs = (char**)malloc((argc - i + 1) * sizeof(char*));
            int offset = i;
            for (; i < argc; i++)
            {
                appArgs[i - offset] = argv[i];
            }
            appArgs[argc - offset] = NULL;
#endif
            break;
        }
    }

    if( unknownOption ||
#if defined(_WIN32)
        commandLine.size() == 0
#else
        appArgs == NULL
#endif
        )
    {
        std::string defaultDumpDir = getDefaultDumpDirectory();
        fprintf(stdout,
            "cliloader - A utility to simplify using the Intercept Layer for OpenCL Applications\n"
            "  Version: %s, from %s\n"
            "\n"
            "Usage: cliloader [OPTIONS] COMMAND\n"
            "\n"
            "Options:\n"
            "  --debug                          Enable cliloader Debug Messages\n"
            "  --controls                       Print All Controls and Exit\n"
            "  --metrics                        Print All MDAPI Metrics and Exit\n"
#if !defined(_WIN32)
            "  --no-LD_PRELOAD                  Do not set LD_PRELOAD\n"
            "  --no-LD_LIBRARY_PATH             Do not set LD_LIBRARY_PATH\n"
#endif
            "\n"
            "  --quiet [-q]                     Disable Logging\n"
            "  --call-logging [-c]              Trace Host API Calls\n"
            "  --error-logging [-e]             Detect and Log API Errors\n"
            "  --tid                            Include Thread ID in the API Call Log\n"
            "  --appendpid                      Include Process ID in the Dump Directory\n"
            "  --demangle                       Demangle Kernel Names\n"
            "  --dump-source [-dsrc]            Dump Input Program Source\n"
            "  --dump-spirv [-dspv]             Dump Input Program IL (SPIR-V)\n"
            "  --dump-output-binaries           Dump Output Program Binaries\n"
            "  --dump-kernel-isa-binaries       Dump Kernel ISA Binaries (Intel GPU Only)\n"
            "  --device-timing [-d]             Report Device Execution Time\n"
            "  --device-timing-verbose [-dv]    Report More Detailed Device Execution Time\n"
            "  --chrome-call-logging [-ccl]     Record Host API Calls to a JSON Trace File\n"
            "  --chrome-device-timeline [-cdt]  Record Per-Queue Device Timeline to a JSON Trace File\n"
            "  --chrome-kernel-timeline [-ckt]  Record Per-Kernel Device Timeline to a JSON Trace File\n"
            "  --chrome-device-stages [-cds]    Record Device Timeline Stages to a JSON Trace File\n"
            "  --driver-diagnostics [-ddiag]    Log Driver Diagnostics\n"
            "  --mdapi-ebs                      Report Event-Based MDAPI Metrics (Intel GPU Only)\n"
            "  --mdapi-tbs                      Report Time-Based MDAPI Metrics (Intel GPU Only)\n"
            "  --mdapi-group <NAME>             Choose MDAPI Metrics to Collect (Intel GPU Only)\n"
            "  --host-timing [-h]               Report Host API Execution Time\n"
            "  --leak-checking [-l]             Track and Report OpenCL Leaks\n"
            "  --output-to-file [-f]            Log and Report to Files vs. stderr\n"
            "  --dump-dir <DIR>                 Specify the dump directory for log and report files,\n"
            "                                    default: %s\n"
            "\n"
            "For more information, please visit the Intercept Layer for OpenCL Applications page:\n"
            "    %s\n"
            "\n",
            g_scGitDescribe,
            g_scGitRefSpec,
            defaultDumpDir.c_str(),
            g_scURL );
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
    // Parse arguments
    if (!parseArguments(argc, argv))
    {
        return 1;
    }

    // Get full path to the directory for this process:
    std::string path = getProcessDirectory();

#if defined(_WIN32)

    // Get the existing value of the "SuppressLogging" control.
    // We will suppress logging while loading the intercept DLL
    // into this process, to avoid seeing loading twice.
    char* envVal = NULL;
    size_t  len = 0;
    errno_t err = _dupenv_s( &envVal, &len, "CLI_SuppressLogging" );
    DEBUG("CLI_SuppressLogging is currently: %s\n", envVal ? envVal : "");

    SETENV("CLI_SuppressLogging", "1");

    std::string dllpath = path + "\\opencl.dll";
    DEBUG("path to OpenCL.dll is: %s\n", dllpath.c_str());

    // First things first.  Load the intercept DLL into this process, and
    // try to get the function pointer to the init function.  If we can't
    // do this, there's no need to go further.
    HMODULE dll = LoadLibraryA(dllpath.c_str());
    if( dll == NULL )
    {
        die("loading DLL");
    }
    DEBUG("loaded DLL\n");

    SETENV("CLI_SuppressLogging", envVal ? envVal : "");
    if( envVal )
    {
        free( envVal );
    }

    LPTHREAD_START_ROUTINE cliprof_init = (LPTHREAD_START_ROUTINE)GetProcAddress(
        dll,
        "cliprof_init" );
    if( cliprof_init == NULL )
    {
        die("getting initialization function from DLL");
    }
    DEBUG("got pointer to init function\n");

    // The DLL exists and we're able to get the initialization function.

    // Create child process in suspended state:
    DEBUG("creating child process with command line: %s\n", commandLine.c_str());
    PROCESS_INFORMATION pinfo = { 0 };
    STARTUPINFOA sinfo = { 0 };
    sinfo.cb = sizeof(sinfo);
    if( CreateProcessA(
            NULL,                   // lpApplicationName
            (LPSTR)commandLine.c_str(),// lpCommandLine
            NULL,                   // lpProcessAttributes
            NULL,                   // lpThreadAttributes
            FALSE,                  // bInheritHandles
            CREATE_SUSPENDED,       // dwCreationFlags
            NULL,                   // lpEnvironment - use the cliloader environment
            NULL,                   // lpCurrentDirectory - use the cliloader drive and directory
            &sinfo,                 // lpStartupInfo
            &pinfo) == FALSE )      // lpProcessInformation (out)
    {
        die("creating child process");
    }
    DEBUG("created child process\n");

    // Check that we don't have a 32-bit and 64-bit mismatch:
    if( checkWow64(GetCurrentProcess(), pinfo.hProcess) )
    {
        // There is no 32-bit and 64-bit mismatch.
        // Start intercepting.

        // Allocate child memory for the full DLL path:
        void *childPath = VirtualAllocEx(
            pinfo.hProcess,
            NULL,
            dllpath.size() + 1,
            MEM_COMMIT,
            PAGE_READWRITE );
        if( childPath == NULL )
        {
            die("allocating child memory");
        }
        DEBUG("allocated child memory\n");

        // Write DLL path to child:
        if( WriteProcessMemory(
                pinfo.hProcess,
                childPath,
                (void*)dllpath.c_str(),
                dllpath.size() + 1,
                NULL ) == FALSE )
        {
            die("writing child memory");
        }
        DEBUG("wrote dll path to child memory\n");

        // Create a thread to load the intercept DLL in the child process:
        HANDLE childThread = CreateRemoteThread(
            pinfo.hProcess,
            NULL,
            0,
            (LPTHREAD_START_ROUTINE)GetProcAddress(
                GetModuleHandleA("kernel32.dll"),
                "LoadLibraryA"),
            childPath,
            0,
            NULL );
        if( childThread == NULL )
        {
            die("loading DLL in child process");
        }
        DEBUG("created child thread to load DLL\n");

        // Wait for child thread to complete:
        if( WaitForSingleObject(childThread, INFINITE) != WAIT_OBJECT_0 )
        {
            die("waiting for DLL loading");
        }
        DEBUG("child thread to load DLL completed\n");
        CloseHandle(childThread);
        VirtualFreeEx(pinfo.hProcess, childPath, dllpath.size() + 1, MEM_RELEASE);
        DEBUG("cleaned up child thread to load DLL\n");

        childThread = CreateRemoteThread(
            pinfo.hProcess,
            NULL,
            0,
            cliprof_init,
            NULL,
            0,
            NULL );
        if( childThread == NULL )
        {
            die("replacing functions in child thread");
        }
        DEBUG("created child thread to replace functions\n");

        // Wait for child thread to complete:
        if( WaitForSingleObject(childThread, INFINITE) != WAIT_OBJECT_0 )
        {
            die("waiting for initialization thread");
        }
        DEBUG("child thread to replace functions completed\n");
        CloseHandle(childThread);
        DEBUG("cleaned up child thread to replace functions\n");
    }

    FreeModule(dll);
    DEBUG("closed dll handle\n");

    // Resume child process:
    DEBUG("resuming child process\n");
    if( ResumeThread(pinfo.hThread) == -1 )
    {
        die("resuming thread");
    }
    DEBUG("child process resumed\n");

    // Wait for child process to finish
    if( WaitForSingleObject(pinfo.hProcess, INFINITE) != WAIT_OBJECT_0 )
    {
        die("waiting for child process failed");
    }
    DEBUG("child process completed, getting exit code\n");

    // Get return code and forward it
    DWORD retval = 0;
    if( GetExitCodeProcess(pinfo.hProcess, &retval) == FALSE )
    {
        die("getting child process exit code");
    }
    DEBUG("child process completed with exit code %u (%08X)\n", retval, retval);

    return retval;

#else // not Windows

    if( set_LD_PRELOAD || set_LD_LIBRARY_PATH )
    {
        std::string ld_preload;
        std::string ld_library_path;

        // Look for the CLIntercept shared library.
        // First, check the current directory.
        bool found = getEnvVars( path, ld_preload, ld_library_path );
        if( found == false )
        {
            // Next, check the parent directory.
            std::string libPath = path + "/..";
            found = getEnvVars( libPath, ld_preload, ld_library_path );
        }
        if( found == false )
        {
            // Next, check a lib directory.
            std::string libPath = path + "/../" + CLILOADER_LIB_DIR;
            found = getEnvVars( libPath, ld_preload, ld_library_path );
        }
        if( found == false )
        {
            // Next, check for an intercept directory.
            // This is for running cliloader straight from a CMake directory.
            std::string libPath = path + "/../intercept";
            found = getEnvVars( libPath, ld_preload, ld_library_path );
        }
        if( found )
        {
            if( set_LD_PRELOAD )
            {
                DEBUG("New %s is %s\n", LD_PRELOAD_ENV, ld_preload.c_str());
                SETENV(LD_PRELOAD_ENV, ld_preload.c_str());
            }

            if( set_LD_LIBRARY_PATH )
            {
                DEBUG("New %s is %s\n", LD_LIBRARY_PATH_ENV, ld_library_path.c_str());
                SETENV(LD_LIBRARY_PATH_ENV, ld_library_path.c_str());
            }
        }
        else
        {
            DEBUG("Couldn't find CLIntercept shared library!\n");
        }
    }

#ifdef __APPLE__
    SETENV("DYLD_FORCE_FLAT_NAMESPACE", "1");
#endif

    // Launch target application:
    if( execvp(appArgs[0], appArgs) == -1 )
    {
        die("failed to launch target application");
    }

#endif
}
