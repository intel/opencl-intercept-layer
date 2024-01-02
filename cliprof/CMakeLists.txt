# Copyright (c) 2018-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT

# This uses modules from: https://github.com/rpavlik/cmake-modules
# to get Git revision information and put it in the generated files:
#   git_version.h - version information for cliprof
configure_file(git_version.h.in "${CMAKE_CURRENT_BINARY_DIR}/git_version.h" @ONLY)

set( CLIPROF_SOURCE_FILES
    cliprof.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/git_version.h"
)
source_group( Source FILES
    ${CLIPROF_SOURCE_FILES}
)

add_executable(cliprof
    ${CLIPROF_SOURCE_FILES}
)
add_dependencies(cliprof OpenCL)
target_include_directories(cliprof PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
        install(TARGETS cliprof DESTINATION ${OUTPUTCONFIG} CONFIGURATIONS ${OUTPUTCONFIG})
    endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
else()
    include(GNUInstallDirs)
    target_compile_definitions(cliprof PRIVATE CLIPROF_LIB_DIR="${CMAKE_INSTALL_LIBDIR}")
    install(TARGETS cliprof DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
