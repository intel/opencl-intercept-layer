/*
// Copyright (c) 2025-2026 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/
#pragma once

#include <atomic>
#include <cinttypes>
#include <map>
#include <vector>
#include <sstream>

#include <stdint.h>

#include "common.h"

struct SCommandBufferRecord
{
    void    recordCreate(
                cl_command_buffer_khr cmdbuf,
                bool isInOrder)
    {
        queueIsInOrder = isInOrder;
        dotstring << "digraph {\n";
        dotstring << "  // " << (queueIsInOrder ? "in-order" : "out-of-order") << " command-buffer\n";
    }

    void    recordCommand(
                cl_command_queue queue,
                const char* cmd,
                const std::string& tag,
                cl_uint num_sync_points_in_wait_list,
                const cl_sync_point_khr* sync_point_wait_list,
                cl_sync_point_khr* sync_point)
    {
        SCommandBufferId id =
            sync_point == nullptr ?
            makeInternalId() :
            makeSyncPointId(*sync_point);

        dotstring << "  " << (id.isInternal ? "internal" : "syncpoint") << id.id
              << " [shape=oval, label=\"" << cmd;
        if( !tag.empty() )
        {
            dotstring << "( " << tag << " )";
        }
        dotstring << "\"]\n";

        for( cl_uint s = 0; s < num_sync_points_in_wait_list; s++ )
        {
            dotstring << "  syncpoint" << sync_point_wait_list[s]
                  << " -> "
                  << (id.isInternal ? "internal" : "syncpoint") << id.id
                  << " // explicit dependency\n";
        }

        for( const auto& dep : implicitDeps )
        {
            dotstring << "  " << (dep.isInternal ? "internal" : "syncpoint") << dep.id
                  << " -> "
                  << (id.isInternal ? "internal" : "syncpoint") << id.id
                  << " [style=dashed] // implicit dependency\n";
        }

        if( queueIsInOrder )
        {
            implicitDeps.clear();
            implicitDeps.push_back(id);
        }
        else
        {
            outstandingIds.push_back(id);
        }
    }

    void    recordBarrier(
                cl_command_queue queue,
                const char* cmd,
                cl_uint num_sync_points_in_wait_list,
                const cl_sync_point_khr* sync_point_wait_list,
                cl_sync_point_khr* sync_point)
    {
        SCommandBufferId id =
            sync_point == nullptr ?
            makeInternalId() :
            makeSyncPointId(*sync_point);

        dotstring << "  " << (id.isInternal ? "internal" : "syncpoint") << id.id
            << " [shape=octagon, label=\"" << cmd << "\"]\n";

        // If there is a sync point wait list, then the barrier depends on all
        // of the commands in the sync point wait list. Otherwise, the barrier
        // depends on all of the outstanding ids.
        if( num_sync_points_in_wait_list > 0 )
        {
            for( cl_uint s = 0; s < num_sync_points_in_wait_list; s++ )
            {
                dotstring << "  syncpoint" << sync_point_wait_list[s]
                    << " -> "
                    << (id.isInternal ? "internal" : "syncpoint") << id.id
                    << " // explicit dependency\n";
            }
        }
        else
        {
            for( const auto& dep : outstandingIds )
            {
                dotstring << "  " << (dep.isInternal ? "internal" : "syncpoint") << dep.id
                    << " -> "
                    << (id.isInternal ? "internal" : "syncpoint") << id.id
                    << " [style=dotted] // barrier dependency\n";
            }
            outstandingIds.clear();
        }

        // Add the implicit dependencies.
        for( const auto& dep : implicitDeps )
        {
            dotstring << "  " << (dep.isInternal ? "internal" : "syncpoint") << dep.id
                << " -> "
                << (id.isInternal ? "internal" : "syncpoint") << id.id
                << " [style=dashed] // implicit dependency\n";
        }

        // Now, the only implicit dependency that remains is this barrier.
        implicitDeps.clear();
        implicitDeps.push_back(id);
    }

    void recordFinalize()
    {
        dotstring << "}\n";
    }

    // Note: this cannot return a reference, because the underlying string is a
    // temporary object.
    const std::string getRecording() const
    {
        return dotstring.str();
    }

  private:
    struct SCommandBufferId
    {
        bool        isInternal = false;
        uint32_t    id = 0;
    };

    std::ostringstream dotstring;

    std::atomic<uint32_t> nextInternalId;

    bool    queueIsInOrder = false;

    std::vector<SCommandBufferId>  implicitDeps;
    std::vector<SCommandBufferId>  outstandingIds;

    SCommandBufferId makeInternalId()
    {
        SCommandBufferId id;
        id.isInternal = true;
        id.id = nextInternalId.fetch_add(1, std::memory_order_relaxed);
        return id;
    }

    SCommandBufferId makeSyncPointId(
        cl_sync_point_khr sync_point)
    {
        SCommandBufferId id;
        id.isInternal = false;
        id.id = sync_point;
        return id;
    }
};
