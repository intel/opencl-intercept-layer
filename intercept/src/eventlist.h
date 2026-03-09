/*
// Copyright (c) 2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#include <chrono>
#include <list>
#include <mutex>
#include <string>

#include "common.h"

class CEventList
{
public:
// !!! TODO: figure out what to do here!
#if defined(CLINTERCEPT_HIGH_RESOLUTON_CLOCK)
    using clock = std::chrono::high_resolution_clock;
#else
    using clock = std::chrono::steady_clock;
#endif

    struct Node
    {
        cl_device_id        Device;
        unsigned int        QueueNumber;
        std::string         Name;
        uint64_t            EnqueueCounter;
        clock::time_point   QueuedTime;
        bool                UseProfilingDelta;
        int64_t             ProfilingDeltaNS;
        cl_event            Event;
    };

    using const_iterator = std::list<Node>::const_iterator;

    CEventList() = default;
    ~CEventList() = default;
    CEventList( const CEventList& ) = delete;
    CEventList& operator=( const CEventList& ) = delete;

    void    addNode( Node&& node )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_EventList.push_back( std::move(node) );
    }

    void    erase( const_iterator iterator )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_EventList.erase( iterator );
    }

    const_iterator begin() const
    {
        return m_EventList.begin();
    }

    const_iterator end() const
    {
        return m_EventList.end();
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_EventList.size();
    }

private:
    std::mutex  m_Mutex;

    std::list<Node> m_EventList;
};
