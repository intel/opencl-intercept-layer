/*
// Copyright (c) 2018-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include "OS_mac_common.h"

namespace OS
{

const char* Services_Common::ENV_PREFIX = "";
const char* Services_Common::CONFIG_FILE = "config.conf";
const char* Services_Common::SYSTEM_DIR = "/etc";
const char* Services_Common::LOG_DIR = NULL;
bool Services_Common::APPEND_PID = false;

Services_Common::Services_Common()
{
}

Services_Common::~Services_Common()
{
}

bool Services_Common::GetControl(
    const std::string& name,
    void* pValue,
    size_t size ) const
{
    // Look at environment variables first:
    {
        std::string envName(ENV_PREFIX);
        envName += name;
        const char *envVal = getenv(envName.c_str());
        if( ( envVal != NULL ) && ( size == sizeof(unsigned int) ) )
        {
            unsigned int *puVal = (unsigned int *)pValue;
            *puVal = atoi(envVal);
            return true;
        }
        else if( ( envVal != NULL ) && ( strlen(envVal) < size ) )
        {
            char* pStr = (char*)pValue;
            strcpy( pStr, envVal );
            return true;
        }
    }

    // Look at config files second:
    bool    found = false;
    std::string fileName;

    // First, check for a config file in the HOME directory.
    const char *envVal = getenv("HOME");
    if( !found && envVal != NULL )
    {
        fileName = envVal;
        fileName += "/";
        fileName += CONFIG_FILE;
        found = GetControlFromFile(
            fileName,
            name,
            pValue,
            size );
    }

    // Finally, check the "system" directory.
    if( !found )
    {
        fileName = SYSTEM_DIR;
        fileName += "/";
        fileName += CONFIG_FILE;
        found = GetControlFromFile(
            fileName,
            name,
            pValue,
            size );
    }

    return found;
}

bool Services_Common::GetControlFromFile(
    const std::string& fileName,
    const std::string& controlName,
    void* pValue,
    size_t size ) const
{
    bool    found = false;

    std::ifstream   is;
    std::string     s;

    is.open( fileName.c_str() );
    if( is.fail() )
    {
        return false;
    }

    while( !is.eof() && !found )
    {
        std::getline(is, s);

        // skip blank lines
        if( s.length() == 0 )
        {
            continue;
        }
        // skip "comment" lines
        if( s.find(";") == 0 || s.find("#") == 0 || s.find("//") == 0 )
        {
            continue;
        }

        size_t  pos = s.find('=');
        if( pos != std::string::npos )
        {
            std::string var = s.substr( 0, pos );
            var.erase(std::remove_if(var.begin(), var.end(), ::isspace), var.end());

            std::string value = s.substr( pos + 1 );
            value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());

            if( var == controlName )
            {
                if( size == sizeof(unsigned int) )
                {
                    unsigned int* pUIValue = (unsigned int*)pValue;
                    std::istringstream iss(value);
                    iss >> pUIValue[0];
                    found = true;
                }
                else if( value.length() < size )
                {
                    char* pStr = (char*)pValue;
                    strcpy( pStr, value.c_str() );
                    found = true;
                }
            }
        }
    }

    is.close();
    return found;
}

}
