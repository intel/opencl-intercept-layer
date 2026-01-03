/*
// Copyright (c) 2018-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include "OS_windows.h"

namespace OS
{

const char* Services::ENV_PREFIX = "";
const char* Services::REGISTRY_KEY = "SOFTWARE\\INTEL\\IGFX";
const char* Services::LOG_DIR = NULL;
bool Services::APPEND_PID = false;

Services::Services( void* pGlobalData )
{
    m_hInstance = (HINSTANCE)pGlobalData;
}

}
