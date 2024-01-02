/*
// Copyright (c) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
*/

#include "chrometracer.h"

void CChromeTracer::init(
    const std::string& fileName,
    uint64_t processId,
    uint32_t bufferSize,
    bool addFlowEvents )
{
    m_ProcessId = processId;
    m_BufferSize = bufferSize;
    m_AddFlowEvents = addFlowEvents;

    if( m_BufferSize != 0 )
    {
        m_RecordBuffer.reserve( m_BufferSize );
    }

    m_TraceFile.open(
        fileName.c_str(),
        std::ios::out | std::ios::binary );
    m_TraceFile << "[\n";
}

// Notes for the future:
// Printing into a pre-allocated string buffer and then writing is
// measured to be faster than calling fprintf or stream insertion
// operators.
// Handling each of these four cases separately eliminates the need
// to concatenate strings and reduces overhead.

// Call Logging
void CChromeTracer::writeCallLogging(
    const char* name,
    uint64_t threadId,
    uint64_t startTime,
    uint64_t delta )
{
    int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
        "{\"ph\":\"X\",\"pid\":%" PRIu64 ",\"tid\":%" PRIu64 ",\"name\":\"%s\""
        ",\"ts\":%.3f,\"dur\":%.3f},\n",
        m_ProcessId,
        threadId,
        name,
        startTime / 1000.0,
        delta /1000.0 );
    m_TraceFile.write(m_StringBuffer, size);
}

// Call Logging with Tag
void CChromeTracer::writeCallLogging(
    const char* name,
    const char* tag,
    uint64_t threadId,
    uint64_t startTime,
    uint64_t delta )
{
    int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
        "{\"ph\":\"X\",\"pid\":%" PRIu64 ",\"tid\":%" PRIu64 ",\"name\":\"%s( %s )\""
        ",\"ts\":%.3f,\"dur\":%.3f},\n",
        m_ProcessId,
        threadId,
        name,
        tag,
        startTime / 1000.0,
        delta / 1000.0 );
    m_TraceFile.write(m_StringBuffer, size);
}

// Call Logging with Id
void CChromeTracer::writeCallLogging(
    const char* name,
    uint64_t threadId,
    uint64_t startTime,
    uint64_t delta,
    uint64_t id )
{
    int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
        "{\"ph\":\"X\",\"pid\":%" PRIu64 ",\"tid\":%" PRIu64 ",\"name\":\"%s\""
        ",\"ts\":%.3f,\"dur\":%.3f,\"args\":{\"id\":%" PRIu64 "}},\n",
        m_ProcessId,
        threadId,
        name,
        startTime / 1000.0,
        delta / 1000.0,
        id );
    m_TraceFile.write(m_StringBuffer, size);

    if( m_AddFlowEvents )
    {
        int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
            "{\"ph\":\"s\",\"pid\":%" PRIu64 ",\"tid\":%" PRIu64 ",\"name\":\"Command\""
            ",\"cat\":\"Commands\",\"ts\":%.3f,\"id\":%" PRIu64 "},\n",
            m_ProcessId,
            threadId,
            startTime / 1000.0,
            id );
        m_TraceFile.write(m_StringBuffer, size);
    }
}

// Call Logging with Tag and Id
void CChromeTracer::writeCallLogging(
    const char* name,
    const char* tag,
    uint64_t threadId,
    uint64_t startTime,
    uint64_t delta,
    uint64_t id )
{
    int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
        "{\"ph\":\"X\",\"pid\":%" PRIu64 ",\"tid\":%" PRIu64 ",\"name\":\"%s( %s )\""
        ",\"ts\":%.3f,\"dur\":%.3f,\"args\":{\"id\":%" PRIu64 "}},\n",
        m_ProcessId,
        threadId,
        name,
        tag,
        startTime / 1000.0,
        delta / 1000.0,
        id );
    m_TraceFile.write(m_StringBuffer, size);

    if( m_AddFlowEvents )
    {
        int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
            "{\"ph\":\"s\",\"pid\":%" PRIu64 ",\"tid\":%" PRIu64 ",\"name\":\"Command\""
            ",\"cat\":\"Commands\",\"ts\":%.3f,\"id\":%" PRIu64 "},\n",
            m_ProcessId,
            threadId,
            startTime / 1000.0,
            id );
        m_TraceFile.write(m_StringBuffer, size);
    }
}

// Device Timing
void CChromeTracer::writeDeviceTiming(
    const char* name,
    uint32_t queueNumber,
    uint64_t startTime,
    uint64_t endTime,
    uint64_t id )
{
    if( m_AddFlowEvents )
    {
        int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
            "{\"ph\":\"f\",\"pid\":%" PRIu64 ",\"tid\":%u.1,\"name\":\"Command\""
            ",\"cat\":\"Commands\",\"ts\":%.3f,\"id\":%" PRIu64 "},\n",
            m_ProcessId,
            queueNumber,
            startTime / 1000.0,
            id );
        m_TraceFile.write(m_StringBuffer, size);
    }

    int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
        "{\"ph\":\"X\",\"pid\":%" PRIu64 ",\"tid\":%u.1,\"name\":\"%s\""
        ",\"ts\":%.3f,\"dur\":%.3f,\"args\":{\"id\":%" PRIu64 "}},\n",
        m_ProcessId,
        queueNumber,
        name,
        startTime / 1000.0,
        (endTime - startTime) / 1000.0,
        id );
    m_TraceFile.write(m_StringBuffer, size);
}

// Device Timing Per Kernel
void CChromeTracer::writeDeviceTiming(
    const char* name,
    uint64_t startTime,
    uint64_t endTime,
    uint64_t id )
{
    if( m_AddFlowEvents )
    {
        int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
            "{\"ph\":\"f\",\"pid\":%" PRIu64 ",\"tid\":\"%s\",\"name\":\"Command\""
            ",\"cat\":\"Commands\",\"ts\":%.3f,\"id\":%" PRIu64 "},\n",
            m_ProcessId,
            name,
            startTime / 1000.0,
            id );
        m_TraceFile.write(m_StringBuffer, size);
    }

    int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
        "{\"ph\":\"X\",\"pid\":%" PRIu64 ",\"tid\":\"%s\",\"name\":\"%s\""
        ",\"ts\":%.3f,\"dur\":%.3f,\"args\":{\"id\":%" PRIu64 "}},\n",
        m_ProcessId,
        name,
        name,
        startTime / 1000.0,
        (endTime - startTime) / 1000.0,
        id );
    m_TraceFile.write(m_StringBuffer, size);
}

// Shared lookup tables:
static const size_t cNumStates = 3;
static const char* colours[cNumStates] = {
    "thread_state_runnable",
    "cq_build_running",
    "thread_state_iowait"
};
static const char* suffixes[cNumStates] = {
    "(Queued)",
    "(Submitted)",
    "(Execution)"
};

// Device Timing In Stages
void CChromeTracer::writeDeviceTiming(
    const char* name,
    uint32_t count,
    uint32_t queueNumber,
    uint64_t queuedTime,
    uint64_t submitTime,
    uint64_t startTime,
    uint64_t endTime,
    uint64_t id )
{
    const double    usStarts[cNumStates] = {
        queuedTime / 1000.0,
        submitTime / 1000.0,
        startTime / 1000.0,
    };
    const double    usDeltas[cNumStates] = {
        (submitTime - queuedTime) / 1000.0,
        (startTime - submitTime) / 1000.0,
        (endTime - startTime) / 1000.0,
    };

    for( size_t state = 0; state < cNumStates; state++ )
    {
        int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
            "{\"ph\":\"X\",\"pid\":%" PRIu64 ",\"tid\":%u.%u,\"name\":\"%s %s\""
            ",\"ts\":%.3f,\"dur\":%.3f,\"cname\":\"%s\",\"args\":{\"id\":%" PRIu64 "}},\n",
            m_ProcessId,
            count,
            queueNumber,
            name,
            suffixes[state],
            usStarts[state],
            usDeltas[state],
            colours[state],
            id );
        m_TraceFile.write(m_StringBuffer, size);
    }
}

// Device Timing In Stages Per Kernel
void CChromeTracer::writeDeviceTiming(
    const char* name,
    uint64_t queuedTime,
    uint64_t submitTime,
    uint64_t startTime,
    uint64_t endTime,
    uint64_t id )
{
    const double    usStarts[cNumStates] = {
        queuedTime / 1000.0,
        submitTime / 1000.0,
        startTime / 1000.0,
    };
    const double    usDeltas[cNumStates] = {
        (submitTime - queuedTime) / 1000.0,
        (startTime - submitTime) / 1000.0,
        (endTime - startTime) / 1000.0,
    };

    for( size_t state = 0; state < cNumStates; state++ )
    {
        int size = CLI_SPRINTF(m_StringBuffer, CLI_STRING_BUFFER_SIZE,
            "{\"ph\":\"X\",\"pid\":%" PRIu64 ",\"tid\":\"%s\",\"name\":\"%s %s\""
            ",\"ts\":%.3f,\"dur\":%.3f,\"cname\":\"%s\",\"args\":{\"id\":%" PRIu64 "}},\n",
            m_ProcessId,
            name,
            name,
            suffixes[state],
            usStarts[state],
            usDeltas[state],
            colours[state],
            id );
        m_TraceFile.write(m_StringBuffer, size);
    }
}

void CChromeTracer::flushRecords()
{
    for( const auto& rec : m_RecordBuffer )
    {
        switch( rec.Type )
        {
        case RecordType::CallLogging:
            writeCallLogging(
                rec.Name.c_str(),
                rec.CallLogging.ThreadId,
                rec.CallLogging.StartTime,
                rec.CallLogging.Delta );
            break;
        case RecordType::CallLoggingTag:
            writeCallLogging(
                rec.Name.c_str(),
                rec.Tag.c_str(),
                rec.CallLogging.ThreadId,
                rec.CallLogging.StartTime,
                rec.CallLogging.Delta );
            break;
        case RecordType::CallLoggingId:
            writeCallLogging(
                rec.Name.c_str(),
                rec.CallLogging.ThreadId,
                rec.CallLogging.StartTime,
                rec.CallLogging.Delta,
                rec.CallLogging.Id );
            break;
        case RecordType::CallLoggingTagId:
            writeCallLogging(
                rec.Name.c_str(),
                rec.Tag.c_str(),
                rec.CallLogging.ThreadId,
                rec.CallLogging.StartTime,
                rec.CallLogging.Delta,
                rec.CallLogging.Id );
            break;

        case RecordType::DeviceTiming:
            writeDeviceTiming(
                rec.Name.c_str(),
                rec.DeviceTiming.QueueNumber,
                rec.DeviceTiming.StartTime,
                rec.DeviceTiming.EndTime,
                rec.DeviceTiming.Id );
            break;
        case RecordType::DeviceTimingPerKernel:
            writeDeviceTiming(
                rec.Name.c_str(),
                rec.DeviceTiming.StartTime,
                rec.DeviceTiming.EndTime,
                rec.DeviceTiming.Id );
            break;
        case RecordType::DeviceTimingInStages:
            writeDeviceTiming(
                rec.Name.c_str(),
                rec.DeviceTiming.Count,
                rec.DeviceTiming.QueueNumber,
                rec.DeviceTiming.QueuedTime,
                rec.DeviceTiming.SubmitTime,
                rec.DeviceTiming.StartTime,
                rec.DeviceTiming.EndTime,
                rec.DeviceTiming.Id );
            break;
        case RecordType::DeviceTimingInStagesPerKernel:
            writeDeviceTiming(
                rec.Name.c_str(),
                rec.DeviceTiming.QueuedTime,
                rec.DeviceTiming.SubmitTime,
                rec.DeviceTiming.StartTime,
                rec.DeviceTiming.EndTime,
                rec.DeviceTiming.Id );
            break;

        default: CLI_ASSERT(0); break;
        }
    }

    m_RecordBuffer.clear();
}
