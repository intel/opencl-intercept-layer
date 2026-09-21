/*
// Copyright (c) 2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#include <list>
#include <mutex>
#include <string>

#include "common.h"

class CEventList
{
public:
    // This mutex serializes checking the events in the event list.  It is
    // held by the caller for the duration of a check, which ensures that only
    // one thread checks events at a time.
    std::mutex  CheckMutex;

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

    using CNodeList = std::list<Node>;

    CEventList() = default;
    ~CEventList() = default;
    CEventList( const CEventList& ) = delete;
    CEventList& operator=( const CEventList& ) = delete;

    void    addNode( Node&& node )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_EventList.push_back( std::move(node) );
    }

    // Moves all nodes out of the event list and into the caller's list, so
    // the caller can check the events without holding a lock.
    void    takeNodes( CNodeList& nodes )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        nodes.splice( nodes.end(), m_EventList );
    }

    // Returns nodes that were not processed to the front of the event list,
    // so the nodes in the event list remain ordered oldest to newest, even if
    // nodes were added while the events were being checked.
    void    returnNodes( CNodeList& nodes )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_EventList.splice( m_EventList.begin(), nodes );
    }

    size_t size()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_EventList.size();
    }

private:
    std::mutex  m_Mutex;

    CNodeList   m_EventList;
};
