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

set(CMAKE_MFC_FLAG 2)

set(CLICONFIG_ICON_FILES
    clintercept_logo.ico
    default.ico
    disabled.ico
    modified.ico
    modified_default.ico
    modified_nondefault.ico
    nondefault.ico
    separator.ico
)
source_group( Icons FILES
    ${CLICONFIG_ICON_FILES}
)

set( CLICONFIG_SOURCE_FILES
    CLIConfig.cpp
    CLIConfig.h
    CLIConfig.rc
    CLIConfig_version.rc2
    envVars.h
    resource.h
)
source_group( Source FILES
    ${CLICONFIG_SOURCE_FILES}
)

add_executable(CLIConfig WIN32
    ${CLICONFIG_ICON_FILES}
    ${CLICONFIG_SOURCE_FILES}
)
target_compile_definitions(CLIConfig PRIVATE _AFXDLL)
target_compile_definitions(CLIConfig PRIVATE UNICODE _UNICODE)
target_include_directories(CLIConfig PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../intercept)
set_target_properties(CLIConfig PROPERTIES LINK_FLAGS "/ENTRY:\"wWinMainCRTStartup\"")

foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    install(TARGETS CLIConfig DESTINATION ${OUTPUTCONFIG}/Config CONFIGURATIONS ${OUTPUTCONFIG})
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
