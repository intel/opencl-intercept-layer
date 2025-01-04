/*
// Copyright (c) 2018-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#include <vector>

#include <stdio.h>

struct cliControl
{
    bool    IsSeparator;
    const char* Name;
    const char* Type;
    const char* Description;
};

#define CLI_CONTROL( _type, _name, _init, _desc )           \
{                                                           \
    false,                                                  \
    #_name,                                                 \
    #_type,                                                 \
    _desc,                                                  \
},

#define CLI_CONTROL_SEPARATOR( _name )                      \
{                                                           \
    true,                                                   \
    #_name,                                                 \
    "",                                                     \
    "",                                                     \
},

static const std::vector<cliControl> controls =
{
    { true, "Startup Controls:", "", ""},
    { false, "BreakOnLoad", "bool", "If set to a nonzero value, the Intercept Layer for OpenCL Applications will break into the debugger when the DLL is loaded." },
    { false, "std::string", "OpenCLFileName", "Used to control the DLL or Shared Library that the Intercept Layer for OpenCL Applications loads to make real OpenCL calls. If present, only this file name is loaded. If omitted, the Intercept Layer for OpenCL Applications will search a default set of real OpenCL file names." },
#include "src/controls.h"
};

#undef CLI_CONTROL
#undef CLI_CONTROL_SEPARATOR

static void printControls()
{
    for (const auto& control : controls )
    {
        if (control.IsSeparator)
        {
            printf("%s\n", control.Name);
            printf("========================================\n\n");
        }
        else
        {
            printf("%s (%s):\n", control.Name, control.Type);
            printf("%s\n\n", control.Description);
        }
    }
}
