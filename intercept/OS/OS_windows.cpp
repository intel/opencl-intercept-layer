/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include "OS_windows.h"

namespace OS
{

Services::Services( void* pGlobalData )
{
    m_hInstance = (HINSTANCE)pGlobalData;
}

Services::~Services()
{
}

}
