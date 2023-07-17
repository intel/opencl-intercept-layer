/*
// Copyright (c) 2018-2023 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#pragma once

#include <chrono>
#include <cinttypes>
#include <fstream>
#include <mutex>
#include <vector>

#include <stdint.h>

#include "common.h"

class CLIntercept;

class CChromeTracer
{
public:
    CChromeTracer( const CLIntercept* pIntercept ) :
        m_pIntercept(pIntercept)
    {
        m_ProcessId = 0;
    }

    ~CChromeTracer()
    {
        flush();
        m_InterceptTrace.close();
    }

    void init( const std::string& fileName );

    void addProcessMetadata(
            uint64_t threadId,
            const std::string& processName )
    {
        m_InterceptTrace
            << "{\"ph\":\"M\", \"name\":\"process_name\", \"pid\":" << m_ProcessId
            << ", \"tid\":" << threadId
            << ", \"args\":{\"name\":\"" << processName
            << "\"}},\n";
    }

    void addThreadMetadata(
            uint64_t threadId,
            uint32_t threadNumber )
    {
        m_InterceptTrace
            << "{\"ph\":\"M\", \"name\":\"thread_name\", \"pid\":" << m_ProcessId
            << ", \"tid\":" << threadId
            << ", \"args\":{\"name\":\"Host Thread " << threadId
            << "\"}},\n";
        m_InterceptTrace
            << "{\"ph\":\"M\", \"name\":\"thread_sort_index\", \"pid\":" << m_ProcessId
            << ", \"tid\":" << threadId
            << ", \"args\":{\"sort_index\":\"" << threadNumber + 10000
            << "\"}},\n";
    }

    void addStartTimeMetadata(
            uint64_t threadId,
            uint64_t startTime )
    {
        m_InterceptTrace
            << "{\"ph\":\"M\", \"name\":\"clintercept_start_time\", \"pid\":" << m_ProcessId
            << ", \"tid\":" << threadId
            << ", \"args\":{\"start_time\":" << startTime
            << "}},\n";
    }

    void addQueueMetadata(
            uint32_t queueNumber,
            const std::string& queueName )
    {
        m_InterceptTrace
            << "{\"ph\":\"M\", \"name\":\"thread_name\", \"pid\":" << m_ProcessId
            << ", \"tid\":-" << queueNumber
            << ", \"args\":{\"name\":\"" << queueName
            << "\"}},\n";
        m_InterceptTrace
            << "{\"ph\":\"M\", \"name\":\"thread_sort_index\", \"pid\":" << m_ProcessId
            << ", \"tid\":-" << queueNumber
            << ", \"args\":{\"sort_index\":\"" << queueNumber
            << "\"}},\n";
    }

    // Call Logging
    void addCallLogging(
            const char* name,
            uint64_t threadId,
            uint64_t startTime,
            uint64_t delta )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if( m_BufferSize == 0 )
        {
            writeCallLogging(
                name,
                threadId,
                startTime,
                delta );
        }
        else
        {
            m_RecordBuffer.emplace_back(RecordType::CallLogging, name);

            Record& rec = m_RecordBuffer.back();
            rec.CallLogging.ThreadId = threadId;
            rec.CallLogging.StartTime = startTime;
            rec.CallLogging.Delta = delta;

            checkFlushRecords();
        }
    }

    // Call Logging with Tag
    void addCallLogging(
            const char* name,
            const std::string& tag,
            uint64_t threadId,
            uint64_t startTime,
            uint64_t delta )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if( m_BufferSize == 0 )
        {
            writeCallLogging(
                name,
                tag.c_str(),
                threadId,
                startTime,
                delta );
        }
        else
        {
            m_RecordBuffer.emplace_back(RecordType::CallLoggingTag, name, tag);

            Record& rec = m_RecordBuffer.back();
            rec.CallLogging.ThreadId = threadId;
            rec.CallLogging.StartTime = startTime;
            rec.CallLogging.Delta = delta;

            checkFlushRecords();
        }
    }

    // Call Logging with Id
    void addCallLogging(
            const char* name,
            uint64_t threadId,
            uint64_t startTime,
            uint64_t delta,
            uint64_t id )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if( m_BufferSize == 0 )
        {
            writeCallLogging(
                name,
                threadId,
                startTime,
                delta,
                id );
        }
        else
        {
            m_RecordBuffer.emplace_back(RecordType::CallLoggingId, name);

            Record& rec = m_RecordBuffer.back();
            rec.CallLogging.ThreadId = threadId;
            rec.CallLogging.StartTime = startTime;
            rec.CallLogging.Delta = delta;
            rec.CallLogging.Id = id;

            checkFlushRecords();
        }
    }

    // Call Logging with Tag and Id
    void addCallLogging(
            const char* name,
            const std::string& tag,
            uint64_t threadId,
            uint64_t startTime,
            uint64_t delta,
            uint64_t id )
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if( m_BufferSize == 0 )
        {
            writeCallLogging(
                name,
                tag.c_str(),
                threadId,
                startTime,
                delta,
                id );
        }
        else
        {
            m_RecordBuffer.emplace_back(RecordType::CallLoggingTagId, name, tag);

            Record& rec = m_RecordBuffer.back();
            rec.CallLogging.ThreadId = threadId;
            rec.CallLogging.StartTime = startTime;
            rec.CallLogging.Delta = delta;
            rec.CallLogging.Id = id;

            checkFlushRecords();
        }
    }

    // temp
    std::ostream& write( const char* str, std::streamsize count )
    {
        return m_InterceptTrace.write(str, count);
    }

    // temp?
    std::ostream& flush()
    {
        if( m_RecordBuffer.size() > 0 )
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            flushRecords();
        }
        return m_InterceptTrace.flush();
    }

private:
    const CLIntercept* m_pIntercept;

    std::mutex  m_Mutex;

    uint64_t    m_ProcessId;
    uint32_t    m_BufferSize;

    std::ofstream   m_InterceptTrace;
    mutable char    m_StringBuffer[CLI_STRING_BUFFER_SIZE];

    enum class RecordType
    {
        CallLogging,
        CallLoggingTag,
        CallLoggingId,
        CallLoggingTagId,

        DeviceTiming,
        DeviceTimingPerKernel,
        DeviceTimingInStages,
        DeviceTimingInStagesPerKernel,
    };

    struct Record
    {
        struct Record() = default;
        struct Record( RecordType rt, const char* name ) :
            Type(rt), Name(name) {}
        struct Record( RecordType rt, const char* name, const std::string& tag ) :
            Type(rt), Name(name), Tag(tag) {}

        RecordType  Type;

        std::string Name;
        std::string Tag;

        union
        {
            struct
            {
                uint64_t    ThreadId;
                uint64_t    StartTime;
                uint64_t    Delta;
                uint64_t    Id;
            } CallLogging;

            struct
            {
                uint32_t    Count;
                uint32_t    QueueNumber;
                uint64_t    QueuedTime;
                uint64_t    SubmitTime;
                uint64_t    StartTime;
                uint64_t    EndTime;
                uint64_t    Id;
            } DeviceTiming;
        };
    };

    std::vector< Record >   m_RecordBuffer;

    // Call Logging
    void writeCallLogging(
            const char* name,
            uint64_t threadId,
            uint64_t startTime,
            uint64_t delta );

    // Call Logging with Tag
    void writeCallLogging(
            const char* name,
            const char* tag,
            uint64_t threadId,
            uint64_t startTime,
            uint64_t delta );

    // Call Logging with Id
    void writeCallLogging(
            const char* name,
            uint64_t threadId,
            uint64_t startTime,
            uint64_t delta,
            uint64_t id );

    // Call Logging with Tag and Id
    void writeCallLogging(
            const char* name,
            const char* tag,
            uint64_t threadId,
            uint64_t startTime,
            uint64_t delta,
            uint64_t id );

    void checkFlushRecords()
    {
        if( m_RecordBuffer.size() >= m_BufferSize )
        {
            flushRecords();
        }
    }

    void flushRecords();

    DISALLOW_COPY_AND_ASSIGN( CChromeTracer );
};
