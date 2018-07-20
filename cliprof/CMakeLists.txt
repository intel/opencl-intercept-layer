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

# This uses modules from: https://github.com/rpavlik/cmake-modules
# to get Git revision information and put it in the generated files:
#   git_version.h - version information for cliprof
configure_file(git_version.h.in "${CMAKE_CURRENT_BINARY_DIR}/git_version.h" @ONLY)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
)

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

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
        install(TARGETS cliprof DESTINATION ${OUTPUTCONFIG} CONFIGURATIONS ${OUTPUTCONFIG})
    endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
else()
    install(TARGETS cliprof DESTINATION "lib")
endif()
