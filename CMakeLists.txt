# Copyright (c) 2018-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_VERBOSE_MAKEFILE ON)

project(CLIntercept VERSION 3.0.3 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to RelWithDebInfo")
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE PATH "Build Type" FORCE)
endif()

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CLINTERCEPT_PLATFORM_NAME "x64")
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(CLINTERCEPT_PLATFORM_NAME "Win32")
    else()
        set(CLINTERCEPT_PLATFORM_NAME "Unknown")
    endif()

    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/Builds/${CLINTERCEPT_PLATFORM_NAME}" CACHE PATH "Install Path" FORCE)
    endif()
elseif(UNIX)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install" CACHE PATH "Install Path" FORCE)
    endif()
endif()

option(ENABLE_CLILOADER "Enable cliloader Support and Build the Executable" ON)
option(ENABLE_CLIPROF "Enable cliprof Support and Build the Executable")
option(ENABLE_ITT "Enable ITT (Instrumentation Tracing Technology) API Support")
option(ENABLE_MDAPI "Enable MDAPI Support" ON)
option(ENABLE_HIGH_RESOLUTION_CLOCK "Use the high_resolution_clock for timing instead of the steady_clock")
if(WIN32)
    option(ENABLE_CLICONFIG "Build the cliconfig Configuration Utility" ON)
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    option(ENABLE_KERNEL_OVERRIDES "Enable Embedding Kernel Override Strings" ON)
    option(ENABLE_SCRIPTS "Enable Embedding Script Strings" ON)
endif()

# This uses modules from: https://github.com/rpavlik/cmake-modules
# to get Git revision information and put it in the generated files:
#   git_version.cpp - version information for CLIntercept log
#   git_version.rc2 - DLL version information (Windows only)
include(cmake_modules/GetGitRevisionDescription.cmake)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(GIT_DESCRIBE)

find_package(Threads)

add_subdirectory(intercept)

# CLIntercept config app (optional - Windows only)
if(WIN32 AND ENABLE_CLICONFIG)
    add_subdirectory(cliconfig)
endif()

# cliprof Executable (optional)
if(ENABLE_CLIPROF)
    add_subdirectory(cliprof)
endif()

# cliloader Executable (optional)
if(ENABLE_CLILOADER)
    add_subdirectory(cliloader)
endif()

# cpack
include(cmake_modules/package.cmake)
