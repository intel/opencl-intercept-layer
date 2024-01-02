# Copyright (c) 2018-2024 Intel Corporation
#
# SPDX-License-Identifier: MIT

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
add_dependencies(CLIConfig OpenCL)
target_compile_definitions(CLIConfig PRIVATE _AFXDLL)
target_compile_definitions(CLIConfig PRIVATE UNICODE _UNICODE)
target_include_directories(CLIConfig PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../intercept)
set_target_properties(CLIConfig PROPERTIES LINK_FLAGS "/ENTRY:\"wWinMainCRTStartup\"")

foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    install(TARGETS CLIConfig DESTINATION ${OUTPUTCONFIG}/Config CONFIGURATIONS ${OUTPUTCONFIG})
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
