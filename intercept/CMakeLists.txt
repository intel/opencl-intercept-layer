# Copyright (c) 2018-2019 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set(CLINTERCEPT_CL_HEADERS
    CL/cl.h
    CL/cl_gl.h
    CL/cl_platform.h
    CL/cl_version.h
)
source_group(CL FILES
    ${CLINTERCEPT_CL_HEADERS}
)

set(CLINTERCEPT_OS_FILES
    OS/OS.h
    OS/OS_timer.h
)
if(WIN32)
    list(APPEND CLINTERCEPT_OS_FILES
        OS/OS_windows.cpp
        OS/OS_windows.h
        OS/OS_windows_common.cpp
        OS/OS_windows_common.h
    )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR
       ${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    list(APPEND CLINTERCEPT_OS_FILES
        OS/OS_linux.cpp
        OS/OS_linux.h
        OS/OS_linux_common.cpp
        OS/OS_linux_common.h
    )
elseif(APPLE)
    list(APPEND CLINTERCEPT_OS_FILES
        OS/OS_mac.cpp
        OS/OS_mac.h
        OS/OS_mac_common.cpp
        OS/OS_mac_common.h
    )
endif()
source_group(OS FILES
    ${CLINTERCEPT_OS_FILES}
)

set(CLINTERCEPT_RESOURCE_FILES
    kernels/builtin_kernels.cl
    kernels/precompiled_kernels.cl
    resource/clIntercept.rc
    resource/clIntercept_resource.h
    "${CMAKE_CURRENT_BINARY_DIR}/git_version.rc2"
)
source_group(Resources FILES
    ${CLINTERCEPT_RESOURCE_FILES}
)

set(CLINTERCEPT_SOURCE_FILES
    src/clIntercept.def
    src/clIntercept.map
    src/cli_ext.h
    src/cliprof_init.cpp
    src/common.h
    src/controls.h
    src/dispatch.cpp
    src/dispatch.h
    src/enummap.cpp
    src/enummap.h
    src/instrumentation.h
    src/intercept.cpp
    src/intercept.h
    src/main.cpp
    src/objtracker.cpp
    src/objtracker.h
    src/stubs.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/git_version.cpp"
)
source_group(Source FILES
    ${CLINTERCEPT_SOURCE_FILES}
)

add_library(OpenCL SHARED
    ${CLINTERCEPT_CL_HEADERS}
    ${CLINTERCEPT_OS_FILES}
    ${CLINTERCEPT_RESOURCE_FILES}
    ${CLINTERCEPT_SOURCE_FILES}
    ${CLINTERCEPT_MDAPI_FILES}
)
set_target_properties(OpenCL PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
target_link_libraries(OpenCL ${CMAKE_DL_LIBS})

# This uses modules from: https://github.com/rpavlik/cmake-modules
# to get Git revision information and put it in the generated files:
#   git_version.cpp - version information for CLIntercept log
#   git_version.rc2 - DLL version information (Windows only)
target_compile_definitions(OpenCL PRIVATE CLINTERCEPT_CMAKE)
configure_file(src/git_version.cpp.in "${CMAKE_CURRENT_BINARY_DIR}/git_version.cpp" @ONLY)
configure_file(src/git_version.rc.in "${CMAKE_CURRENT_BINARY_DIR}/git_version.rc2" @ONLY)

if(ENABLE_CLIPROF)
    target_compile_definitions(OpenCL PRIVATE CLINTERCEPT_CLILOADER)
endif()

if(ENABLE_CLILOADER)
    target_compile_definitions(OpenCL PRIVATE CLINTERCEPT_CLILOADER)
endif()

target_include_directories(OpenCL PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Kernel Override Support (optional)
if(WIN32)
    # always enabled on Windows
    target_compile_definitions(OpenCL PRIVATE USE_KERNEL_OVERRIDES)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR
       ${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    # conditionally enabled on Linux and Android
    if(ENABLE_KERNEL_OVERRIDES)
        target_compile_definitions(OpenCL PRIVATE USE_KERNEL_OVERRIDES)

        set(CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/kernels)
        add_custom_command(OUTPUT ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/precompiled_kernels.o
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}
            COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_LINKER} -r -b binary
                kernels/precompiled_kernels.cl
                -o ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/precompiled_kernels.o
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/kernels/precompiled_kernels.cl
        )
        add_custom_command(OUTPUT ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/builtin_kernels.o
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}
            COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_LINKER} -r -b binary
                kernels/builtin_kernels.cl
                -o ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/builtin_kernels.o
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/kernels/builtin_kernels.cl
        )
        target_sources(OpenCL PRIVATE
            ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/precompiled_kernels.o
            ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/builtin_kernels.o
        )
    endif()
elseif(APPLE)
    # never enabled on OSX (for now)
endif()

# MDAPI Support (optional)
if(ENABLE_MDAPI)
    target_compile_definitions(OpenCL PRIVATE USE_MDAPI)
    include_directories(mdapi)
    set(CLINTERCEPT_MDAPI_FILES
        mdapi/DriverStorePath.h
        mdapi/intercept_mdapi.cpp
        mdapi/MetricsDiscoveryHelper.cpp
        mdapi/MetricsDiscoveryHelper.h
        mdapi/metrics_discovery_api.h
    )
    source_group(MDAPI FILES
        ${CLINTERCEPT_MDAPI_FILES}
    )
    target_sources(OpenCL PRIVATE
        ${CLINTERCEPT_MDAPI_FILES}
    )
endif()

if(WIN32)
    target_compile_options(OpenCL PRIVATE /EHsc)
    target_link_libraries(OpenCL SetupAPI Shlwapi)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_compile_options(OpenCL PRIVATE -Wall)
    # TODO: Move this to target_link_libraries?
    set_target_properties(OpenCL PROPERTIES LINK_FLAGS "-Wl,--version-script -Wl,${CMAKE_CURRENT_SOURCE_DIR}/src/clIntercept.map")
    target_link_libraries(OpenCL ${CMAKE_THREAD_LIBS_INIT})
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    target_compile_options(OpenCL PRIVATE -Wall)
    # TODO: Move this to target_link_libraries?
    set_target_properties(OpenCL PROPERTIES LINK_FLAGS "-Wl,--version-script -Wl,${CMAKE_CURRENT_SOURCE_DIR}/src/clIntercept.map")
    target_link_libraries(OpenCL ${CMAKE_THREAD_LIBS_INIT} log)
elseif(APPLE)
    find_package(OpenCL REQUIRED)
    # TODO: Can we switch this to target_compile_options?  Something like:
    #target_compile_options(OpenCL PRIVATE -Wall -stdlib=libc++ -arch i386 -arch x86_64)
    set_target_properties(OpenCL PROPERTIES COMPILE_FLAGS "-g -Wall -stdlib=libc++ -arch i386 -arch x86_64")
    target_link_libraries(OpenCL OpenCL::OpenCL)
endif()

# ITT Support (optional)
set(PROGRAMFILES_X86 "PROGRAMFILES(X86)")
find_path(VTUNE_INCLUDE_DIR ittnotify.h
    HINTS
    /opt/intel/vtune_amplifier_xe/include
    "$ENV{${PROGRAMFILES_X86}}/Intel/VTune\ Amplifier\ XE/include")
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    #message(STATUS "Searching for 32-bit ittnotify lib...")
    find_library(VTUNE_ITTNOTIFY_LIB NAMES ittnotify libittnotify
        HINTS
        /opt/intel/vtune_amplifier_xe/lib32
        "$ENV{${PROGRAMFILES_X86}}/Intel/VTune\ Amplifier\ XE/lib32")
else()
    #message(STATUS "Searching for 64-bit ittnotify lib...")
    find_library(VTUNE_ITTNOTIFY_LIB NAMES ittnotify libittnotify
        HINTS
        /opt/intel/vtune_amplifier_xe/lib64
        "$ENV{${PROGRAMFILES_X86}}/Intel/VTune\ Amplifier\ XE/lib64")
endif()
if(ENABLE_ITT)
    target_compile_definitions(OpenCL PRIVATE USE_ITT)
    target_include_directories(OpenCL PRIVATE ${VTUNE_INCLUDE_DIR})
    message(STATUS "VTune ITTNotify Lib is: ${VTUNE_ITTNOTIFY_LIB}")
    target_link_libraries(OpenCL ${VTUNE_ITTNOTIFY_LIB} ${CMAKE_THREAD_LIBS_INIT})
endif()

if(WIN32)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)

        # For Windows debug builds, we want to statically link the C runtime.
        # If we do not statically link the C runtime then the target machine
        # must install Visual Studio, which is not desirable.
        if(${OUTPUTCONFIG_UPPER} MATCHES "DEBUG")
            #message(STATUS "DEBUG build detected!")
            #message(STATUS "  'CMAKE_CXX_FLAGS_${OUTPUTCONFIG_UPPER}': ${CMAKE_CXX_FLAGS_${OUTPUTCONFIG_UPPER}}")
            #message(STATUS "->")
            if(CMAKE_CXX_FLAGS_${OUTPUTCONFIG_UPPER} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_${OUTPUTCONFIG_UPPER} "${CMAKE_CXX_FLAGS_${OUTPUTCONFIG_UPPER}}")
            endif()
            #message(STATUS "  'CMAKE_CXX_FLAGS_${OUTPUTCONFIG_UPPER}': ${CMAKE_CXX_FLAGS_${OUTPUTCONFIG_UPPER}}")
        endif()

        install(TARGETS OpenCL DESTINATION ${OUTPUTCONFIG} CONFIGURATIONS ${OUTPUTCONFIG})
    endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
elseif(UNIX)
    include(GNUInstallDirs)
    install(TARGETS OpenCL DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()