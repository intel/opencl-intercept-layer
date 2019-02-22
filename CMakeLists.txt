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

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CONFIGURATION_TYPES Debug Release)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
#set(CMAKE_VERBOSE_MAKEFILE ON)

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
        add_subdirectory(cliconfig)
    endif()
endif()

# cliprof Executable (optional)
set( ENABLE_CLIPROF CACHE BOOL "Enable cliprof Support and Build the Executable" )
if( ENABLE_CLIPROF )
    add_definitions("-DCLINTERCEPT_CLILOADER")
    add_subdirectory(cliprof)
endif()

# cliloader Executable (optional)
set( ENABLE_CLILOADER CACHE BOOL "Enable cliloader Support and Build the Executable" )
if( ENABLE_CLILOADER )
    add_definitions("-DCLINTERCEPT_CLILOADER")
    add_subdirectory(cliloader)
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
    OS/OS_timer.h
)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    list( APPEND CLINTERCEPT_OS_FILES
        OS/OS_windows.cpp
        OS/OS_windows.h
        OS/OS_windows_common.cpp
        OS/OS_windows_common.h
    )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR
       ${CMAKE_SYSTEM_NAME} STREQUAL "Android" )
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
# Kernel Override Support (optional)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    # always enabled on Windows
    add_definitions("-DUSE_KERNEL_OVERRIDES")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR
       ${CMAKE_SYSTEM_NAME} STREQUAL "Android" )
    # conditionally enabled on Linux and Android
    set( ENABLE_KERNEL_OVERRIDES TRUE CACHE BOOL "Enable Embedding Kernel Override Strings" )
    if( ENABLE_KERNEL_OVERRIDES )
        add_definitions("-DUSE_KERNEL_OVERRIDES")

        set(CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/Kernels)
        add_custom_command(OUTPUT ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/precompiled_kernels.o
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}
            COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_LINKER} -r -b binary
                Kernels/precompiled_kernels.cl
                -o ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/precompiled_kernels.o
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Kernels/precompiled_kernels.cl
        )
        add_custom_command(OUTPUT ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/builtin_kernels.o
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}
            COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_LINKER} -r -b binary
                Kernels/builtin_kernels.cl
                -o ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/builtin_kernels.o
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Kernels/builtin_kernels.cl
        )
        list( APPEND CLINTERCEPT_RESOURCE_FILES
            ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/precompiled_kernels.o
            ${CLINTERCEPT_KERNELS_OUTPUT_DIRECTORY}/builtin_kernels.o
        )
    endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    # never enabled on OSX (for now)
endif()

set( CLINTERCEPT_SOURCE_FILES
    Src/clIntercept.def
    Src/clIntercept.map
    Src/cli_ext.h
    Src/cliprof_init.cpp
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
    Src/objtracker.cpp
    Src/objtracker.h
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
    CL/cl_version.h
)
source_group( CL FILES
    ${CLINTERCEPT_CL_HEADERS}
)

# MDAPI Support (optional)
set( ENABLE_MDAPI TRUE CACHE BOOL "Enable MDAPI Support" )
if( ENABLE_MDAPI )
    add_definitions("-DUSE_MDAPI")
    include_directories( mdapi )
    set( CLINTERCEPT_MDAPI_FILES
        mdapi/DriverStorePath.h
        mdapi/intercept_mdapi.cpp
        mdapi/MetricsDiscoveryHelper.cpp
        mdapi/MetricsDiscoveryHelper.h
        mdapi/metrics_discovery_api.h
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
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set_target_properties(OpenCL PROPERTIES COMPILE_FLAGS "-g -Wall -std=gnu++11")
    set_target_properties(OpenCL PROPERTIES LINK_FLAGS "-Wl,--version-script -Wl,${CMAKE_SOURCE_DIR}/Src/clIntercept.map")
    target_link_libraries( OpenCL ${CMAKE_THREAD_LIBS_INIT} log)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    find_package(OpenCL REQUIRED)
    set_target_properties(OpenCL PROPERTIES COMPILE_FLAGS "-g -Wall -std=c++11 -stdlib=libc++ -arch i386 -arch x86_64")
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
elseif(UNIX)
    include(GNUInstallDirs)

    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install" CACHE PATH "Install Path" FORCE)
    endif()
    install(TARGETS OpenCL DESTINATION ${CMAKE_INSTALL_LIBDIR})

    if(CLINTERCEPT_CPACK_GENERATOR)
        set(CPACK_GENERATOR "${CLINTERCEPT_CPACK_GENERATOR}")
    else()
        # If generators list was not define build native package for current distro
        if(EXISTS "/etc/debian_version")
            set(CPACK_GENERATOR "DEB")
        elseif(EXISTS "/etc/redhat-release")
            set(CPACK_GENERATOR "RPM")
        else()
            set(CPACK_GENERATOR "TXZ")
        endif()
    endif()

    set(CPACK_PACKAGE_NAME "clintercept")
    set(CPACK_PACKAGE_VENDOR "Intel(R) Corporation")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Intercept Layer for OpenCL(tm) Applications")
    set(CPACK_PACKAGE_ARCHITECTURE "x86_64")

    # TODO: Figure out how to get this from a git tag.
    set(CPACK_PACKAGE_VERSION_MAJOR 2)
    set(CPACK_PACKAGE_VERSION_MINOR 2)
    set(CPACK_PACKAGE_VERSION_PATCH 1)
    set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")

    set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CLINTERCEPT_INSTALL_TIME_ROOT_DIR})
    set(CPACK_SET_DESTDIR TRUE)
    set(CPACK_PACKAGE_RELOCATABLE FALSE)

    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Intel")
    set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")

    set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
    set(CPACK_RPM_PACKAGE_RELEASE 1)
    set(CPACK_RPM_COMPRESSION_TYPE "xz")

    if(CMAKE_VERSION VERSION_GREATER 3.6 OR CMAKE_VERSION VERSION_EQUAL 3.6)
        set(CPACK_DEBIAN_CLINTERCEPT_FILE_NAME "clintercept_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")
        set(CPACK_RPM_CLINTERCEPT_FILE_NAME "clintercept-${CPACK_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}%{?dist}.${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm")
        set(CPACK_ARCHIVE_CLINTERCEPT_FILE_NAME "clintercept-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_ARCHITECTURE}")
    else()
        if (CPACK_GENERATOR STREQUAL "DEB")
            set(CPACK_PACKAGE_FILE_NAME "clintercept_${CPACK_PACKAGE_VERSION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
        elseif(CPACK_GENERATOR STREQUAL "RPM")
            set(CPACK_PACKAGE_FILE_NAME "clintercept-${CPACK_PACKAGE_VERSION}-${CPACK_RPM_PACKAGE_RELEASE}%{?dist}.${CPACK_RPM_PACKAGE_ARCHITECTURE}.rpm")
        else()
            set(CPACK_PACKAGE_FILE_NAME "clintercept-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_ARCHITECTURE}")
        endif()
    endif()

    set(CPACK_COMPONENT_INSTALL ON)
    set(CPACK_DEB_COMPONENT_INSTALL ON)
    set(CPACK_RPM_COMPONENT_INSTALL ON)
    set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

    include (CPack)
endif()
