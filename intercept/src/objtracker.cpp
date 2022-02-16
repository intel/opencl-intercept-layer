/*
// Copyright (c) 2018-2022 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include <ostream>
#include <string>

#include "objtracker.h"

void CObjectTracker::ReportHelper(
    const std::string& label,
    const CObjectTracker::CTracker& tracker,
    std::ostream& os )
{
    if( tracker.NumReleases < tracker.NumAllocations + tracker.NumRetains )
    {
        os << "Possible leak of type " << label << "!" << std::endl;
        os << "    Number of Allocations: " << tracker.NumAllocations << std::endl;
        os << "    Number of Retains:     " << tracker.NumRetains << std::endl;
        os << "    Number of Releases:    " << tracker.NumReleases << std::endl;
    }
    else if( tracker.NumReleases > tracker.NumAllocations + tracker.NumRetains )
    {
        // If there are more releases than allocations or retains then this
        // is an unexpected situation.  It usually means that some allocations
        // aren't tracked correctly, or that a retain or release returned
        // an error.
        os << "Unexpected counts for type " << label << "!" << std::endl;
        os << "    Number of Allocations: " << tracker.NumAllocations << std::endl;
        os << "    Number of Retains:     " << tracker.NumRetains << std::endl;
        os << "    Number of Releases:    " << tracker.NumReleases << std::endl;
    }
    else if( tracker.NumAllocations )
    {
        os << "No " << label << " leaks detected." << std::endl;
    }
}

void CObjectTracker::writeReport( std::ostream& os )
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    os << std::endl;
    ReportHelper( "cl_device_id",       m_Devices,          os );
    ReportHelper( "cl_context",         m_Contexts,         os );
    ReportHelper( "cl_command_queue",   m_CommandQueues,    os );
    ReportHelper( "cl_mem",             m_MemObjects,       os );
    ReportHelper( "cl_sampler",         m_Samplers,         os );
    ReportHelper( "cl_program",         m_Programs,         os );
    ReportHelper( "cl_kernel",          m_Kernels,          os );
    ReportHelper( "cl_event",           m_Events,           os );
    ReportHelper( "cl_semaphore_khr",   m_Semaphores,       os );
    ReportHelper( "cl_command_buffer_khr", m_CommandBuffers, os );
}
