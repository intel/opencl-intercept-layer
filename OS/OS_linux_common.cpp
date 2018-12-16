/*
// Copyright (c) 2018 Intel Corporation
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

#include "OS_linux_common.h"
#ifdef __ANDROID__
#include <android/log.h>
#include <algorithm>
#include <cctype>
#endif

namespace OS
{

const char* Services_Common::ENV_PREFIX = "";
const char* Services_Common::CONFIG_FILE = "config.conf";
const char* Services_Common::LOG_DIR = NULL;
bool Services_Common::APPEND_PID = false;

Services_Common::Services_Common()
{
}

Services_Common::~Services_Common()
{
    pthread_mutex_destroy( &m_CriticalSection );
}

bool Services_Common::ReadRegistry(
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

    // Look at the config file second:
    bool    found = false;

    std::ifstream   is;
    std::string     s;

    std::string     configFile;

    const char *envVal = getenv("HOME");
#ifdef __ANDROID__
    // if ho HOME on Android then use sdcard folder
    if( envVal == NULL )
    {
        configFile = "/sdcard";
    }
    else
    {
        configFile = envVal;
    }
#else
    configFile = envVal;
#endif
    configFile += "/";
    configFile += CONFIG_FILE;

    is.open( configFile.c_str() );
    if( is.fail() )
    {
#ifdef __ANDROID__
        __android_log_print( ANDROID_LOG_WARN, "clIntercept", "Failed to open config file: %s\n", configFile.c_str() );
#endif
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

            if( var == name )
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
