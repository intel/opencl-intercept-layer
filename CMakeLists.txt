# Copyright (c) 2018-2020 Intel Corporation
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

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_VERBOSE_MAKEFILE ON)

project(CLIntercept VERSION 1.2 LANGUAGES CXX)

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
if(WIN32)
    option(ENABLE_CLICONFIG "Build the cliconfig Configuration Utility" ON)
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux" OR ${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    option(ENABLE_KERNEL_OVERRIDES "Enable Embedding Kernel Override Strings" ON)
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

if(UNIX)
    include(GNUInstallDirs)

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
    set(CPACK_PACKAGE_VERSION_MAJOR 3)
    set(CPACK_PACKAGE_VERSION_MINOR 0)
    set(CPACK_PACKAGE_VERSION_PATCH 0)
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
        if(CPACK_GENERATOR STREQUAL "DEB")
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
