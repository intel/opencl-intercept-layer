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
    else if( ( tracker.NumReleases > tracker.NumAllocations + tracker.NumRetains ) ||
             ( tracker.NumAllocations == 0 && ( tracker.NumReleases || tracker.NumRetains ) ) )
    {
        // If there are more releases than allocations or retains then this
        // is an unexpected situation.  It usually means that some allocations
        // aren't tracked correctly, or that a retain or release returned
        // an error.
        // Similarly, if we have no allocations but do have retains or
        // releases, we probably missed an allocation.
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

void CObjectTracker::writeReport( std::ostream& os ) const
{
    os << std::endl;
    ReportHelper( "cl_device_id",       m_Devices,          os );
    ReportHelper( "cl_context",         m_Contexts,         os );
    ReportHelper( "cl_command_queue",   m_CommandQueues,    os );
    ReportHelper( "cl_mem",             m_MemObjects,       os );
    ReportHelper( "cl_sampler",         m_Samplers,         os );
    ReportHelper( "cl_program",         m_Programs,         os );
    ReportHelper( "cl_kernel",          m_Kernels,          os );
    ReportHelper( "cl_event",           m_Events,           os );
}
