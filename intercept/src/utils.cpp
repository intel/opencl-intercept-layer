/*
// Copyright (c) 2024-2025 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include "utils.h"

#include <algorithm>
#include <fstream>
#include <string>

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

}
