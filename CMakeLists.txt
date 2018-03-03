# Copyright (c) 2018 Intel Corporation
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

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CONFIGURATION_TYPES Debug Release)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE PATH "Build Type" FORCE)
endif()

project(CLIntercept)

find_package( Threads )

# This uses modules from: https://github.com/rpavlik/cmake-modules
# to get Git revision information and put it in the generated files:
#   git_version.cpp - version information for CLIntercept log
#   git_version.rc2 - DLL version information (Windows only)
add_definitions("-DCLINTERCEPT_CMAKE")
include(cmake_modules/GetGitRevisionDescription.cmake)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(GIT_DESCRIBE)
configure_file(Src/git_version.cpp.in "${CMAKE_CURRENT_BINARY_DIR}/git_version.cpp" @ONLY)
configure_file(Src/git_version.rc.in "${CMAKE_CURRENT_BINARY_DIR}/git_version.rc2" @ONLY)

# Build the CLIntercept config app for 32-bit Windows builds, but not for 64-bit or other OSes.
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        add_subdirectory(config)
    endif()
endif()

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Src
)
link_directories(
)

set( CLINTERCEPT_OS_FILES
    OS/OS.h
)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    list( APPEND CLINTERCEPT_OS_FILES
        OS/OS_timer.h
        OS/OS_windows.cpp
        OS/OS_windows.h
        OS/OS_windows_common.cpp
        OS/OS_windows_common.h
    )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    list( APPEND CLINTERCEPT_OS_FILES
        OS/OS_linux.cpp
        OS/OS_linux.h
        OS/OS_linux_common.cpp
        OS/OS_linux_common.h
    )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    list( APPEND CLINTERCEPT_OS_FILES
        OS/OS_mac.cpp
        OS/OS_mac.h
        OS/OS_mac_common.cpp
        OS/OS_mac_common.h
    )
endif()
source_group( OS FILES
    ${CLINTERCEPT_OS_FILES}
)

set( CLINTERCEPT_RESOURCE_FILES
    Kernels/builtin_kernels.cl
    Kernels/precompiled_kernels.cl
    resource/clIntercept.rc
    resource/clIntercept_resource.h
    "${CMAKE_CURRENT_BINARY_DIR}/git_version.rc2"
)
source_group( Resources FILES
    ${CLINTERCEPT_RESOURCE_FILES}
)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/Kernels)
    #TODO: This is currently hard-coded for 64-bit Linux builds.
    set(CLINTERCEPT_KERNELS_OUTPUT_FORMAT elf64-x86-64)
    add_custom_command(OUTPUT ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/precompiled_kernels.o
        COMMAND mkdir -p ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}
        COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && objcopy -I binary -O ${CLINTERCEPT_KERNELS_OUTPUT_FORMAT} --binary-architecture i386 
            Kernels/precompiled_kernels.cl 
            ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/precompiled_kernels.o
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Kernels/precompiled_kernels.cl
    )
    add_custom_command(OUTPUT ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/builtin_kernels.o
        COMMAND mkdir -p ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}
        COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR} && objcopy -I binary -O ${CLINTERCEPT_KERNELS_OUTPUT_FORMAT} --binary-architecture i386 
            Kernels/builtin_kernels.cl 
            ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/builtin_kernels.o
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Kernels/builtin_kernels.cl
    )
    list( APPEND CLINTERCEPT_RESOURCE_FILES 
        ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/precompiled_kernels.o
        ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/builtin_kernels.o
    )
endif()

set( CLINTERCEPT_SOURCE_FILES
    Src/clIntercept.def
    Src/clIntercept.map
    Src/cli_ext.h
    Src/common.h
    Src/controls.h
    Src/dispatch.cpp
    Src/dispatch.h
    Src/enummap.cpp
    Src/enummap.h
    Src/instrumentation.h
    Src/intercept.cpp
    Src/intercept.h
    Src/main.cpp
    Src/stubs.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/git_version.cpp"
)
source_group( Source FILES
    ${CLINTERCEPT_SOURCE_FILES}
)

set( CLINTERCEPT_CL_HEADERS
    CL/cl.h
    CL/cl_gl.h
    CL/cl_platform.h
)
source_group( CL FILES
    ${CLINTERCEPT_CL_HEADERS}
)

# MDAPI Support (optional)
set( ENABLE_MDAPI CACHE BOOL "Enable MDAPI Support" )
if( ENABLE_MDAPI )
    add_definitions("-DUSE_MDAPI")
    include_directories( embargo/mdapi )
    set( CLINTERCEPT_MDAPI_FILES
        embargo/mdapi/DriverStorePath.h
        embargo/mdapi/intercept_mdapi.cpp
        embargo/mdapi/MetricsDiscoveryHelper.cpp
        embargo/mdapi/MetricsDiscoveryHelper.h
        embargo/mdapi/metrics_discovery_api.h
        embargo/mdapi/windef4linux.h
    )
    source_group( MDAPI FILES
        ${CLINTERCEPT_MDAPI_FILES}
    )
endif()

add_library( OpenCL SHARED
    ${CLINTERCEPT_CL_HEADERS}
    ${CLINTERCEPT_OS_FILES}
    ${CLINTERCEPT_RESOURCE_FILES}
    ${CLINTERCEPT_SOURCE_FILES}
    ${CLINTERCEPT_MDAPI_FILES}
)
set_target_properties( OpenCL PROPERTIES VERSION "1.2" SOVERSION "1" )
target_link_libraries( OpenCL ${CMAKE_DL_LIBS} )

if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
    target_link_libraries( OpenCL SetupAPI Shlwapi )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set_target_properties(OpenCL PROPERTIES COMPILE_FLAGS "-g -Wall -std=gnu++11")
    set_target_properties(OpenCL PROPERTIES LINK_FLAGS "-Wl,--version-script -Wl,${CMAKE_SOURCE_DIR}/Src/clIntercept.map")
    target_link_libraries( OpenCL ${CMAKE_THREAD_LIBS_INIT} )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    find_package(OpenCL REQUIRED)
    set_target_properties(OpenCL PROPERTIES COMPILE_FLAGS "-g -Wall -arch i386 -arch x86_64")
    target_link_libraries( OpenCL OpenCL::OpenCL )
endif()

# ITT Support (optional)
set( ENABLE_ITT CACHE BOOL "Enable ITT (Instrumentation Tracing Technology) API Support" )
set( PROGRAMFILES_X86 "PROGRAMFILES(X86)" )
find_path( VTUNE_INCLUDE_DIR ittnotify.h
    HINTS
    /opt/intel/vtune_amplifier_xe/include
    "$ENV{${PROGRAMFILES_X86}}/Intel/VTune\ Amplifier\ XE/include" )
if (CMAKE_SIZEOF_VOID_P EQUAL 4)
    #message( STATUS "Searching for 32-bit ittnotify lib..." )
    find_library( VTUNE_ITTNOTIFY_LIB NAMES ittnotify libittnotify
        HINTS
        /opt/intel/vtune_amplifier_xe/lib32
        "$ENV{${PROGRAMFILES_X86}}/Intel/VTune\ Amplifier\ XE/lib32" )
else()
    #message( STATUS "Searching for 64-bit ittnotify lib..." )
    find_library( VTUNE_ITTNOTIFY_LIB NAMES ittnotify libittnotify
        HINTS
        /opt/intel/vtune_amplifier_xe/lib64
        "$ENV{${PROGRAMFILES_X86}}/Intel/VTune\ Amplifier\ XE/lib64" )
endif()
if( ENABLE_ITT )
    add_definitions("-DUSE_ITT")
    include_directories( ${VTUNE_INCLUDE_DIR} )
    message( STATUS "VTune ITTNotify Lib is: ${VTUNE_ITTNOTIFY_LIB}" )
    target_link_libraries( OpenCL ${VTUNE_ITTNOTIFY_LIB} ${CMAKE_THREAD_LIBS_INIT} )
endif()

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CLINTERCEPT_PLATFORM_NAME "x64")
    elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(CLINTERCEPT_PLATFORM_NAME "Win32")
    else()
        set(CLINTERCEPT_PLATFORM_NAME "Unknown")
    endif()

    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/Builds/${CLINTERCEPT_PLATFORM_NAME}" CACHE PATH "Install Path" FORCE)
    endif()

    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
        string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER )

        # For Windows debug builds, we want to statically link the C runtime.
        # If we do not statically link the C runtime then the target machine
        # must install Visual Studio, which is not desirable.
        if( ${OUTPUTCONFIG_UPPER} MATCHES "DEBUG" )
            #message( STATUS "DEBUG build detected!" )
            #message( STATUS "  'CMAKE_CXX_FLAGS_${OUTPUTCONFIG_UPPER}': ${CMAKE_CXX_FLAGS_${OUTPUTCONFIG_UPPER}}" )
            #message( STATUS "->" )
            if( CMAKE_CXX_FLAGS_${OUTPUTCONFIG_UPPER} MATCHES "/MD" )
                string( REGEX REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_${OUTPUTCONFIG_UPPER} "${CMAKE_CXX_FLAGS_${OUTPUTCONFIG_UPPER}}" )
            endif()
            #message( STATUS "  'CMAKE_CXX_FLAGS_${OUTPUTCONFIG_UPPER}': ${CMAKE_CXX_FLAGS_${OUTPUTCONFIG_UPPER}}" )
        endif()
        install(TARGETS OpenCL DESTINATION ${OUTPUTCONFIG} CONFIGURATIONS ${OUTPUTCONFIG})
    endforeach()
else()
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install" CACHE PATH "Install Path" FORCE)
    endif()
    install(TARGETS OpenCL DESTINATION "lib")
endif()
