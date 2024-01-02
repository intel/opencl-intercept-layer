/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

enum CONTROL_TYPE
{
    CONTROL_TYPE_INT        = 0,
    CONTROL_TYPE_BOOL       = 1,
    CONTROL_TYPE_STRING     = 2,
    CONTROL_TYPE_SEPARATOR  = 3,
};

template<class T>
static CONTROL_TYPE GetControlType()
{
    return CONTROL_TYPE_INT;
}

template<>
static CONTROL_TYPE GetControlType<bool>()
{
    return CONTROL_TYPE_BOOL;
}

template<>
static CONTROL_TYPE GetControlType<std::string>()
{
    return CONTROL_TYPE_STRING;
}

template<class T>
static const char* GetStringDefault( T dummy )
{
    return "";
}
template<>
static const char* GetStringDefault<const char*>( const char* init )
{
    return init;
}

template<class T>
static const int GetIntDefault( T init )
{
    return init;
}
template<>
static const int GetIntDefault<const char*>( const char* dummy )
{
    return 0;
}

struct VarDescription
{
    CONTROL_TYPE    Type;
    const std::string   Name;
    const std::string   defStrValue;
    int                 defIntValue;
    const std::string   HelpText;
};

#define CLI_CONTROL( _type, _name, _init, _desc )           \
{                                                           \
    GetControlType<_type>(),                                \
    #_name,                                                 \
    GetStringDefault(_init),                                \
    GetIntDefault(_init),                                   \
    _desc,                                                  \
},

#define CLI_CONTROL_SEPARATOR( _name )                      \
{                                                           \
    CONTROL_TYPE_SEPARATOR,                                 \
    #_name,                                                 \
    "",                                                     \
    0,                                                      \
    "",                                                     \
},

static const VarDescription cVars[] =
{
    { CONTROL_TYPE_BOOL, "BreakOnLoad", "", 0, "If set to a nonzero value, the Intercept Layer for OpenCL Applications will break into the debugger when the DLL is loaded." },
    { CONTROL_TYPE_STRING, "OpenCLFileName", "", 0, "Used to control the DLL or Shared Library that the Intercept Layer for OpenCL Applications loads to make real OpenCL calls. If present, only this file name is loaded. If omitted, the Intercept Layer for OpenCL Applications will search a default set of real OpenCL file names." },
#include "src/controls.h"
};

const int cNumVars = sizeof(cVars) / sizeof(cVars[0]);

#undef CLI_CONTROL
#undef CLI_CONTROL_SEPARATOR
