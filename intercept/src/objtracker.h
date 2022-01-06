/*
// Copyright (c) 2018-2022 Intel Corporation
//
// SPDX-License-Identifier: MIT
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
    CTracker    m_Semaphores;
    CTracker    m_CommandBuffers;
    //cl_accelerator_intel?

    CTracker&   GetTracker( cl_device_id )      { return m_Devices;         }
    CTracker&   GetTracker( cl_context )        { return m_Contexts;        }
    CTracker&   GetTracker( cl_command_queue )  { return m_CommandQueues;   }
    CTracker&   GetTracker( cl_mem )            { return m_MemObjects;      }
    CTracker&   GetTracker( cl_sampler )        { return m_Samplers;        }
    CTracker&   GetTracker( cl_program )        { return m_Programs;        }
    CTracker&   GetTracker( cl_kernel )         { return m_Kernels;         }
    CTracker&   GetTracker( cl_event )          { return m_Events;          }
    CTracker&   GetTracker( cl_semaphore_khr )  { return m_Semaphores;      }
    CTracker&   GetTracker( cl_command_buffer_khr ) { return m_CommandBuffers; }

    static void ReportHelper(
        const std::string& label,
        const CTracker& tracker,
        std::ostream& os );

    DISALLOW_COPY_AND_ASSIGN( CObjectTracker );
};
