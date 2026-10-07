/*
// Copyright (c) 2024-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#include <cstdint>
#include <fstream>
#include <string>

namespace Utils
{

std::string GetUniqueFileName(const std::string& fileName);
uint32_t CountLeadingZeroes(uint64_t value);

// Opens an output file stream, using the \\?\ long-path prefix on Windows
// to support paths longer than MAX_PATH (260 characters).
void OpenOutputFile(
    std::ofstream& os,
    const std::string& fileName,
    std::ios::openmode mode = std::ios::out );

}
