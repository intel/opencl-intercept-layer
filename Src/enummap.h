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

#pragma once

#include "common.h"
#include "cli_ext.h"

#include <map>
#include <string>

class CEnumNameMap
{
public:
    CEnumNameMap();

    #define GENERATE_MAP_AND_FUNC( _name, _type )                   \
        private:                                                    \
            std::map< _type, std::string >      m_##_type;          \
        public:                                                     \
            std::string _name( _type e ) const                      \
            {                                                       \
                std::map< _type, std::string >::const_iterator i =  \
                    m_##_type.find( e );                            \
                if( i == m_##_type.end() )                          \
                {                                                   \
                    return "**UNKNOWN ENUM**";                      \
                }                                                   \
                else                                                \
                {                                                   \
                    return (*i).second;                             \
                }                                                   \
            }

    #define GENERATE_MAP_AND_BITFIELD_FUNC( _name, _type )          \
    private:                                                        \
        std::map< _type, std::string > m_##_type;                   \
    public:                                                         \
        std::string _name( _type e ) const                          \
        {                                                           \
            std::string ret = "";                                   \
            int bit = 0;                                            \
            std::map< _type, std::string >::const_iterator i =      \
                m_##_type.find( e );                                \
            if( i != m_##_type.end() )                              \
            {                                                       \
                ret += (*i).second;                                 \
            }                                                       \
            else                                                    \
            {                                                       \
                while( e != 0 )                                     \
                {                                                   \
                    _type check = (_type)1 << bit;                  \
                    if( e & check )                                 \
                    {                                               \
                        i = m_##_type.find( check );                \
                        if( ret.length() )                          \
                        {                                           \
                            ret += " | ";                           \
                        }                                           \
                        if( i != m_##_type.end() )                  \
                        {                                           \
                            ret += (*i).second;                     \
                        }                                           \
                        else                                        \
                        {                                           \
                            ret += "<unknown>";                     \
                        }                                           \
                        e &= ~check;                                \
                    }                                               \
                    ++bit;                                          \
                }                                                   \
            }                                                       \
            return ret;                                             \
        }

    // This type doesn't exist in CL.h, but the enums conflict with
    // other regular old cl_int enums.
    typedef cl_int cl_command_exec_status;

    // CL bitfield values and plain uints may collide and need their own map.
    // GL enums need their own map.
    // CL enums that are allocated from the Khronos registry are unique and
    // can go into the main/default cl_int map.
    GENERATE_MAP_AND_FUNC(          name,                            cl_int                          );
    GENERATE_MAP_AND_BITFIELD_FUNC( name_bool,                       cl_bool                         );
    GENERATE_MAP_AND_FUNC(          name_build_status,               cl_build_status                 );
    GENERATE_MAP_AND_FUNC(          name_command_exec_status,        cl_command_exec_status          );
    GENERATE_MAP_AND_BITFIELD_FUNC( name_command_queue_properties,   cl_command_queue_properties     );
    GENERATE_MAP_AND_BITFIELD_FUNC( name_device_affinity_domain,     cl_device_affinity_domain       );
    GENERATE_MAP_AND_BITFIELD_FUNC( name_device_exec_capabilities,   cl_device_exec_capabilities     );
    GENERATE_MAP_AND_BITFIELD_FUNC( name_device_fp_config,           cl_device_fp_config             );
    GENERATE_MAP_AND_FUNC(          name_device_local_mem_type,      cl_device_local_mem_type        );
    GENERATE_MAP_AND_FUNC(          name_device_mem_cache_type,      cl_device_mem_cache_type        );
    GENERATE_MAP_AND_BITFIELD_FUNC( name_device_svm_capabilities,    cl_device_svm_capabilities      );
    GENERATE_MAP_AND_BITFIELD_FUNC( name_device_type,                cl_device_type                  );
    GENERATE_MAP_AND_BITFIELD_FUNC( name_kernel_arg_type_qualifier,  cl_kernel_arg_type_qualifier    );
    GENERATE_MAP_AND_BITFIELD_FUNC( name_map_flags,                  cl_map_flags                    );
    GENERATE_MAP_AND_BITFIELD_FUNC( name_mem_flags,                  cl_mem_flags                    );
    GENERATE_MAP_AND_BITFIELD_FUNC( name_mem_migration_flags,        cl_mem_migration_flags          );
    GENERATE_MAP_AND_FUNC(          name_program_binary_type,        cl_program_binary_type          );
    GENERATE_MAP_AND_BITFIELD_FUNC( name_svm_mem_flags,              cl_svm_mem_flags                );
    GENERATE_MAP_AND_FUNC(          name_gl,                         GLenum                          );

    #undef GENERATE_MAP_AND_FUNC
    #undef GENERATE_MAP_AND_BITFIELD_FUNC

private:
    DISALLOW_COPY_AND_ASSIGN( CEnumNameMap );
};
