/*
// Copyright (c) 2024-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include "utils.h"

#include <algorithm>
#include <fstream>
#include <string>

#ifdef __cpp_lib_bitops
#include <bit>
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#include <intrin.h>
#endif

#ifndef __has_builtin
#define __has_builtin(x)    0
#endif

namespace Utils
{

std::string GetUniqueFileName(const std::string& fileName)
{
    auto fileExists = [](const std::string &name) -> bool {
        std::ifstream f(name.c_str());
        return f.good();
    };

    if (!fileExists(fileName))
    {
        return fileName;
    }

    // Note: Assumes that the "/" is used as a path separator!
    std::size_t lastSlashPos = fileName.find_last_of("/");
    std::size_t dotPos = fileName.find_last_of('.');
    if( lastSlashPos != std::string::npos &&
        dotPos != std::string::npos &&
        dotPos < lastSlashPos )
    {
        dotPos = std::string::npos;
    }

    std::string baseName =
        (dotPos == std::string::npos) ?
        fileName :
        fileName.substr(0, dotPos);
    std::string extension =
        (dotPos == std::string::npos) ?
        "" :
        fileName.substr(dotPos);

    int counter = 0;
    std::string newFileName;
    do
    {
        newFileName = baseName + "-" + std::to_string(counter++) + extension;
    }
    while (fileExists(newFileName));

    return newFileName;
}

uint32_t CountLeadingZeroes(uint64_t value)
{
#ifdef __cpp_lib_bitops
    return std::countl_zero(value);
#elif defined(_WIN32) && !defined(__MINGW32__)
    if( value == 0 ) { return 64; }

    unsigned long index;

    if( value < 1ULL << 32 )
    {
        _BitScanReverse(&index, static_cast<unsigned long>(value));
        return 63 - index;
    }

    value >>= 32;
    _BitScanReverse(&index, static_cast<unsigned long>(value));
    return 31 - index;
#elif __has_builtin(__builtin_clz)
    if( value == 0 ) { return 64; }

    if( value < 1ULL << 32 )
    {
        return 32 + __builtin_clz(static_cast<uint32_t>(value));
    }

    value >>= 32;
    return __builtin_clz(static_cast<uint32_t>(value));
#else
    if( value == 0 ) { return 64; }

    uint32_t count = 0;
    uint64_t mask = 1ULL << 63;
    while( (value & mask) == 0 )
    {
        count++;
        mask >>= 1;
    }

    return count;
#endif
}

} // namespace Utils