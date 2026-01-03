/*
// Copyright (c) 2024-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#include <cstdint>
#include <string>

namespace Utils
{

std::string GetUniqueFileName(const std::string& fileName);
uint32_t CountLeadingZeroes(uint64_t value);

}
