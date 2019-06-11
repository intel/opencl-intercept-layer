/*
// Copyright (c) 2018-2019 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
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
#include "src\controls.h"
};

const int cNumVars = sizeof(cVars) / sizeof(cVars[0]);

#undef CLI_CONTROL
#undef CLI_CONTROL_SEPARATOR
