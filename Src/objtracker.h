/*
// Copyright (c) 2018-2019 Intel Corporation
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

class CObjectTracker
{
public:
    CObjectTracker() {}

    void    writeReport( std::ostream& os ) const;

    template<class T>
    void    AddAllocation( T obj )
    {
        if( obj )
        {
            GetTracker(obj).NumAllocations++;   // todo: atomic?
        }
    }

    template<class T>
    void    AddRetain( T obj )
    {
        if( obj )
        {
            GetTracker(obj).NumRetains++;
        }
    }

    template<class T>
    void    AddRelease( T obj )
    {
        if( obj )
        {
            GetTracker(obj).NumReleases++;
        }
    }

private:
    struct CTracker
    {
        CTracker() :
            NumAllocations(0),
            NumRetains(0),
            NumReleases(0) {};

        size_t  NumAllocations;
        size_t  NumRetains;
        size_t  NumReleases;
    };

    CTracker    m_Devices;
    CTracker    m_Contexts;
    CTracker    m_CommandQueues;
    CTracker    m_MemObjects;
    CTracker    m_Samplers;
    CTracker    m_Programs;
    CTracker    m_Kernels;
    CTracker    m_Events;
    //cl_accelerator_intel?

    CTracker&   GetTracker( cl_device_id )      { return m_Devices;         }
    CTracker&   GetTracker( cl_context )        { return m_Contexts;        }
    CTracker&   GetTracker( cl_command_queue )  { return m_CommandQueues;   }
    CTracker&   GetTracker( cl_mem )            { return m_MemObjects;      }
    CTracker&   GetTracker( cl_sampler )        { return m_Samplers;        }
    CTracker&   GetTracker( cl_program )        { return m_Programs;        }
    CTracker&   GetTracker( cl_kernel )         { return m_Kernels;         }
    CTracker&   GetTracker( cl_event )          { return m_Events;          }

    static void ReportHelper(
        const std::string& label,
        const CTracker& tracker,
        std::ostream& os );

    DISALLOW_COPY_AND_ASSIGN( CObjectTracker );
};
