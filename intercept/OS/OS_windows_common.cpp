/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include "OS_windows_common.h"

namespace OS
{

const char* Services_Common::ENV_PREFIX = "";
const char* Services_Common::REGISTRY_KEY = "SOFTWARE\\INTEL\\IGFX";
const char* Services_Common::LOG_DIR = NULL;
bool Services_Common::APPEND_PID = false;

Services_Common::Services_Common()
{
}

Services_Common::~Services_Common()
{
}

}
